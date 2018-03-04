/*Código realizado por Carlos Barón y Andrés Cocunubo*/
//Librerías utilizadas
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "Utils.h"
#include "StockMarket.h"

//Declaración Varibles Globales
int fdb;//Pipe usado solamente por el broker y el stockmarket
orden respuesta;//Estructura que se usa como respuesta a las solicitudes que realiza el Broker
int montoInicial=0; //Saldo del broker usado para comprar y vender.
empresa emp[MAXEMP];//Arreglo donde se almacenan las empresas usadas por el Broker
int tamEmpresas=0;//Contador de las empresas que utiliza el Broker

//Declaración de firmas de las funcione utilizadas
int registrarEmpresas(FILE* fe, char *nombre, int acciones);
void asignarEmp( char *nombre, int acciones, int pos);
void mostrarEmp(int pos);
void registroStock(int fd,int tam,char*nomBroker);
void comprarAccion(int fd,char* nEmp,int acciones,int valor,int *montoInicial,int cant,char*nombreBroker);
void venderAccion(int fd,char nEmp[MAXNEMP],int acciones,int valor,int cant,char nombreBroker[NAME]);
void venderTodo(int fd,char *codAccion,int acciones,char *nomBroker);
void consultaEmpresa(int fd,char* nEmp,char*nombreBroker);
void montoBroker();
int buscarEmpresa(char empresa[NAME]);
void procesarCompra();
void procesarVenta();
void procesarQuery();
void desconexion(int fd,char*nomPipe,char*nombreBroker);

//Declaración de la función usada por la señal
typedef void (*sighandler_t)(int);
//Implementación de la función usada por la señal.
/*Cuando recibe la señal por parte del StockMarket el Broker ejecutará las
acciones correspondientes al tipo de respuesta que reciba*/
sighandler_t leer_pipe(void){
  int i;
  read(fdb, &respuesta, sizeof(orden));
  if(respuesta.tipo=='C'){
    procesarCompra();
  }
  else if(respuesta.tipo=='V'){
    procesarVenta();
  }
  else if(respuesta.tipo=='Q'){
    procesarQuery();
  }
}
//Programa principal
int main (int argc, char **argv){
  signal(SIGUSR1,(sighandler_t)leer_pipe);//Creación de la señal que usará el Broker
  //Control del la cantidad de parámetros
  if( argc == 5){
    char nomBroker[NAME];//Declaración del nombre del Broker
    char nomPipe[NAME];//Declaración del nombre del pipe
    FILE *fe;//Declaración variable de lectura de archivo
    //Declaración varibles
    int i, fd, acciones=0, comando=0, valor=0,find=0;
    char nomEmp[MAXNEMP],line[MAXCOMANDO], codAccion[MAXNEMP];
    char *opcion;
    strcpy(nomBroker,argv[1]);//Copiar el nombre del Broker de los argumentos.
    strcpy(nomPipe,argv[2]);//Copiar el nombre del Broker de los argumentos.
    fe = abrirArchivo(argv[3]);//Llamado a función para abrir el archivo con los recursos_iniciales del Broker
    //Control de apertura de apertura de archivo con recursos_iniciales
    if(fe!=NULL){
      montoInicial=atoi(argv[4]);//Asignación del montoInicial
      fd= abrirPipe(nomPipe,O_WRONLY);//Abrir pipe general en modo escritura.
      i=registrarEmpresas(fe,nomEmp,acciones);//Llamado a función para registrar empresas
      registroStock(fd,i,nomBroker);//Función para registrar el Broker con el StockMarket
      mostrarEmp(i);//Función que muestra las empresas que maneja el Broker
      //Ciclo que controla el menú y sus acciones
      while(1){
        //Impresión menú
        printf("\n++++++++++++++++++++++++++++++++++++++++++++++++\n");
        printf("\n\t\tUtilice alguno de los siguientes comandos:\n");
        printf("\tcompra:<código de la acción>:<cantidad>:<valor>\n");
        printf("\tventa:<código de la acción>:<cantidad>:<valor>\n");
        printf("\tconsulta:<código de la acción>::\n");
        printf("\tmonto:::\n");
        printf("\tsalir\n\t" );
        printf("\n++++++++++++++++++++++++++++++++++++++++++++++++\n>");
        //lectura y asignación de las variables necesarias.
        scanf("%s", line);
        opcion=strtok(line,":");
        if(strcmp(opcion,"compra")==0){
          comando=1;
          opcion=strtok(NULL,":");
          strcpy(codAccion,opcion);
          printf("%s\n",codAccion);
          opcion=strtok(NULL,":");
          acciones=atoi(opcion);
          printf("%d\n",acciones);
          opcion=strtok(NULL,":");
          valor=atoi(opcion);
          printf("%d\n",valor);
        }else if(strcmp(opcion,"venta")==0){
          comando=2;
          opcion=strtok(NULL,":");
          strcpy(codAccion,opcion);
          opcion=strtok(NULL,":");
          if(opcion==NULL){
            acciones=-1;
          }else {
            acciones=atoi(opcion);
          }
          opcion=strtok(NULL,":");
          if(opcion==NULL){
            valor=-1;
          }else {
            valor=atoi(opcion);
          }
        }else if(strcmp(opcion,"consulta")==0){
          comando=3;
          opcion=strtok(NULL,":");
          strcpy(codAccion,opcion);
        }else if(strcmp(opcion,"monto")==0){
          comando=4;
        }else if(strcmp(opcion,"salir")==0){
          comando=5;
        }else{
          printf("Opción inválida\nVuelva a intentarlo\n");
        }
        //Se evalúa la variable comando y se ejecuta la función según el menú
        switch (comando) {
          case 1:
          //El broker compra una cantidad de acciones de una empresa
          comprarAccion(fd,codAccion,acciones, valor,&montoInicial,i,nomBroker);
          break;
          case 2:
          //El broker compra una cantidad de acciones de una empresa
          if(acciones==-1 && valor==-1){
            find=buscarEmpresa(codAccion);
            venderTodo(fd,codAccion,emp[find].acciones,nomBroker);
          }else{
            venderAccion(fd,codAccion,acciones, valor,i,nomBroker);
          }
          break;
          case 3:
          //El broker consulta el precio de las acciones de una empresa
          consultaEmpresa(fd,codAccion,nomBroker);
          break;
          case 4:
          //El broker consulta su monto y empresas que tiene a su cargo (nombre y cantidad de acciones)
          montoBroker();
          break;
          case 5:
          //Desconexión del Broker con el StockMarket
          desconexion(fd,nomPipe,nomBroker);
          return(0);
          break;
        }
      }
    }else{
      perror("error: ");
    }
  }else{
    perror("error:");
    printf("Faltan parámetros\nModo de uso: $broker nombreBroker nombrePipe recursos_iniciales monto\n" );
  }

}

