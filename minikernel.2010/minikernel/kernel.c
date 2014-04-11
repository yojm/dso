/*
 *  kernel/kernel.c
 *
 *  Minikernel. Versión 1.0
 *
 *  Fernando Pérez Costoya
 *
 */

/*
 *
 * Fichero que contiene la funcionalidad del sistema operativo
 *
 */

#include "kernel.h"	/* Contiene defs. usadas por este modulo */

/*
 *
 * Funciones relacionadas con la tabla de procesos:
 *	iniciar_tabla_proc buscar_BCP_libre
 *
 */

/*
 * Función que inicia la tabla de procesos
 */
static void iniciar_tabla_proc(){
	int i;

	for (i=0; i<MAX_PROC; i++)
		tabla_procs[i].estado=NO_USADA;
}

/*
 * Función que busca una entrada libre en la tabla de procesos
 */
static int buscar_BCP_libre(){
	int i;

	for (i=0; i<MAX_PROC; i++)
		if (tabla_procs[i].estado==NO_USADA)
			return i;
	return -1;
}

/*
 *
 * Funciones que facilitan el manejo de las listas de BCPs
 *	insertar_ultimo eliminar_primero eliminar_elem
 *
 * NOTA: PRIMERO SE DEBE LLAMAR A eliminar Y LUEGO A insertar
 */

/*
 * Inserta un BCP al final de la lista.
 */
static void insertar_ultimo(lista_BCPs *lista, BCP * proc){
	if (lista->primero==NULL)
		lista->primero= proc;
	else
		lista->ultimo->siguiente=proc;
	lista->ultimo= proc;
	proc->siguiente=NULL;
}

/*
 * Elimina el primer BCP de la lista.
 */
static void eliminar_primero(lista_BCPs *lista){

	if (lista->ultimo==lista->primero)
		lista->ultimo=NULL;
	lista->primero=lista->primero->siguiente;
}

/*
 * Elimina un determinado BCP de la lista.
 */
static void eliminar_elem(lista_BCPs *lista, BCP * proc){
	BCP *paux=lista->primero;

	if (paux==proc)
		eliminar_primero(lista);
	else {
		for ( ; ((paux) && (paux->siguiente!=proc));
			paux=paux->siguiente);
		if (paux) {
			if (lista->ultimo==paux->siguiente)
				lista->ultimo=paux;
			paux->siguiente=paux->siguiente->siguiente;
		}
	}
}

/*
 *
 * Funciones relacionadas con la planificacion
 *	espera_int planificador
 */

/*
 * Espera a que se produzca una interrupcion
 */
static void espera_int(){
	int nivel;

	printk("-> NO HAY LISTOS. ESPERA INT\n");

	/* Baja al mínimo el nivel de interrupción mientras espera */
	nivel=fijar_nivel_int(NIVEL_1);
	halt();
	fijar_nivel_int(nivel);
}

/*
 * Función de planificacion que implementa un algoritmo FIFO.
 */
static BCP * planificador(){
	while (lista_listos.primero==NULL)
		espera_int();		/* No hay nada que hacer */
	
	lista_listos.primero->t_rr = TICKS_POR_RODAJA;
	lista_listos.primero->estado = EJECUCION;
	return lista_listos.primero;
}

/*
 *
 * Funcion auxiliar que termina proceso actual liberando sus recursos.
 * Usada por llamada terminar_proceso y por rutinas que tratan excepciones
 *
 */
int sis_cerrar_mutex_aux(int descriptor_m2close);
 
