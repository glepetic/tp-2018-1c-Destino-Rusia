/*
 * funcionesCoordinador.h
 *
 *  Created on: 1 may. 2018
 *      Author: utnso
 */

#ifndef FUNCIONESCOORDINADOR_H_
#define FUNCIONESCOORDINADOR_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/string.h>
#include "../Recursos/estructuras.h"
#include "../Recursos/protocolo.h"
#include "../Biblioteca/biblio_sockets.h"
#include "../Biblioteca/file_cleaner.h"
#include <commons/collections/list.h>

#define LISTA_INSTANCIAS_VACIA -2
#define INSTANCIA_DESCONECTADA -5

typedef struct {
	int socketESI;
} t_argumentos_thESI;

typedef struct {
	int socketPlanificador;
} t_argumentos_thPlanificador;

typedef struct {
	int id;
	char * key;
	char* value;
	char* accion;
} t_solicitud;

typedef struct {
	char* nombre;
	int socketInstancia;
	t_list * claves;
	int entradasOcupadas;
} t_instancia;

pthread_t threadAtencionConsolaPlanificador;
pthread_t threadEscucharConsola;

int coordinador_Puerto_Escucha;
char* coordinador_Algoritmo;
int cantidad_Entradas;
int tamanio_Entrada;
int retardo;
t_log_level T;
t_log_level I;
t_log_level E;
t_log* logT;
t_log* logI;
t_log* logE;

FILE* logOp;

t_argumentos_thPlanificador * argsPlanificador;
t_argumentos_thPlanificador * argsConsolaPlanificador;
pthread_mutex_t mx_logOp;
t_esperar_conexion *esperarConexion;

int proxima_posicion_instancia;
t_list * instancias;
t_list * claves_sin_instancia;

void *esperarConexiones(void *args);
void cargarConfiguracion();
void despachar_solicitud(t_instancia* unaInstancia, t_solicitud una_solicitud);
void cargar_configuracion();
void configureLoggers();
void destroyLoggers();
void* atenderESI(void *args);
void atenderPlanificador(void *args);
void* atenderConsolaPlanificador(void *args);
int recibirMensajeConsolaPlanificador(int socket);
void recibirMensajePlanificador(int socket);
int crearInstancia(int nuevoSocket);
void eliminarInstancia(t_instancia * instancia);
int enviarKey(char * key, int socket );
int enviarValue(char * value, int socket);
int enviarEntradaInstancia(char * key , char * value, t_instancia * instancia);
int bloquearKey(char * key);
void liberar_clave(char * key);
int ejecutarOperacionGET(int socket);
int ejecutar_operacion_set(int socket, char* clave, char * valor);
int ejecutar_operacion_set_instancia(char * key, char * value, t_instancia * instancia);
int ejecutar_operacion_store(int socket);
int ejecutar_operacion_store_instancia(char * key, t_instancia * instancia);
int contieneClaveInstancia(t_instancia * instancia, char * key);
int buscarInstanciaContenedora(char * key, t_instancia ** instancia);
int simularBuscarInstanciaContenedora(char * key, t_instancia** instancia);
int elegirInstancia(t_instancia ** instancia,char * key, bool esSimulacion);
void recibirMensajeESI(int socket);
void generarlogDeOperaciones();
int logueaOperacion(char* operacion, char* key, char* value, int socket);
bool key_creada(char * key);
bool existeInstancia(char* nombreInstancia, t_instancia ** instancia);
int ejecutarAlgoritmoLSU(t_instancia** instancia);
void actualizarEntradasOcupadas(t_instancia *instancia);
t_list* obtenerInstanciasConectadas();
int obtenerValue(char * key, char** value);
void procesarObtenerValorKey(int socket);
void procesarDondeEstaLaClave(int socket);
void informarCompactacion(t_instancia * instancia);
void remover_clave(t_instancia* instancia, char * key);
void enviarClavesAGuardar(t_instancia * instancia);
void exit_gracefully();
int siguiente(int, int);

/***** mini consola del Coordinador *****/
void* escucharConsola();
void * cls();

//funciones a borrar
void simulaEntrada(int socket);


#endif /* FUNCIONESCOORDINADOR_H_ */
