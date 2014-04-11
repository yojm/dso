/*
 *  memon/memon.c
 *
 *  Fernando P�rez Costoya
 *
 */

/*
 *
 * Fichero que contiene el programa principal del monitor.
 * Instala la rutina de tratamiento de SEGV, inicializa la funci�n de reemplazo,
 * averigua el tama�o de la p�gina,
 * invoca al programa a monitorizar e imprime estad�sticas
 *
 *      NO SE DEBE MODIFICAR
 *
 */


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

#include "apoyo.h"
#include "marcos.h"


/* Decalaraciones de variables y funciones externas */

extern int fallos_total;
extern int fallos_forzados;
extern int fallos_no_forzados;
extern int fallos_con_reemplazo;
extern int fallos_sin_reemplazo;
extern int fallos_sin_lectura;
extern int fallos_con_lectura_fichero;
extern int fallos_con_lectura_swap;
extern int escrituras_en_fichero;
extern int escrituras_en_swap;

extern void fallo_pagina(void *); /* funci�n que trata el fallo de p�gina */

extern int (*reemplazo)(); /* puntero a la funci�n que contiene el algoritmo de
			      reemplazo elegido (la inicializa este m�dulo) */

/* Variable global que almacena el tama�o de p�gina en el sistema */
long tam_pagina;

/* Imprime resultados de la monitorizaci�n */
void imprime_estadisticas() {
	printf("Fallos de p�gina %d\n", fallos_total);
	printf("Fallos no forzados %d\n", fallos_no_forzados);
	printf("Fallos forzados %d\n", fallos_forzados);
	printf("Fallos sin reemplazo %d\n", fallos_sin_reemplazo);
	printf("Fallos con reemplazo %d\n", fallos_con_reemplazo);
	printf("Fallos sin lectura %d\n", fallos_sin_lectura);
	printf("Fallos con lectura fichero %d\n", fallos_con_lectura_fichero);
	printf("Fallos con lectura swap %d\n", fallos_con_lectura_swap);
	printf("Escrituras en fichero %d\n", escrituras_en_fichero);
	printf("Escrituras en swap %d\n", escrituras_en_swap);
}

/* Funci�n "main" del monitor */
int main(int argc, char **argv){
	int num_marcos;

	/* Para forzar escrituras inmediatas se elimina el buffering de "stdout" */
	setbuf(stdout, NULL);

	if (argc<3){
                fprintf(stderr, "Uso: %s num_marcos programa args... \n", argv[0]);
                exit(1);
        }

	if (strstr(argv[0], "FIFO"))
		reemplazo=reemplazo_FIFO;
	else if (strstr(argv[0], "reloj"))
		reemplazo=reemplazo_reloj;
	else {
		fprintf(stderr, "El nombre del ejecutable debe contener la palabra FIFO o reloj\n");
		exit(1);
	}
	
	num_marcos=atoi(argv[1]);

	/* Inicia la tabla de marcos */
	crear_tabla_marcos(num_marcos);

	/* Establece qu� rutina tratar� los fallos de p�gina.
	La rutina "fallo_pagina" se encuentar en el m�dulo "fallo.c". */
	tratar_SEGV(fallo_pagina);

	/* Obtiene el tama�o de la p�gina en el sistema y lo almacena
	en una variable global */
	tam_pagina=sysconf(_SC_PAGESIZE);

	/* Invoca la ejecuci�n del programa */
	ejecutar_programa((argc-2), &argv[2]);

	/* En este punto se muestran las estad�sticas */
	imprime_estadisticas();
	_exit(0);
}
