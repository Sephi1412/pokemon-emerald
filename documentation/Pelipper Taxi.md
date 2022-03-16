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

``` C
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

- Carga de paletas:
``` C
static const u16 sPelipperTaxi_Pal[16] = INCBIN_U16("graphics/field_effects/palettes/pelipper.gbapal");
```

- Modificaciones Relevantes:
  
``` C
bool8 FldEff_FieldMoveShowMonInit(void)
{
    struct Pokemon *pokemon;
    u32 flag = gFieldEffectArguments[0] & 0x80000000;
    pokemon = &gPlayerParty[(u8)gFieldEffectArguments[0]];

    /**
       * * gFieldEffectArguments[0] = Contiene la especie del pokémon que aparecerá en la animación    
       * * gFieldEffectArguments[1] = Contiene el OT_ID del pokémon
       * * gFieldEffectArguments[2] = Contiene la "PERSONALITY DATA"
       * ! Si gFieldEffectArguments[1] = gFieldEffectArguments[2], el sprite será shiny
    */
   
    gFieldEffectArguments[0] = GetMonData(pokemon, MON_DATA_SPECIES);
    gFieldEffectArguments[1] = GetMonData(pokemon, MON_DATA_OT_ID);
    gFieldEffectArguments[2] = GetMonData(pokemon, MON_DATA_PERSONALITY);

    gFieldEffectArguments[0] |= flag;
    FieldEffectStart(FLDEFF_FIELD_MOVE_SHOW_MON);
    FieldEffectActiveListRemove(FLDEFF_FIELD_MOVE_SHOW_MON_INIT);
    return FALSE;
}

static void Task_FieldMoveShowMonOutdoors(u8 taskId)
{
    if (gFieldEffectArguments[7] == TRUE)
        sFieldPelipperTaxiCry[gTasks[taskId].tState](&gTasks[taskId]);
    else
        sFieldMoveShowMonOutdoorsEffectFuncs[gTasks[taskId].tState](&gTasks[taskId]);
}

static void FieldMoveShowMonOutdoorsEffect_PelipperCry(struct Task *task)
{
    PlayCry1(SPECIES_PELIPPER, 6);  
    task->tState++;
}

u8 FldEff_UseFly(void)
{
    if (gFieldEffectArguments[7] == TRUE)
    {   
        /*
            Aquí iría lo de pelipper en vez de FlyOut
        */
        u8 taskId = CreateTask(Task_PelipperOut, 254);
        gTasks[taskId].tMonId = gFieldEffectArguments[0];
        return 0;
    }
    else
    {
        u8 taskId = CreateTask(Task_FlyOut, 254);
        gTasks[taskId].tMonId = gFieldEffectArguments[0];
        return 0;    
    }
    
}

static void FlyOutFieldEffect_FieldMovePose(struct Task *task)
{
    /*
        * Obtiene puntero al evento asociado al jugador
        * Verifica que el juego no esté haciendo nada que interrumpa el proceso del evento
        * Impide que el jugador se mueva
        * Notifica al juego que el "Avatar" esta realizando un field move
    */
    struct ObjectEvent *objectEvent = &gObjectEvents[gPlayerAvatar.objectEventId];
    if (!ObjectEventIsMovementOverridden(objectEvent) || ObjectEventClearHeldMovementIfFinished(objectEvent))
    {
        task->tAvatarFlags = gPlayerAvatar.flags;
        gPlayerAvatar.preventStep = TRUE;
        SetPlayerAvatarStateMask(PLAYER_AVATAR_FLAG_ON_FOOT);
        SetPlayerAvatarFieldMove();
        ObjectEventSetHeldMovement(objectEvent, MOVEMENT_ACTION_START_ANIM_IN_DIRECTION);
        task->tState++;
    }
}

u8 FldEff_FlyIn(void)
{
    if (gFieldEffectArguments[7] == TRUE)
    {
        CreateTask(Task_PelipperIn, 254);
        return 0;
    }
    else
    {
        CreateTask(Task_FlyIn, 254);
        return 0;
    }
    
}
```




- Estructuras que segmentan la ejecución del evento:
``` C
void (*const sFieldPelipperTaxiCry[])(struct Task *) = {
    FieldMoveShowMonOutdoorsEffect_Init,
    FieldMoveShowMonOutdoorsEffect_PelipperCry,
    FieldMoveShowMonOutdoorsEffect_End,
};
```




- Nuevas Funciones:

``` C
#define tState          data[0]
#define tMonId          data[1]
#define tBirdSpriteId   data[1] //re-used
#define tTimer          data[2]
#define tAvatarFlags    data[15]

// Sprite data for the fly bird
#define sPlayerSpriteId data[6]
#define sAnimCompleted  data[7]


