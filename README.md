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
**portServer** = puerto en el que se pone a la escucha<br />
**newClient** = referencia a función que se quiere que se llame cuando se conecta un proceso<br />
**fns** = referencias de funciones del proceso que los otros procesos conectados van a poder llamar<br />
**client_connectionClosed** = referencia a función que se quiere que se llame cuando se desconecta un proceso<br />
**data** = estructura de datos que se quiere mantener compartida entre las funciones del proceso<br />

El proceso que se quiere conectar al que está en escucha utiliza:<br />
`connectServer(ip, port, fns, &server_connectionClosed, data);`<br />
**IP** = IP del proceso a conectarse<br />
**Port** = Puerto del proceso a conectarse<br />
**fns** = referencias de funciones del proceso que los otros procesos conectados van a poder llamar<br />
**server_connectionClosed** = referencia a función que se quiere que se llame cuando se desconecta un proceso. (puede ingresarse NULL para que no se llame ninguna función).<br />
**data** = estructura de datos que se quiere mantener compartida entre las funciones del proceso. (puede ser NULL)<br />
La función connectServer nos devuelve el número de socket de la conexión.<br />

Para que un proceso ejecute el proceso de otro proceso se utiliza la función:<br />
`runFunction(socket_server, "client_saludar", 3, "hola", "como", "estas");`<br />
**socket_server** = Numero de socket del proceso al que se le quiere correr la función en este ejemplo se le llama a la función “client_saludar” y se le envía 3 parametros.

Para finalizar la conexión con otro proceso:<br />
`close(socket);`<br />
**socket** = Numero de socket destinado al proceso que se quiere finalizar

## Estructura 'fns'
desarrollar...

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
