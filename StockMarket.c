/*Código realizado por Carlos Barón y Andrés Cocunubo*/
//Librerías utilizadas
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include "StockMarket.h"
#include "Utils.h"
//Declaración Varibles Globales.
usuario users[MAXUSR]; //Arreglo donde se almacenan los usuarios del sistema.
int tamUsers = 0; //Tamaño del arreglo de usuarios.
libroOrdenes libroEmpresas[MAXEMP]; //Arreglo donde se alamacenan las ordenes de cada empresa.s
int tamEmpresas = 0;// Tamaño del arreglo de ordenes por empresa.

//Firmas de funciones utilizadas
void procesarSolicitud(solicitud sol);
void registro(solicitud sol);
void desconexion(solicitud sol);
void registrarUsuario(solicitud sol);
void registrarEmpresas(solicitud sol);
void recibirOrden(orden x);
void registrarOperacion(orden x, int emp);
void procesarCompra(orden x, int emp);
void procesarVenta(orden x, int emp);
void consultarPrecio(solicitud sol);
int venderAcciones(orden x, int emp);
int comprarAcciones(orden x, int emp);
int buscarEmpresa(char empresa[NAME]);
int buscarUsuario(char user[NAME]);
int consultarLibro(int emp);
void eliminarOrden(int emp, char tipo);

//Función principal.
int main (int argc, char **argv){

  int fd; //Entero donde se guarda el pipe_id principal.
  solicitud sol; //Estructura donde las llegaran las solicitudes al sistema.

  //Verificación parámetros.
  if(argc > 2 || argc < 2){
    perror("Los parametros no son correctos");
    printf("Modo de uso: ./StockMarket nombre_de_pipe\n");
  }
  else{
    crearPipe(argv[1]); //Llamado de función para crear el pipe en donde llegarán las solicitudes.
    fd = abrirPipe(argv[1], O_RDONLY); //Función para abrir el pipe, modo lectura.
    printf ("Se abrio el pipe principal\n");
    sleep(1);

    while(read (fd, &sol, sizeof(solicitud))){ //Ciclo donde se ejecuta el servidor mientras le llegan solicitudes.
      procesarSolicitud(sol); //Llamado a función para procesar solicitudes.
    }
    cerrarPipe(argv[1], fd); //Función para cerrar el pipe principal.
  }
  exit(0);
}

//Función donde se procesan las solicitudes que llegan al sistema, independiente de su tipo.
void procesarSolicitud(solicitud sol){
  printf("----------------Tipo de solicitud: %c----------------\n\n", sol.tipo);
  switch (sol.tipo) {
    case 'T':
      recibirOrden(sol.ord); //Caso para solicitudes de compra o venta.
      break;
    case 'Q':
      consultarPrecio(sol); //Caso para solicitudes de consulta.
      break;
    case 'R':
      registro(sol); //Caso para solicitudes de registo.
      break;
    case 'D':
      desconexion(sol); //Caso para solicitudes de desconexón.
      break;
  }
  printf("\n");
}

//Función para terminar la conexión con un usuario, recibe una solicitud de desconexón.
void desconexion(solicitud sol){
  int u= buscarUsuario(sol.broker); //Se busca al usuario que envia la solicitud en el sistema.
  if(u == -1){
    perror("No existe usuario para la desconexion.");
  }
  else{
    cerrarPipe(users[u].pipe, users[u].pipe_id); //Función para cerra la comunicación con el sistema.
    printf("El usuario %s con pipe %s se ha desconectado del sistema. \n", users[u].user_name, users[u].pipe);
  }
}

