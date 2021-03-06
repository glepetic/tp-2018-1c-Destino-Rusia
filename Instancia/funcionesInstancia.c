#include "funcionesInstancia.h"

int calcularSiguienteEntrada(int lenValue, t_entrada ** entrada, int socket){

	int seleccionarAlgoritmo(){
		int pos_aux;
		switch (reemplazo_Algoritmo){
			case CIRCULAR  :
			  pos_aux = calculoCircular(lenValue, entrada);
			  break;
			case LRU  :
			  pos_aux = calculoLRU(lenValue, entrada);
			  break;
			case BSU  :
			  pos_aux = calculoBSU(lenValue,entrada);
			  break;
			default:
			  pos_aux = reemplazo_Algoritmo;
			  close_gracefully();
			  exit(1);
		}
		return pos_aux;
	}

	*entrada = NULL;
	int pos = 0;
	int n = calculoCantidadEntradas(lenValue);
	pos = findNFreeBloques(t_inst_bitmap, n);

	if(pos==-1 && socket!=0){
		int bloques_libres = cuentaBloquesLibre(t_inst_bitmap);
		if(bloques_libres>= n){
			log_trace(logT,"No hay %d bloques contiguos, es necesario compactar",n);
			if(enviarInt(socket,COMPACTACION)<=0){
				log_error(logE,"Error al enviar mensaje de compactacion al coordinador");
			}
			if(compactar()){
				return findNFreeBloques(t_inst_bitmap, n);
			}else{
				return -1;
			}
		}else{
			log_error(logE,"No hay %d bloques libres, se buscara reemplar entrada",n);
			int resultado = seleccionarAlgoritmo();

			if(n == 1){
				return -1;
			}
			if(bloques_libres+resultado>=n){
				eliminarEntrada((*entrada)->key);
				if(enviarInt(socket,COMPACTACION)<=0){
					log_error(logE,"Error al enviar mensaje de compactacion al coordinador");
				}
				if(compactar()){
					return findNFreeBloques(t_inst_bitmap, n);
				}else{
					return -1;
				}
			}else{

			}
		}
	}
	return pos;
}


int  almacenarEntrada(char * key, t_entrada * entrada, int largoValue){

	strcpy(entrada->key,key);
	entrada->size = largoValue;

	int i;
	int entradas_ocupadas = calculoCantidadEntradas(entrada->size);
	for(i=0;i<entradas_ocupadas;i++){
		bitarray_clean_bit(t_inst_bitmap,entrada->entry+i);
	}
	entrada->ultimaRef = operacionNumero;

	return 1;
}

bool obtenerEntrada(char * key,t_entrada ** entrada){

	bool retorno = false;
	bool findByKey(void* parametro) {
		t_entrada* entrada_aux = (t_entrada*) parametro;
		if(strcmp(entrada_aux->key,key) == 0){
			retorno = true;
		}
		return retorno;
	}

	t_entrada *otra_entrada =(t_entrada *) list_find(tablaEntradas,findByKey);
	if(retorno){
		*entrada = otra_entrada;
	}

	return retorno;
}

void eliminarEntrada(char * key){
	bool findByKey(void* parametro) {
		t_entrada* entrada = (t_entrada*) parametro;
		return (strcmp(entrada->key,key)==0);
	}

	t_entrada * entrada =(t_entrada *) list_remove_by_condition(tablaEntradas,findByKey);
	int bloques = calculoCantidadEntradas(entrada->size);
	int i;
	for(i=0;i<bloques;i++){
		bitarray_clean_bit(t_inst_bitmap,entrada->entry+i);
	}
	free(entrada);
}

void inicializarPuntoMontaje(char * path, char * filename){
	/*creo carpeta de Montaje*/
	int status = mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	char* nuevoArchivo = string_new();

	if (status < 0 && (errno != EEXIST)){
		/*informar error en la creación de la carpeta y salir de manera ordenada*/
		log_error(logE, "Fallo al generar el archivo .dat de la instancia.");
		exit(EXIT_FAILURE);
	}

	string_append(&nuevoArchivo, path);
	string_append(&nuevoArchivo, filename);
	string_append(&nuevoArchivo, ".dat");

	FILE* instanciaDat = fopen(nuevoArchivo,"w+");
	if (instanciaDat == NULL){
			log_error(logE, "Fallo al generar el archivo .dat de la instancia.");
			exit(EXIT_FAILURE);
	}

	free(nuevoArchivo);
	int fd = fileno(instanciaDat);
	posix_fallocate(fd,0,qEntradas*tamanioEntrada);
	fclose(instanciaDat);
}

