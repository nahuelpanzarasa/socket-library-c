#include "socket.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <commons/string.h>
#include <commons/collections/dictionary.h>

#include <pthread.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>

#include <commons/collections/list.h>

#define BACKLOG 10			// Cantidad de conexiones pendientes  que se mantienen en cola
#define MAXDATASIZE 25000 	// máximo número de bytes que se pueden leer de una vez


//Recibe y procesa un mensaje recibido
void receiptMessage(void * arguments)
{
	args_receiptMessage * args = arguments;

	int numbytes;				/* bytes recibidos por cada lectura (0 = desconexion del cliente) */
	char buffer[MAXDATASIZE];	/* buffer de lectura */
	bool bufferUsed;			/* determina si se analizo por lo menos una vez un nuevo set de datos */

	unsigned int n = -1;		/* cantidad de argumentos del packete que recibo */
	int i = 0;					/* ayuda a saber la ultima posicion de lectura en el buffer (tambien se usa como i en otros casos) */
	int j = 0;					/* j de for */
	int seps = 0;				/* cantidad de separadores ,(coma) contados en el encabezado del paquete */
	char * packet_head;			/* encabezado del paquete */
	char * packet_body;			/* cuerpo del paquete */
	char * exceeded;			/* datos excedidos del final del paquete */
	char * str_tmp;				/* temporar para operaciones de string */
	bool isExceeded = false;	/* determina si hay datos excedidos cargados en el buffer */

	bool fullHead = false;		/* determina si ya fue recibido el encabezado del paquete */
	int bytesBody = 0;			/* determina los bytes que contendrá el cuerpo del paquete */
	char ** head = NULL;		/* encabezado divido por n, name y bytes de los args */
	char * b;					/* utilizado para extraer bytes del buffer */
	void (*fn)();				/* utilizado para guardar la funcion que hay que ejecutar del cliente */
	char ** packet_args;		/* Argumentos que se envian por la funcion */


	//loopeo
	while(1)
	{
		//si en la ultima lectura dejo informacion excedida la guardo
		if(isExceeded)
			exceeded = string_duplicate(buffer);

		//Pongo en blanco el buffer
		buffer[0] = '\0';

		//Espero a recibir un mensaje del job, si se produce un error lo informo error y finalizo hilo
		if ((numbytes=recv(args->connection->socket, buffer, MAXDATASIZE-1, 0)) == -1)
		{
			//perror("recv");
			break;
		}

		//si en la ultima lectura dejo informacion excedida la agrego al inicio del buffer
		if(isExceeded)
		{
			exceeded = string_new();
			string_append(&exceeded, buffer);

			buffer[sizeof(exceeded)] = '\0'; //mejor que memset?
			strcpy(buffer, exceeded);
			free(exceeded);
		}

		//finalizo el hilo
		if(numbytes == 0)
			break;

		else
		{
			//Corta el bufer hasta donde recibe informacion
			buffer[numbytes] = '\0';

			//analizo el buffer si es la primera vez que lo voy a analizar o si queda informacion excedida para seguir analizando
			//(Analizar significa, buscar paquetes completos y ejecutarlos)
			bufferUsed = false;
			while(!bufferUsed || isExceeded)
			{
				bufferUsed = true;
				isExceeded = false;
				i = 0;

				//si comienzo a leer un nuevo paquete inicializo el head y el body
				if(n == -1)
				{
					packet_head = string_new();
					packet_body = string_new();
				}

				//recorro buffer hasta armar cabezal si no esta armado
				if(!fullHead)
				{
					for(i = 0; i < strlen(buffer); i++)
					{
						b = buffer[i];

						//primer caracter es [n] (cantidad de args)
						if(n == -1)
							n = atoi(&b);
						//cuento comas
						else if(b == ',')
							seps++;

						//voy completando cabezal
						string_append(&packet_head, &b);

						//fianlizo cuando llego al ultimo separador
						if(n+2 == seps)
						{
							fullHead = true;
							i++; //dejo posicion en el primer byte del cuerpo del paquete
							break;
						}
					}
				}

				//cabezal completo
				if(fullHead)
				{
					//si el cabezal no fue explodiado, le doy explode y calculo los bytes del cabezal
					if(head == NULL)
					{
						//hago explode
						head = string_n_split(packet_head, n+2, ",");
						for(j = 2; j < n+2; j++)
							bytesBody += atoi(head[j]);
					}

					//Agrego al cuerpo del packete todos los datos
					str_tmp = string_substring_from(buffer, i);
					string_append(&packet_body, str_tmp);
					free(str_tmp);

					//paquete completo
					if(bytesBody <= strlen(packet_body))
					{
						//si el paquete esta excedido, corto el paquete en la posicion correspondiente y guardo el excedente sobre el buffer
						if(bytesBody < strlen(packet_body))
						{
							isExceeded = true;

							exceeded = string_substring_from(packet_body, bytesBody);
							str_tmp = string_substring_until(packet_body, bytesBody);
							free(packet_body);
							packet_body = str_tmp;

							buffer[0] = '\0';
							strcpy(buffer, exceeded);
							free(exceeded);
						}

						//llamo a la funcion que maneja los mensajes recibidos
						if(args->fns_receipts != NULL)
						{
							i = 0;

							//armo argumentos en array
							packet_args = malloc(sizeof(char*)*n);
							for(j = 2; j < n+2; j++)
							{
								char * part = string_substring(packet_body, i, atoi(head[j]));
								packet_args[j-2] = part;
								i += atoi(head[j]);
							}

							//Si n=0 el split no toma el vacio despues de la coma final.. quito ultima coma
							if(n==0)
								head[1][strlen(head[1])-1] = '\0';

							//Ejecuto la función solicitada, si no existe la función muestro error
							fn = dictionary_get(args->fns_receipts, head[1]);
							if(fn != NULL)
								fn(args->connection, packet_args);
							else
								printf("Cliente SOCK%d(%s:%d) intento ejecutar la función inexistente '%s'.\n", args->connection->socket, args->connection->ip, args->connection->port, head[1]);

							//libero argumentos
							for(i = 0; i < n; i++)
								free(packet_args[i]);
							free(packet_args);
						}

						free(packet_head);
						free(packet_body);
						for(i = 0; i < n+3; i++)
							free(head[i]);
						free(head);

						n = -1;
						seps = 0;
						fullHead = false;
						bytesBody = 0;
						head = NULL;
					}
				}
			}
		}
	}

	//cierro socket
	close(args->connection->socket);

	//Informo a la funcion que maneja los cierres (si existe y la estructura de conexion lo permite)
	if(args->connection->run_fn_connectionClosed && args->fn_connectionClosed != NULL)
		args->fn_connectionClosed(args->connection);

	//libero memoria
	if(isExceeded)
		free(exceeded);
	if(n != -1)
	{
		free(packet_head);
		free(packet_body);
	}
	free(args);
}