//Función para consultar el valor de las acciones de una empresa, recibe una solicitud de consulta.
void consultarPrecio(solicitud sol){
  int r; //Variable donde se guarda el precio de la acción.
  orden ord; //Estructura para responder solicitud del usuario.
  ord.tipo= 'Q';
  int e= buscarEmpresa(sol.empresa); //Variable donde se guadar la posición de la empresa en el libro.
  if(e==-1){
    perror("Fallo en consulta: La empresa no existe.");
    r= -1; //Asignación si la empresa no existe.
  }
  else{
    if(libroEmpresas[e].libro.tamVentas>0){ //Verificación de que la empresa posea ventas.
      r= libroEmpresas[e].libro.ventas[0].precio; //Asignación del precio de las acciones.
    }
    else{
      perror("Fallo en consulta: No hay ventas de la empresa.");
      r= 0; //Asignación si la empresa no posee ventas.
    }
  }
  int u = buscarUsuario(sol.broker); //Busqueda del usuario de la solicitud en el sistema.
  if(u==-1){
    perror("Fallo en consulta: no existe el usuario");
  }
  else{
    ord.precio = r; //Asignación del precio de las acciones.
    strcpy(ord.empresa,libroEmpresas[e].empresa); //Asignación nombre de la empresa.
    write(users[u].pipe_id,&ord,sizeof(orden)); //Escritura en el pipe del usuario.
    kill(users[u].pid, SIGUSR1); //Envio de señal para que el usuario lea del pipe.
    printf("El usuario %s ha realizado una consulta de la empresa %s\n",
      users[u].user_name, ord.empresa);
  }
}

//Funcion para registrar un usuario en el sistema, recibe una solicitud de registro.
void registro(solicitud sol){
  registrarUsuario(sol); //Registro del usuario.
  registrarEmpresas(sol); //Registo de sus empresas.
}

//Funcion auxiliar para registrar un usuario en el sistema, recibe una solicitud de registro.
void registrarUsuario(solicitud sol){
  int u= buscarUsuario(sol.broker); //Verificación de que el usuario no exista.
  if(u==-1){
    usuario user; //Creación de un nuevo usuario.
    user.user_id=tamUsers;
    char name[NAME]= "pipe";
    strcat(name,sol.broker);
    strcpy(user.pipe,name);
    strcpy(user.user_name,sol.broker); //Asignación identificador unico del usuario.
    user.pid= sol.pid; //Asignación del p_id del proceso.
    user.pipe_id= abrirPipe(user.pipe,O_WRONLY | O_NONBLOCK); //Funcion para abrir el pipe unico del usuario;
    users[tamUsers]=user; //Registro del usuario en el sistema.
    tamUsers+=1;
    printf("El usuario %s se ha registrado en el sistema con p_id %d y pipe unico %s.\n", user.user_name, user.pid, user.pipe);
  }
  else{
    printf("El usuario %s ya se encuentra en el sistema.\n", sol.broker);
  }
}

//Función auxiliar para registrar las empresas que maneja cada usuario, recibe una solicitud de registo..
void registrarEmpresas(solicitud sol){
  for(int i=0;i<sol.tamEmpresas;i++){ //Recorrido de las funciones que llegan en la solicitud.
    int e= buscarEmpresa(sol.emp[i].nombre);
    if(e==-1){ //Acción si la empresa no esta registrada.
      strcpy(libroEmpresas[tamEmpresas].empresa, sol.emp[i].nombre); //Asignación identificador unico de la empresa.
      libroEmpresas[tamEmpresas].libro.tamVentas=0;
      libroEmpresas[tamEmpresas].libro.tamCompras=0;
      tamEmpresas+=1;
      printf("Se ha registrado la empresa %s en el sistema.\n", libroEmpresas[tamEmpresas-1].empresa);
    }
    else{
      printf("La empresa %s ya se encontraba registrada en el sistema.\n", libroEmpresas[e].empresa);
    }
  }
}

//Función auxiliar para buscar un usuario en el sistema.
int buscarUsuario(char user[NAME]){
  int retorno = -1; //Retorna -1 si no lo encuentra.
  for(int i=0; i<tamUsers; i++){
    if(strcmp(user, users[i].user_name)==0){
      retorno= i; //Retorna la posición del usuario en el sistema.
    }
  }
  return retorno;
}

//Función auxiliar para buscar una empresa en el sistema.
int buscarEmpresa(char empresa[NAME]){
  int retorno=-1; //Retorna -1 si no lo encuentra.
  for(int i=0; i<MAXEMP;i++){
    if(strcmp(empresa, libroEmpresas[i].empresa)==0){
      retorno=i; //Retorna la posición de la empresa en el sistema.
    }
  }
  return retorno;
}