static void SpriteCB_PelipperArriving(struct Sprite *sprite)
{
    if (sprite->sAnimCompleted == FALSE)
    {
        if (sprite->data[0] == 0)
        {
            sprite->oam.affineMode = ST_OAM_AFFINE_DOUBLE;
            sprite->affineAnims = sAffineAnims_FlyBird;
            InitSpriteAffineAnim(sprite);
            StartSpriteAffineAnim(sprite, 0);
            sprite->x = 0x76;
            sprite->y = -0x30;
            sprite->data[0]++;
            sprite->data[1] = 0x40;
            sprite->data[2] = 0x100;
            sprite->data[3] = 30;   
        }
        sprite->data[1] += 81; // sprite->data[1] += (sprite->data[2] >> 8);
        if (sprite->data[2] < 0x800)
        {
            sprite->data[2] += 0x60;
        }
        if ((--sprite->data[3]) == 0 )
        {
            sprite->sAnimCompleted++;
            sprite->oam.affineMode = ST_OAM_AFFINE_OFF;
            FreeOamMatrix(sprite->oam.matrixNum);
            CalcCenterToCornerVec(sprite, sprite->oam.shape, sprite->oam.size, ST_OAM_AFFINE_OFF);
        }
    }
}

static u8 CreatePelipperTaxiSprite(void)
{
    u8 spriteId;
    struct Sprite *sprite;
    spriteId = CreateSprite(gFieldEffectObjectTemplatePointers[FLDEFFOBJ_PELIPPER_TAXI], 0xff, 0xb4, 0x1);
    sprite = &gSprites[spriteId];
    sprite->oam.paletteNum = 15;
    sprite->oam.priority = 1;
    sprite->callback = SpriteCB_PelipperArriving;
    return spriteId;
}

bool8 FldEff_FieldMovePelipperCry(void)
{
    
    u32 flag = gFieldEffectArguments[0] & 0x80000000;

    /**
       * * gFieldEffectArguments[0] = Contiene la especie del pokémon que aparecerá en la animación    
       * * gFieldEffectArguments[1] = Contiene el OT_ID del pokémon
       * * gFieldEffectArguments[2] = Contiene la "PERSONALITY DATA"
       * ! Si gFieldEffectArguments[1] = gFieldEffectArguments[2], el sprite será shiny
    */
    /* PlaySE() */
    gFieldEffectArguments[0] = 310;
    gFieldEffectArguments[1] = 69420;
    gFieldEffectArguments[2] = 0;
    gFieldEffectArguments[7] = FALSE;
    
    gFieldEffectArguments[0] |= flag;
    FieldEffectStart(FLDEFF_FIELD_MOVE_SHOW_MON);
    FieldEffectActiveListRemove(FLDEFF_FIELD_MOVE_SHOW_PELIPPER);
    return FALSE;
}

u8 FldEff_UsePelipperTaxi(void)
{
    u8 taskId = CreateTask(Task_PelipperOut, 254);
    gTasks[taskId].tMonId = gFieldEffectArguments[0];
    return 0;
}

void (*const sPelipperOutFieldEffectFuncs[])(struct Task *) = {
    FlyOutFieldEffect_CallingPelipper, // FlyOutFieldEffect_FieldMovePose
    FlyOutFieldEffect_ShowPelipper, // FlyOutFieldEffect_ShowMon
    FlyOutFieldEffect_PelipperArriving, //FlyOutFieldEffect_BirdLeaveBall,
    FlyOutFieldEffect_WaitBirdLeave,
    FlyOutFieldEffect_BirdSwoopDown,
    FlyOutFieldEffect_JumpOnBird,
    FlyOutFieldEffect_FlyOffWithBird,
    FlyOutFieldEffect_WaitFlyOff,
    FlyOutFieldEffect_End,
};

static void Task_PelipperOut(u8 taskId)
{
    sPelipperOutFieldEffectFuncs[gTasks[taskId].tState](&gTasks[taskId]);
}

static void FlyOutFieldEffect_CallingPelipper(struct Task *task)
{
    struct ObjectEvent *objectEvent = &gObjectEvents[gPlayerAvatar.objectEventId];
    if (!ObjectEventIsMovementOverridden(objectEvent) || ObjectEventClearHeldMovementIfFinished(objectEvent))
    {
        task->tAvatarFlags = gPlayerAvatar.flags;
        gPlayerAvatar.preventStep = TRUE;
        SetPlayerAvatarStateMask(PLAYER_AVATAR_FLAG_ON_FOOT);
        SetPlayerAvatarFieldMove();
        ObjectEventSetHeldMovement(objectEvent, MOVEMENT_ACTION_START_ANIM_IN_DIRECTION);
        task->tState++;
    }
}