int archivoAentrada(char* filename){

	FILE* arch;
	t_entrada* entrada = NULL;
	char* filePath = string_new();
	char* value = string_new();

	string_append(&filePath, punto_Montaje);
	string_append(&filePath, filename);

	arch = fopen(filePath,"r");

	char * line = NULL;
	size_t len = 0;

	while(!feof(arch)){
		getline(&line,&len,arch);
		string_append(&value, line);
	}

	int lenValue = strlen(value)+1;

	int pos = calcularSiguienteEntrada(lenValue, &entrada, 0);

	if(pos<0 && entrada == NULL ){
		return -1;
	}else if(entrada == NULL){
		entrada = malloc(sizeof(t_entrada));
		entrada->entry = pos;
		list_add(tablaEntradas,entrada);
	}

	almacenarEntrada(filename,entrada,lenValue);

	escribirEntrada(value, pos, nombre_Instancia);

	fclose(arch);
	free(line);
	free(filePath);
	free(value);

	return 1;

}

int reviewPuntoMontaje(t_list * whitelist){

	size_t count = 0;
	struct dirent *res;
	struct stat sb;
	const char *path = punto_Montaje;

	char* instancia = string_new();

	string_append(&instancia, nombre_Instancia);
	string_append(&instancia, ".dat");

	if (stat(path, &sb) == 0 && S_ISDIR(sb.st_mode)){
		DIR *folder = opendir (path);

		if (access (path, F_OK) != -1 ){
			if (folder){
				while ((res = readdir(folder))){

					bool _esKey(void *key) {
						return strcmp((char *)key,res->d_name)==0;
					}

					if (list_any_satisfy(whitelist,_esKey)){
						archivoAentrada(res->d_name);
						count++;
					}
				}
				closedir ( folder );
			}else{
				log_error(logE, "Error al acceder al punto de montaje.");
				exit(EXIT_FAILURE);
			}
		}

	}else{
		printf("La ruta %s no puede ser abierta o no es un directorio.\n", path);
		exit( EXIT_FAILURE);
	}

	free(instancia);
	return count;
}

int abrirArchivoDatos(char * path, char * filename){

	char* archivoDat = string_new();

	string_append(&archivoDat, path);
	string_append(&archivoDat, filename);
	string_append(&archivoDat, ".dat");
	int instanciaDat = open(archivoDat,O_RDWR,O_APPEND);
	if (instanciaDat < 0){
			log_error(logE, "Fallo al abrir el archivo .dat de la instancia.");
	}

	free(archivoDat);

	return instanciaDat;

}


void escribirEntrada(char * escribir, int pos, char * nombre_archivo){

	unsigned char* map;

	int data = abrirArchivoDatos(punto_Montaje, nombre_archivo);

	map = (unsigned char*) mmap(NULL, qEntradas * tamanioEntrada , PROT_READ | PROT_WRITE, MAP_SHARED, data, 0); // sizeof(unsigned char)*numEntradaActual*tamanioEntrada

	if (map == MAP_FAILED){
		close(data);
		log_error(logE,"Error en el mapeo de instancia.dat.\n");
		exit(EXIT_FAILURE);
	   }

	int lenValue = strlen(escribir);
	if(lenValue % tamanioEntrada != 0){
		lenValue++;
	}
	int exactPos = pos*tamanioEntrada;


	/*for(b=0;b<lenValue;b++){
		map[exactPos+b] = escribir[b];
	}
	*/

	memcpy(map+exactPos,escribir,lenValue);

	int entradasOcupadas = calculoCantidadEntradas(lenValue);

	int i = pos;
	for(;i < pos+entradasOcupadas;i++){
		bitarray_set_bit(t_inst_bitmap,i);
	}


	log_trace(logT,"Se escribió con exito sobre la entrada %d y con un total de %d entradas. Valor:%s", pos, entradasOcupadas,escribir);

	munmap(map,qEntradas * tamanioEntrada);

	close(data);

}

