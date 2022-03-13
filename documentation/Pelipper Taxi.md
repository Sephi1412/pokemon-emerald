# Como definirlo:

## data/field_effect_scripts.s

- Creamos un puntero que permita identificar el proceso a ejecutar y luego definimos dicho proceso. Por lo general este último usa un `field_eff_callnative` para llamar una función de ".c" definida en otro archivo.

``` asm
.4byte gFieldEffectScript_FieldMoveShowPelipper     @ FLDEFF_FIELD_MOVE_SHOW_PELIPPER

.
.
.

gFieldEffectScript_FieldMoveShowPelipper::
	field_eff_callnative FldEff_FieldMovePelipperCry
	field_eff_end

```

<br>

## include/constants/event_object_movement
- Definimos una variable global `ANIM_FIELD_NAV = 1` que nos permita diferenciar cuando un *field effect* es ejecutado desde el PokeNav o desde un ataque de un Pokémon.

``` C
const struct SpriteTemplate gFieldEffectObjectTemplate_Pelipper;
...
[FLDEFFOBJ_PELIPPER_TAXI]         = &gFieldEffectObjectTemplate_Pelipper,
```


<br>

## include/constants/field_effects.h
- Definimos dos constantes: Una para referenciar al proceso que se encarga de mostrar el "movimiento a ejecutar" y por otro, una que referencia al sprite/obj de Pelipper. 
``` c
#define FLDEFF_FIELD_MOVE_SHOW_PELIPPER  67
#define FLDEFFOBJ_PELIPPER_TAXI         37
```

<br>

## src/data/field_effects/field_effect_object_template_pointers.h
- Definimos las referencias/punteros al ObjectTemplate que representa a Pelipper.

<br>

## src/data/field_effects/field_effect_objects.h
- Aquí es donde definimos las características de los objetos/sprites referenciados en `field_effect_object_template_pointers.h`

``` c
static const struct SpriteFrameImage sPicTable_Pelipper[] = {
    obj_frame_tiles(gFieldEffectObjectPic_Pelipper),
};

static const union AnimCmd sAnim_Pelipper[] =
{
    ANIMCMD_FRAME(0, 1),
    ANIMCMD_END,
};

static const union AnimCmd *const sAnimTable_Pelipper[] =
{
    sAnim_Pelipper,
};

const struct SpriteTemplate gFieldEffectObjectTemplate_Pelipper = {
    .tileTag = TAG_NONE,
    .paletteTag = 5526,
    .oam = &gObjectEventBaseOam_32x32,
    .anims = sAnimTable_Pelipper,
    .images = sPicTable_Pelipper,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCallbackDummy,
};
```

<br>

## src/data/object_events/object_event_graphics.h
- Aquí es donde se cargan los sprites/imágenes utilizadas para la creación del SpriteTemplate.
``` c
const u32 gFieldEffectObjectPic_Pelipper[] = INCBIN_U32("graphics/field_effects/pics/pelipper.4bpp");
```

<br>

## src/field_effect.c

- Aquí se gestiona toda la lógica y parte de los gráficos empleados en el proceso de hacer funcionar el *Pelipper Taxi*.
- Listado de funciones a usar:
``` C
u8 FldEff_UsePelipperTaxi(void);
static void FieldMoveShowMonOutdoorsEffect_PelipperCry(struct Task *task);
static void Task_PelipperOut(u8 taskId);
static void FlyOutFieldEffect_CallingPelipper (struct Task *task);
static void FlyOutFieldEffect_ShowPelipper (struct Task *task);
static void FlyOutFieldEffect_PelipperArriving(struct Task *task);
static void Task_PelipperIn(u8 taskId);
static void FlyInFieldEffect_PelipperSwoopDown(struct Task *task);
```