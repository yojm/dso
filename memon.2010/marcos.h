/*
 *  memon/marcos.h
 *
 *  Fernando P�rez Costoya
 *
 */

/*
 *
 * Fichero de cabecera que contiene los prototipos de las funciones de
 * interfaz del m�dulo marcos.c.
 *
 *       NO SE DEBE MODIFICAR
 *
 */

#ifndef _MARCOS_H
#define _MARCOS_H


#include "mapa.h"

/* crear_tabla_marcos: crea la tabla de marcos del tama�o indicado */
void crear_tabla_marcos(int tam);

/* Reserva un marco libre. Devuelve el n�mero de marco reservado o -1 si no
   hay ninguno libre */
int reservar_marco_libre();

/* Aplica el algoritmo de reemplazo FIFO. Devuelve n. marco seleccionado */
int reemplazo_FIFO();

/* Aplica el algoritmo de reemplazo del reloj. Devuelve n. marco seleccionado */
int reemplazo_reloj();

/* Rellena la entrada especificada de la tabla de marcos con la p�gina
   recibida como par�metro */
void rellenar_entrada_marco(int nmarco, int reg, int pag);

/* Obtiene qu� p�gina est� almacenada en un marco */
entrada_tabla_paginas *leer_entrada_marco(int nmarco);

/* Libera un marco debido a que la p�gina que contiene ya no existe */
void eliminar_pagina_de_marco(int nmarco);

/* S�lo para depuraci�n */
void imprimir_tabla_marcos();

#endif /* _MARCOS_H */

