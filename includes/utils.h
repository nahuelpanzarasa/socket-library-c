#ifndef UTILS_H_
#define UTILS_H_
	#include <commons/collections/dictionary.h>
	#include <commons/collections/list.h>

	#define min(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })

	#define max(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })


	unsigned long long getTimeNow();

	char* getFileContentUtils(char* fileName);

	//Dado un archivo devuelve un buffer con el contenido del archivo.
	char* getDataStreamFromFile(char* fileName);

	//Crea un archivo en un path con un contenido data
	void createFile(char * path, char * data);

	// Retorna el tamaño de un fileDescriptor
	int fileSize(int file_descriptor);

	//devuelve el length de un char **
	int charArray_length(char ** array);

	//Libera de memoria un char ** (testeado solo con generados de split)
	void freeCharArray(char ** array);

	//Convierte un array de string a una cadena de string separados por un delimitador especifico
	char* implode(char ** array, char * separator);

	//Converte un array de chars en una lista de chars t_list
	t_list* getListFromCharsArray(char** charsArray);

	//Converte una lista de chars en un array de chars
	char** getCharsArrayFromList(t_list* charsList);

	char* serializeInfoForReduce(t_list* data);

	typedef struct nodoFile
	 {
		 char *ip ;
		 int port;
		 char *fileNameInicial;
	} nodoFile;

	void free_nodoFile(nodoFile* nf);
	//
	//char ** nodoFiles_deserialize (char* str);
	t_list* nodoFiles_deserialize (char* str);

	int getTimestamp();

	//-###############################################################################################-//
	//-###-[SINCRONIZACIÓN]-##########################################################################-//
	//-###############################################################################################-//

	// Inicialización de semaforos y contador
	void initSyn();

	// Comienzo de lectura
	void read_start();

	// Finalizacion de lectura
	void read_end();

	// Comienzo de escritura
	void write_start();

	// Finalizacion de escritura
	void write_end();

#endif
