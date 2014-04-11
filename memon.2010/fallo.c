/*
 *  memon/fallo.c
 *
 *  Fernando Pérez Costoya
 *
 */

/*
 *
 * Fichero que contiene la rutina que trata el fallo de página.
 * Esta rutina se encarga de llevar las estadísticas de la monitorización
 *
 *      SE DEBE MODIFICAR
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/mman.h>
#include <unistd.h>

#include "mapa.h"
#include "marcos.h"
#include "apoyo.h"
extern entrada_tabla_regiones tabla_regiones[16];
extern long tam_pagina;

/* Variables que almacenan las estadísticas de la monitorización */
int fallos_total=0;

/* ¿El fallo de página se debe a la falta de memoria física? 
*/
int fallos_no_forzados=0;  /* Sí */
int fallos_forzados=0; /* No, ocurre debido a la carga por demanda */

/* ¿El fallo de página ha causado un reemplazo? */
int fallos_sin_reemplazo=0;
int fallos_con_reemplazo=0;

/* ¿El fallo de página implica una operación de lectura de fichero, de
   swap o no implica lectura? */
int fallos_sin_lectura=0; /* sin lectura (o sea, rellenar con 0) */
int fallos_con_lectura_fichero=0; /* con lectura de fichero */
int fallos_con_lectura_swap=0; /* con lectura de swap */

/* Número de operaciones de escritura (pageouts) */
int escrituras_en_fichero=0;	/* se ha escrito en fichero */
int escrituras_en_swap=0;	/* se ha escrito en swap */

/* Rutina de reemplazo que se va a usar (iniciado por memon.c) */
int (*reemplazo)();

/* Rutina que trata el fallo de página */



void fallo_pagina(void *dir_fallo) {
  int i,
    numero_pagina,
    numero_region = -1,
    nuevo_marco,
    marco_remplazo;
    
  for ( i = 0; i < 16; i++){ //BUSCAR SI LA DIR ESTA DENTRO DE UNA REGION DE FORMA CORRECTA
    if ((tabla_regiones[i].usada) && 
      (dir_fallo >= tabla_regiones[i].dir_inicial) &&
      (dir_fallo < (tabla_regiones[i].dir_inicial) + ((tabla_regiones[i]).npags)*tam_pagina)) 
	numero_region = i;
  }
    
  if (numero_region == -1) {
    printf("acceso a memoria inválido %p\n",dir_fallo);
    _exit(1);
  }
  
  numero_pagina = (dir_fallo - tabla_regiones[numero_region].dir_inicial)/tam_pagina;
  
  if (tabla_regiones[numero_region].tabla_paginas[numero_pagina].numMarco != -1){
    if (PROT_ISSET(PROT_WRITE,tabla_regiones[numero_region].prot)) {
      tabla_regiones[numero_region].tabla_paginas[numero_pagina].pagModif = 1;
      tabla_regiones[numero_region].tabla_paginas[numero_pagina].pagOrigen = 0;
      mprotect(tabla_regiones[numero_region].tabla_paginas[numero_pagina].dir_inicial,
	       tam_pagina,
	       tabla_regiones[numero_region].prot);
      return;
    } else {
      printf("escritura en memoria inválida %p\n",dir_fallo);
      _exit(2);
    } 
  }
  
  if(tabla_regiones[numero_region].compartida && 
     tabla_regiones[numero_region].ino) fallos_con_lectura_fichero++;
  else { 
    if(tabla_regiones[numero_region].tabla_paginas[numero_pagina].pagOrigen && tabla_regiones[numero_region].ino) fallos_con_lectura_fichero++;
    if(!tabla_regiones[numero_region].tabla_paginas[numero_pagina].pagOrigen) fallos_con_lectura_swap++;
    if(tabla_regiones[numero_region].tabla_paginas[numero_pagina].pagOrigen && !tabla_regiones[numero_region].ino) fallos_sin_lectura++;
  }
  
  nuevo_marco = reservar_marco_libre();
  
  if(nuevo_marco == -1) {
    fallos_con_reemplazo++;
    marco_remplazo =reemplazo();
    entrada_tabla_paginas *pagina_aux = leer_entrada_marco(marco_remplazo);
    if ((pagina_aux->pagModif) && (pagina_aux->region->compartida)) escrituras_en_fichero++;
    if ((pagina_aux->pagModif) && (!pagina_aux->region->compartida)) escrituras_en_swap++;
    pagina_aux->numMarco = -1;
    mprotect(pagina_aux->dir_inicial, tam_pagina, PROT_NONE);
    nuevo_marco = marco_remplazo;
  } else fallos_sin_reemplazo++;
  
  rellenar_entrada_marco(nuevo_marco,numero_region,numero_pagina);
  tabla_regiones[numero_region].tabla_paginas[numero_pagina].numMarco = nuevo_marco;
  tabla_regiones[numero_region].tabla_paginas[numero_pagina].pagModif = 0;
  
  fallos_total++;
  
  if(tabla_regiones[numero_region].tabla_paginas[numero_pagina].pagAcceso) fallos_no_forzados++;
  else {
    tabla_regiones[numero_region].tabla_paginas[numero_pagina].pagAcceso = 1;
    fallos_forzados++;
  }
  if (mprotect(tabla_regiones[numero_region].tabla_paginas[numero_pagina].dir_inicial, tam_pagina, PROT_CLR(PROT_WRITE, tabla_regiones[numero_region].prot)) < 0){
    perror("Error devolviendo permisos\n");
    _exit(1);
  }
  return;
}