static void liberar_proceso(){
	BCP * p_proc_anterior;
	int i;
	int nivel_anterior;
	int do_aux = 0;
	
	/*
	 * Deberiamos liberar todos los descriptores del proceso a liberar*/
	do_aux = p_proc_actual->descriptores_ocupados;
	 
	nivel_anterior=fijar_nivel_int(NIVEL_3);
	for (i=0; i<do_aux; i++) {
		if (p_proc_actual->descrip_mutex[i].usado == OCUPADO) {			
			sis_cerrar_mutex_aux(p_proc_actual->descrip_mutex[i].descriptor);		
		}
	}
	fijar_nivel_int(nivel_anterior);

	liberar_imagen(p_proc_actual->info_mem); /* liberar mapa */

	p_proc_actual->estado=TERMINADO;
	eliminar_primero(&lista_listos); /* proc. fuera de listos */

	/* Realizar cambio de contexto */
	p_proc_anterior=p_proc_actual;
	p_proc_actual=planificador();
	

	printk("-> C.CONTEXTO POR FIN: de %d a %d\n",
			p_proc_anterior->id, p_proc_actual->id);

	liberar_pila(p_proc_anterior->pila);
	cambio_contexto(NULL, &(p_proc_actual->contexto_regs));
        return; /* no debería llegar aqui */
}

/*
 *
 * Funciones relacionadas con el tratamiento de interrupciones
 *	excepciones: exc_arit exc_mem
 *	interrupciones de reloj: int_reloj
 *	interrupciones del terminal: int_terminal
 *	llamadas al sistemas: llam_sis
 *	interrupciones SW: int_sw
 *
 */

/*
 * Tratamiento de excepciones aritmeticas
 */
static void exc_arit(){

	if (!viene_de_modo_usuario())
		panico("excepcion aritmetica cuando estaba dentro del kernel");


	printk("-> EXCEPCION ARITMETICA EN PROC %d\n", p_proc_actual->id);
	liberar_proceso();

        return; /* no debería llegar aqui */
}

/*
 * Tratamiento de excepciones en el acceso a memoria
 */
static void exc_mem(){

	if (!viene_de_modo_usuario() && (control_memoria==0))
		panico("excepcion de memoria cuando estaba dentro del kernel");


	printk("-> EXCEPCION DE MEMORIA EN PROC %d\n", p_proc_actual->id);
	liberar_proceso();

        return; /* no debería llegar aqui */
}

/*
 * Tratamiento de interrupciones de terminal
 */
static void int_terminal(){
	char car;
	int nivel_anterior;
	BCP *paux;

	/* Leemos el caracter */
	nivel_anterior = fijar_nivel_int(NIVEL_2);
	car = leer_puerto(DIR_TERMINAL);
	printk("-> TRATANDO INT. DE TERMINAL %c\n", car);
	if (buffer_lect.longitud < TAM_BUF_TERM){
		buffer_lect.buffer[buffer_lect.index_e] = car;
		buffer_lect.longitud++;
		if (buffer_lect.index_e == TAM_BUF_TERM-1){
			buffer_lect.index_e = 0;
		}else{
			buffer_lect.index_e++;
		}
	}
	
	/* Desbloqueamos al siguiente proceso esperando por un caracter */
	//fijar_nivel_int(nivel_anterior);
	//nivel_anterior = fijar_nivel_int(NIVEL_3);
	paux = lista_bloqueados_caracter.primero;
	if (paux != NULL){
		fijar_nivel_int(NIVEL_3);
		eliminar_primero(&lista_bloqueados_caracter);
		paux->estado=LISTO;
		insertar_ultimo(&lista_listos, paux);	
	}
	fijar_nivel_int(nivel_anterior);

        return;
}


/*
 * Tratamiento de interrupciones de reloj
 */
static void int_reloj(){
	BCP *paux, *paux_sig;
	
	//printk("-> TRATANDO INT. DE RELOJ\n");
	
	tiempo_sistema_ON++;
	
	/*Actualizar todos los procesos bloqueados (dormidos)*/
	paux=lista_dormidos.primero;
	while (paux != NULL) {
		paux->sleep_time--;
		paux_sig = paux->siguiente;
		if (paux->sleep_time == 0){
			paux->estado = LISTO;
			eliminar_elem(&lista_dormidos, paux);	
			insertar_ultimo(&lista_listos, paux);
		}
		paux=paux_sig;		
	}
	
	/*Actualizar tiempos de ejecucion del proceso en el sistema*/
	if (p_proc_actual->estado == EJECUCION){
		if (viene_de_modo_usuario()){
			p_proc_actual->tiempos_sys.usuario = p_proc_actual->tiempos_sys.usuario +1;
		} else {
			p_proc_actual->tiempos_sys.sistema = p_proc_actual->tiempos_sys.sistema +1;
		}
		p_proc_actual->t_rr--;
		if (p_proc_actual->t_rr == 0){
			activar_int_SW();
		}
	}
	
		
        return;
}

