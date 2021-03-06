#include "planificador.h"

int main(){

	//testearDeadlock();

	configureLogger();
	cargar_configuracion();

	//inicializa un semaforo no compartido de valor 0
	sem_init(&productorConsumidor, 0, 0);

	int socketEscucha;
	fd_set fdSocketsEscucha;

	iniciarVariablesGlobales();
	inicializarSemaforos();

	FD_ZERO(&fdSocketsEscucha);
	socketEscucha = escuchar(planificador_Puerto_Escucha);

	FD_ZERO(&fdConexiones);

	FD_SET(socketEscucha, &fdSocketsEscucha);

	conectarCoordinador();
	conectarConsolaACoordinador();

	inicializarColas();

	pthread_t threadEscucharConsola;
	pthread_t threadPlanificar;
	pthread_t threadConexionesNuevas;
	pthread_t threadCoordinador;
	t_esperar_conexion *esperarConexion;

	esperarConexion = malloc(sizeof(t_esperar_conexion));
	esperarConexion->fdSocketEscucha = fdSocketsEscucha;
	esperarConexion->socketEscucha = socketEscucha;

	pthread_create(&threadEscucharConsola,NULL,iniciaConsola,NULL);
	pthread_create(&threadPlanificar, NULL,planificar, NULL);
	pthread_create(&threadConexionesNuevas, NULL,esperarConexionesClientes,(void*) esperarConexion);
	pthread_create(&threadCoordinador, NULL,escucharCoordinador,NULL);

	pthread_join(threadEscucharConsola, NULL);
	pthread_join(threadPlanificar, NULL);
	pthread_join(threadConexionesNuevas, NULL);
	pthread_join(threadCoordinador, NULL);

	free(esperarConexion);

	exit_gracefully(0);

	return 0;
}

void iniciarVariablesGlobales(){
	desalojado = false;
	ordenDeLlegada = 0;
	keySolicitada = malloc(LONGITUD_CLAVE);
	esiBloqueoSistema = malloc(sizeof(t_proceso_esi));
	esiBloqueoSistema->clavesTomadas = list_create();
	esiBloqueoSistema->fd = BLOQUEO_SISTEMA;
	esiBloqueoSistema->id = BLOQUEO_SISTEMA;
	esiBloqueoSistema->rafagaActual = BLOQUEO_SISTEMA;
	esiBloqueoSistema->rafagaEstimada = BLOQUEO_SISTEMA;
	esiBloqueoSistema->tiempoEspera = BLOQUEO_SISTEMA;
}

void inicializarColas(){
	listaKeys = list_create();
	deadlocks = list_create();
	colaListos = queue_create();
	colaTerminados = queue_create();

	cargarKeysBloqueadasIniciales();
}

void cargarKeysBloqueadasIniciales(){

	t_clave* nuevaKey;
	char* clave;
	int i;

	for(i=0; claves_Ini_Bloqueadas[i] != NULL; i++){

		clave = claves_Ini_Bloqueadas[i];

		size_t tamanioClave = strlen(clave);

		if(tamanioClave > LONGITUD_CLAVE){
			log_error(logPlan, "la clave %s es demasiado larga", clave);
			exit_gracefully(-1);
		}
		nuevaKey = malloc(sizeof(t_clave));
		convertirABarra0(nuevaKey->nombre, LONGITUD_CLAVE);
		strncpy(nuevaKey->nombre, clave, tamanioClave);
		nuevaKey->colaBloqueados = queue_create();
		nuevaKey->esi_poseedor = esiBloqueoSistema;
		list_add(esiBloqueoSistema->clavesTomadas, nuevaKey);
		list_add(listaKeys, nuevaKey);

		enviarInt(socketConsolaCoordinador, CREAR_KEY_INICIALMENTE_BLOQUEADA);
		enviarMensaje(socketConsolaCoordinador, clave);

	}


	liberarParametros(claves_Ini_Bloqueadas);


}

void convertirABarra0(char* aConvertir, int longitud){

	int i;

	for(i=0; i<longitud; i++){

		aConvertir[i] = '\0';

	}


}


void inicializarSemaforos(){
	pausarPlanificacion = false;
	comandoConsola = false;
	seQuitoUnEsiDeListos = false;
	conexionEsi = false;
	pthread_mutex_init(&pausarPlanificacionSem, NULL);
	pthread_mutex_init(&iniciarConsolaSem, NULL);
	pthread_mutex_init(&esperarConsolaSem, NULL);
	pthread_mutex_init(&nuevoEsiSem, NULL);
	pthread_mutex_init(&esperarNuevoEsiSem, NULL);
	pthread_mutex_lock(&iniciarConsolaSem);
	pthread_mutex_lock(&esperarConsolaSem);
	pthread_mutex_unlock(&pausarPlanificacionSem);
}

