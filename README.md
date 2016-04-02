# Socket Library para C

Esta librería permite implementar la comunicación entre procesos con arquitectura [cliente-servidor](https://es.wikipedia.org/wiki/Cliente-servidor) a través de sockets de un modo bastante simple.

Para comprender el funcionamiento de la librería y cómo funcionan los sockets en C, recomiendo [esta guía](http://www.tyr.unlu.edu.ar/tyr/TYR-trab/satobigal/documentacion/beej/index.html) la cual me ayudo a realizar esta librería.

Este repositorio cuenta con 2 proyectos los cuales son el cliente y el servidor, y con un proyecto shared library "Includes", donde se encuentra la libreria de socket.c la cual implementan tanto el cliente como el servidor. Además hay una librería util.c, la cual contiene un ejemplo de cómo realizar serialización y un método para implementar sincronización bastante simple.

## Lógica de funcionamiento
El funcionamiento es algo similar a la [arquitectura SOA](https://es.wikipedia.org/wiki/Arquitectura_orientada_a_servicios), en donde tanto el servidor y cliente van a tener declaradas funciones, las cuales podrán ser llamadas por los procesos que estén conectados. Es decir, el cliente va a poder llamar a las funciones del servidor y el servidor va a poder llamar a las funciones del cliente. Claramente estas funciones no tienen un return, para eso la misma función si tiene que devolverle un resultado al proceso que lo llamo, tiene que hacer llamado a una función con el mismo procedimiento. (En la arquitectura SOA, solo el cliente "consume" del servidor "servicios" (no al revés) y estos si tienen un return; sería interesante modificar la librería para que permita un "return", en algún futuro con tiempo lo voy a desarrollar).

En el ejemplo de este proyecto, se inicializa un proceso servidor, el cual se pone a la escucha definiendo una única función client_saludar() (la cual podrá ser llamada por los clientes que se conecten). Luego se inicializan los clientes, los cuales se conectan al proceso anteriormente inicializado definiendo una función server_saludar() (la cual podrá ser llamada por el servidor). Una vez que un cliente se conecta llama a la función client_saludar del servidor, la cual recibe 3 parámetros. El servidor al recibir esto lo imprime por pantalla y a continuación llama a la función server_saludar del cliente que recibe 2 parámetros. El cliente imprime lo que le envía el servidor, y vuelve a llamar a la función saludar del servidor. Esto provoca un bucle infinito de saludos mutuos.

Para comprender 100% a utilizar esta librería recomiendo modificar este ejemplo haciendo un chat.

## Procedimiento para realizar la prueba
* Importar al eclipse los 3 proyectos.
* Instalar la [Commons Library](https://github.com/sisoputnfrba/so-commons-library) y configurarla en los 3 proyectos.
* Configurar en los 3 proyectos la librería pthread.
* Configurar la shared library. ([Video paso a paso youtube](https://www.youtube.com/watch?v=s5ac8CPDkMg))
* Compilar la shared library (entrar en socket.c o útil.c y compilar), el cliente y el servidor.
* Finalmente se ejecutar una instancia de servidor y una o varias instancias de clientes.

## Funciones
El proceso que se quiera poner a la escucha utiliza la función:<br />
`createListen(portServer, &newClient, fns, &client_connectionClosed, data);` <br />
**portServer** = puerto en el que se pone a la escucha.<br />
**newClient** = referencia a función que se quiere que se llame cuando se conecta un proceso. (puede ingresarse NULL para que no se llame ninguna función).<br />
**fns** = referencias de funciones del proceso que los otros procesos conectados van a poder llamar.<br />
**client_connectionClosed** = referencia a función que se quiere que se llame cuando se desconecta un proceso. (puede ingresarse NULL para que no se llame ninguna función).<br />
**data** = estructura de datos que se quiere mantener compartida entre las funciones del proceso.
> Devuelve 1 en caso de éxito, caso contrario devuelve -1.

El proceso que se quiere conectar al que está en escucha utiliza:<br />
`connectServer(ip, port, fns, &server_connectionClosed, data);`<br />
**IP** = IP del proceso a conectarse<br />
**Port** = Puerto del proceso a conectarse<br />
**fns** = referencias de funciones del proceso que los otros procesos conectados van a poder llamar<br />
**server_connectionClosed** = referencia a función que se quiere que se llame cuando se desconecta un proceso. (puede ingresarse NULL para que no se llame ninguna función).<br />
**data** = estructura de datos que se quiere mantener compartida entre las funciones del proceso. (puede ser NULL)<br />
> Devuelve el número de socket de la conexión.

Para que un proceso ejecute el proceso de otro proceso se utiliza la función:<br />
`runFunction(socket_server, nombre_funcion, N, arg1, arg2, arg3, ..., argN);`<br />
**socket_server** = Numero de socket del proceso al que se le quiere correr la función.<br />
**nombre_funcion** = nombre de la función que se quiere llamar. (este string deberá coincidir con el string declarado en el diccionario de funciones del proceso receptor).<br />
**N** = Número de argumentos que se envian a la funcion a llamar.<br />
**argN** = Argumentos que se envian a la funcion a llamar.<br />
> Devuelve true en caso de éxito, caso contrario devuelve false.

Para finalizar la conexión con otro proceso:<br />
`close(socket);`<br />
**socket** = Numero de socket destinado al proceso que se quiere finalizar

## Estructura 'fns'
Tanto las funciones `createListen()` como `connectServer()` necesitan recibir una lista de todas las funciones que podrán ser llamadas desde otros procesos con la función `runFunction()`. Esta lista esta hecha con la estructura “t_dictionary” de la Commons library.

Se debe inicializar de la siguiente manera:<br />
`t_dictionary * fns;`<br />
`fns = dictionary_create();`

Luego se agrega todas las funciones que necesitemos de la siguiente manera:<br />
`dictionary_put(fns, "miFuncion1", &miFuncion1);`<br />
`dictionary_put(fns, "miFuncion2", &miFuncion2);`<br />
.<br />
.<br />
`dictionary_put(fns, "miFuncion2", &miFuncionN);`

El &nombreDeUnaFuncion lo que hace es traer el puntero a la referencia de la función, gracias a esto la librería socket internamente va poder llamarla cuando sea necesario.

Las funciones “miFuncionN” son funciones que tienen que estar desarrolladas dentro del proyecto, las cuales reciben de forma obligatoria 2 parámetros:
* socket_connection * connection
* char ** args

La estructura connection contiene los siguientes campos:<br />
**int socket** = Número de socket asignado por el sistema operativo a la conexión con el proceso que llamo a esta función. 
**char * ip** = IP del proceso que llamo a esta función.<br />
**int port** = Puerto del proceso que llamo a esta función.<br />
**void * data** = Estructura de datos que se mantiene durante toda la sesión de conexión con el proceso que llamo a esta función.<br />
**bool run_fn_connectionClosed** = Determina si al finalizar la conexión con el proceso que lo llamo se tiene que llamar a la función connectionClosed (ya explicada) en caso de que exista.

La estructura args contiene un array de strings los cuales serían los “parametros” de esta función, que fueron trazados en el proceso que llamo a esta función a través de la función `runFunction()`.

Veamos un ejemplo: el proceso A ejecuta la siguiente instrucción:<br />
`runFunction(numero_socket_procesoB, "procesoB_saludar", 3, "hola", "como", "estas");`

A continuación en el proceso B se produce una invocación a la función `procesoB_saludar()`, el array args[] contendrá la siguiente información:<br />
args[0] = “hola”<br />
args[1] = “como”<br />
args[2] = “estas”<br />

¿Se puede enviar otra cosa que no sean strings? La respuesta es **NO**, para eso uno tiene que parsear el string de acuerdo a la necesidad que tengamos. Si el parámetro args[0] tendría que recibir un entero, por ejemplo el número 4, en realidad recibiría un “4” como string y habría que convertirlo a entero a través del método `atoi()` y desde el lado del proceso que lo llamo a la función habría que pasar de entero a string antes de pasar el dato a la función `runFunction()` con el método `string_itoa()` de la commons library (recordar que luego hay que realizar un free).

¿Cómo enviar un array? Para esto hay que [serializar]( https://es.wikipedia.org/wiki/Serializaci%C3%B3n) el array que se quiere enviar y luego en el proceso que lo recibe deserializarlo.

## Estructura 'data'
La estructura data es simplemente para mantener a mano una estructura de datos por cada conexión. Veamos un ejemplo. El servidor lo que hace es sumar y multiplicar números que el cliente le va dando. Es decir que por cada cliente se tiene que tener 2 contadores uno para las sumas y otra para las multiplicaciones. Entonces la estructura tendría estos dos contadores y cada vez que un cliente llama a la función “tomaElNumerito” del servidor, dicha función ya tiene a mano la estructura del cliente para hacer:<br />
`data->contadorSuma = data->contadorSuma + Numero;`<br />
`data->contadorMult = data->contadorMult * Numero;`<br />
Depende de lo que necesiten hacer los va a ayudar simplemente para reducir código. Ya que la segunda alternativa seria tener una lista de estas estructuras y buscar la estructura del cliente a través del número de socket o IP+PUERTO.

## Hilos
La librería maneja de forma interna un hilo por cada nueva conexión.

Ejemplos:<br />
* 1 servidor con 10 clientes conectados, va a tener su hilo principal de ejecución y 10 hilos más (uno por cada cliente).
* 1 cliente conectado a un servidor, va tener su hilo principal de ejecución y 1 hilo más por la conexión con el servidor.
* 1 cliente conectado a 2 servidores distintos. Va tener su hilo principal de ejecución y 2 hilos más por cada conexión al servidor.

Tener en cuenta que si 2 clientes de forma simultanea llaman a una función del servidor, las mismas se van a ejecutar de forma paralela por lo que hay que implementar sincronización. Ahora bien si un cliente llama a una función del servidor y a continuación llama a otra del mismo servidor, la segunda función no se va a ejecutar hasta que no termine la primera.

## Sincronización
En el archivo útil.c están las funciones necesarias para implementar sincronización a través del [problema lectura-escritura](https://en.wikipedia.org/wiki/Readers%E2%80%93writers_problem) <br />
Es necesario inicializar el semáforo antes que nada.