/*
 * Tratamiento de llamadas al sistema
 */
static void tratar_llamsis(){
	int nserv, res;

	nserv=leer_registro(0);
	if (nserv<NSERVICIOS)
		res=(tabla_servicios[nserv].fservicio)();
	else
		res=-1;		/* servicio no existente */
	escribir_registro(0,res);
	return;
}

/*
 * Tratamiento de interrupciuones software
 */
static void int_sw(){

	BCP *p_anterior;
	int nivel_anterior;
	
	if (p_proc_actual->t_rr <= 0){
		printk("-> TRATANDO INT. SW\n");
	
		nivel_anterior=fijar_nivel_int(NIVEL_3);
		p_proc_actual->estado=LISTO;
		p_anterior = p_proc_actual;
		eliminar_elem(&lista_listos, p_proc_actual);
		insertar_ultimo(&lista_listos, p_proc_actual);
		p_proc_actual = planificador();
		cambio_contexto(&(p_anterior->contexto_regs),&(p_proc_actual->contexto_regs));
		fijar_nivel_int(nivel_anterior);
	}

	return;
}

/*
 *
 * Funcion auxiliar que crea un proceso reservando sus recursos.
 * Usada por llamada crear_proceso.
 *
 */
static int crear_tarea(char *prog){
	void * imagen, *pc_inicial;
	int error=0;
	int proc;
	BCP *p_proc;
	int  nivel_anterior;

	proc=buscar_BCP_libre();
	if (proc==-1) {
		return -1;	/* no hay entrada libre */
			
	}

	/* A rellenar el BCP ... */
	p_proc=&(tabla_procs[proc]);

	/* crea la imagen de memoria leyendo ejecutable */
	imagen=crear_imagen(prog, &pc_inicial);
			//printk("Crea LA imagen de MEMORIA (==0->Error)--> %d\n", imagen);
	if (imagen)
	{
		p_proc->info_mem=imagen;
		p_proc->pila=crear_pila(TAM_PILA);
		nivel_anterior=fijar_nivel_int(NIVEL_3);
		fijar_contexto_ini(p_proc->info_mem, p_proc->pila, TAM_PILA,
			pc_inicial,
			&(p_proc->contexto_regs));
		p_proc->id=proc;
		p_proc->estado=LISTO;
		p_proc->sleep_time=0;
		p_proc->descriptores_ocupados=0;
		
		p_proc->tiempos_sys.usuario=0;
		p_proc->tiempos_sys.sistema=0;

		/* lo inserta al final de cola de listos */
		insertar_ultimo(&lista_listos, p_proc);
		fijar_nivel_int(nivel_anterior);
		error= 0;
	}
	else {
		error= -1; /* fallo al crear imagen */
	}

	return error;
}

/*
 *
 * Rutinas que llevan a cabo las llamadas al sistema
 *	sis_crear_proceso sis_escribir
 *
 */

/*
 * Tratamiento de llamada al sistema crear_proceso. Llama a la
 * funcion auxiliar crear_tarea sis_terminar_proceso
 */
int sis_crear_proceso(){
	char *prog;
	int res;


	printk("-> PROC %d: CREAR PROCESO\n", p_proc_actual->id);
	prog=(char *)leer_registro(1);
	res=crear_tarea(prog);

	return res;
}

/*
 * Tratamiento de llamada al sistema escribir. Llama simplemente a la
 * funcion de apoyo escribir_ker
 */
