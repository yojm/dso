/*
 *  memon/mapa.h
 *
 *  Fernando P�rez Costoya
 *
 */

/*
 *
 * Fichero de cabecera que contiene prototipos exportados por mapa.c
 * y la definici�n de la entrada de la tabla de regiones y de p�ginas.
 *
 *
 *      SE DEBE MODIFICAR PARA COMPLETAR LA DEFINICI�N DE LA ENTRADA DE
 *	LA TABLA DE P�GINAS. ADEM�S, SE PUEDE MODIFICAR PARA INCLUIR
 *      FUNCIONES ADICIONALES (como, por ejemplo, para activar el
 *	de modificado o dada una direcci�n encontrar la p�gina)
 *
 */

#ifndef _MAPA_H
#define _MAPA_H


/* Prototipos de las funciones usadas por el entorno de apoyo
   para notificar los cambios en el mapa del proceso.
	NO SE DEBEN MODIFICAR
*/

/* nos informan de que se ha creado una regi�n
        dir: direcci�n de comienzo de la regi�n
        inodo: fichero al que est� vinculada (0 si es an�nima)
        prot: permisos de acceso a la regi�n
        tamano: tama�o de la regi�n
	compartida: �es una regi�n de tipo compartida?
*/
void creacion_region(void *dir, int inodo, int prot,
			int tamano, int compartida);
/* nos informan de que ha cambiado el tama�o de una regi�n
        dir: direcci�n de comienzo de la regi�n
        tamano: nuevo tama�o de la regi�n
*/
void cambio_tam_region(void *dir, int tamano);
/* nos informan de que se ha eliminado una regi�n
        dir: direcci�n de comienzo de la regi�n
*/
void eliminacion_region(const void *dir);

/* Definiciones de las entradas de la tabla de p�ginas y de regiones */

struct entrada_tabla_regiones_t;

/* Tipo que define una entrada en la tabla de p�ginas */
typedef struct {
  void *dir_inicial;	/* direcci�n inicial de la p�gina */
  struct entrada_tabla_regiones_t *region; /* regi�n a la que pertenece */
  int numMarco;         /*Indica el numero de marco, -1 e.o.c.*/
  int pagModif;         /*Si esta o no modificada */
  int pagAcceso;        /*Si se accedio a la misma */
  int enMemoria;        /*Si esta en memoria principal */
  int pagRef;           /*pagina referenciada */
  int pagOrigen;
} entrada_tabla_paginas;


/* Tipo que define una entrada en la tabla de regiones */
typedef struct entrada_tabla_regiones_t {
        int usada;
        void *dir_inicial;	/* direcci�n inicial de la regi�n */
        int ino;		/* inodo del fichero o 0 si an�nima */
        int prot;		/* protecci�n de la regi�n */
        int npags;		/* tama�o en p�ginas */
        int compartida;		/* �regi�n compartida? */
        entrada_tabla_paginas *tabla_paginas;/* Tabla de p�ginas de la regi�n */
} entrada_tabla_regiones;

/* Funciones auxiliares que a partir de la direcci�n de una entrada
   obtienen su posici�n en la tabla correspondiente */
int regnum(entrada_tabla_regiones *reg);

int pagnum(entrada_tabla_paginas *pag);

/* Funci�n auxiliar que a partir de un n�mero de p�gina de una
regi�n devuelve la direcci�n de su entrada en la tabla */
entrada_tabla_paginas *entrada_pagina(int regnum, int pagnum);

// FUNCIONES AUXILIARES
int busqueda_region(void *dir, int numero);
#endif /* _MAPA_H */