//Función para recibir las ordes de compra o venta en el sitema, recibe una orden.
void recibirOrden(orden x){
  int emp=buscarEmpresa(x.empresa); //Verificación de la empresa en el sistema.
  if(emp == -1){
    perror("Error de transacción: no existe empresa.");
  }
  else{
    registrarOperacion(x, emp); //Funcion para procesar la orden.
  }
}

//Funcion auxiliar para registrar una compra o venta, recibe la orden y la posición de la empresa en el sistema.
void registrarOperacion(orden x, int emp){
  int oper = consultarLibro(emp); //Llmado a función para saber el tipo de operación a realizar;
  if(oper == 1){ //Registro de operación inicial.
    if(x.tipo == 'C'){ //Compra.
      libroEmpresas[emp].libro.compras[0]=x;
      libroEmpresas[emp].libro.tamCompras=1;
      printf("Se ha registrado una compra de la empresa %s de %d acciones a %d COP por parte del broker %s\n",
        x.empresa, x.acciones, x.precio, x.broker);
    }
    else{ //Venta.s
      libroEmpresas[emp].libro.ventas[0]=x;
      libroEmpresas[emp].libro.tamVentas=1;
      printf("Se ha registrado una venta de la empresa %s de %d acciones a %d COP por parte del broker %s\n",
        x.empresa, x.acciones, x.precio, x.broker);
    }
  }
  else{ //Registro de operación ordinaria.
    if(x.tipo =='C'){ //Compra
      procesarCompra(x, emp); //Funcion compras.
    }
    else{ //Venta
      procesarVenta(x, emp); //Funcion ventas.
    }
  }
}

//Funcion auxiliar para consultar el libro de empresas.
//Retorna 1 si no hay operaciones inicales, 2 de los contrario, 0 en caso de error.
int consultarLibro(int emp){
  int retorno=0;
  if(libroEmpresas[emp].libro.tamCompras==0 && libroEmpresas[emp].libro.tamVentas==0){
    retorno=1; //Operación inicial.
  }
  if(libroEmpresas[emp].libro.tamCompras>=1 || libroEmpresas[emp].libro.tamVentas>=1){
    retorno=2; //Operación ordinaria.
  }
  return retorno;
}

//Función auxiliar para eliminar una orden el libro de una empresa, recibe la posición de la empresa y el tipo de orden.
void eliminarOrden(int emp, char tipo){
  if(tipo=='C'){ //Eliminación compra.
    for(int i=0;i<libroEmpresas[emp].libro.tamCompras-1;i++){
      libroEmpresas[emp].libro.compras[i]=libroEmpresas[emp].libro.compras[i+1];
    }
    libroEmpresas[emp].libro.tamCompras-=1;
  }
  else{ //Eliminación venta.
    for(int i=0;i<libroEmpresas[emp].libro.tamVentas-1;i++){
      libroEmpresas[emp].libro.ventas[i]=libroEmpresas[emp].libro.ventas[i+1];
    }
    libroEmpresas[emp].libro.tamVentas-=1;
  }
}

//Función auxiliar para procesar una solicitud de compra, recibe la orden de compra y la posición de la empresa.
void procesarCompra(orden x, int emp){
  int op = comprarAcciones(x, emp); //Compra de acciones
  if(op == 0){ //No se compraron las acciones.
    //Registro de la compra en el libro de la empresa, de acuerdo a su precio.
    int flag=0;
    for(int i=0;i<libroEmpresas[emp].libro.tamCompras;i++){
      if(x.precio>=libroEmpresas[emp].libro.compras[i].precio){
        for(int j=libroEmpresas[emp].libro.tamCompras+1;j>i;j--){
          libroEmpresas[emp].libro.compras[j]=libroEmpresas[emp].libro.compras[j-1];
        }
        libroEmpresas[emp].libro.compras[i]=x;
        flag=1;
        break;
      }
    }
    if(flag==0){
      libroEmpresas[emp].libro.compras[libroEmpresas[emp].libro.tamCompras]=x;
    }
    printf("Se ha registrado una compra de la empresa %s de %d acciones a %d COP por parte del broker %s\n",
      x.empresa, x.acciones, x.precio, x.broker);
    libroEmpresas[emp].libro.tamCompras+=1;
  }
}

