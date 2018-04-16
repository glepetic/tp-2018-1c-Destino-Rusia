#include "esi.h"
#include "../Biblioteca/biblio_sockets.c"

int main(int args, char* argv[]) {

	/* Descomentar mas tarde...
	  if(args != 2){

		log_error(error_log, "Cantidad de parametros incorrectos.");
		exit(0);

	}*/

	ruta_script_ejecuciones = argv[1];

	t_config* config = cargar_configuracion();

	coordinador_socket = 0;

	planificador_socket = 0;

	//Intenta conectarse hasta que se conecta a ambos

	while(coordinador_socket == 0 || planificador_socket == 0){

		if(coordinador_socket == 0) coordinador_socket = conectarseA(coordinador_IP, coordinador_puerto);
		if(planificador_socket == 0) planificador_socket = conectarseA(planificador_IP, planificador_puerto);

	}

	enviarInt(coordinador_socket, ESI);
	enviarInt(planificador_socket, ESI);

	leerScript(ruta_script_ejecuciones);

	config_destroy(config);

	return 0;

}
