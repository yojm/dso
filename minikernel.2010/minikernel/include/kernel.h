/*
 *  minikernel/include/kernel.h
 *
 *  Minikernel. Versión 1.0
 *
 *  Fernando Pérez Costoya
 *
 */

/*
 *
 * Fichero de cabecera que contiene definiciones usadas por kernel.c
 *
 *      SE DEBE MODIFICAR PARA INCLUIR NUEVA FUNCIONALIDAD
 *
 */

#ifndef _KERNEL_H
#define _KERNEL_H

#include "const.h"
#include "HAL.h"
#include "llamsis.h"
#include "string.h"

#define NO_RECURSIVO 0
#define RECURSIVO 1

#define NO_BLOQUEADO_MUTEX 0
#define BLOQUEADO_MUTEX 1

#define OCUPADO 1
#define LIBRE 0

/*Estructura para el manejo de tiempos*/
struct tiempos_ejec {
    	int usuario;
   	int sistema;
};

struct mutex_proc {
	int descriptor;
	int usado; /* estado del descriptor. LIBRE|OCUPADO */
};

/*
 *
 * Definicion del tipo que corresponde con el BCP.
 * Se va a modificar al incluir la funcionalidad pedida.
 *
 */
typedef struct BCP_t *BCPptr;

typedef struct BCP_t {
        int id;					/* ident. del proceso */
        int estado;				/* TERMINADO|LISTO|EJECUCION|BLOQUEADO*/
        contexto_t contexto_regs;		/* copia de regs. de UCP */
        void * pila;				/* dir. inicial de la pila */
	BCPptr siguiente;			/* puntero a otro BCP */
	void *info_mem;				/* descriptor del mapa de memoria */
	int sleep_time;				/* tiempo que ha de estar dormido el proceso */
	struct tiempos_ejec tiempos_sys;	/* tiempos de ejecuticon del proceso */
	struct mutex_proc descrip_mutex[NUM_MUT_PROC];	/* descriptores para mutex */
	int descriptores_ocupados;		/* numero de descriptores ocupados */
	int t_rr;
	
} BCP;

/*
 *
 * Definicion del tipo que corresponde con la cabecera de una lista
 * de BCPs. Este tipo se puede usar para diversas listas (procesos listos,
 * procesos bloqueados en semáforo, etc.).
 *
 */

typedef struct{
	BCP *primero;
	BCP *ultimo;
} lista_BCPs;


/*Estructura para el uso de MUTEX */
struct mutex_t {
	char *nombre_mutex;				/* nombre del mutex */
	int tipo; 					/* RECURSIVO|NO_RECURSIVO*/
	int num_bloqueos;				/* Si es recursivo, para ver cuantos unlock */
	int descriptor;					/* descriptor del mutex */
	lista_BCPs lista_bloqueados_mutex;		/* procesos esperando por que se abra el mutex */ 
	int estado;					/* BLOQUEADO_MUTEX|NO_BLOQUEADO_MUTEX*/
	int id_proc_duenio;				/* dueño del mutex, es decir, proceso por el que el mutex esta bloqueado */
};

struct array_mutex_t {
	struct mutex_t mutex;
	int usado;		/* LIBRE|OCUPADO */
	int n_descrip_asociados;
};

struct array_mutex_t array_mutex[NUM_MUT];

/* Control mutex maximos en el sistema */
int num_mutex = 0;

/*
 * Variable global que identifica el proceso actual
 */

BCP * p_proc_actual=NULL;

/*
 * Variable global que representa la tabla de procesos
 */

BCP tabla_procs[MAX_PROC];

/*
 * Variable global que representa la cola de procesos listos
 */
lista_BCPs lista_listos= {NULL, NULL};

/*
 * Variable global que representa la cola de procesos dormidos
 */
lista_BCPs lista_dormidos= {NULL, NULL};

/*
 * Variable global que representa la cola de procesos bloqueados por
 * no haber suficiente espacio para mas mutex
 */
lista_BCPs lista_bloqueados= {NULL, NULL};

/*
 * Variable global que representa la cola de procesos bloqueados a
 * la espera de leer un caracter
 */
lista_BCPs lista_bloqueados_caracter = {NULL, NULL};

 /*
 * Estructura para 
 */
typedef struct {
	char buffer[TAM_BUF_TERM];
	int longitud;
	int index_e;
	int index_l;
}t_buffer;
t_buffer buffer_lect = {{NULL}, 0, 0, 0};

/*
 *
 * Definición del tipo que corresponde con una entrada en la tabla de
 * llamadas al sistema.
 *
 */
typedef struct{
	int (*fservicio)();
} servicio;

/* Tiempo que el sistema esta activo */
unsigned int tiempo_sistema_ON = 0;

/* Control acceso a memoria en modo sistema */
int control_memoria = 0;

/*
 * Prototipos de las rutinas que realizan cada llamada al sistema
 */
int sis_crear_proceso();
int sis_terminar_proceso();
int sis_escribir();
int sis_obtener_id_pr();
int sis_dormir();
int sis_tiempos_proceso();
int sis_crear_mutex();
int sis_abrir_mutex();
int sis_lock();
int sis_unlock();
int sis_cerrar_mutex();
int sis_leer_caracter();

/*
 * Variable global que contiene las rutinas que realizan cada llamada
 */
servicio tabla_servicios[NSERVICIOS]={	{sis_crear_proceso},
					{sis_terminar_proceso},
					{sis_escribir},
					{sis_obtener_id_pr},
					{sis_dormir},
					{sis_tiempos_proceso},
					{sis_crear_mutex},
					{sis_abrir_mutex},
					{sis_lock},
					{sis_unlock},
					{sis_cerrar_mutex},
					{sis_leer_caracter}};

#endif /* _KERNEL_H */

