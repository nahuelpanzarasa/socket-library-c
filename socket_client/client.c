#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <commons/string.h>
#include <commons/collections/dictionary.h>
#include <string.h>
#include <socket.h>

int					socket_server;			/* Socket que apunta a servidor */
char *				ip = "127.0.0.1";		/* IP de server */
int					port = 28280;			/* Puerto de server */
t_dictionary * 		fns;					/* Funciones de socket */
pthread_mutex_t 	mx_main;				/* Semaforo de main */

typedef struct
{
	int x;
	int y;
} data_client;

// El servidor me saluda
void server_saludar(socket_connection * connection, char ** args)
{
	printf("Servidor (Socket = %d, IP = %s, Puerto = %d) me saluda = %s %s.\n", connection->socket, connection->ip, connection->port, args[0], args[1]);

	//Seteo data e imprimo valores
	data_client * data = connection->data;
	printf("Datos de conexión: x = %d, y = %d.\n", data->x, data->y);

	//vuelvo a saludar
	runFunction(socket_server, "client_saludar", 3, "hola", "como", "estas");
}

// Funcion que se produce cuando se desconecta el servidor
void server_connectionClosed(socket_connection * connection)
{
	printf("Se ha desconectado el servidor. Socket = %d, IP = %s, Puerto = %d.\n", connection->socket, connection->ip, connection->port);
	exit(1);
}

int main(void)
{
	//Diccionario de funciones de comunicacion
	fns = dictionary_create();
	dictionary_put(fns, "server_saludar", &server_saludar);

	//Creo estrucutra de datos para esta conexion
	data_client * data = malloc(sizeof(data_client));
	data->x = 2;
	data->y = 9;

	//Me conecto a servidor, si hay error informo y finalizo
	if((socket_server = connectServer(ip, port, fns, &server_connectionClosed, data)) == -1)
	{
		printf("Error al intentar conectar a servidor. IP = %s, Puerto = %d.\n", ip, port);
		exit(1);
	}
	printf("Se ha conectado exitosamente a servidor. Socket = %d, IP = %s, Puerto = %d.\n", socket_server, ip, port);

	//saludo a servidor
	runFunction(socket_server, "client_saludar", 3, "hola", "como", "estas");

	//Dejo bloqueado main
	pthread_mutex_init(&mx_main, NULL);
	pthread_mutex_lock(&mx_main);
	pthread_mutex_lock(&mx_main);

	return EXIT_SUCCESS;
}