//Función auxiliar para realizar una compra de acciones, recibe la orden de compra y la posición de la empresa.
//Retorna 0 si no se compraron o no se compraron todas las acciones, 1 si se pudieron comprar todas;
int comprarAcciones(orden x, int emp){
  int retorno = 0; //No se realiza compra.
  if(libroEmpresas[emp].libro.tamVentas>0){ //Verificación de que existan ventas disponibles.
    if(x.precio>=libroEmpresas[emp].libro.ventas[0].precio){ //Verificación de que el precio permita realizar la compra.
      int v = libroEmpresas[emp].libro.ventas[0].acciones - x.acciones; //Diferencia en la transacción.
      if(v == 0){ //Caso en donde se compran y se venden todas las acciones.
        orden respuestaV, respuestaC; //Creación de respuestas.
        respuestaC.tipo='C';
        respuestaC.acciones= x.acciones;
        respuestaC.precio= libroEmpresas[emp].libro.ventas[0].precio;
        strcpy(respuestaC.empresa, libroEmpresas[emp].empresa);
        respuestaV.tipo='V';
        respuestaV.acciones= x.acciones;
        respuestaV.precio= libroEmpresas[emp].libro.ventas[0].precio;
        strcpy(respuestaV.empresa, libroEmpresas[emp].empresa);
        //Usuarios implicados en la transacción.
        int uc = buscarUsuario(x.broker); //Comprador.
        int uv = buscarUsuario(libroEmpresas[emp].libro.ventas[0].broker); //Vendedor.
        //Escritura en el pipe de cada usuario.
        write(users[uc].pipe_id,&respuestaC, sizeof(orden));
        write(users[uv].pipe_id,&respuestaV, sizeof(orden));
        //Envio de señales para avisar de lectura en el pipe.
        kill(users[uc].pid,SIGUSR1);
        kill(users[uv].pid,SIGUSR1);
        printf("El broker %s ha realizado una compra de %d acciones de la empresa %s a %d COP.\n",
          x.broker, x.acciones, respuestaC.empresa, respuestaC.precio);
        printf("El broker %s ha realizado una venta de %d acciones de la empresa %s a %d COP.\n",
          libroEmpresas[emp].libro.ventas[0].broker, x.acciones, respuestaC.empresa, respuestaC.precio);
        eliminarOrden(emp, 'V'); //Eliminar orden de venta.
        retorno = 1; //Compra realizada.
      }
      if(v > 0){ //Caso en donde se compra una porción de la venta.
        orden nuevaVenta; //Creacion de la nueva venta a procesar.
        nuevaVenta.tipo ='V';
        nuevaVenta.acciones =v;
        nuevaVenta.precio = libroEmpresas[emp].libro.ventas[0].precio;
        strcpy(nuevaVenta.empresa, libroEmpresas[emp].libro.ventas[0].empresa);
        orden respuestaV, respuestaC; //Creación de las respuestas.
        respuestaC.tipo='C';
        respuestaC.acciones= x.acciones;
        respuestaC.precio= libroEmpresas[emp].libro.ventas[0].precio;
        strcpy(respuestaC.empresa, libroEmpresas[emp].empresa);
        respuestaV.tipo='V';
        respuestaV.acciones= x.acciones;
        respuestaV.precio= libroEmpresas[emp].libro.ventas[0].precio;
        strcpy(respuestaV.empresa, libroEmpresas[emp].empresa);
        //Usuarios implicados en la transacción.
        int uc = buscarUsuario(x.broker); //Comprador.
        int uv = buscarUsuario(libroEmpresas[emp].libro.ventas[0].broker); //Vendedor.
        //Escritura en el pipe de cada usuario.
        write(users[uc].pipe_id,&respuestaC, sizeof(orden));
        write(users[uv].pipe_id,&respuestaV, sizeof(orden));
        //Envio de señales para avisar de lectura en el pipe.
        kill(users[uc].pid,SIGUSR1);
        kill(users[uv].pid,SIGUSR1);
        printf("El broker %s ha realizado una compra de %d acciones de la empresa %s a %d COP.\n",
          x.broker, x.acciones, respuestaC.empresa, respuestaC.precio);
        printf("El broker %s ha realizado una venta de %d acciones de la empresa %s a %d COP.\n",
          libroEmpresas[emp].libro.ventas[0].broker, x.acciones, respuestaC.empresa, respuestaC.precio);
        eliminarOrden(emp, 'V'); //Eliminar orden de venta vieja
        strcpy(nuevaVenta.broker, libroEmpresas[emp].libro.ventas[0].broker);
        procesarVenta(nuevaVenta, emp); //Procesar nueva orden de venta.
        retorno=1; //Compra realizada.
      }
      if(v < 0){ //Caso en donde se satisface una porción de la compra.
        x.acciones=v*-1; //Actulizar la orden.
        orden respuestaV, respuestaC; //Creación de las respuestas.
        respuestaC.tipo='C';
        respuestaC.acciones= libroEmpresas[emp].libro.ventas[0].acciones;
        respuestaC.precio= libroEmpresas[emp].libro.ventas[0].precio;
        strcpy(respuestaC.empresa, libroEmpresas[emp].empresa);
        respuestaV.tipo='V';
        respuestaV.acciones= libroEmpresas[emp].libro.ventas[0].acciones;
        respuestaV.precio= libroEmpresas[emp].libro.ventas[0].precio;
        strcpy(respuestaV.empresa, libroEmpresas[emp].empresa);
        //Usuarios implicados en la transacción.
        int uc = buscarUsuario(x.broker); //Comprador.
        int uv = buscarUsuario(libroEmpresas[emp].libro.ventas[0].broker); //Vendedor.
        //Escritura en el pipe de cada usuario.
        write(users[uc].pipe_id,&respuestaC, sizeof(orden));
        write(users[uv].pipe_id,&respuestaV, sizeof(orden));
        //Envio de señales para avisar de lectura en el pipe.
        kill(users[uc].pid,SIGUSR1);
        kill(users[uv].pid,SIGUSR1);
        printf("El broker %s ha realizado una compra de %d acciones de la empresa %s a %d COP.\n",
          x.broker, respuestaC.acciones, respuestaC.empresa, respuestaC.precio);
        printf("El broker %s ha realizado una venta de %d acciones de la empresa %s a %d COP.\n",
          libroEmpresas[emp].libro.ventas[0].broker, respuestaV.acciones, respuestaC.empresa, respuestaC.precio);
        eliminarOrden(emp, 'V'); //Eliminar orden de venta.
        procesarCompra(x, emp); //Procesar la nueva compra.
        retorno=1; //Compra realizada.
      }
    }
    else{
      perror("Compra de acciones: El precio no permite la compra.");
      printf("Se almacenara la compra de %s por %d de la empresa %s a %d COP.\n",
        x.broker, x.acciones, x.empresa, x.precio);
    }
  }
  else{
    perror("Compra de acciones: No existen ventas disponibles");
    printf("Se almacenara la compra de %s por %d de la empresa %s a %d COP.\n",
      x.broker, x.acciones, x.empresa, x.precio);
  }
  return retorno;
}