//Función auxiliar que con el nombre de la empresa, retorna la posición de dicha empresa en el arreglo
int buscarEmpresa(char empresa[NAME]){
  int retorno=-1; //Declaración variable de retorno
  //Ciclo para recorrer arreglo de empresas y encontrar la posición de la misma
  for(int i=0; i<MAXEMP;i++){
    if(strcmp(empresa, emp[i].nombre)==0){
      retorno=i;//Asiganción posición de la empresa
    }
  }
  return retorno;
}

//Función auxiliar que obtiene la empresas del archivo y la guarda en el sistema
int registrarEmpresas(FILE *fe, char *nombre, int acciones){
  int i = 0; //Contador para ciclo
  //Ciclo para leer del archivo de recursos_iniciales
  while (!feof(fe))  {
    fscanf(fe, "%s %d\n", nombre,&acciones);//Obtener información del archivo
    asignarEmp( nombre,acciones, i++);//Llamado función para cargar empresa al sistema
  }
  fclose(fe);//Se cierra el archivo
  return i;
}

//Función auxiliar para asignar la información de la empresa a la estructura
void asignarEmp(char *nombre, int acciones, int pos){
  //Revisa que el tope de empresas no se exceda.
  if(pos<MAXEMP){
    emp[pos].acciones = acciones;//Se asignan las acciones
    strcpy(emp[pos].nombre, nombre);//Se asignan el nombre
    tamEmpresas +=1;
  }else{
    printf("La cantidad de empresas se ha excedido, no se agregó la empresa %s. \n",emp[pos].nombre );
  }
}

//Función auxiliar para imprimir la empresa
void mostrarEmp(int cant){
  int i;//Declaración contador
  //Impresión por pantalla de las empresas que posee el Broker
  printf("\n\t\tEmpresas:\n");
  printf("Nombre\t\t\tCantidad de acciones\n" );
  for(i=0; i < cant; i++){
    printf("%s\t\t\t%d\n",  emp[i].nombre, emp[i].acciones);
  }
}

