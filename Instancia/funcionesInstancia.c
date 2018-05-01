/*
 * funcionesInstancia.c
 *
 *  Created on: Apr 27, 2018
 *      Author: utnso
 */
#include "funcionesInstancia.h"


int  almacenarEntrada(char key[40], FILE* archivoDatos, void * value){

	t_entrada * entrada = malloc(sizeof(t_entrada));

	strcpy(entrada->key,key);
	entrada->entry = escribirEntrada(entrada,archivoDatos, value);/* numero de entrada */
	entrada->size = strlen(value); /* largo de value */

	list_add(tablaEntradas,entrada);

	return 1;
}

void eliminarEntrada(char * key){
	bool* findByKey(void* parametro) {
		t_entrada* entrada = (t_entrada*) parametro;
		return (strcmp(entrada->key,key));
	}

	t_entrada * entrada =(t_entrada *) list_remove_by_condition(entradas,findByKey);
	free(entrada);
}

FILE* inicializarPuntoMontaje(char * path, char * filename){
	/*creo carpeta de Montaje*/
	int status = mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	char* nuevoArchivo = string_new();

	if (status < 0){
		/*informar error en la creación de la carpeta y salir de manera ordenada*/
		log_error(logE, "Fallo al generar el archivo .dat de la instancia.");
		exit(EXIT_FAILURE);
	}

	string_append(&nuevoArchivo, path);
	string_append(&nuevoArchivo, filename);
	string_append(&nuevoArchivo, ".dat");

	FILE* instanciaDat = fopen(nuevoArchivo,"rw");
	if (instanciaDat == NULL){
			log_error(logE, "Fallo al generar el archivo .dat de la instancia.");
			exit(EXIT_FAILURE);
	}

	int fd = fileno(instanciaDat);
	fallocate(fd,0,0,qEntradas*tamanioEntrada); /* se alloca la memoria a mapear */

	return instanciaDat;
}


int escribirEntrada(t_entrada * entrada, FILE* archivoDatos, void * escribir){

	unsigned char* map;

	int numEntrada = 0; /* ToDo: depende del algoritmo la asignación de la entrada a utilizar*/

	int data = fileno(archivoDatos);

	map = (unsigned char*) mmap(NULL, qEntradas * tamanioEntrada , PROT_READ | PROT_WRITE, MAP_SHARED, data, sizeof(unsigned char)*numEntrada*tamanioEntrada);

	if (map == MAP_FAILED){
		close(data);
		log_error(logE,"Error en el mapeo de instancia.dat.\n");
		exit(EXIT_FAILURE);
	   }

	int i = 0;

	for (;i<strlen(escribir);i++){
			map[i]=escribir[i];
	}

	int entradasOcupadas = strlen(escribir)/tamanioEntrada;

	if (strlen(escribir) % tamanioEntrada > 0){
		log_trace(logT,"Se escribió con exito sobre la entrada %d y con un total de %d entradas.", numEntrada, entradasOcupadas + 1);
	} else {
		log_trace(logT,"Se escribió con exito sobre la entrada %d y con un total de %d entradas.", numEntrada, entradasOcupadas);

	}
	munmap(map,qEntradas * tamanioEntrada);

	//free(bloqueArchivo);
	close(data);

}

void * recibirValue(socketConn){

		int largoMensaje = 0;
		int bytesRecibidos = 0;
		int recibido = 0;

		recibirInt(socketConn,&largoMensaje);

		char * bloqueArchivo;
			bloqueArchivo = malloc((size_t)largoMensaje);

		while(recibido<largoMensaje){
		int bytesAleer = 0;
		recibirInt(socketConn,&bytesAleer);
			while(bytesRecibidos<bytesAleer){
						bytesRecibidos += recv(socketConn,bloqueArchivo,(size_t)bytesAleer-bytesRecibidos,NULL);

			}
			recibido += bytesRecibidos;
			bytesRecibidos = 0;

		}
		if(recibido <= 0){
			log_error(logT,"no se recibio nada del socket %d",socketConn);
			return -1;

		}
		return bloqueArchivo;
}


void configureLoggers(char* instName){

	T = LOG_LEVEL_TRACE;
	I = LOG_LEVEL_INFO;
	E = LOG_LEVEL_ERROR;

	char* logPath = string_new();
	string_append(logPath,"../Logs/");
	string_append(logPath,instName);
	string_append(logPath,".log");

	logT = log_create("../Logs/ESI.log","ESI", false, T);
	logI = log_create("../Logs/ESI.log", "ESI", false, I);
	logE = log_create("../Logs/ESI.log", "ESI", true, E);

	/* 	free(logPath); SE LIBERA LA MEMORIA DE LAS CADENAS ARMADAS CON LAS COMMONS?*/
}

void destroyLoggers(){
	log_destroy(logT);
	log_destroy(logI);
	log_destroy(logE);
}

