/*
 *  memon/apoyo.h
 *
 *  Fernando P�rez Costoya
 *
 */

/*
 *
 * Fichero de cabecera que contiene macros de utilidad para manejar la
 * m�scara de protecci�n y el prototipo de ejecutar_programa y tratar_SEGV
 *
 *      NO SE DEBE MODIFICAR
 *
 */

#ifndef _APOYO_H
#define _APOYO_H


/* Macros que facilitan el manejo de la m�scara de protecci�n */

/* PROT_ISSET:	Permite comprobar si en la proteccion est� activo un 
		determinado permiso.
		Ejemplo: comprueba si en la m�scara est� activo
		el permiso de escritura
 
			if (PROT_ISSET(PROT_WRITE,prot))
*/

#define PROT_ISSET(prot_flag, proteccion) (prot_flag & proteccion)

/* PROT_SET:	Activa un determinado permiso en una m�scara de
		protecci�n devolviendo la m�scara actualizada.
		Ejemplo: A�ade a una m�scara el permiso de escritura

			prot=PROT_SET(PROT_WRITE,prot);
*/
#define PROT_SET(prot_flag, proteccion) (proteccion | prot_flag )


/* PROT_CLR:	Desactiva un determinado permiso en una m�scara de
		protecci�n devolviendo la m�scara actualizada.
		Ejemplo: Elimina en una m�scara el permiso de escritura

			prot=PROT_CLR(PROT_WRITE,prot);
*/
#define PROT_CLR(prot_flag, proteccion) (proteccion & ~prot_flag)


/* Rutina que establece qu� funci�n realiza el tratamiento de SEGV */
void tratar_SEGV(void (*funcion)(void *));

/* Rutina que arranca la ejecucion del programa */
void ejecutar_programa(int argc, char **args);

#endif /* _APOYO_H */

