/*
 *  memon/marcos.c
 *
 *  Fernando P�rez Costoya
 *
 */

/*
 *
 * Fichero que contiene la gesti�n de una tabla de marcos.
 * Est� ya programado un algoritmo FIFO.
 *
 *	SE DEBE MODIFICAR S�LO PARA INCLUIR EL ALGORITMO DE
 *	REEMPLAZO DEL RELOJ. NO DEBE CAMBIAR LA INTERFAZ.
 *
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "marcos.h"

/* Definici�n de una entrada de la tabla de marcos */
struct entrada_tabla_marcos {
	int ocupado;	/* �est� ocupado el marco? */
	int region; /* reg. almacenada (si ocupado) */
	int pagina; /* pag. almacenada (si ocupado) */
	struct entrada_tabla_marcos *anterior; /* anterior en lista FIFO */
	struct entrada_tabla_marcos *siguiente; /* siguiente en lista FIFO */
};

/* Cabecera de lista FIFO de marcos. Va a ser una lista circular doblemente
   encadenada donde aparecen siempre todos los marcos: primero los marcos
   libres, si los hay, y despu�s los ocupados, si los hay, en orden FIFO */

static struct entrada_tabla_marcos *lista_marcos;

/* Tabla de marcos */
static struct {
	int tam; /* n�mero de marcos */
	struct entrada_tabla_marcos *marcos; /* vector de marcos */
} tabla_marcos;

/* Macro que calcula a partir de la direcci�n de la entrada el n�mero de
   marco que correponde */
#define MARCO(dir) (dir-tabla_marcos.marcos)

/* Funciones de interfaz del m�dulo */

/* crear_tabla_marcos: crea la tabla de marcos con el n. de marcos indicado */
void crear_tabla_marcos(int tam){
	int i;

	/* El tama�o debe ser mayor que cero */
	assert(tam>0);

	/* Reserva espacio e inicializa */
	tabla_marcos.marcos= malloc(tam * sizeof(struct entrada_tabla_marcos));
	tabla_marcos.tam= tam;

	/* Construye la lista circular inicial donde todos los marcos est�n
	   libres */
	lista_marcos=tabla_marcos.marcos;

	for (i=0; i<tam; i++) {
		tabla_marcos.marcos[i].ocupado=0;

		/* establece enlace a anterior y posterior */
		tabla_marcos.marcos[i].siguiente=&tabla_marcos.marcos[(i+1)%tam];
		tabla_marcos.marcos[i].anterior=&tabla_marcos.marcos[(i+tam-1)%tam];
	}
}

/* Reserva un marco libre. Devuelve el n�mero de marco reservado o -1 si no
   hay ninguno libre */
int reservar_marco_libre() {
	int nmarco;
	
	/* El primero de la lista no est� libre: No hay libres */
	if (lista_marcos->ocupado)
		nmarco=-1;
	else {
		/* Calcula el n�m. de marco */
		nmarco=MARCO(lista_marcos);
		/* Reservo el primero de la lista */
		lista_marcos->ocupado=1;

		/* Avanzo el puntero: marco reservado pasa a �ltimo de FIFO */
		lista_marcos=lista_marcos->siguiente;
	}
	return nmarco;
}

/* Aplica el algoritmo de reemplazo FIFO. Devuelve n. marco seleccionado */
int reemplazo_FIFO() {
	int nmarco;
	
	/* El primero de la lista debe estar ocupado */
	assert(lista_marcos->ocupado);
	/* Calcula el n�m. de marco */
	nmarco=MARCO(lista_marcos);
	/* Avanzo el puntero: marco seleccionado pasa a �ltimo de FIFO */
	lista_marcos=lista_marcos->siguiente;
	return nmarco;
}

/* Aplica el algoritmo de reemplazo del reloj. Devuelve n. marco seleccionado */
int reemplazo_reloj() {
	/* DEBE COMPLETARSE */

	return 0;
}

/* Rellena la entrada especificada de la tabla de marcos con la p�gina
   recibida como par�metro */
void rellenar_entrada_marco(int nmarco, int reg, int pag) {
	/* Compruebo que no se sale de la tabla de marcos y que est� ocupado */
	assert((nmarco>=0) && (nmarco<tabla_marcos.tam) &&
			tabla_marcos.marcos[nmarco].ocupado);

	tabla_marcos.marcos[nmarco].region=reg;
	tabla_marcos.marcos[nmarco].pagina=pag;
}

/* Obtiene qu� p�gina est� almacenada en un marco */
entrada_tabla_paginas *leer_entrada_marco(int nmarco) {
	entrada_tabla_paginas *pagina;

	/* Compruebo que no se sale de la tabla de marcos y que est� ocupado */
	assert((nmarco>=0) && (nmarco<tabla_marcos.tam) &&
			tabla_marcos.marcos[nmarco].ocupado);


	pagina=entrada_pagina(tabla_marcos.marcos[nmarco].region,
			tabla_marcos.marcos[nmarco].pagina);
	return pagina;
}

/* Libera un marco debido a que la p�gina que contiene ya no existe.
   Esta funci�n se usar� cuando se elimina una regi�n invoc�ndola una vez
   por cada p�gina residente de la regi�n que se elimina. El marco
   que conten�a la p�gina eliminada se pone como libre y se coloca
   al principio de la lista (no es necesario mantener un orden FIFO con los
   marcos libres) */

void eliminar_pagina_de_marco(int nmarco) {
	struct entrada_tabla_marcos *marco_a_eliminar;
	struct entrada_tabla_marcos *anterior;
	struct entrada_tabla_marcos *posterior;

	/* Compruebo que est� ocupado */
	assert(tabla_marcos.marcos[nmarco].ocupado);

	/* Busco la posici�n de la lista donde est� el marco */
	marco_a_eliminar=&tabla_marcos.marcos[nmarco];

	/* Si es el primero de la lista s�lo hace falta liberarlo.
	   En caso contrario, antes hay que hacer m�s trabajo */
	if (lista_marcos!=marco_a_eliminar) {
		/* Primero elimino de la lista el marco */
		anterior=marco_a_eliminar->anterior;
		posterior=marco_a_eliminar->siguiente;
		anterior->siguiente=posterior;
		posterior->anterior=anterior;

		/* Despu�s lo inserto al principio de todo (no es necesario
                mantener un orden FIFO con los libres) */
		anterior=lista_marcos->anterior;
		marco_a_eliminar->siguiente=lista_marcos;
		marco_a_eliminar->anterior=anterior;
		lista_marcos->anterior=marco_a_eliminar;
		anterior->siguiente=marco_a_eliminar;
		lista_marcos=marco_a_eliminar;
	}
	marco_a_eliminar->ocupado=0;
}

/* S�lo para depuraci�n */
void imprimir_tabla_marcos() {
	int nmarco;
	struct entrada_tabla_marcos *aux=lista_marcos;

	do {
		nmarco=MARCO(aux);
		if (aux->ocupado)
			printf("M[%d](r %d p %d) ", nmarco,
					aux->region, aux->pagina);
		else
			printf("M[%d](LIBRE) ", nmarco); 
		aux=aux->siguiente;
	} while (aux!=lista_marcos);
	printf("\n");
}