int sis_escribir()
{
	char *texto;
	unsigned int longi;

	texto=(char *)leer_registro(1);
	longi=(unsigned int)leer_registro(2);

	escribir_ker(texto, longi);
	return 0;
}

/*
 * Tratamiento de llamada al sistema terminar_proceso. Llama a la
 * funcion auxiliar liberar_proceso
 */
int sis_terminar_proceso(){
	
	printk("-> FIN PROCESO %d\n", p_proc_actual->id);


	liberar_proceso();

        return 0; /* no debería llegar aqui */
}

/*
 * Tratamiento de llamada al sistema obtener_id_or
 */
int sis_obtener_id_pr(){
	return p_proc_actual->id;
}

/*
 * Tratamiento de llamada al sistema 
 */
int sis_dormir() {
	unsigned int tiempo;
	BCP *p_proc_anterior;
	int nivel_anterior;
	
	
	tiempo=(unsigned int)leer_registro(1) * TICK;
	
	if (tiempo>0) {
		
		nivel_anterior=fijar_nivel_int(NIVEL_3);
		
		p_proc_actual->estado=BLOQUEADO;
		p_proc_actual->sleep_time=tiempo;
	
		p_proc_anterior=p_proc_actual;
		eliminar_elem(&lista_listos, p_proc_actual);
		insertar_ultimo(&lista_dormidos, p_proc_actual);
	
		/* Realizar cambio de contexto */
		p_proc_actual=planificador();

		printk("-> C.CONTEXTO POR DORMIR: de %d a %d\n",
				p_proc_anterior->id, p_proc_actual->id);

		cambio_contexto(&(p_proc_anterior->contexto_regs), &(p_proc_actual->contexto_regs));
		
		fijar_nivel_int(nivel_anterior);
	
	}
	
	return 0;
}

/*
 * Tratamiento de llamada al sistema 
 */
int sis_tiempos_proceso(){
	struct tiempos_ejec *tiempos_aux;
	int nivel_anterior;
	
	tiempos_aux = (struct tiempos_ejec *)leer_registro(1); /*Guardar los tiempos en memoria*/
	if (tiempos_aux){
	
		nivel_anterior=fijar_nivel_int(NIVEL_3);
	
		/*Accediendo a memoria -> Control*/
		control_memoria=1;
		tiempos_aux->usuario= p_proc_actual->tiempos_sys.usuario;
		tiempos_aux->sistema= p_proc_actual->tiempos_sys.sistema;
		

		control_memoria=0;
		
		fijar_nivel_int(nivel_anterior);
	}
	
	return tiempo_sistema_ON;
}