int recibirValue(int socketConn, char** value){

		*value = recibirMensajeArchivo(socketConn);

		if(strcmp(*value,"-1") == 0){
			return -1;
		}
		return 1;
}

int recibirKey(int socket, char ** key){


	/*
	int size = LONGITUD_CLAVE;
	*key = malloc(size);

		int largoLeido = 0, llegoTodo = 0, totalLeido = 0;
	while(!llegoTodo){
			largoLeido = recv(socket, *key + totalLeido, size, 0);

			//toda esta fumada es para cuando algun cliente se desconecta.
			if(largoLeido == -1){
				printf("Socket dice: Cliente en socket N° %d se desconecto\n", socket);
				log_error(logT,"no se recibio nada del socket %d",socket);
			}

			totalLeido += largoLeido;
			size -= largoLeido;
			if(size <= 0) llegoTodo = 1;
		}
 	 */
	*key = recibirMensajeArchivo(socket);
	if(strcmp(*key,"-1")==0){
		return -1;
	} else {
		return strlen(*key);
	}

}




/********** OPERACION SET ************/
int recibirEntrada(int socket){

	char * key;
	char * value;
	//value = string_new();
	if(recibirKey(socket,&key)<=0){
		return -1;
	}
	if(recibirValue(socket,&value)<=0){
		return -1;
	}

	int lenValue = strlen(value);
	int mod = lenValue % tamanioEntrada;
	if( mod != 0){
		lenValue++;;
	}
	int entradasAOcupar = calculoCantidadEntradas(lenValue);

	t_entrada* entrada = NULL;
	bool encontro = obtenerEntrada(key,&entrada);
	if(!encontro){
		int pos = calcularSiguienteEntrada(lenValue, &entrada, socket);

			if(pos<0 && entrada == NULL){
				return pos;
			}else if(entrada == NULL){
				entrada = malloc(sizeof(t_entrada));
				entrada->entry = pos;
				list_add(tablaEntradas,entrada);
			}
	}else{
		int entradasOcupadas = calculoCantidadEntradas(entrada->size);
		if(entradasAOcupar>entradasOcupadas){
			int pos = findNFreeBloques(t_inst_bitmap,entradasAOcupar - entradasOcupadas);
			if(pos != entradasOcupadas + entrada->entry + 1){
				log_error(logE,"No se puede reemplazar entrada");
				free(key);
				free(value);
				return -1;
			}
		}else{
			int i;
			for(i=0;i<entradasOcupadas;i++){
				bitarray_clean_bit(t_inst_bitmap,i+entrada->entry);
			}
		}
	}

	almacenarEntrada(key, entrada, lenValue);
	escribirEntrada(value, entrada->entry, nombre_Instancia);

	free(key);
	free(value);

	return entradasAOcupar;

}
/******** FIN OPERACION SET **********/


/******** OPERACION STORE **********/
int ejecutarStore(int coordinador_socket){
		char * key;
		if(recibirKey(coordinador_socket,&key)<=0){
			log_trace(logE, "error al recibir clave para persistir");
			return -1;
		}else{
			if(persistir_clave(key)<=0){
				free(key);
				log_trace(logE, "error al persistir clave");
				return -1;
			}
		}
		free(key);
		return 1;
}


int persistir_clave(char *key){


	char* path_final = string_new();

	string_append(&path_final, punto_Montaje);
	string_append(&path_final, key);

	FILE* keyStore = fopen(path_final,"w+");

	if (keyStore == NULL){
			log_error(logE, "Fallo al generar el STORE de la key %s.", key);
			return -1;
	}

	t_entrada* entradaElegida;

	if(!obtenerEntrada(key,&entradaElegida)){
		log_error(logE,"No se encontro la clave %s",key);
		return -1;
	}

	entradaElegida->ultimaRef = operacionNumero;

	char * value;
	leer_entrada(entradaElegida, &value);

	fprintf(keyStore,"%s", value);

	fclose(keyStore);

	free(value);

	free(path_final);
	return 1;
}