static void FlyOutFieldEffect_ShowPelipper(struct Task *task)
{
    struct ObjectEvent *objectEvent = &gObjectEvents[gPlayerAvatar.objectEventId];
    if (ObjectEventClearHeldMovementIfFinished(objectEvent))
    {
        task->tState++;
        gFieldEffectArguments[0] = task->tMonId;
        FieldEffectStart(FLDEFF_FIELD_MOVE_SHOW_MON);
    }
}

static void FlyOutFieldEffect_PelipperArriving(struct Task *task)
{
    if (!FieldEffectActiveListContains(FLDEFF_FIELD_MOVE_SHOW_MON))
    {
        struct ObjectEvent *objectEvent = &gObjectEvents[gPlayerAvatar.objectEventId];
        if (task->tAvatarFlags & PLAYER_AVATAR_FLAG_SURFING)
        {
            SetSurfBlob_BobState(objectEvent->fieldEffectSpriteId, BOB_JUST_MON);
            SetSurfBlob_DontSyncAnim(objectEvent->fieldEffectSpriteId, FALSE);
        }
        LoadPalette(sPelipperTaxi_Pal, (16 * 15) + 0x100, 32);
        task->tBirdSpriteId = CreatePelipperTaxiSprite(); // Does "leave ball" animation by default
        task->tState++;
    }
}


/*
    ! Fly In
*/

static void PelipperInFieldEffect_FieldMovePose(struct Task *task)
{
    struct ObjectEvent *objectEvent;
    struct Sprite *sprite;
    if (GetFlyBirdAnimCompleted(task->tBirdSpriteId))
    {
        objectEvent = &gObjectEvents[gPlayerAvatar.objectEventId];
        sprite = &gSprites[objectEvent->spriteId];
        objectEvent->inanimate = FALSE;
        MoveObjectEventToMapCoords(objectEvent, objectEvent->currentCoords.x, objectEvent->currentCoords.y);
        sprite->x2 = 0;
        sprite->y2 = 0;
        sprite->coordOffsetEnabled = TRUE;
        SetPlayerAvatarFieldMove();
        ObjectEventSetHeldMovement(objectEvent, MOVEMENT_ACTION_START_ANIM_IN_DIRECTION);
        task->tState++;
    }
}


static void PelipperInFieldEffect_ReturnToBall(struct Task *task)
{
    if (ObjectEventClearHeldMovementIfFinished(&gObjectEvents[gPlayerAvatar.objectEventId]))
    {
        task->tState++;
    }
}


void (*const sPelipperInFieldEffectFuncs[])(struct Task *) = {
    FlyInFieldEffect_PelipperSwoopDown,
    FlyInFieldEffect_FlyInWithBird,
    FlyInFieldEffect_JumpOffBird,
    PelipperInFieldEffect_FieldMovePose,
    PelipperInFieldEffect_ReturnToBall,
    FlyInFieldEffect_WaitBirdReturn,
    FlyInFieldEffect_End,
};

static void Task_PelipperIn(u8 taskId)
{
    sPelipperInFieldEffectFuncs[gTasks[taskId].tState](&gTasks[taskId]);
}

static void FlyInFieldEffect_PelipperSwoopDown(struct Task *task)
{
    struct ObjectEvent *objectEvent;
    objectEvent = &gObjectEvents[gPlayerAvatar.objectEventId];
    if (!ObjectEventIsMovementOverridden(objectEvent) || ObjectEventClearHeldMovementIfFinished(objectEvent))
    {
        task->tState++;
        task->tTimer = 17;
        task->tAvatarFlags = gPlayerAvatar.flags;
        gPlayerAvatar.preventStep = TRUE;
        SetPlayerAvatarStateMask(PLAYER_AVATAR_FLAG_ON_FOOT);
        if (task->tAvatarFlags & PLAYER_AVATAR_FLAG_SURFING)
        {
            SetSurfBlob_BobState(objectEvent->fieldEffectSpriteId, BOB_NONE);
        }
        ObjectEventSetGraphicsId(objectEvent, GetPlayerAvatarGraphicsIdByStateId(PLAYER_AVATAR_STATE_SURFING));
        CameraObjectReset2();
        ObjectEventTurn(objectEvent, DIR_WEST);
        StartSpriteAnim(&gSprites[objectEvent->spriteId], ANIM_GET_ON_OFF_POKEMON_WEST);
        objectEvent->invisible = FALSE;
        task->tBirdSpriteId = CreatePelipperTaxiSprite(); /* OJO */
        StartFlyBirdSwoopDown(task->tBirdSpriteId);
        SetFlyBirdPlayerSpriteId(task->tBirdSpriteId, objectEvent->spriteId);
    }
}



#undef tState
#undef tMonId
#undef tBirdSpriteId
#undef tTimer
#undef tAvatarFlags
#undef sPlayerSpriteId
#undef sAnimCompleted
```