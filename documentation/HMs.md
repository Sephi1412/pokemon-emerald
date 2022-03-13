<h1 align="center"> Surf </h1>

## Principio base


> &emsp; Esto está definido en `GetInteractedWaterScript`:
>
> - Verifica si tenemos la quinta medalla
> - **Verifica si nuestro *team* cuenta con un pokémon que tenga Surf**
> - Verifica si estamos mirando hacia una superficie sobre la cual podemos usar dicho movimiento.
> - Si todas las condiciones previas se cumplen, se ejecuta ``EventScript_UseSurf``, el cual es un *macro*

<br>

## Como lo podemos adaptar

> - Modificamos la función anterior para que chequee si un pokémon puede aprender Surf en vez de si alguien del team lo tiene. Esto supone modificar `GetInteractedWaterScript` y `EventScript_UseSurf`.




## `GetInteractedWaterScript`
> - Redefinimos ``PartyHasMonWithSurf``. Esta recorre todo el team y chequea cada miembro del team si tiene el ataque Surf. Ahora lo que debemos hacer es emplear `CanMonLearnTMHM(mon, ITEM_HM03)` iterativamente del mismo modo en que se chequea si alguien tiene el ataque.

## `EventScript_UseSurf`
> - El tema con esta función es que se crea a partir de un *script*, es decir, existe en un archivo ".inc", así que la manera de trabajar aquí es distinta.
> - De primeras, el principal punto a modificar es que también revisa si alguien tiene surf mediante el macro `checkpartymove MOVE_SURF`, así que debemos emplear un macro que revise si alguien lo puede aprender en vez de tenerlo.

## Extras:
1) FLDEFF_USE_SURF
2) #define VAR_RESULT                    0x800D
3) bool8 ScrCmd_checkpartymove(struct ScriptContext *ctx)
4) callnative