void leer_entrada(t_entrada* entrada, char** value){

	int data = abrirArchivoDatos(punto_Montaje,nombre_Instancia);
	struct stat fileStat;
	if (fstat(data, &fileStat) < 0){
		log_error(logE,"Error fstat --> %s");
		exit(EXIT_FAILURE);
	}

	unsigned char* map = (unsigned char*) mmap(NULL, qEntradas * tamanioEntrada , PROT_READ | PROT_WRITE, MAP_SHARED, data, 0);

	if (map == MAP_FAILED){
		close(data);
		log_error(logE,"Error en el mapeo del archivo.dat.\n");
		exit(EXIT_FAILURE);
	   }


	int exactPos = entrada->entry*tamanioEntrada;
	*value = malloc(entrada->size);

	 memcpy(*value,map+exactPos,entrada->size);
	log_trace(logT,"Se leyó con exito el value de la clave %s.", entrada->key);
	munmap(map,fileStat.st_size);
	close(data);

}

/********* FIN OPERACION STORE *********/


void configureLoggers(char* instName){

	T = LOG_LEVEL_TRACE;
	I = LOG_LEVEL_INFO;
	E = LOG_LEVEL_ERROR;

	char* logPath = string_new();

	/* para correr desde ECLIPSE

	string_append(&logPath,"../Recursos/Logs/");*/



	/* para correr desde CONSOLA
	string_append(&logPath,"../../Recursos/Logs/");
 */

	/* para correr en la VM Server
	string_append(&logPath,"");*/


	string_append(&logPath,instName);
	string_append(&logPath,".log");


	logT = log_create(logPath,"Instancia", true, T);
	logI = log_create(logPath, "Instancia", true, I);
	logE = log_create(logPath, "Instancia", true, E);


	 free(logPath);
}

void destroyLoggers(){
	log_destroy(logT);
	log_destroy(logI);
	log_destroy(logE);
}

int algoritmoR(char* algoritmo){
	int value;

	if(!strncmp(algoritmo, "CIRCULAR", 8)){
		value = CIRCULAR;
	} else if (!strncmp(algoritmo, "LRU", 3)){
		value = LRU;
	} else if (!strncmp(algoritmo, "BSU", 3)){
		value = BSU;
	} else {value = -1;}

	return value;
}


int calculoLRU(int bloques, t_entrada ** entrada){

	int menos_usado = -1;;
	int min_size = INT_MAX;
	void buscarMenosUsado(void* parametro) {

		t_entrada* entrada_aux = (t_entrada*) parametro;
		int bloques = calculoCantidadEntradas(entrada_aux->size);
		if(min_size > bloques){
			min_size = bloques;
		}

		if(menos_usado < (operacionNumero - entrada_aux->ultimaRef)
				&& (1 == bloques)){
			menos_usado = operacionNumero - entrada_aux->ultimaRef;
			*entrada = entrada_aux;
		}
	}

	list_iterate(tablaEntradas,buscarMenosUsado);

	if(menos_usado == -1){
		if(min_size > 1){
			return SIN_ENTRADA;
		}
		return -1;
	}else{
		return 1;
	}


}

int calculoBSU(int bloques, t_entrada ** entrada){

	int mayor_tamanio= -1;
	int size = list_size(tablaEntradas);
	int i;
	int min_size = INT_MAX;
	for(i=0;i<size;i++){
		t_entrada* entrada_aux = list_get(tablaEntradas, i);
		int bloques_entrada = calculoCantidadEntradas(entrada_aux->size);
		if(min_size > bloques_entrada){
			min_size = bloques_entrada;
		}
		if(entrada_aux->size>mayor_tamanio && bloques_entrada == 1){
			mayor_tamanio = entrada_aux->size;
			*entrada = entrada_aux;
		}
	}
	if(min_size > 1){
					return SIN_ENTRADA;
				}
	return mayor_tamanio;


}
int calculoCircular(int bloques, t_entrada ** entrada){
	int posInicial = numEntradaActual;
	int min_size = INT_MAX;
	while(1){
		bool encontro = false;

		bool entradaConEntry(void* parametro){

			t_entrada * entrada_aux = (t_entrada*) parametro;
			int bloques = calculoCantidadEntradas(entrada_aux->size);
			if(min_size > bloques){
				min_size = bloques;
			}
			encontro =  (entrada_aux->entry == numEntradaActual);
			return encontro;
		}

		t_entrada* entrada_aux = list_find(tablaEntradas,entradaConEntry);
		numEntradaActual++;
		if(encontro){
			if(calculoCantidadEntradas(entrada_aux->size) == 1){
				*entrada = entrada_aux;
				return 1;
			}
		}
		if(numEntradaActual > list_size(tablaEntradas)){
			numEntradaActual = 0;
		}
		if(numEntradaActual == posInicial){
			if(min_size > 1){
				return SIN_ENTRADA;
			}
			return -1;
		}
	}


}

