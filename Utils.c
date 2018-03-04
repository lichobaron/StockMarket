//Librerías utilizadas
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include "Utils.h"

//Funcion para crear un nuevo pipe, recibe el nombre de este.
//Retorna 0 en caso de exito.
int crearPipe(char pipe[NAME]) {
  unlink(pipe);
  mode_t fifo_mode = S_IRUSR | S_IWUSR;
  if (mkfifo (pipe, fifo_mode) == -1) {
     perror("mkfifo");
     return -1;
  }
  return 0;
}

//Funcion para abrir un pipe creado, recibe el nombre de esto y sus banderas.
//Retorna el id del pipe abierto
int abrirPipe(const char* pathname, int flags){
  int abierto = 0, id_pipe;
  do {
    id_pipe = open(pathname, flags);
    if (id_pipe == -1) {
      perror(pathname);
      printf("Se volvera a intentar despues\n");
      sleep(1);
    } else
      abierto = 1;
  } while (abierto == 0);
  return id_pipe;
}

//Funcion para cerrar un pipe, recibe el nombre del pipe y su ids
void cerrarPipe(char pipe[NAME], int fd){
  close(fd);
  unlink(pipe);
}

//Función para abrir un archivo, recibe el nombre del archivo.
//Retorna un apuntador a este.
FILE *abrirArchivo(char* nArchivo){
  FILE *fe;
  fe = fopen(nArchivo, "r");
  if(fe!= NULL){
    return fe;
  }
  return NULL;
}
