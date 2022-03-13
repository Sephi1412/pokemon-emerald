# Overworld scripts y comandos de script

## **¿Que son?**


> Por un lado tenemos los scripts, que son secuencias que emplean funciones de bajo nivel que se encargan de manejar la lógica de los eventos del overworld, como el uso de HMs, dialogos, movimiento, etc. Por el otro tenemos los comandos de scripts, que son funciones empleadas por los dichos scripts.
>


## **¿Como se crean los comandos?**

> Están conformados por dos partes principales:
> - La declaración: Aquí definimos la lógica y parametros que recibe el comando.  Esta siempre sigue la misma estructura
> ``` C
> bool8 ScrCmd_<name>(struct ScriptContext *ctx) {
>   ...
>   return FALSE or TRUE
>   /*
>       Si retorna TRUE, el comando finaliza la ejecución del script
>   */
> }
> ```
>  &emsp; La única condición que debe satisfacer la declaración es que el puntero recibido como input sea un puntero válido a **una función**. Más allá de que el puntero tenga el contenido esperado, es necesario leer lo justo y necesario. Por ejemplo, si el parametro que recibimos es un u16, nos interesan solo los primeros 16 bits, así que empleamos `ScriptReadHalfword` para extraer el parametro que recibe la función comando
> 
> | Función            	| Valor de Retorno 	| Desfase 	| Equivalente a |
> |--------------------	|------------------	|---------	|---------------	|
> | `ScriptReadByte`   	| u8               	| 1       	| .byte         	|
> | `ScriptReadHalfword`| u16              	| 2       	| .2byte        	|
> | `ScriptReadWord`    | u32              	| 4       	| .4byte        	|
>
>
> &emsp;
>
> - Asignación de identificador: Todas las funciones comando viven en una tabla, especificamente en el archivo `script_cmd_table.inc`. Aquí, cada línea corresponde a un comando. El formato con el que se definen los elementos en la tabla es 
> ``` 
>       .4byte ScrCmd_<command_name>                @ <offset>
> ```
> Donde el "@" actúa como un comentario referencial, ya que este offset es empleado en los **macros**. En cualquier caso, esto se caclula mediante la expresión `offset = (index - 1) * 4`



## **Ejemplo**

<br>

> &emsp; La presente código representa un script, en particular, el que está acargo de manejar la lógica del uso de Surf cuando uno interactúa frente a un tile de agua
>
> ```
> EventScript_UseSurf::
> 	checkpartymove MOVE_SURF
> 	compare VAR_RESULT, PARTY_SIZE
> 	goto_if_eq EventScript_EndUseSurf
> 	bufferpartymonnick 0, VAR_RESULT
> 	setfieldeffectargument 0, VAR_RESULT
> 	lockall
> 	msgbox gText_WantToUseSurf, MSGBOX_YESNO
> 	compare VAR_RESULT, NO
> 	goto_if_eq EventScript_ReleaseUseSurf
> 	msgbox gText_PlayerUsedSurf, MSGBOX_DEFAULT
> 	dofieldeffect FLDEFF_USE_SURF
> 
> ```
> 
> La primera linea de código emplea la función comando `checkpartymove` que > recibe como parámetro `MOVE_SURF`. La declaración de esta es la siguiente:
> 
> ``` C
> bool8 ScrCmd_checkpartymove(struct ScriptContext *ctx)
> {
>     u8 i;
>     u16 moveId = ScriptReadHalfword(ctx); // MOVE_SURF es u16
> 
>     gSpecialVar_Result = PARTY_SIZE; // PARTY_SIZE es global y equivale a 6
>     for (i = 0; i < PARTY_SIZE; i++)
>     {
>         u16 species = GetMonData(&gPlayerParty[i], MON_DATA_SPECIES, NULL);
>         if (!species)
>             /* 
>                 Terminamos de recorrer el arreglo y no se encontró un
>                 pokémon que sepa Surf
>             */
>             break;
>         if (!GetMonData(&gPlayerParty[i], MON_DATA_IS_EGG) && MonKnowsMove(&> gPlayerParty[i], moveId) == TRUE)
>         {
>             /*
>                 Si existe un pokémon que tenga surf y no es huevo, entonces 
> empleamos la variable global `gSpecialVar_Result` para almacenar el índice > del pokémon que posee el ataque, mientras que `gSpecialVar_0x8004` contiene > la especie del pokémon que tiene el ataque
>             */
>             gSpecialVar_Result = i;
>             gSpecialVar_0x8004 = species;
>             break;
>         }
>     }
>     return FALSE; // Retorna falso pues la intención de esta función no es > finalizar el script 
> ```
> &emsp;

# Macros

- Son utilizados para simplificar el uso de la lista de comandos. Existen dos tipos: Aquellos que están asociados a un comando en especifico y aquellos que unifican múltiples comandos.
- La declaración de los macros es realizada en `event.inc`
- La estructura general es la siguiente:
```
    .macro <name> <args>
    .byte <id>
    <instructions>
    .endm
```
- Así que si extrapolamos esto a `checkpartymove` tenemos:
``` C
    .macro checkpartymove index:req  // La función recibe el ID del movimiento
	.byte 0x7c                       // Offset de la tabla
	.2byte \index                    // Notifica que la variable es de 16 bits   
	.endm                            // Finaliza el macro
```

<br>

# Ejemplos interesantes:

## **`turnboject`**

1) Tabla de comandos:
```
.4byte ScrCmd_turnobject                @ 0x5b
```
2) Declaración de macro:
``` C
.macro turnobject index:req, direction:req
	.byte 0x5b          // Invoca a ScrCmd_turnobject
	.2byte \index       // Entrega el primer argumento (16 bits)
 	.byte \direction    // Entrega el segundo argumento (8 bits)
	.endm               // Finaliza el macro
```

3) Declaración de comando
``` C
bool8 ScrCmd_turnobject(struct ScriptContext *ctx)
{
    /*
        - Primero recibe el primer argumento "index". Dado que es de 16 bits, entonces lo hace mediante "ReadHalfword"

        - Luego recibe el segundo argumento. Dado que este es de 8 bits, usa ReadByte

        - Después gestiona la lógica de la función

        - Finaliza retornando false dado que la intención de este comando no es finalizar el overworld script que le ejecute.

    */
    u16 localId = VarGet(ScriptReadHalfword(ctx)); 
    u8 direction = ScriptReadByte(ctx);
    ObjectEventTurnByLocalIdAndMap(localId, gSaveBlock1Ptr->locationmapNum, gSaveBlock1Ptr->location.mapGroup, direction);
    return FALSE;
}

```
 