int calculoCantidadEntradas(int length){

	int qEntradas = length / tamanioEntrada;
	if(length % tamanioEntrada > 0){
		qEntradas++;
	}
	return qEntradas;
}

void cargar_configuracion(){

	t_config* infoConfig;

	/* SI SE CORRE DESDE ECLIPSE
	infoConfig = config_create("../Recursos/Configuracion/instancia.config");

 */
	/* SI SE CORRE DESDE CONSOLA
	infoConfig = config_create("../../Recursos/Configuracion/instancia.config");
*/

	/* SI SE CORRE EN LA VM SERVER */

	infoConfig = config_create("instancia.config");

	if(config_has_property(infoConfig, "IP_COORDINADOR")){
		coordinador_IP = string_from_format("%s",config_get_string_value(infoConfig, "IP_COORDINADOR"));


	}

	if(config_has_property(infoConfig, "PUERTO_COORDINADOR")){
		coordinador_Puerto = config_get_int_value(infoConfig, "PUERTO_COORDINADOR");
	}

	if(config_has_property(infoConfig, "ALGORITMO")){

		reemplazo_Algoritmo = algoritmoR(config_get_string_value(infoConfig, "ALGORITMO"));

	}

	if(config_has_property(infoConfig, "PUNTO_MONTAJE")){
		punto_Montaje = string_from_format("%s",config_get_string_value(infoConfig, "PUNTO_MONTAJE"));
	}

	if(config_has_property(infoConfig, "NOMBRE")){
		nombre_Instancia = string_from_format("%s",config_get_string_value(infoConfig, "NOMBRE"));


	}

	if(config_has_property(infoConfig, "INTERVALO_DUMP")){
		intervalo_dump = config_get_int_value(infoConfig, "INTERVALO_DUMP");

	}

	config_destroy(infoConfig);
}


/* ********** FUNCIONES BITMAP ********** */

t_bitarray* creaAbreBitmap(char* nombre_Instancia){

	int bloquesEnBits = qEntradas; // (tamanioEntrada / (1024*1024)) ;

	if(bloquesEnBits % 8 == 0)
	{
		bloquesEnBits = bloquesEnBits/8;
	}else{
		bloquesEnBits = bloquesEnBits/8 + 1;
	}


	t_bitarray* t_fs_bitmap;

	t_fs_bitmap = crearBitmapVacio(tamanioEntrada);

	return t_fs_bitmap;
}

t_bitarray *crearBitmapVacio() {
	int cantBloq = qEntradas;
	size_t bytes = ROUNDUP(cantBloq, CHAR_BIT);
	char *bitarray = calloc(bytes, sizeof(char));
	return bitarray_create_with_mode(bitarray, bytes, LSB_FIRST);
}


int findFreeBloque(t_bitarray* t_fs_bitmap){

	int bloques = qEntradas;
	int pos = 0, i = 0;
	for (i = 0; i < bloques; i++) {
		if(bitarray_test_bit(t_fs_bitmap, i) == 0){
			pos = i;
			break;
		}
	}
	return pos;
}

int findNFreeBloques(t_bitarray* t_fs_bitmap, int n){

	int bloques = qEntradas;
	int pos = -1, i = 0, j = 0;
	for (i = 0; i < bloques; i++) {
		if(bitarray_test_bit(t_fs_bitmap, i) == 0){
			j++;
			if(j == n){
				pos = i-j + 1;
				break;
			}
		}else{
			j = 0;
		}

	}

	return pos;
}

int cuentaBloquesLibre(t_bitarray* t_fs_bitmap){

	int bloques = qEntradas; // (tamanioEntrada / (1024*1024)) ;

	int libre = 0;
	int i = 0;
	for (; i < bloques; i++) {
		if(!bitarray_test_bit(t_fs_bitmap, i)){
			libre++;
		}
	}
	return libre;
}

