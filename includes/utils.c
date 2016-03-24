#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <commons/collections/dictionary.h>
#include <commons/string.h>
#include <commons/collections/list.h>
#include <commons/collections/queue.h>
#include <pthread.h>
#include <time.h>
#include "utils.h"



char* getFileContentUtils(char* fileName) {

	FILE *fp;
	long fileSize;
	char *buffer;

	//Abro el archivo para lectura
	fp = fopen(fileName, "r");

	if (!fp) {
		printf("El %s archivo no existe", fileName);
		exit(1);
	}

	//Llevo el puntero de archivo al final de archivo y obtengo el tamañp total.
	fseek(fp, 0L, SEEK_END);
	fileSize = ftell(fp);
	//Llevo el puntero nuevamente al comienzo del archivo.
	rewind(fp);

	/* Pido memoria para el tamaño total del archivop */
	buffer = calloc(1, fileSize + 1);
	if (!buffer) {
		fclose(fp);
		printf("Error de alocación de memoria");
		exit(1);
	}

	/* Copio el archivo en el buffer */
	if (1 != fread(buffer, fileSize, 1, fp)) {
		close(fp);
		free(buffer);
		printf("Error de lectura");
		exit(1);
	}

	/* Aca desarrollar la funcioladad específica que desees, el buffer ya contiene
	 todo el texto del archivo */

	fclose(fp);
	return buffer;

}

//Crea un archivo en un path con un contenido data
void createFile(char * path, char * data)
{
	//abro archivo
	FILE *fp;
	fp = fopen(path, "rb+");

	//Si no existe archivo lo creo
	if(fp == NULL)
		fp = fopen(path, "wb");

	//Escribo
	fprintf(fp, "%s", data);

	//Cierro
	fclose(fp);
}

// Retorna el tamaño de un fileDescriptor
int fileSize(int file_descriptor)
{
	struct stat buffer;
	fstat(file_descriptor, &buffer);
	return buffer.st_size;
}

//devuelve el length de un char **
//todo esto funciona realmente?
int charArray_length(char ** array)
{
	int i = 0;
	while(array[i] != NULL)
		i++;
	return i;
}

//Libera de memoria un char ** (testeado solo con generados de split)
void freeCharArray(char ** array)
{
	int i;
	for(i = 0; i < charArray_length(array); i++)
		free(array[i]);
	free(array);
}

//convierte un array de string a una cadena de string separados por un delimitador especifico
char* implode(char ** array, char * separator)
{
	char * string = string_new();
	int i = 0;
	for(i = 0; i < charArray_length(array); i++)
	{
		string_append(&string, array[i]);
		if(i+1 != charArray_length(array))
			string_append(&string, ",");
	}

	return string;

}

char* serializeInfoForReduce(t_list* data){

	char* info = string_new();
	int i = 0;
	nodoFile* file;
	for(i = 0; i < list_size(data); i++){
		file = list_get(data, i);
		char* port = string_itoa(file->port);
		string_append(&info, file->ip);
		string_append(&info, ",");
		string_append(&info, port);
		string_append(&info, ",");
		string_append(&info, file->fileNameInicial);

		free(port);

		if(i + 1 != list_size(data)) //Si no es el último
			string_append(&info, ";");
	}

	return info;
}



//char ** nodoFiles_deserialize (char* str)
t_list* nodoFiles_deserialize (char* str)
{
	//String sin parsear que recibe la función por parámetro
	char ** tmp = string_split(str, ";");

	//char **nodoFiles;
	t_list* nodoFiles;
	int i;
	char **tmp_final;
	nodoFile *nf;

	//nodoFiles = malloc(sizeof(nodoFile)*charArray_length(tmp));
	nodoFiles = list_create();

	for(i = 0; i < charArray_length(tmp); i++)
	{
		tmp_final = string_split(tmp[i], ",");

		nf = malloc(sizeof(nodoFile));
		nf->ip =  string_duplicate(tmp_final[0]);
		nf->port = atoi(tmp_final[1]);
		nf->fileNameInicial = string_duplicate(tmp_final[2]);

		list_add(nodoFiles, nf);
		//nodoFiles[i] = nf;

		free(tmp_final[0]);
		free(tmp_final[1]);
		free(tmp_final[2]);
		free(tmp_final);
	}

	for(i = 0; i < charArray_length(tmp); i++)
		free(tmp[i]);
	free(tmp);

	return nodoFiles;
}

t_list* getListFromCharsArray(char** charsArray){
	t_list* list = list_create();
	int j = 0;
	for(j = 0; j < charArray_length(charsArray); j++){
		list_add(list, string_duplicate(charsArray[j]));
	}
	return list;
}

char** getCharsArrayFromList(t_list* charsList){

	if(list_is_empty(charsList)){
		return NULL;
	}

	char** arrayChar = malloc(sizeof(char*)*list_size(charsList));
	char* single;
	int j = 0;
	for(j = 0; j < list_size(charsList); j++){
		single = list_get(charsList, j);
		arrayChar[j] = string_new();
		string_append(&arrayChar[j], single);

	}

	return arrayChar;
}

void free_nodoFile(nodoFile* nf){
	free(nf->ip);
	free(nf->fileNameInicial);
	free(nf);
}


int getTimestamp()
{
	return time(NULL);
}

//-###############################################################################################-//
//-###-[SINCRONIZACIÓN]-##########################################################################-//
//-###############################################################################################-//

pthread_mutex_t 	mx_write;
pthread_mutex_t 	mx_readCount;
int					readCount;

// Inicialización de semaforos y contador
void initSyn()
{
	pthread_mutex_init(&mx_write, NULL);
	pthread_mutex_init(&mx_readCount, NULL);
	readCount = 0;
}

// Comienzo de lectura
void read_start()
{
	pthread_mutex_lock(&mx_readCount);
	if(readCount++ == 0)
		pthread_mutex_lock(&mx_write);
	pthread_mutex_unlock(&mx_readCount);
}

// Finalizacion de lectura
void read_end()
{
	pthread_mutex_lock(&mx_readCount);
	if(readCount-- == 1)
		pthread_mutex_unlock(&mx_write);
	pthread_mutex_unlock(&mx_readCount);
}

// Comienzo de escritura
void write_start()
{
	pthread_mutex_lock(&mx_write);
}

// Finalizacion de escritura
void write_end()
{
	pthread_mutex_unlock(&mx_write);
}

unsigned long long getTimeNow(){
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (unsigned long long)(tv.tv_sec) * 1000 +
		(unsigned long long)(tv.tv_usec) / 1000;
}