//Función para registrar un Broker ante el StockMarket
void registroStock(int fd,int tam, char*nombreBroker){
  int i;//Declaración contador
  solicitud s;//Declaración estructura que se enviará por el pipe
  char name[NAME]="pipe";//Declaración parte del nombre del pipe
  s.tipo='R';//Asiganción tipo de solicitud
  //Asignación de cada una de las empresas que posee el Broker en la estructura
  for(i=0;i<tam;i++){
    s.emp[i]=emp[i];
  }
  s.pid= getpid();//Obtención del pipe del proceso del Broker
  s.tamEmpresas=tam;//Asignación de la cantidad de empresas que maneja el Broker
  strcpy(s.broker,nombreBroker);//Asignación nombre del Broker
  strcat(name,s.broker);//Asignación pipe para nombre del pipe usado solo por este Broker y el StockMarket
  crearPipe(name);//Función para crear el pipe
  fdb= abrirPipe(name, O_RDONLY | O_NONBLOCK);//Función para abrir el pipe
  write(fd,&s,sizeof(solicitud));//Envío de la estructura solicitud con los campos necesarios a través del pipe
  printf("Se realizó el registro del %s con éxito\n", nombreBroker);
}

//Función para comprar acciones de una empresa específicada
void comprarAccion(int fd,char* nEmp,int acciones,int valor,int *montoInicial,int cant,char*nombreBroker){
  solicitud s;//Declaración estructura que se enviará por el pipe
  orden o;//Declaración subestructura orden
  s.tipo='T';//Asiganción tipo de solicitud
  s.tamEmpresas=cant;//Asignación de la cantidad de empresas que maneja el Broker
  strcpy(s.broker,nombreBroker);//Asignación nombre del Broker
  o.tipo='C';//Asiganción tipo de orden
  o.acciones=acciones;//Asignación cantidad de acciones de la orden
  o.precio=valor;//Asignación precio de las acción de la orden
  strcpy(o.broker,nombreBroker);//Asignación nombre del Broker
  strcpy(o.empresa,nEmp);//Asignación nombre empresa a la orden
  s.ord=o;//Asignación de la orden a la estructura solicitud
  //Comprobar que el Broker tenga el saldo suficiente para comprar
  if((valor*acciones)<=*montoInicial){
    write(fd,&s,sizeof(solicitud));//Enviar la solicitud
    printf("Su compra de %d acciones a %d COP cada una, se envío al mercado.\n",acciones,valor);
  }else{
    printf("El saldo del broker es insuficiente.\n" );
  }
}

//Función para venta de acciones de una empresa específicada
void venderAccion(int fd,char nEmp[MAXNEMP],int acciones,int valor,int cant,char nombreBroker[NAME]){
  solicitud s;//Declaración estructura que se enviará por el pipe
  orden o;//Declaración subestructura orden
  int i=buscarEmpresa(nEmp);//Función que retorna la posición de la empresa
  //Condición para saber si la empresa existe
  if(i!=-1){
    s.tipo='T';//Asiganción tipo de solicitud
    s.tamEmpresas=cant;//Asignación de la cantidad de empresas que maneja el Broker
    strcpy(s.broker,nombreBroker);//Asignación nombre del Broker
    o.tipo='V';//Asiganción tipo de orden
    strcpy(o.empresa,nEmp);//Asignación nombre empresa a la orden
    strcpy(o.broker,nombreBroker);//Asignación nombre del Broker
    o.acciones=acciones;//Asignación cantidad de acciones de la orden
    o.precio=valor;//Asignación precio de las acción de la orden
    s.ord=o;//Asignación de la orden a la estructura solicitud
    write(fd,&s,sizeof(solicitud));//Envío estructura a través del pipe
    printf("Su venta de %d acciones a %d COP cada una, se envío al mercado.\n",acciones,valor);
  }else{
    printf("La empresa no se encuentra en las manejadas por el broker\n" );
  }
}