int sis_crear_mutex(){
	
	BCP *p_proc_anterior;
	int nivel_anterior;
	int descriptor = -1;
	int descriptor_proc = -1;
	int i;
	char * nombre = (char *)leer_registro(1);
	
	/* Comprobamos longitud del nombre para el mutex */
	if (strlen(nombre)>MAX_NOM_MUT)
		return -1;
	//printk("Comprobacion: Nombre del mutex\n");
		
	/* Comprobamos si existe al menos un descriptor libre en el proceso */
	if (p_proc_actual->descriptores_ocupados >= NUM_MUT_PROC)
		return -1;
	//printk("Comprobacion: Descriptor libre en el proceso\n");	
		
	/* Comprobamos si no se ha llegado al limite de mutex en el sistema 	*/
	/* Si es asi, bloqueamos el proceso a la espera de un hueco		*/
	while (num_mutex == NUM_MUT) {
		nivel_anterior=fijar_nivel_int(NIVEL_3);
		
		p_proc_actual->estado=BLOQUEADO;
	
		p_proc_anterior=p_proc_actual;
		
		eliminar_elem(&lista_listos, p_proc_actual);
		insertar_ultimo(&lista_bloqueados, p_proc_actual);
	
		/* Realizar cambio de contexto */
		p_proc_actual=planificador();

		printk("-> C.CONTEXTO POR CREAR MUTEX (cupo de mutex en sistema lleno): de %d a %d\n",
				p_proc_anterior->id, p_proc_actual->id);

		cambio_contexto(&(p_proc_anterior->contexto_regs), &(p_proc_actual->contexto_regs));
		
		fijar_nivel_int(nivel_anterior);	
	}
	//printk("Comprobacion: Limite de mutex en el sistema\n");
	
	/* Comprobamos si existe algun mutex con nombre repetido */		
	for (i=0; i<NUM_MUT; i++){
		if (array_mutex[i].usado==OCUPADO) {
			if (strcmp(nombre,array_mutex[i].mutex.nombre_mutex)==0) {
				return -1;
			
			} 
		}
	}
	//printk("Comprobacion: Nombre repetido de mutex: %s, num_de mutex %d\n", nombre, num_mutex);
	
	/* Creamos el mutex */
	i=0;
	while ((i<NUM_MUT) && (descriptor==-1)) {
		if (array_mutex[i].usado != OCUPADO) {
			descriptor = i;
		} else {
			i++;
		}
		//printk("DESCRIPTOR NUMERO %d %d\n",descriptor,i );
	}
	
	num_mutex++;
	array_mutex[descriptor].n_descrip_asociados=1;
	array_mutex[descriptor].usado = OCUPADO;
	array_mutex[descriptor].mutex.descriptor = descriptor;
	array_mutex[descriptor].mutex.nombre_mutex = strdup(nombre);
	array_mutex[descriptor].mutex.num_bloqueos = 0;
	array_mutex[descriptor].mutex.tipo = (int)leer_registro(2);
	array_mutex[descriptor].mutex.estado = NO_BLOQUEADO_MUTEX;
	array_mutex[descriptor].mutex.id_proc_duenio = -1;
	
	i=0;
	while ((i<NUM_MUT_PROC) && (descriptor_proc==-1)) {
		if (p_proc_actual->descrip_mutex[i].usado == LIBRE)
			descriptor_proc = i;
		else
			i++;
	}
		
	p_proc_actual->descriptores_ocupados++;
	p_proc_actual->descrip_mutex[descriptor_proc].usado = OCUPADO;
	p_proc_actual->descrip_mutex[descriptor_proc].descriptor = descriptor;
	
	return descriptor;
}


int sis_abrir_mutex(){
	
	//int nivel_anterior;
	int descriptor = 0;
	int descriptor_proc = -1;
	int encontrado = 0;
	int i;
	char * nombre = (char *)leer_registro(1);
	
	/* Comprobamos si existe al menos un descriptor libre en el proceso */
	if (p_proc_actual->descriptores_ocupados >= NUM_MUT_PROC)
		return -1;
	//printk("Comprobacion Abrir: descriptor libre proc\n");	
		
	/* Comprobamos si existe algun mutex con ese nombre */		
	i=0;
	while ((i<NUM_MUT) && (encontrado==0)) {
		if (array_mutex[i].usado==OCUPADO) {
			//printk("NOMBRE: %s - %s diff: %d\n",array_mutex[i].mutex.nombre_mutex, nombre, strcmp(nombre,array_mutex[i].mutex.nombre_mutex));
			if (strcmp(nombre,array_mutex[i].mutex.nombre_mutex)==0) {
				encontrado=1;
			}
		}
		i++;
		
	}
	//printk("Comprobacion Abrir: existe nombre del mutex\n");	
	
	/* Si no existe ese nombre de mutex -> Error */
	if (encontrado==0)
		return -1;
	//printk("Comprobacion Abrir: NO EXISTE nombre del mutex\n");
		
	/* Si existe asignamos descriptor */
	descriptor = i-1;
	i=0;
	while ((i<NUM_MUT_PROC) && (descriptor_proc==-1)) {
		if (p_proc_actual->descrip_mutex[i].usado == LIBRE)
			descriptor_proc = i;
		else
			i++;
	}
	
	array_mutex[descriptor].n_descrip_asociados++;	
	p_proc_actual->descriptores_ocupados++;
	p_proc_actual->descrip_mutex[descriptor_proc].usado = OCUPADO;
	p_proc_actual->descrip_mutex[descriptor_proc].descriptor = descriptor;	
	
	return descriptor;	
}