//envia un mensaje a un socket
bool sendMessage(int socket, char * message)
{
	int total = 0;
	int n = 0;
	int len = string_length(message);

	//bucle hasta enviar todoo el packete (send devuelve los byte que envio)
	while(total < len)
	{
		if ((n = send(socket, message, len, 0)) == -1)
		{
			//perror("send");
			close(socket);
			return false;
		}

		total += n;
	}

	free(message);
	return true;
}

//Corre una funcion de otro proceso
bool runFunction(int socket, char * name, int n, ...)
{
	/*	ESTRUCTURA = [n],[name],[size(Ar_1)],[size(Ar_2)],...,[size(Ar_n)][Ar_1][Ar_2]...[Ar_n]
	 *	n = cantidad de argumentos a enviar
	 *	name = nombre de la funcion a ejecutar
	 *	Ar_n = Argumento n
	 *	size(Ar_n) = dimension del argumento n
	 */

	//Creacion de lista de argumentos dinamicos
	va_list arguments;
	va_start(arguments, n);
	char * arg;
	char * tmp;
	int i;

	char * packet_head = string_itoa(n); 	/* Cabeza del packete, lo inicio con la cantidad de argumentos que tendrá */
	string_append(&packet_head, ",");		/* Continuo con coma */
	string_append(&packet_head, name); 		/* Continuo con el nombre de la funcion */
	string_append(&packet_head, ",");		/* Continuo con coma */

	char * packet_boody = string_new();		/* Cuerpo del packete */

	//Recorro argumentos recibidos
	for (i = 0; i < n; i++)
	{
		arg = va_arg(arguments, char*); /* Extraigo argumento */

		if(arg == NULL)
		{
			printf("Error, no se encuentran los parametos indicados");
			exit(1);
		}

		//Agreo las dimensiones de cada argumento y las separo por comas (en el cabezal)
		tmp = string_itoa(strlen(arg));
		string_append(&packet_head, tmp);
		free(tmp);
		string_append(&packet_head, ",");

		//Junto todos los argumentos sin separadores (en el cuerpo)
		string_append(&packet_boody, arg);
	}

	//eliminación de lista de argumentos dinamicos
	va_end(arguments);

	//Dejo en el head el packete final
	string_append(&packet_head, packet_boody);

	free(packet_boody);

	return sendMessage(socket, packet_head);
}

//Elimina procesos muertos
void sigchld_handler(int s)
{
	while(wait(NULL) > 0);
}

