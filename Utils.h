//Librería utilizada para implementar funciones de pipes y archivos.
#define NAME 10 //Tamaño maximo de nombre.

//Firmas de funciones.
int crearPipe(char pipe[NAME]);
int abrirPipe(const char* pathname, int flags);
void cerrarPipe(char pipe[NAME], int fd);
FILE *abrirArchivo(char* nArchivo);