//Función auxiliar para procesar una solicitud de venta, recibe la orden de venta y la posición de la empresa.
void procesarVenta(orden x, int emp){
  int op = venderAcciones(x, emp); //Venta de acciones.
  if(op == 0){ //No se vendieron las acciones.
    //Registro de la venta en el libro de la empresa, de acuerdo a su precio.
    int flag=0;
    for(int i=0;i<libroEmpresas[emp].libro.tamVentas;i++){
      if(x.precio<=libroEmpresas[emp].libro.ventas[i].precio){
        for(int j=libroEmpresas[emp].libro.tamVentas+1;j>i;j--){
          libroEmpresas[emp].libro.ventas[j]=libroEmpresas[emp].libro.ventas[j-1];
        }
        libroEmpresas[emp].libro.ventas[i]=x;
        flag=1;
        break;
      }
    }
    if(flag==0){
      libroEmpresas[emp].libro.ventas[libroEmpresas[emp].libro.tamVentas]=x;
    }
    printf("Se ha registrado una venta de la empresa %s de %d acciones a %d COP por parte del broker %s\n",
      x.empresa, x.acciones, x.precio, x.broker);
    libroEmpresas[emp].libro.tamVentas+=1;
  }
}

//Función auxiliar para realizar una venta de acciones, recibe la orden de venta y la posición de la empresa.
//Retorna 0 si no se vendieron o no se vendieron todas las acciones, 1 si se pudieron vender todas;
int venderAcciones(orden x, int emp){
  int retorno=0; //No se realiza venta.
  if(libroEmpresas[emp].libro.tamCompras>0){ //Verificación de que existan compras disponibles.
    if(x.precio<=libroEmpresas[emp].libro.compras[0].precio){ //Verificación de que el precio permita realizar la venta.
      int v = libroEmpresas[emp].libro.compras[0].acciones - x.acciones; //Diferencia en la transacción.
      if(v == 0){  //Caso en donde se compran y se venden todas las acciones.
        orden respuestaV, respuestaC; //Creacion de respuestas.
        respuestaC.tipo='C';
        respuestaC.acciones= x.acciones;
        respuestaC.precio= libroEmpresas[emp].libro.compras[0].precio;
        strcpy(respuestaC.empresa, libroEmpresas[emp].empresa);
        respuestaV.tipo='V';
        respuestaV.acciones= x.acciones;
        respuestaV.precio= libroEmpresas[emp].libro.compras[0].precio;
        strcpy(respuestaV.empresa, libroEmpresas[emp].empresa);
        //Usuarios implicados en la transacción.
        int uc = buscarUsuario(libroEmpresas[emp].libro.compras[0].broker); //Comprador.
        int uv = buscarUsuario(x.broker); //Vendedor.
        if(uc == -1 || uv == -1){
          perror(" No existe el usuario ");
        }
        else{
          printf("USER 1: %d USER2: %d\n", users[uc].pid, users[uv].pid);
        }
        //Escritura en el pipe de cada usuario.
        write(users[uc].pipe_id,&respuestaC, sizeof(orden));
        write(users[uv].pipe_id,&respuestaV, sizeof(orden));
        //Envio de señales para avisar de lectura en el pipe.
        kill(users[uc].pid,SIGUSR1);
        kill(users[uv].pid,SIGUSR1);
        printf("El broker %s ha realizado una compra de %d acciones de la empresa %s a %d COP.\n",
          libroEmpresas[emp].libro.compras[0].broker, x.acciones, respuestaC.empresa, respuestaC.precio);
        printf("El broker %s ha realizado una venta de %d acciones de la empresa %s a %d COP.\n",
          x.broker, x.acciones, respuestaC.empresa, respuestaC.precio);
        eliminarOrden(emp, 'C'); //Eliminar orden de compra.
        retorno = 1; //Venta realizada.
      }
      if(v < 0){ //Caso en donde se vende una porción de las acciones.
        orden respuestaV, respuestaC; //Creacion de respuestas.
        respuestaC.tipo='C';
        respuestaC.acciones=libroEmpresas[emp].libro.compras[0].acciones;
        respuestaC.precio= libroEmpresas[emp].libro.compras[0].precio;
        strcpy(respuestaC.empresa, libroEmpresas[emp].empresa);
        respuestaV.tipo='V';
        respuestaV.acciones= libroEmpresas[emp].libro.compras[0].acciones;
        respuestaV.precio= libroEmpresas[emp].libro.compras[0].precio;
        strcpy(respuestaV.empresa, libroEmpresas[emp].empresa);
        //Usuarios implicados en la transacción.
        int uc = buscarUsuario(libroEmpresas[emp].libro.compras[0].broker); //Comprador.
        int uv = buscarUsuario(x.broker); //Vendedor.
        //Escritura en el pipe de cada usuario.
        write(users[uc].pipe_id,&respuestaC, sizeof(orden));
        write(users[uv].pipe_id,&respuestaV, sizeof(orden));
        //Envio de señales para avisar de lectura en el pipe.
        kill(users[uc].pid,SIGUSR1);
        kill(users[uv].pid,SIGUSR1);
        printf("El broker %s ha realizado una compra de %d acciones de la empresa %s a %d COP.\n",
          libroEmpresas[emp].libro.compras[0].broker, respuestaC.acciones, respuestaC.empresa, respuestaC.precio);
        printf("El broker %s ha realizado una venta de %d acciones de la empresa %s a %d COP.\n",
          x.broker, respuestaV.acciones, respuestaC.empresa, respuestaC.precio);
        eliminarOrden(emp, 'C'); //Eliminar orden de compra.
        x.acciones = v*-1; //Actualizar acciones de la orden de venta.
        procesarVenta(x,emp); //Procesar nueva venta.
        retorno = 1; //Venta realizada.
      }
      if(v > 0){ //Caso en donde se venden todas las acciones pero no se satisface toda la compra
        orden nuevaCompra; //Creación de la nueva compra a procesar.
        nuevaCompra.tipo ='C';
        nuevaCompra.acciones =v;
        nuevaCompra.precio = libroEmpresas[emp].libro.compras[0].precio;
        strcpy(nuevaCompra.empresa, libroEmpresas[emp].libro.compras[0].empresa);
        strcpy(nuevaCompra.broker, libroEmpresas[emp].libro.compras[0].broker);
        orden respuestaV, respuestaC; //Creacion de respuestas.
        respuestaC.tipo='C';
        respuestaC.acciones= x.acciones;
        respuestaC.precio= libroEmpresas[emp].libro.compras[0].precio;
        strcpy(respuestaC.empresa, libroEmpresas[emp].empresa);
        respuestaV.tipo='V';
        respuestaV.acciones= x.acciones;
        respuestaV.precio= libroEmpresas[emp].libro.compras[0].precio;
        strcpy(respuestaV.empresa, libroEmpresas[emp].empresa);
        //Usuarios implicados en la transacción.
        int uc = buscarUsuario(libroEmpresas[emp].libro.compras[0].broker); //Comprador.
        int uv = buscarUsuario(x.broker); //Vendedor.
        //Escritura en el pipe de cada usuario.
        write(users[uc].pipe_id,&respuestaC, sizeof(orden));
        write(users[uv].pipe_id,&respuestaV, sizeof(orden));
        //Envio de señales para avisar de lectura en el pipe.
        kill(users[uc].pid,SIGUSR1);
        kill(users[uv].pid,SIGUSR1);
        printf("El broker %s ha realizado una compra de %d acciones de la empresa %s a %d COP.\n",
          libroEmpresas[emp].libro.compras[0].broker, respuestaC.acciones, respuestaC.empresa, respuestaC.precio);
        printf("El broker %s ha realizado una venta de %d acciones de la empresa %s a %d COP.\n",
          x.broker, respuestaV.acciones, respuestaC.empresa, respuestaC.precio);
        eliminarOrden(emp, 'C'); //Eliminar orden de compra vieja.
        procesarCompra(nuevaCompra, emp); //Procesar nueva orden de compra.
        retorno=1; //Compra realiza.
      }
    }
    else{
      perror("Venta de acciones: El precio no permite la venta.");
      printf("Se almacenara la venta de %s por %d de la empresa %s a %d COP.\n",
        x.broker, x.acciones, x.empresa, x.precio);
    }
  }
  else{
    perror("Venta de acciones: No existen compras disponibles");
    printf("Se almacenara la venta de %s por %d de la empresa %s a %d COP.\n",
      x.broker, x.acciones, x.empresa, x.precio);
  }
  return retorno;
}
