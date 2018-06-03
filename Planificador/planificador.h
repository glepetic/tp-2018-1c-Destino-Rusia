#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/string.h>
#include <commons/collections/queue.h>
#include <readline/readline.h> // Para usar readline
#include "../Recursos/estructuras.h"
#include "../Recursos/protocolo.h"
#include "../Biblioteca/biblio_sockets.h"
#include <semaphore.h>

#ifndef PLANIFICADOR_H_
#define PLANIFICADOR_H_


int socketCoordinador;

//semáforos
sem_t* pausarPlanificacion;

//structs
typedef struct {
	int id;
	int fd;
	t_list* clavesTomadas;
	int rafagaEstimada;
	int rafagaActual;
	int tiempoEspera;
} t_proceso_esi;

typedef struct {
	int idProceso;
	char claveValor[LONGITUD_CLAVE];
	t_queue* colaBloqueados;
} t_clave;

//archivo de config y logs
int planificador_Algoritmo;
char* planificador;
char* coordinador_IP;
int estimacion_inicial;
int coordinador_Puerto;
int planificador_Puerto_Escucha;
char** claves_Ini_Bloqueadas;
t_log_level LogL;
t_log* logPlan;

//select y fds
int fdMaxConexionesActivas;
fd_set fdConexiones;

//colas y listas
t_list* listaKeys;
t_queue* colaListos;
t_queue* colaTerminados;

//esi en ejecucion
t_proceso_esi* esi_ejecutando;

//funciones de log y config
void configureLogger();
void cargar_configuracion();

//funciones sockets
int conectarCoordinador();
void *esperarConexiones(void *args);
t_proceso_esi* recibirNuevoESI(int idESI, int fd);
void esperarConexionesESIs(void* esperarConexion);
bool recibirMensajeCliente(int socketCliente);
bool recibirMensajeEsi(int socketCliente);
void recibirMensajeCoordinador(int socketCliente);

//funciones de planificacion
void planificar(void *args);
void moverAListos(t_proceso_esi* procesoEsi);
void ordenarListos();
void quick(t_list* unaLista, int limite_izq, int limite_der);
void actualizarColaListos();
void cambiarEstimado(void* unEsi);
int promedioExponencial(t_proceso_esi* unEsi);
int estimacionHRRN(t_proceso_esi* unEsi);
void enviarMejorEsiAEjecutar();

//funciones de keys
void liberarKeys(t_proceso_esi* esi);
void liberarKey(void* key);

//funciones consola
void * iniciaConsola();
void exit_gracefully(int return_nr);
bool coincideValor(void*);
void block(char*, int);
void unblock(char*);


#endif
