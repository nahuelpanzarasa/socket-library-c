#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <commons/string.h>
#include <commons/collections/dictionary.h>
#include <string.h>
#include <socket.h>

t_dictionary * 		fns;	/* Funciones de socket */
pthread_mutex_t mx_main;	/* Semaforo de main */
int portServer = 28280;		/* Puerto de escucha */

typedef struct
{
	int a;
	int b;
} data_server;

// Funcion que se produce cuando se conecta un nuevo cliente
void newClient(socket_connection * connection)
{
	printf("Se ha conectado un nuevo cliente. Socket = %d, IP = %s, Puerto = %d.\n", connection->socket, connection->ip, connection->port);
}

// El cliente me saluda
void client_saludar(socket_connection * connection, char ** args)
{
	printf("Cliente (Socket = %d, IP = %s, Puerto = %d) me saluda = %s %s %s.\n", connection->socket, connection->ip, connection->port, args[0], args[1], args[2]);

	//Seteo data e imprimo valores
	data_server * data = connection->data;
	printf("Datos de conexión: a = %d, b = %d.\n", data->a, data->b);

	//devuelvo saludo
	runFunction(connection->socket, "server_saludar", 2, "bien", "gracias");
}

// Funcion que se produce cuando se desconecta un cliente
void client_connectionClosed(socket_connection * connection)
{
	printf("Se ha desconectado un cliente. Socket = %d, IP = %s, Puerto = %d.\n", connection->socket, connection->ip, connection->port);
}

int main(void)
{
	//Diccionario de funciones de comunicacion
	fns = dictionary_create();
	dictionary_put(fns, "client_saludar", &client_saludar);

	//Creo estrucutra de datos para esta conexion
	data_server * data = malloc(sizeof(data_server));
	data->a = 3;
	data->b = 7;

	//creo escucha
	if(createListen(portServer, &newClient, fns, &client_connectionClosed, data) == -1)
	{
		printf("Error al crear escucha en puerto %d.\n", portServer);
		exit(1);
	}
	printf("Escuchando nuevos clientes en puerto %d.\n", portServer);

	/* Se queda esperando */
	pthread_mutex_init(&mx_main, NULL);
	pthread_mutex_lock(&mx_main);
	pthread_mutex_lock(&mx_main);

	return EXIT_SUCCESS;
}