//Se conecta a un servidor
int connectServer(char * ip, int port, t_dictionary * fns, void (*fn_connectionClosed)(), void * data)
{
	pthread_t th_receiptMessage;

	int socket_server;
	struct hostent *hostent_server;
	struct sockaddr_in addr_server; // información de la dirección de destino


	//Obtengo información de servidor, si hay error finalizo
	if ((hostent_server=gethostbyname(ip)) == NULL) {
		perror("gethostbyname");
		return -1;
	}

	//Abro socket, si hay error finalizo
	if ((socket_server = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("socket");
		return -1;
	}

	//Configuración de dirección de servidor
	addr_server.sin_family = AF_INET;    // Ordenación de bytes de la máquina
	addr_server.sin_port = htons(port);  // short, Ordenación de bytes de la red
	addr_server.sin_addr = *((struct in_addr *)hostent_server->h_addr);
	memset(&(addr_server.sin_zero), 0, 8);  // poner a cero el resto de la estructura

	//Conecto con servidor, si hay error finalizo
	if (connect(socket_server, (struct sockaddr *)&addr_server, sizeof(struct sockaddr)) == -1)
	{
		perror("connect");
		return -1;
	}

	//Creo estructura connection
	socket_connection * connection = malloc(sizeof(socket_connection));
	connection->socket = socket_server;
	connection->ip = ip;
	connection->port = port;
	connection->data = data;
	connection->run_fn_connectionClosed = true;

	//Creo receptor de mensajes
	args_receiptMessage *args_rm = malloc(sizeof(args_receiptMessage));
	args_rm->connection = connection;
	args_rm->fns_receipts = fns;
	args_rm->fn_connectionClosed = fn_connectionClosed;
	pthread_create(&th_receiptMessage, NULL, (void*) receiptMessage, args_rm);
	//pthread_join(th_receiptMessage, NULL);

	return socket_server;
}

//Hilo que escucha clientes y crea hilo de recepcion de mensajes
void listenClients(void * arguments)
{
	args_listenClients * args = arguments;


	int newConnection;					// Escuchar sobre socket_client, nuevas conexiones sobre newConnection
	pthread_t th_receiptMessage;		// hilo para crear receptor de mensajes
	struct sockaddr_in addr_client;		// información sobre la dirección del cliente
	socklen_t sin_size;

	//Loopeo hasta encontrar nueva conexion
	while(1)
	{
		//Quedo a la espera de la proxima conexion, cuando ocurre la acepto (accept se queda bloqueado hasta que aparesca una nueva conexion)
		//Si hay error lo muestro pero NO FINALIZO
		sin_size = sizeof(struct sockaddr_in);
		if ((newConnection = accept(args->socket_client, (struct sockaddr *)&addr_client, &sin_size)) == -1)
		{
			perror("accept");
			break;
		}

		//Creo estructura connection
		socket_connection * connection = malloc(sizeof(socket_connection));
		connection->socket = newConnection;
		connection->ip = string_duplicate(inet_ntoa(addr_client.sin_addr));
		connection->port = args->port;
		connection->data = args->data;
		connection->run_fn_connectionClosed = true;

		//aviso que se conecto un socket
		if(args->fn_newClient != NULL)
			args->fn_newClient(connection);

		//creo receptor de mensajes
		args_receiptMessage *args_rm = malloc(sizeof(args_receiptMessage));
		args_rm->connection = connection;
		args_rm->fns_receipts = args->fns_receipts;
		args_rm->fn_connectionClosed = args->fn_connectionClosed;
		pthread_create(&th_receiptMessage, NULL, (void*) receiptMessage, args_rm);
	}

	//Libero memoria
	free(args);
}

//Crea hilo de escucha de clientes
int createListen(int port, void (*fn_newClient)(), t_dictionary * fns, void (*fn_connectionClosed)(), void * data)
{

	int socket_client;					// Escuchar sobre socket_client, nuevas conexiones sobre newConnection
	struct sockaddr_in addr_server;		// información sobre la direccion del servidor


	struct sigaction sa;
	int yes=1;

	//Abro socket, si hay error finalizo
	if ((socket_client = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("socket");
		return -1;
	}

	//Libero socket (en caso que este colgado), si no se puede finalizo
	if (setsockopt(socket_client,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int)) == -1)
	{
		perror("setsockopt");
		return -1;
	}

	//Configuración de dirección de servidor
	addr_server.sin_family = AF_INET;			// Ordenación de bytes de la máquina (Siempre va AF_INET)
	addr_server.sin_port = htons(port);			// short, Ordenación de bytes de la red
	addr_server.sin_addr.s_addr = INADDR_ANY;	// Rellenar con mi dirección IP
	memset(&(addr_server.sin_zero), '\0', 8);	// Poner a cero el resto de la estructura

	//Asocio socket con puerto, en caso de error finalizo
	if (bind(socket_client, (struct sockaddr *)&addr_server, sizeof(addr_server)) == -1) {
		perror("bind");
		return -1;
	}

	//Pongo a la escucha el socket (en caso de error finalizo)
	if (listen(socket_client, BACKLOG) == -1)
	{
		perror("listen");
		return -1;
	}

	//Elimino procesos muertos
	sa.sa_handler = sigchld_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1)
	{
		perror("sigaction");
		return -1;
	}

	//Preparo argumentos para crear el listen
	args_listenClients *args = malloc(sizeof(args_listenClients));
	args->port = port;
	args->socket_client = socket_client;
	args->fn_newClient = fn_newClient;
	args->fns_receipts = fns;
	args->fn_connectionClosed = fn_connectionClosed;
	args->data = data;

	//Creo el hilo listen
	pthread_t th_listenClient;
	pthread_create(&th_listenClient, NULL, (void*)listenClients, args);

	return 1;
}
