# Field Effects

Operan similar a los ``Overworld Scripts``, pero estos gestionan los efectos visuales asociados a dichos scripts, como por ejemplo, al usar Surf o Fly, el sprite del jugador cambia su postura para:

1) Mirar al frente mientras sostiene una poké ball
2) En pantalla mostrar el pokémon que utiliza la HM en cuestión sobre un fondo negro animado
3) Aparecer una silueta negra que representa al pokémon que utiliza la HM


<br>

## ¿Como operan?
---
1) En `field_effect_scripts.s` tenemos una suerte de arreglo llamado `gFieldEffectScriptPointers` el cual contiene los nombres de distintos *Field Effects*
2) En el mismo archivo `field_effect_scripts.s`, se definen los fields effects previamente referenciados en el arreglo.
3) Los field effects definidos en ese archivo lo están como si fuesen "ASM". Dentro de dichas referencias solo invocan una función ".C" definida en `field_effect.c`
4) En el archivo previamente mencionado, la función que es llamada desde el ".s" inicializa una task que a su vez va a encadenada a otras
    - Todos los parametros que la secuencia de ``task`` necesita para ejecutarse son almacenados en el arreglo "data". Por comodidad, cada slot del arreglo recibe un nombre adecuado para el contexto
    ``` C
        // Task data for Task_FlyOut/FlyIn
        #define tState          data[0]
        #define tMonId          data[1]
        #define tBirdSpriteId   data[1] //re-used
        #define tTimer          data[2]
        #define tAvatarFlags    data[15]
        // Sprite data for the fly bird
        #define sPlayerSpriteId data[6]
        #define sAnimCompleted  data[7]
        •
        •
        •
        #undef tState
        #undef tMonId
        #undef tBirdSpriteId
        #undef tTimer
        #undef tAvatarFlags
        #undef sPlayerSpriteId
        #undef sAnimCompleted
    ```


# Extras

- FlyOut es la animación que se ejecuta una vez seleccionas la localización del mapa a la que se desea volar.