int cuentaBloquesUsados(t_bitarray* t_fs_bitmap){

	int bloques = qEntradas; // (tamanioEntrada / (1024*1024)) ;

	int usado = 0;
	int i = 0;
		for (; i < bloques; i++) {
			 if(bitarray_test_bit(t_fs_bitmap, i)){
					usado++;
			}
		}
	return usado;
}

t_bitarray *limpiar_bitmap(char* nombre_Instancia, t_bitarray* bitmap) {
	memset(bitmap->bitarray, 0, bitmap->size);
	return bitmap;
}

void destruir_bitmap(t_bitarray* bitmap) {
	free(bitmap->bitarray);
	bitarray_destroy(bitmap);
}
/* ******** FIN FUNCIONES BITMAP ******** */

/* ******** PARA STATUS ******** */

int entregarValue(int socket){
	char * key = recibirMensajeArchivo(socket);
	t_entrada* entrada;
	if(!obtenerEntrada(key,&entrada)){
		log_error(logE,"no se encontro entrada con la key %s",key);
		return enviarInt(socket, CLAVE_INEXISTENTE);
	}
	entrada->ultimaRef = operacionNumero;
	char * value;

	leer_entrada(entrada,&value);
	if(enviarInt(socket,CLAVE_ENCONTRADA)<=0){
			log_error(logE,"error al enviar el valor de la clave %s al coordinador",key);
			return -1;
	}
	if(enviarMensaje(socket,value)<=0){
		log_error(logE,"error al enviar el valor de la clave %s al coordinador",key);
		return -1;
	}
	free(value);
	return 1;

}


/* ******** FIN STATUS ******** */

/* ******** COMPACTACION Y DUMP ******** */

bool compactar(){

	bool ejecucion_ok = true;
	char * nombre_archivo = string_new();
	string_append(&nombre_archivo, "compact");
	inicializarPuntoMontaje(punto_Montaje,nombre_archivo);

	int size = list_size(tablaEntradas);
	int i;
	int pos = 0;
	for(i=0;i<size;i++){
		t_entrada * entrada = (t_entrada*)list_get(tablaEntradas,i);
		char * value;
		leer_entrada(entrada,&value);
		escribirEntrada(value,pos,nombre_archivo);

		entrada->entry=pos;
		int len = strlen(value);
		if(len%tamanioEntrada !=0){
			len++;
		}
		pos += calculoCantidadEntradas(strlen(value)+1);
		free(value);
	}

	char * punto_montaje = string_from_format("%s%s.dat",punto_Montaje,nombre_Instancia);
	char * nuevo_nombre_archivo = string_from_format("%s%s.dat",punto_Montaje,nombre_archivo);

	if(rename(nuevo_nombre_archivo,punto_montaje)<0){
		ejecucion_ok = false;
		log_error(logE, "no se pudo realizar compactacion ya que no es posible reemplazar el punto de montaje");
	}else{
		limpiar_bitmap(nombre_Instancia,t_inst_bitmap);
		int a;
		for(a=0;a<pos;a++){
			bitarray_set_bit(t_inst_bitmap,a);
		}
	}
	free(nuevo_nombre_archivo);
	free(punto_montaje);
	free(nombre_archivo);

	return ejecucion_ok;
}

void* dump(){

	int size;
	int i,e;
	t_entrada* actual;

	while(1){

		sleep(intervalo_dump);

		pthread_mutex_lock(&mx_Dump);

		log_trace(logT,"Orden de efectuar DUMP recibida.");

		size = list_size(tablaEntradas);
		e = 0;

		for(i = 0;i<size;i++){
			actual = list_get(tablaEntradas,i);

			if(persistir_clave(actual->key)<0){
				log_error(logE,"Error al efectuar DUMP de la key %s.",actual->key);
				e++;
			}
		}

		log_trace(logT,"Dump completado. Se generaron %d archivos. Errores: %d.",i-e,e);


		pthread_mutex_unlock(&mx_Dump);

	}
}

/* ******** FIN COMPACT Y DUMP ******** */


t_list * recibirClavesAMantener(int coordinador_socket){
	bool finalizar = false;
	t_list* list = list_create();
	while(!finalizar){
		int status;
		recibirInt(coordinador_socket,&status);
		switch(status){
			case MANTENER_KEY:
				list_add(list,recibirMensajeArchivo(coordinador_socket));
				break;
			case FIN:
				finalizar = true;
				break;
		}
	}

	return list;


}