void destruirSemaforos(){
	pthread_mutex_destroy(&pausarPlanificacionSem);
	pthread_mutex_destroy(&iniciarConsolaSem);
	pthread_mutex_destroy(&esperarConsolaSem);
	pthread_mutex_destroy(&esperarNuevoEsiSem);
	pthread_mutex_destroy(&nuevoEsiSem);
}

void * iniciaConsola(){

	log_trace(logPlan,"Se inicializa la consola del planificador.");

	//comandos de consola
	char* exit = "exit";
	char* pausar = "pausar";
	char* continuar = "continuar";
	char* bloquear = "bloquear";
	char* desbloquear = "desbloquear";
	char* listar = "listar";
	char* terminados = "terminados";
	char* status = "status";
	char* kill = "kill";
	char* deadlock = "deadlock";
	char* info = "info";

	char* linea;
	char** parametros;

	while(1) {
		linea = readline("DR-Planificador:" );

		if(linea)
			add_history(linea);

		if(!strncmp(linea, exit, strlen(exit))) {
			log_trace(logPlan,"Consola recibe ""%s""", exit);
			parametros = string_split(linea, " ");
			if(parametros[1] != NULL){
				log_error(logPlan, "La funcion no lleva argumentos.");
			}else{
				exit_gracefully(0);
			}
			free(linea);
		} else if(!strncmp(linea, pausar, strlen(pausar)))
		{
			log_trace(logPlan,"Consola recibe ""%s""", pausar);
			parametros = string_split(linea, " ");
			if(parametros[1] != NULL){
				log_error(logPlan, "La funcion no lleva argumentos.");
			}else{
				pauseScheduler();
			}
			free(linea);

		} else if(!strncmp(linea, continuar, strlen(continuar)))
		{
			log_trace(logPlan,"Consola recibe ""%s""", continuar);
			parametros = string_split(linea, " ");
			if(parametros[1] != NULL){
				log_error(logPlan, "La funcion no lleva argumentos.");
			}else{
				goOn();
			}
			free(linea);

		} else if(!strncmp(linea, bloquear, strlen(bloquear)))
		{
			log_trace(logPlan,"Consola recibe ""%s""", bloquear);
			parametros = string_split(linea, " ");
			if(parametros[1] == NULL){
				log_error(logPlan, "Faltan argumentos: bloquear [clave] [ID]");
			}else{
				if (parametros[2] == NULL){
					log_error(logPlan, "Faltan argumentos: bloquear [clave] [ID]");
				}else{
					if(parametros[3] != NULL){
						log_error(logPlan, "Demasiados argumentos: bloquear [clave] [ID]");
					}else{
						char* key = parametros[1];
						int ESI_ID = atoi(parametros[2]);
						if(pausarPlanificacion || queue_is_empty(colaListos)){
							block(key, ESI_ID);
						}else{
							esperarPlanificador();
							block(key, ESI_ID);
							continuarPlanificador();
						}
					}
				}
			}

			free(linea);

		} else if(!strncmp(linea, desbloquear, strlen(desbloquear)))
		{
			log_trace(logPlan,"Consola recibe ""%s""", desbloquear);
			parametros = string_split(linea, " ");
			if(parametros[1] == NULL){
				log_error(logPlan, "Faltan argumentos: desbloquear [clave]");
			}else{
				if(parametros[2] != NULL){
					log_error(logPlan, "Demasiados argumentos: desbloquear [clave]");
				}else{
					char* key = parametros[1];
					if(pausarPlanificacion || queue_is_empty(colaListos)){
						unblock(key);
					}
					else{
						esperarPlanificador();
						unblock(key);
						continuarPlanificador();
					}

				}
			}
			free(linea);

		} else if(!strncmp(linea, listar, strlen(listar)))
		{
			log_trace(logPlan,"Consola recibe ""%s""", listar);
			parametros = string_split(linea, " ");
			if(parametros[1] == NULL){
				log_error(logPlan, "Faltan argumentos: listar [clave]");
			} else if(parametros[2] != NULL){
				log_error(logPlan, "Demasiados argumentos: listar [clave]");
			}else{
				char* key = parametros[1];
				if(pausarPlanificacion|| queue_is_empty(colaListos)){
					listBlockedProcesses(key);
				}else{
					esperarPlanificador();
					listBlockedProcesses(key);
					continuarPlanificador();
				}
			}

			free(linea);

		}else if(!strncmp(linea, terminados, strlen(terminados))){

			log_trace(logPlan,"Consola recibe ""%s""", terminados);

			parametros = string_split(linea, " ");
			if(parametros[1] != NULL){
				log_error(logPlan, "La funcion no lleva argumentos.");
			}else{
				listFinished();
			}
			free(linea);

		} else if(!strncmp(linea, status, strlen(status)))
		{
			log_trace(logPlan,"Consola recibe ""%s""", status);
			parametros = string_split(linea, " ");
			if(parametros[1] == NULL){
				log_error(logPlan, "Faltan argumentos: status [clave]");
			}else{
				if(parametros[2] != NULL){
					log_error(logPlan, "Demasiados argumentos: status [clave]");
				}else{
					char* key = parametros[1];
					if(pausarPlanificacion|| queue_is_empty(colaListos)){
						getStatus(key);
					}else{
						esperarPlanificador();
						getStatus(key);
						continuarPlanificador();
					}
				}
			}

			free(linea);

		} else if(!strncmp(linea, kill, strlen(kill)))
		{
			log_trace(logPlan,"Consola recibe ""%s""", kill);
			parametros = string_split(linea, " ");
			if(parametros[1] == NULL){
				log_error(logPlan, "Faltan argumentos: kill [id]");
			}else if(parametros[2] != NULL){
				log_error(logPlan, "Demasiados argumentos: kill [id]");
			}else{
				int ESI_ID = atoi(parametros[1]);
				if(pausarPlanificacion|| queue_is_empty(colaListos)){
					matarProceso(ESI_ID);
				}else{
					esperarPlanificador();
					matarProceso(ESI_ID);
					continuarPlanificador();
				}
			}
			free(linea);

		} else if(!strncmp(linea, deadlock, strlen(deadlock)))
		{
			log_trace(logPlan,"Consola recibe ""%s""", deadlock);
			parametros = string_split(linea, " ");
			if(parametros[1] != NULL){
				log_error(logPlan, "La funcion no lleva argumentos.");
			}else if(pausarPlanificacion|| queue_is_empty(colaListos)){
				detectarDeadlock();
			}else{
				esperarPlanificador();
				detectarDeadlock();
				continuarPlanificador();
			}
			free(linea);

		} else if(!strncmp(linea, info, strlen(info)))
		{
			log_trace(logPlan,"Consola recibe ""%s""\n", info);

			parametros = string_split(linea, " ");
			char* help = string_new();

			if(parametros[1] != NULL){
				log_error(logPlan, "La funcion no lleva argumentos.");
			}else{


				string_append(&help, "Destino-Rusia Planificador: Ayuda\n");
				string_append(&help, "Los parámetros se indican con <> \n------\n");
				string_append(&help, "pausar: Pausar planificación : El Planificador no le dará nuevas órdenes de ejecución a ningún ESI mientras se encuentre pausado.\n");
				string_append(&help, "continuar: Continuar planificación : Reanuda el Planificador.\n");
				string_append(&help, "bloquear <clave> <ID>: Se bloqueará el proceso ESI hasta ser desbloqueado, especificado por dicho <ID> en la cola del recurso <clave>..\n");
				string_append(&help, "desbloquear <clave>: Se desbloqueara el proceso ESI con el ID especificado. Solo se bloqueará ESIs que fueron bloqueados con la consola. Si un ESI está bloqueado esperando un recurso, no podrá ser desbloqueado de esta forma.\n");
				string_append(&help, "listar <clave>: Lista los procesos encolados esperando a la clave.\n");
				string_append(&help, "status <clave>: Muestra proceso al que esta asignado, instancia en la que esta guardada la clave o sino existe donde la pondria y los procesos bloqueados por la clave.\n");
				string_append(&help, "terminados: Lista los procesos terminados en orden.\n");
				string_append(&help, "kill <ID>: finaliza el proceso.\n");
				string_append(&help, "deadlock: Analiza los deadlocks que existan en el sistema y a que ESI están asociados.\n");
				string_append(&help, "exit: Sale del programa.\n");

				log_info(logPlan, help);

			}

			free(help);
			free(linea);

		}
		else {
			parametros = string_split(linea, " ");
			log_error(logPlan, "No se reconoce el comando ""%s"".", linea);
			log_info(logPlan, "Para más información utilice el comando ""%s"".", info);
			free(linea);
		}

		liberarParametros(parametros);

	}


}