int sis_lock(){
	
	int i=0;
	int nivel_anterior;
	int encontrado = 0;
	int descrip_m2lock = (int)leer_registro(1);
	BCP *p_proc_anterior;
	
	/* Comprobamos que el numero de descriptor es posible */
	if (descrip_m2lock<0 || descrip_m2lock>=NUM_MUT){
		return -1;
	}
	
	/* Comprobamos que el mutex existe */
	if (array_mutex[descrip_m2lock].usado != OCUPADO){
		return -1;
	}
	
	/* Comprobamos que el proceso tiene el mutex creado/abierto */
	while ((i<NUM_MUT_PROC) && (encontrado == 0)){
		if ((p_proc_actual->descrip_mutex[i].usado == OCUPADO) && (p_proc_actual->descrip_mutex[i].descriptor == descrip_m2lock)) {
			encontrado=1;
		} else {
			i++;
		}
	}
	
	if (encontrado == 0) {
		return -1;
		
	}
	
	/* Si el mutex no esta bloqueado, lo bloqueamos */
	if (array_mutex[descrip_m2lock].mutex.estado == NO_BLOQUEADO_MUTEX) {
		array_mutex[descrip_m2lock].mutex.estado = BLOQUEADO_MUTEX;
		array_mutex[descrip_m2lock].mutex.id_proc_duenio = p_proc_actual->id;
		array_mutex[descrip_m2lock].mutex.num_bloqueos++;
	/* Si esta bloqueado, hay que ver que tipo de mutex es */
	} else {
		/* Si es no recursivo y es el dueño del proceso que ya esta bloqueado:  error*/
		if ((array_mutex[descrip_m2lock].mutex.tipo == NO_RECURSIVO) && 
			(array_mutex[descrip_m2lock].mutex.id_proc_duenio == p_proc_actual->id)) {
				return -1;
				
		/* Se es recursivo, no pasa nada */		
		} else if ((array_mutex[descrip_m2lock].mutex.tipo == RECURSIVO) && 
			(array_mutex[descrip_m2lock].mutex.id_proc_duenio == p_proc_actual->id)) {
				array_mutex[descrip_m2lock].mutex.num_bloqueos++;
				return 0;
		/* Si es no recursivo o no es el dueño hay que bloquearlo */
		} else {
			while (array_mutex[descrip_m2lock].mutex.estado == BLOQUEADO_MUTEX) {
				nivel_anterior=fijar_nivel_int(NIVEL_3);
				p_proc_actual->estado = BLOQUEADO;
			
				p_proc_anterior=p_proc_actual;
		
				eliminar_elem(&lista_listos, p_proc_actual);
				insertar_ultimo(&array_mutex[descrip_m2lock].mutex.lista_bloqueados_mutex, p_proc_actual);
			
				p_proc_actual=planificador();

				printk("-> C.CONTEXTO POR LOCK: de %d a %d\n",
					p_proc_anterior->id, p_proc_actual->id);

				cambio_contexto(&(p_proc_anterior->contexto_regs), &(p_proc_actual->contexto_regs));		
				fijar_nivel_int(nivel_anterior);
			}
		}
	}
	return 0;	
}