void venderTodo(int fd,char *codAccion,int acciones,char *nomBroker){
  solicitud s;//Declaración estructura que se enviará por el pipe
  orden o;//Declaración subestructura orden
  int i=buscarEmpresa(codAccion);//Función que retorna la posición de la empresa
  //Condición para saber si la empresa existe
  if(i!=-1){
    s.tipo='T';//Asiganción tipo de solicitud
    s.tamEmpresas=tamEmpresas;//Asignación de la cantidad de empresas que maneja el Broker
    strcpy(s.broker,nomBroker);//Asignación no    mostrarEmp(tamEmpresas);mbre del Broker
    o.tipo='V';//Asiganción tipo de orden
    strcpy(o.empresa,codAccion);//Asignación nombre empresa a la orden
    strcpy(o.broker,nomBroker);//Asignación nombre del Broker
    o.acciones=acciones;//Asignación cantidad de acciones de la orden
    consultaEmpresa(fd,codAccion,nomBroker);//Conocer el precio de las acciones en el mercado
    pause();//Esperar un tiempo para que el StockMarket retorne el precio
    if(emp[i].precio!=0){
      o.precio=emp[i].precio;//Asignación precio acción
      s.ord=o;
      write(fd,&s,sizeof(solicitud));
      printf("Su venta de %d acciones se envío al mercado.\n",acciones);
    }
  }else{
    printf("La empresa no se encuentra en las manejadas por el broker\n" );
  }
}

//Función para conocer el precio actual de una acción de una empresa específica en el mercado
void consultaEmpresa(int fd,char* nEmp,char*nombreBroker){
  solicitud s;//Declaración estructura que se enviará por el pipe
  s.tipo='Q';//Asiganción tipo de solicitud
  strcpy(s.empresa,nEmp);//Asignación nombre empresa a la orden
  strcpy(s.broker,nombreBroker);//Asignación nombre del Broker
  write(fd,&s,sizeof(solicitud));//Envío solicitud a través del pipe
  printf("La consulta se envío al StockMarket\n" );
}

//Función para conocer el saldo del Broker y las empresas que maneja
void montoBroker(){
  printf("Saldo disponible: %d\n",montoInicial);
  mostrarEmp(tamEmpresas);//Mostrar empresas que maneja el Broker
}

//Función auxiliar cuando se recibe una señal de compra
void procesarCompra(){
  montoInicial= montoInicial-(respuesta.precio*respuesta.acciones);//Actulizar el saldo del Broker
  int e=buscarEmpresa(respuesta.empresa);//Obtener posición de la empresa
  //Se verifica que la empresa exista sino se agrega al arreglo de empresas del Broker
  if(e!=-1){
    emp[e].acciones=emp[e].acciones+respuesta.acciones;//Actualizar la cantidad de acciones de la empresa
  }
  else{
    tamEmpresas +=1;//Aumento de cantidad de empresa que maneja el Broker
    asignarEmp(respuesta.empresa,respuesta.acciones,tamEmpresas);//Asignación empresa que no posea el Broker
  }
}

//Función auxiliar cuando se recibe una señal de venta
void procesarVenta(){
  montoInicial= montoInicial+(respuesta.precio*respuesta.acciones);//Actulizar el saldo del Broker
  int e=buscarEmpresa(respuesta.empresa);//Obtener posición de la empresa
  //Se verifica que la empresa exista
  if(e!=-1){
    emp[e].acciones=emp[e].acciones-respuesta.acciones;//Actualizar la cantidad de acciones de la empresa
  }else{
    printf("Error la empresa %s, no existe\n",respuesta.empresa );
  }
}

//Función auxiliar cuando se recibe una señal de consulta
void procesarQuery(){
  int i;
  printf("precio: %d\n",respuesta.precio );
  if(respuesta.precio!=0){
    i=buscarEmpresa(respuesta.empresa);
    if(i!=-1){
      emp[i].precio=respuesta.precio;
    }
    printf("El precio de las acciones de la empresa %s es %d\n",respuesta.empresa,respuesta.precio);
  }else{
    printf("No hay compras registradas para la empresa %s\n",respuesta.empresa );
  }
}

//Función para salir del mercado por parte del Broker
void desconexion(int fd,char*nomPipe,char*nombreBroker){
  solicitud s;//Declaración estructura que se enviará por el pipe
  s.tipo='D';//Asiganción tipo de solicitud
  strcpy(s.broker,nombreBroker);//Asignación nombre del Broker
  write(fd,&s,sizeof(solicitud));//Envío de la solicitud al mercado através del pipe
  unlink(nomPipe);//Eliminar el pipe
}
