#define MAXEMP 10 //Numero maximo de empresas en el sistemas.
#define MAXNEMP 3 //Tamaño maximo del nombre de las empresas.
#define MAXUSR 10 //Maximo numero de brokers en el sistema.
#define OPRC 50 //Maximo numero de operaciones por empresa.
#define NAME 10 //Tamaño maximo de nombres.
#define MAXCOMANDO 20 //tmano maximo de comando.

//Estructura para almacenar una orden
typedef struct{
  char broker[NAME]; //Nombre del broker que realiza la orden
  char tipo; //Tipo de orden: Venta (V) o Compra (C).
  int acciones; //Acciones a vender.
  int precio; //Precio de cada accion.
  char empresa[MAXNEMP]; //Empresa de las acciones.
}orden;

//Estructura para almacenar las ordenes en una empresa.
typedef struct{
  orden compras[OPRC]; //Tabla de ordenes de compra.
  orden ventas[OPRC]; //Tabla de ordenes de venta.
  int tamCompras; //Tamaño de la tabla de compras.
  int tamVentas; //Tamaño de la tabla de ventas.
}ordenes;

//Estructura para almacenar las empresas y su respectivo libro en el sistema.
typedef struct{
  char empresa[MAXNEMP]; //Nombre de la empresa.
  ordenes libro; //Libro de ordenes de la empresa.
}libroOrdenes;

//Estructura para almacenar los usuarios en el sistema.
typedef struct{
  pid_t pid; //pid del proceso del usuario.
  int user_id; //Numero identificador del usuario.
  char user_name[NAME]; //Nombre identificador del usuario.
  char pipe[NAME]; //Nombre del pipe unico del usuario.
  int pipe_id; //identificador del pipe unico del usuario.
}usuario;

//Estructura para almacenar una empresa que posee el broker y su numero de acciones.
typedef struct{
  char nombre[MAXNEMP]; //Nombre de la empresa.
  int acciones; //Acciones que posee de la empresa.
  int precio;//Precio de una acción
}empresa;

//Estructura para almacenar las solicitudes que se le envian al StockMarket.
typedef struct{
  orden ord; //Orden de compra o venta a procesar, aplica para transacción (T).
  empresa emp[MAXEMP]; //Arreglo de empresas que maneja del usuario, aplica para registo (R).
  char broker[NAME]; //Nombre del broker que envia la solicitud.
  char tipo; //Tipo de solicitud: Transacción (T), Consulta (Q), Registro (R), Desconexón (D).
  pid_t pid; //pid del proceso que envia la solicitud, aplica para registo (R).
  int tamEmpresas; //Tamaño del arreglo anterior, aplica para registo (R).
  char empresa[MAXNEMP]; //Nombre de la empresa a consultar, aplica para consulta (Q).
}solicitud;