int sis_unlock(){

	int nivel_anterior;
	BCP *paux;
	int i=0;
	int encontrado = 0;
	int descrip_m2unlock = (int)leer_registro(1);
	
	/* Comprobamos que el numero de descriptor es posible */
	if (descrip_m2unlock<0 || descrip_m2unlock>=NUM_MUT){
		return -1;
	}
	
	/* Comprobamos que el mutex existe */
	if (array_mutex[descrip_m2unlock].usado != OCUPADO){
		return -1;
	}
	
	/* Comprobamos que el proceso tiene el mutex creado/abierto */
	while ((i<NUM_MUT_PROC) && (encontrado == 0)){
		if ((p_proc_actual->descrip_mutex[i].usado == OCUPADO) && (p_proc_actual->descrip_mutex[i].descriptor == descrip_m2unlock)) {
			encontrado=1;
		} else {
			i++;
		}
	}
	
	if (encontrado == 0)
		return -1;
	
	/* Si es el dueño, se puede desbloquear, sino error */
	if (array_mutex[descrip_m2unlock].mutex.id_proc_duenio == p_proc_actual->id) {
		/* Si es recursivo, y ha sido bloqueado varias veces, se decrementa 
		 * una vez pero seguira siendo el dueño y estara aun bloqueado */
		if ((array_mutex[descrip_m2unlock].mutex.tipo == RECURSIVO) && 
		    (array_mutex[descrip_m2unlock].mutex.num_bloqueos > 1)){
		    	array_mutex[descrip_m2unlock].mutex.num_bloqueos--;
		/* Desbloqueamos el mutex */
		}else {
			array_mutex[descrip_m2unlock].mutex.num_bloqueos = 0;
			array_mutex[descrip_m2unlock].mutex.id_proc_duenio = -1;
			array_mutex[descrip_m2unlock].mutex.estado = NO_BLOQUEADO_MUTEX;
		
			//nivel_anterior = fijar_nivel_int(NIVEL_3);
			paux = array_mutex[descrip_m2unlock].mutex.lista_bloqueados_mutex.primero;
			if (paux != NULL) {
				nivel_anterior = fijar_nivel_int(NIVEL_3);
				paux->estado = LISTO;
				//array_mutex[descrip_m2unlock].mutex.estado = NO_BLOQUEADO_MUTEX;
				array_mutex[descrip_m2unlock].mutex.id_proc_duenio = paux->id;
				eliminar_primero (&array_mutex[descrip_m2unlock].mutex.lista_bloqueados_mutex);
				insertar_ultimo (&lista_listos, paux);
				fijar_nivel_int(nivel_anterior);
			}
			//fijar_nivel_int(nivel_anterior);
		}
	} else {
		return -1;
	}
	return 0;
}

int sis_cerrar_mutex_aux(int descriptor_m2close) {

	int anterior_nivel;
	BCP *paux;
	int eliminado =0;
	int i=0;
	
	/* Comprobamos que el descriptor pertenece a un mutex en uso */
	if (array_mutex[descriptor_m2close].usado != OCUPADO)
		return -1;
	//printk("Comprobacion CERRAR: Descriptor esta en uso\n");
	

	/* Cerramos el descriptor del mutex en el proceso */	
	while ((i<NUM_MUT_PROC) && (eliminado == 0)) {
		if ((p_proc_actual->descrip_mutex[i].usado == OCUPADO) && 
		    (p_proc_actual->descrip_mutex[i].descriptor == descriptor_m2close)) {
		   	
		   	array_mutex[descriptor_m2close].n_descrip_asociados--;
			p_proc_actual->descrip_mutex[i].usado = LIBRE;
			p_proc_actual->descrip_mutex[i].descriptor = -1;
			p_proc_actual->descriptores_ocupados--;
			eliminado = 1;
		} else {
			i++;
		}
	}
	//printk("Comprobacion CERRAR: Cerramos el descriptor del mutex en el proceso\n");
	
	//printk("NUMERO DE DESCRIPTORES ASOCIADOS A ESTE MUTEX: %s %d\n", 
			//array_mutex[descriptor_m2close].mutex.nombre_mutex,array_mutex[descriptor_m2close].n_descrip_asociados);
	
	/* Dejamos paso al siguiente proceso esperando por el mutex 
	 * si este proceso lo tenia bloqueado */
	 
	if (array_mutex[descriptor_m2close].mutex.id_proc_duenio == p_proc_actual->id){
		
		/* Ponemos LISTO el siguiente proceso que esperara por el mutex */
		paux = array_mutex[descriptor_m2close].mutex.lista_bloqueados_mutex.primero;
		array_mutex[descriptor_m2close].mutex.estado = NO_BLOQUEADO_MUTEX;
		if (paux != NULL){
			anterior_nivel = fijar_nivel_int(NIVEL_3);
			eliminar_elem(&array_mutex[descriptor_m2close].mutex.lista_bloqueados_mutex, paux);
			insertar_ultimo(&lista_listos, paux);
			paux->estado=LISTO;
			fijar_nivel_int(anterior_nivel);
			array_mutex[descriptor_m2close].mutex.id_proc_duenio = paux-> id;
			
		} else {
			array_mutex[descriptor_m2close].mutex.id_proc_duenio = -1;

		}
	}
		
	if (array_mutex[descriptor_m2close].n_descrip_asociados == 0) {
		array_mutex[descriptor_m2close].usado = LIBRE;
		array_mutex[descriptor_m2close].mutex.descriptor = -1;
		num_mutex--;
	
		paux=lista_bloqueados.primero;
		
		if (paux != NULL){
			//printk("Comprobacion CERRAR:numero de mutex ocupados sistema: %d, id siguiente proc: %d\n", num_mutex, paux->id);
			anterior_nivel = fijar_nivel_int(NIVEL_3);
			paux->estado = LISTO;
			eliminar_elem(&lista_bloqueados, paux);
			insertar_ultimo(&lista_listos, paux);
			fijar_nivel_int(anterior_nivel);
		}
	}

	return 0;
}

