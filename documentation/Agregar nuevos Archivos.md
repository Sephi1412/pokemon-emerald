# **Agregar nuevos archivos .C**

- Para agregar características nuevas a nuestro proyecto, con tal de mantener orden dentro de todos los archivos que tenemos lo mejor que podemos hacer es crear nuevos archivos ``".C"`` que contengan el código de dichas características.
- Todos los archivos ``".C"`` son almacenados en la carpeta ``"src"``. Cuando ejecutamos el compilador, estos pueden generar múltiples archivos asociados a estos, como archivos binarios ``".o"``, archivos asociados a gráficos, etc.
- Cada vez que se cree un nuevo archivo ``.c`` es importante hacerlo visible para el compilador, así que es necesario *notificar su existencia*.
    - Para ello, debemos escribir en `ld_script.txt` la dirección de los binarios asociados a los nuevos archivos creados. Esto se hace de la siguiente forma:
    ``` C
        src/nombre_fichero.o(.text);    // en la zona .text
        src/nombre_fichero.o(.rodata);  // en la zona .rodata
    ```
- En caso de que el archivo en cuestión haga uso de la `EWRAM`, entonces también debemos notificarlo. Esto se hace en `sym_ewram.txt`. En el solo debemos escribir al final: 
    ```
        .include "src/nombre_fichero.o"
    ```