void liberarParametros(char** parametros){

	int i;

	for(i=0; parametros[i] != NULL; i++){

		free(parametros[i]);

	}

	free(parametros);

}

void configureLogger(){

	LogL = LOG_LEVEL_TRACE;

	/* ejecutar desde ECLIPSE
	vaciarArchivo("../Recursos/Logs/Planificador.log");
	logPlan = log_create("../Recursos/Logs/Planificador.log","Planificador", true, LogL);
	 */


	/* para ejecutar desde CONSOLA
//	vaciarArchivo("../../Recursos/Logs/Planificador.log");
	logPlan = log_create("../../Recursos/Logs/Planificador.log","Planificador", true, LogL);
*/



	/* para ejecutar desde la VM Server*/
//	vaciarArchivo("Planificador.log");


	logPlan = log_create("Planificador.log","Planificador", true, LogL);


	log_trace(logPlan, "inicializacion de logs");

}

void cargar_configuracion(){


	/* para correr desde la VM Server*/


	infoConfig = config_create("planificador.config");


	if(config_has_property(infoConfig, "PUERTO_ESCUCHA")){
		planificador_Puerto_Escucha = config_get_int_value(infoConfig, "PUERTO_ESCUCHA");
		log_debug(logPlan, "puerto que escucha %d", planificador_Puerto_Escucha);
	}

	if(config_has_property(infoConfig, "ALGORITMO")){
		planificador = config_get_string_value(infoConfig, "ALGORITMO");

		if(strcmp(planificador, "SJF_SIN_DESALOJO") == 0){
			planificador_Algoritmo = SJF_SIN_DESALOJO;
		}
		else if(strcmp(planificador, "SJF_CON_DESALOJO") == 0){
			planificador_Algoritmo = SJF_CON_DESALOJO;
		}
		else if(strcmp(planificador, "HRRN") == 0){
			planificador_Algoritmo = HRRN;
		}else{
			log_error(logPlan, "ALGORITMO NO RECONOCIDO, SE SETEO %s", planificador);
			exit(-1);

		}
		log_debug(logPlan, "algoritmo seleccionado %s", planificador);

	}

	if(config_has_property(infoConfig, "ESTIMACION_INICIAL")){
		estimacion_inicial = config_get_int_value(infoConfig, "ESTIMACION_INICIAL");
		log_debug(logPlan, "estimacion inicial %d", estimacion_inicial);
	}

	if(config_has_property(infoConfig, "ALFA")){
		alfa = config_get_int_value(infoConfig, "ALFA") / 100.0;
		if(alfa > 1 || alfa < 0){
			log_error(logPlan, "Ingresar un valor entre 0 y 100");
			exit(-1);
		}
		log_debug(logPlan, "alfa %f", alfa);
	}

	if(config_has_property(infoConfig, "IP_COORDINADOR")){
		coordinador_IP = config_get_string_value(infoConfig, "IP_COORDINADOR");
		log_debug(logPlan, "IP del coordinador %s", coordinador_IP);
	}

	if(config_has_property(infoConfig, "PUERTO_COORDINADOR")){
		coordinador_Puerto = config_get_int_value(infoConfig, "PUERTO_COORDINADOR");
		log_debug(logPlan, "puerto del coordinador %d", coordinador_Puerto);
	}

	if(config_has_property(infoConfig, "CLAVES_INI_BLOQUEADAS")){
		claves_Ini_Bloqueadas = config_get_array_value(infoConfig, "CLAVES_INI_BLOQUEADAS");
		char* msj = string_new();
		string_append(&msj, "claves inicialmente bloqueadas: ");
		int i = 0;
		while(claves_Ini_Bloqueadas[i] != NULL){
			string_append_with_format(&msj, "%s", claves_Ini_Bloqueadas[i]);
			i++;
			if(claves_Ini_Bloqueadas[i] != NULL){
				string_append(&msj, ", ");
			}else{
				string_append(&msj, ".");
			}

		}

		log_warning(logPlan, msj);
		free(msj);


	}

}