int sis_cerrar_mutex(){
	
	return sis_cerrar_mutex_aux((int)leer_registro(1));
	// descriptor_m2close = (int)leer_registro(1);
		
}

int sis_leer_caracter(){
	
	BCP *p_proc_anterior;
	int nivel_anterior;
	int caracter;
	
	nivel_anterior = fijar_nivel_int(NIVEL_2);
	while (buffer_lect.longitud == 0) {
		fijar_nivel_int(NIVEL_3);
		p_proc_actual->estado = BLOQUEADO;
		eliminar_elem(&lista_listos, p_proc_actual);
		insertar_ultimo(&lista_bloqueados_caracter, p_proc_actual);
		p_proc_anterior=p_proc_actual;
		p_proc_actual=planificador();			
		cambio_contexto(&(p_proc_anterior->contexto_regs), &(p_proc_actual->contexto_regs));
		fijar_nivel_int(NIVEL_2);
	}
	fijar_nivel_int(nivel_anterior);
	
	buffer_lect.longitud--;
	caracter = buffer_lect.buffer[buffer_lect.index_l];
	if (buffer_lect.index_l == TAM_BUF_TERM -1)
		buffer_lect.index_l = 0;
	else
		buffer_lect.index_l++;
		
	return caracter;

}

/*
 *
 * Rutina de inicialización invocada en arranque
 *
 */
int main(){
	/* se llega con las interrupciones prohibidas */

	instal_man_int(EXC_ARITM, exc_arit); 
	instal_man_int(EXC_MEM, exc_mem); 
	instal_man_int(INT_RELOJ, int_reloj); 
	instal_man_int(INT_TERMINAL, int_terminal); 
	instal_man_int(LLAM_SIS, tratar_llamsis); 
	instal_man_int(INT_SW, int_sw); 

	iniciar_cont_int();		/* inicia cont. interr. */
	iniciar_cont_reloj(TICK);	/* fija frecuencia del reloj */
	iniciar_cont_teclado();		/* inici cont. teclado */

	iniciar_tabla_proc();		/* inicia BCPs de tabla de procesos */

	/* crea proceso inicial */
	if (crear_tarea((void *)"init")<0)
		panico("no encontrado el proceso inicial");
	
	/* activa proceso inicial */
	p_proc_actual=planificador();
	cambio_contexto(NULL, &(p_proc_actual->contexto_regs));
	panico("S.O. reactivado inesperadamente");
	return 0;
}