void exit_gracefully(int return_nr) {

	void castearYDestruirLista(void* parametro){

		t_list* lista = (t_list*) parametro;

		list_destroy_and_destroy_elements(lista, eliminarEsi);

	}

	sem_destroy(&productorConsumidor);
	destruirSemaforos();
	liberarEsiEnEjecucion();
	quitarBloqueoSistema();
	free(keySolicitada);
	queue_destroy_and_destroy_elements(colaListos, eliminarEsi);
	queue_destroy_and_destroy_elements(colaTerminados, eliminarEsi);
	list_destroy_and_destroy_elements(listaKeys, eliminarKey);
	list_destroy_and_destroy_elements(deadlocks, castearYDestruirLista);
	log_info(logPlan, "planificador finalizado");
	log_destroy(logPlan);
	config_destroy(infoConfig);
	exit(return_nr);
}

void liberarEsiEnEjecucion(){
	if(esi_ejecutando != NULL) eliminarEsi(esi_ejecutando);
}

void eliminarEsi(void* elemento){
	t_proceso_esi* esi = (t_proceso_esi*) elemento;
	list_destroy(esi->clavesTomadas);
	free(esi);
}

void eliminarKey(void* elemento){
	t_clave* key = (t_clave*) elemento;
	queue_destroy_and_destroy_elements(key->colaBloqueados, eliminarEsi);
	key->esi_poseedor=NULL;
	free(key);
}

