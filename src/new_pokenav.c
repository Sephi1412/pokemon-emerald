#include "global.h"
#include "main.h"
#include "event_data.h"
#include "decompress.h"
#include "menu.h"
#include "menu_helpers.h"
#include "data.h"
#include "scanline_effect.h"
#include "palette.h"
#include "sprite.h"
#include "item.h"
#include "task.h"
#include "bg.h"
#include "gpu_regs.h"
#include "window.h"
#include "text.h"
#include "text_window.h"
#include "international_string_util.h"
#include "overworld.h"
#include "constants/songs.h"
#include "constants/rgb.h"
#include "sound.h"
#include "strings.h"
#include "list_menu.h"
#include "malloc.h"
#include "string_util.h"
#include "util.h"
#include "reset_rtc_screen.h"
#include "reshow_battle_screen.h"
#include "trig.h"
#include "region_map.h"
#include "field_effect.h"
#include "new_pokenav.h"

/*
    To do:

    - Ver la gestión de la EWRAM
    - Que carge desde los íconos y no desde la pantalla de desbloqueo
    - Reajustar la posición 

*/
// Move Cursor to Left
#define IncreaseIndex(n) ((n + 1) % 3)
#define DecreaseIndex(n) ((n + 2) % 3)
#define MAX_ICONS 9

#define CURSOR_TAG 0x4005
#define ICON_MIN_TAG 0x4010

static const u8 gText_MainControlsPokeNav[] = _("{DPAD_NONE}Select      {A_BUTTON}Open  {B_BUTTON}Close");
static const u8 gText_UnlockText[] = _(" {A_BUTTON} to Unlock");
static const u8 gText_NewPokenavBlank[] = _("");

struct NewPokenav
{
    u8 taskId;
    u8 state;
    u8 iterator;
    u8 rowIndex;
    u8 columnIndex;
    u8 normalizedIndex;

    u8 cursorId;
    u8 cursorPosX;
    u8 cursorPosY;
    bool8 cursorIsMoving;
    bool8 columnIsMoving;

    bool8 tempFlag;

    u8 iconIds[MAX_ICONS];

    u16 textHorizontalPos;

    MainCallback savedCallback;
};

struct NewPokenavIcon
{
    void (*callback)(void);
};

static const u8 appNames[][24] = 
{
    [0] = _("Music Selector"),
    [1] = _("Vs. Seeker"),
    [2] = _("Peli-Taxi"),
    [3] = _("Remote Mart"),
    [4] = _("Pokémon Storage"),
    [5] = _("Battle Camera"),
    [6] = _("Daycare"),
    [7] = _("Wonder Trade"),
    [8] = _("Match Call"),
};

//void (*appsInitCallbacks[MAX_ICONS])(void) = 
//{
//    CB2_ReturnToFieldWithOpenMenu, 
//    CB2_OpenFlyMap,
//    CB2_OpenFlyMap,
//    CB2_OpenFlyMap,
//    CB2_OpenFlyMap,
//    CB2_OpenFlyMap,
//    CB2_OpenFlyMap,
//    CB2_OpenFlyMap,
//    CB2_OpenFlyMap
//};

/* BACKGROUNDS */

static const u32 margin_tiles[] = INCBIN_U32("graphics/new_pokenav/margin_tiles.4bpp.lz");
static const u32 margin_map[] = INCBIN_U32("graphics/new_pokenav/margin_map.bin.lz");
static const u16 margin_pal[] = INCBIN_U16("graphics/new_pokenav/margin_tiles.gbapal");

static const u32 background_tiles[] = INCBIN_U32("graphics/new_pokenav/background_tiles.4bpp.lz");
static const u32 background_map[] = INCBIN_U32("graphics/new_pokenav/background_map.bin.lz");
static const u16 background_pal[] = INCBIN_U16("graphics/new_pokenav/background_tiles.gbapal");

static const u32 text_bg_tiles[] = INCBIN_U32("graphics/new_pokenav/text_bg_tiles.4bpp.lz");
static const u32 text_bg_map[] = INCBIN_U32("graphics/new_pokenav/text_bg_map.bin.lz");
static const u16 text_bg_pal[] = INCBIN_U16("graphics/new_pokenav/text_bg_tiles.gbapal");

static const u32 Pokeball_Tiles[] = INCBIN_U32("graphics/new_pokenav/pokeball_tiles.4bpp.lz");
static const u32 Pokeball_Map[] = INCBIN_U32("graphics/new_pokenav/pokeball_map.bin.lz");
static const u16 Pokeball_Pal[] = INCBIN_U16("graphics/new_pokenav/pokeball_tiles.gbapal");

/* SPRITES */

static const u16 appIconPal[] = INCBIN_U16("graphics/new_pokenav/oam_icons/icon1.gbapal");
static const u32 appIconSprite[] = INCBIN_U32("graphics/new_pokenav/oam_icons/icons.4bpp.lz");


static const u16 iconSelectorPal[] = INCBIN_U16("graphics/new_pokenav/icons/cursor.gbapal");
static const u32 iconSelectorSprite[] = INCBIN_U32("graphics/new_pokenav/icons/cursor.4bpp.lz");

/* Variables globales */

EWRAM_DATA static struct NewPokenav *sNewPokenavViewPtr = NULL;
EWRAM_DATA static u8 *sBg1TilemapBuffer = NULL;
EWRAM_DATA static u8 *sBg2TilemapBuffer = NULL;
EWRAM_DATA static u8 *sBg3TilemapBuffer = NULL;
EWRAM_DATA static u8 rowPos = 1;
EWRAM_DATA static u8 columnPos = 1;

/* Declaracion implicita de funciones */

void Task_OpenPokeNavFromStartMenu(u8 taskId);
static void NewPokenav_RunSetup(void);
static void NewPokenavGuiInit(MainCallback callback);
static void CreateSelectionCursor(u8 scenario);
static void Task_NewPokenavWaitFadeIn(u8 taskId);
static void Task_NewPokenavMain(u8 taskId);
static void Task_FadePokeballAndText(u8 taskId);
static void Task_LoadTextboxBg(u8 taskId);
static void NewPokenavGuiFreeResources(void);
static void Task_NewPokenavFadeAndExit(u8 taskId);
static void NewPokenavFadeAndExit(void);
static void SpriteCB_moveCursor(struct Sprite *sprite);
static bool8 NewPokenav_InitBgs(void);
static bool8 NewPokenav_LoadGraphics(void);
static bool8 StartNewPokenavCallback(void);
static bool8 NewPokenav_DoGfxSetup(void);

static void NewPokenav_InitWindows(void);
static void UpdatePalette(u8 frameNum);
static void Task_WaitingUnlockPrompt(u8 taskId);

static void Task_LoadIcons(u8 taskId);
static void Task_MoveIcons(u8 taskId);
static void PrintSpecificIcon(u8 iconId, u8 frame);

static void Task_LoadAppName(u8 taskId);
static void Task_FadeOutAndBeginApp(u8 taskId);
bool8 StartMenuFromAppCallback(u8 row, u8 column);
static void Task_OpenNewPokenavFromApp (u8 taskId);
static void NewPokenavGuiInitFromApp(MainCallback callback);

static void NewPokenav_RunSetupFromApp(void);
static bool8 NewPokenav_DoGfxSetupFromApp(void);
static bool8 NewPokenav_LoadGraphicsFromApp(void);
static void Task_FromAppWaitFadeIn(u8 taskId);
static void PrintControlsAndAppName(void);

const struct NewPokenavIcon menuIcons[9] =
    {
        //.callback = OpenMusicSelector,
        [0] = {.callback = CB2_OpenFlyMap},
        [1] = {.callback = CB2_OpenFlyMap},
        [2] = {.callback = CB2_OpenFlyMap},
        [3] = {.callback = CB2_OpenFlyMap},
        [4] = {.callback = CB2_OpenFlyMap},
        [5] = {.callback = CB2_OpenFlyMap},
        [6] = {.callback = CB2_OpenFlyMap},
        [7] = {.callback = CB2_OpenFlyMap},
        [8] = {.callback = CB2_OpenFlyMap},
};


/* TEXT WINDOWS TEMPLATES */

enum
{
    UNLOCK_TEXT,
    APP_TITLE,
    CONTROLS
};

/* STATUS */
enum
{
    FROM_APP,
    FROM_START_MENU
};

static const u8 sFontColourTableType[] = {0, 1, 2, 0};

static const struct WindowTemplate sWindowTemplate_MenuStrings[] =
    {
        [UNLOCK_TEXT] = {
            .bg = 0,
            .tilemapLeft = 11,
            .tilemapTop = 15,
            .width = 8,
            .height = 2,
            .paletteNum = 15,
            .baseBlock = 1,
        },
        [APP_TITLE] = {
            .bg = 0,
            .tilemapLeft = 3,
            .tilemapTop = 2,
            .width = 12,
            .height = 2,
            .paletteNum = 15,
            .baseBlock = 80,
        },
        [CONTROLS] = {
            .bg = 0,
            .tilemapLeft = 11,
            .tilemapTop = 16,
            .width = 17,
            .height = 2,
            .paletteNum = 15,
            .baseBlock = 155,
        }};

static const struct WindowTemplate sWindowTemplate_UnlockScreen[] =
    {
        [UNLOCK_TEXT] = {
            .bg = 0,
            .tilemapLeft = 11,
            .tilemapTop = 15,
            .width = 8,
            .height = 2,
            .paletteNum = 15,
            .baseBlock = 120,
        }};

static const struct BgTemplate sNewPokenavMenuBgTemplates[] =
{
    /**
     * * charBaseIndex corresponde a la marca inicial del territorio que
     * * empleará un background. Va en múltiplos de 0x4000 y su dirección
     * * de partida es 0x8000000
     * ! final_dir = 0x8000000 + (0x4000 * charBaseIndex)
     *
     * * Por otro lado, está mapBaseIndex que señala donde se van a 
     * * descomprimir los gráficos
     * ! final_dir = 0x6000000 + (0x800 * mapBaseIndex)
    */
    {
        .bg = 0,
        .charBaseIndex = 0,
        .mapBaseIndex = 31,
        .priority = 0
    }, 
    {
        .bg = 1,
        .charBaseIndex = 1,
        .mapBaseIndex = 15,
        .priority = 1
    },
    {
        .bg = 2,
        .charBaseIndex = 2,
        .mapBaseIndex = 35,
        .priority = 2
    },
    {
        .bg = 3,
        .charBaseIndex = 3,
        .mapBaseIndex = 50,
        .priority = 3
    }
};

//iconSprite1


//static const u32 sIconArray[9][] =
//{
//    [1] = {iconSprite1},
//    [2] = {iconSprite2},
//    [3] = {iconSprite3},
//    [4] = {iconSprite4},
//    [5] = {iconSprite5},
//    [6] = {iconSprite6},
//    [7] = {iconSprite7},
//    [8] = {iconSprite8},
//    [9] = {iconSprite9}
//}
 
//gui font
static const u8 sFontColor_Black[3] = {TEXT_COLOR_TRANSPARENT, TEXT_COLOR_DARK_GRAY, TEXT_COLOR_LIGHT_GRAY};
static const u8 sFontColor_White[3] = {TEXT_COLOR_TRANSPARENT, TEXT_COLOR_WHITE, TEXT_COLOR_DARK_GRAY};
//search window font
static const u8 sSearchFontColor[3] = {0, 15, 13};

/* GFX Animations */

static const union AnimCmd sSpriteAnim_Icon1[] =
{
    ANIMCMD_FRAME(0, 4),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_Icon2[] =
{
    ANIMCMD_FRAME(16, 4),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_Icon3[] =
{
    ANIMCMD_FRAME(32, 4),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_Icon4[] =
{
    ANIMCMD_FRAME(48, 4),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_Icon5[] =
{
    ANIMCMD_FRAME(64, 4),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_Icon6[] =
{
    ANIMCMD_FRAME(80, 4),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_Icon7[] =
{
    ANIMCMD_FRAME(96, 4),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_Icon8[] =
{
    ANIMCMD_FRAME(112, 4),
    ANIMCMD_END
};

static const union AnimCmd sSpriteAnim_Icon9[] =
{
    ANIMCMD_FRAME(128, 4),
    ANIMCMD_END
};



static const union AnimCmd *const sAnims_IconAppAnim[] =
{
        sSpriteAnim_Icon1,
        sSpriteAnim_Icon2,
        sSpriteAnim_Icon3,
        sSpriteAnim_Icon4,
        sSpriteAnim_Icon5,
        sSpriteAnim_Icon6,
        sSpriteAnim_Icon7,
        sSpriteAnim_Icon8,
        sSpriteAnim_Icon9
};

const union AnimCmd gAnims_CursorIdleAnim[] =
    {
        ANIMCMD_FRAME(0, 30),
        ANIMCMD_FRAME(4, 30),
        ANIMCMD_JUMP(0),
};


static const union AnimCmd *const sAnims_CursorTableAnim[] =
    {
        gAnims_CursorIdleAnim,
};

/* Structs Sprites */

static const struct OamData sAppIconOam =
    {
        .y = 0,
        .affineMode = 0,
        .objMode = 0,
        .mosaic = 0,
        .bpp = 0,
        .shape = 0,
        .x = 0,
        .matrixNum = 0,
        .size = SPRITE_SIZE(32x32),
        .tileNum = 0,
        .priority = 2,
        .paletteNum = 3,
        .affineParam = 0,
};

const struct SpriteTemplate sAppIconSpriteTemplate =
{
    .tileTag = ICON_MIN_TAG,
    .paletteTag = 0xFFFF,
    .oam = &sAppIconOam,
    .anims = sAnims_IconAppAnim,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCallbackDummy,
};

static const struct OamData sSelectionCursorOam =
    {
        .y = 0,
        .affineMode = 0,
        .objMode = 0,
        .mosaic = 0,
        .bpp = 0,
        .shape = 0,
        .x = 0,
        .matrixNum = 0,
        .size = SPRITE_SIZE(16x16),
        .tileNum = 0,
        .priority = 0,
        .paletteNum = 12,
        .affineParam = 0,
};

const struct SpriteTemplate sSelectionCursorSpriteTemplate =
    {
        .tileTag = CURSOR_TAG,
        .paletteTag = 0xFFFF,
        .oam = &sSelectionCursorOam,
        .anims = sAnims_CursorTableAnim,
        .images = NULL,
        .affineAnims = gDummySpriteAffineAnimTable,
        .callback = SpriteCB_moveCursor, //SpriteCB_moveCursor
};


#define tFrameCounter data[0]
#define tOpacityLevel data[1]
#define tCursorXPos   data[2]
#define tCursorYPos   data[3]

/* Definicion de funciones */

static void NewPokenav_VBlankCB()
{
    LoadOam();
    ChangeBgX(3, -60, 2);
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}



static void NewPokenav_MainCB(void)
{
    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    DoScheduledBgTilemapCopiesToVram();
    UpdatePaletteFade();
}

static void SpriteCB_moveCursor(struct Sprite *sprite)
{
    /*
        LEFT COLUMN = 50
        MID COLUMN = 120
        RIGHT COLUMN = 190

        UPPER ROW = 50
        MID ROW = 80
        LOWER ROW = 110
    */
    if (sNewPokenavViewPtr->cursorIsMoving)
    {
        PlaySE(SE_DEX_SCROLL);
        sprite->x2 = 50 + (sNewPokenavViewPtr->columnIndex * 70) - 120;
        sprite->y2 = (sNewPokenavViewPtr->rowIndex * 30) - 30;
        sNewPokenavViewPtr->cursorPosX = sprite->x2;
        sNewPokenavViewPtr->cursorPosY = sprite->y2;
        sNewPokenavViewPtr->cursorIsMoving = FALSE;
    }
}

static void NewPokenav_InitWindows(void)
{
    InitWindows(sWindowTemplate_MenuStrings); // sWindowTemplate_MenuStrings
    DeactivateAllTextPrinters();
    ScheduleBgCopyTilemapToVram(0); // 0 Porque esa es la capa del texto
}

static void CreateSelectionCursor(u8 scenario)
{
    u8 spriteId;
    struct CompressedSpriteSheet spriteSheet;

    spriteSheet.data = iconSelectorSprite;
    spriteSheet.size = 0x200; /* (x_px * y_px) / 2 */
    spriteSheet.tag = CURSOR_TAG;

    LoadCompressedSpriteSheet(&spriteSheet);
    LoadPalette(iconSelectorPal, (16 * sSelectionCursorOam.paletteNum) + 0x100, 32);
    spriteId = CreateSprite(&sSelectionCursorSpriteTemplate, 133, 69, 0);
    sNewPokenavViewPtr->cursorId = spriteId;
    if (scenario == FROM_APP)
    {
        gSprites[spriteId].x2 = 50 + (sNewPokenavViewPtr->columnIndex * 70) - 120;
        gSprites[spriteId].y2 = (sNewPokenavViewPtr->rowIndex * 30) - 30;
    }
    //UpdateCursorPosition();
}

static bool8 NewPokenav_InitBgs(void)
{
    ResetVramOamAndBgCntRegs();
    ResetAllBgsCoordinates();
    sBg1TilemapBuffer = Alloc(0x800);
    sBg2TilemapBuffer = Alloc(0x800);
    sBg3TilemapBuffer = Alloc(0x800);
    if (sBg1TilemapBuffer == NULL)
        return FALSE;

    memset(sBg1TilemapBuffer, 0, 0x800);
    memset(sBg2TilemapBuffer, 0, 0x800);
    memset(sBg3TilemapBuffer, 0, 0x800);
    ResetBgsAndClearDma3BusyFlags(0);
    InitBgsFromTemplates(0, sNewPokenavMenuBgTemplates, NELEMS(sNewPokenavMenuBgTemplates));
    SetBgTilemapBuffer(1, sBg1TilemapBuffer);
    SetBgTilemapBuffer(2, sBg2TilemapBuffer);
    SetBgTilemapBuffer(3, sBg3TilemapBuffer);

    ScheduleBgCopyTilemapToVram(1);
    ScheduleBgCopyTilemapToVram(2);
    ScheduleBgCopyTilemapToVram(3);

    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_1D_MAP | DISPCNT_OBJ_ON);
    SetGpuReg(REG_OFFSET_BLDCNT, 0);
    /* Opacidad */
    //SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_TGT1_BG2 | BLDCNT_EFFECT_BLEND | BLDCNT_TGT2_ALL);
    //SetGpuReg(REG_OFFSET_BLDALPHA, BLDALPHA_BLEND(16, 16));

    ShowBg(0);
    ShowBg(1);
    ShowBg(2);
    ShowBg(3);
    return TRUE;
    // sNewPokenavMenuBgTemplates
}

static bool8 NewPokenav_LoadGraphics(void)
{
    switch (sNewPokenavViewPtr->state)
    {
    case 0:
        ResetTempTileDataBuffers();
        DecompressAndCopyTileDataToVram(1, margin_tiles, 0, 0, 0);
        DecompressAndCopyTileDataToVram(2, Pokeball_Tiles, 0, 0, 0);
        DecompressAndCopyTileDataToVram(3, background_tiles, 0, 0, 0);
        sNewPokenavViewPtr->state++;
        break;
    case 1:
        if (FreeTempTileDataBuffersIfPossible() != TRUE)
        {
            LZDecompressWram(margin_map, sBg1TilemapBuffer);
            LZDecompressWram(Pokeball_Map, sBg2TilemapBuffer);
            LZDecompressWram(background_map, sBg3TilemapBuffer);
            sNewPokenavViewPtr->state++;
        }
        break;
    case 2:
        LoadPalette(margin_pal, 0, 32);
        LoadPalette(background_pal, 0x10, 32);
        LoadPalette(Pokeball_Pal, 0x20, 32);
        SetGpuReg(REG_OFFSET_BG3HOFS, 0);
        sNewPokenavViewPtr->state++;
        break;
    default:
        sNewPokenavViewPtr->state = 0;
        return TRUE;
    }
    return FALSE;
}

static bool8 StartNewPokenavCallback(void)
{
    CreateTask(Task_OpenPokeNavFromStartMenu, 0);
    return TRUE;
}

void Task_OpenPokeNavFromStartMenu(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        CleanupOverworldWindowsAndTilemaps();
        NewPokenavGuiInit(CB2_ReturnToFieldWithOpenMenu);
        DestroyTask(taskId);
    }
}

static void NewPokenavGuiInit(MainCallback callback)
{
    if ((sNewPokenavViewPtr = AllocZeroed(sizeof(struct NewPokenav))) == NULL)
    {
        SetMainCallback2(callback);
        return;
    }
    /*
       El cursor parte en el ícono central, así que debemos establecer 
       las caracteristicas iniciales
    */
    sNewPokenavViewPtr->state = 0;
    sNewPokenavViewPtr->tempFlag = FALSE;
    sNewPokenavViewPtr->savedCallback = callback;

    SetMainCallback2(NewPokenav_RunSetup);
}

static void NewPokenav_RunSetup(void)
{
    while (!NewPokenav_DoGfxSetup())
    {
    }
}

static void PrintUnlockPrompt()
{
    FillWindowPixelBuffer(UNLOCK_TEXT, PIXEL_FILL(TEXT_COLOR_TRANSPARENT));
    PutWindowTilemap(UNLOCK_TEXT);
    //StringCopy(gStringVar1, gText_UnlockText);
    StringExpandPlaceholders(gStringVar4, gText_UnlockText);
    AddTextPrinterParameterized3(UNLOCK_TEXT, 2, 0, 0, sFontColor_White, 0, gStringVar4);
    CopyWindowToVram(UNLOCK_TEXT, 3);
}

static bool8 NewPokenav_DoGfxSetup(void)
{
    u8 taskId;
    switch (gMain.state)
    {
    case 0:
        SetVBlankHBlankCallbacksToNull();
        ClearScheduledBgCopiesToVram();
        gMain.state++;
        break;
    case 1:
        ScanlineEffect_Stop();
        gMain.state++;
        break;
    case 2:
        FreeAllSpritePalettes();
        gMain.state++;
        break;
    case 3:
        ResetPaletteFade();
        ResetSpriteData();
        ResetTasks();
        gMain.state++;
        break;
    case 4:
        if (NewPokenav_InitBgs())
        {
            sNewPokenavViewPtr->state = 0;
            gMain.state++;
        }
        else
        {
            NewPokenavFadeAndExit();
            return TRUE;
        }
        break;
    case 5:
        if (NewPokenav_LoadGraphics() == TRUE)
            gMain.state++;
        break;
    case 6:
        NewPokenav_InitWindows();
        sNewPokenavViewPtr->iterator = 0; /* OJO */
        sNewPokenavViewPtr->cursorPosX = 133;
        sNewPokenavViewPtr->cursorPosY = 69;
        sNewPokenavViewPtr->columnIsMoving = FALSE;
        sNewPokenavViewPtr->textHorizontalPos = 0;
        gMain.state++;
        break;
    case 7:
        PrintUnlockPrompt();
        gMain.state++;
        break;
    case 8:
        taskId = CreateTask(Task_NewPokenavWaitFadeIn, 0);
        gMain.state++;
        break;
    case 9:
        //CreateSelectionCursor(FROM_START_MENU);
        gMain.state++;
        break;
    case 10:
        BlendPalettes(0xFFFFFFFF, 16, RGB_BLACK);
        gMain.state++;
        break;
    case 11:
        BeginNormalPaletteFade(0xFFFFFFFF, 0, 16, 0, RGB_BLACK);
        gMain.state++;
        break;
    default:
        SetVBlankCallback(NewPokenav_VBlankCB);
        SetMainCallback2(NewPokenav_MainCB);
        return TRUE;
    }
    return FALSE;
}

static void NewPokenavFadeAndExit(void)
{
    BeginNormalPaletteFade(0xFFFFFFFF, 0, 0, 16, RGB_BLACK);
    CreateTask(Task_NewPokenavFadeAndExit, 0);
    SetVBlankCallback(NewPokenav_VBlankCB);
    SetMainCallback2(NewPokenav_MainCB);
}

static void NewPokenavGuiFreeResources()
{
    Free(sNewPokenavViewPtr);
    Free(sBg1TilemapBuffer);
    Free(sBg2TilemapBuffer);
    Free(sBg3TilemapBuffer);
    FreeAllWindowBuffers();
}

static void Task_NewPokenavFadeAndExit(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        SetMainCallback2(sNewPokenavViewPtr->savedCallback);
        NewPokenavGuiFreeResources();
        DestroyTask(taskId);
    }
}

static void Task_NewPokenavWaitFadeIn(u8 taskId)
{
    if (!gPaletteFade.active)
        gTasks[taskId].func = Task_WaitingUnlockPrompt;
}


static void Task_WaitingUnlockPrompt(u8 taskId)
{
    struct Task *task = &gTasks[taskId];
    if (JOY_NEW(A_BUTTON))
    {
        /* Destroy window, play sound, generate App Icons */
        SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_TGT1_BG2 | BLDCNT_EFFECT_BLEND | BLDCNT_TGT2_ALL);
        PlaySE(SE_POKENAV_ON);
        //gTasks[taskId].tOpacityLevel = 9;
        //InitBgsFromTemplates(0, sNewPokenavMenuBgTemplates, NELEMS(sNewPokenavMenuBgTemplates));
        //HideBg(0);
        gFieldEffectArguments[7] = TRUE;
        FillWindowPixelBuffer(UNLOCK_TEXT, PIXEL_FILL(TEXT_COLOR_TRANSPARENT));
        AddTextPrinterParameterized3(UNLOCK_TEXT, 2, 0, 0, sFontColor_White, 0, gText_NewPokenavBlank);
        //SetBgTilemapBuffer(2, AllocZeroed(BG_SCREEN_SIZE));
        gTasks[taskId].func = Task_FadePokeballAndText;
    }

    if (JOY_NEW(B_BUTTON))
    {
        PlaySE(SE_POKENAV_OFF);
        BeginNormalPaletteFade(0xFFFFFFFF, 0, 0, 16, RGB_BLACK);
        task->func = Task_NewPokenavFadeAndExit;
    }
    else
    {
        gTasks[taskId].tFrameCounter++;
        UpdatePalette(gTasks[taskId].tFrameCounter);
    }
    
}

static void Task_FadePokeballAndText(u8 taskId)
{
    if (gTasks[taskId].tOpacityLevel >= 0)
    {
        SetGpuReg(REG_OFFSET_BLDALPHA, BLDALPHA_BLEND(gTasks[taskId].tOpacityLevel, 16));
        gTasks[taskId].tOpacityLevel -= 1;
    }
    else
    {
        //FreeAllWindowBuffers();
        DecompressAndLoadBgGfxUsingHeap(2, text_bg_tiles, 0x1800, 0, 0);
        LoadPalette(text_bg_pal, 0x20, 32);
        CopyToBgTilemapBuffer(2, text_bg_map, 0, 0);
        CopyBgTilemapBufferToVram(2);
        CopyBgTilemapBufferToVram(3);
        //SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_TGT1_BG2 | BLDCNT_EFFECT_BLEND | BLDCNT_TGT2_ALL);
        gTasks[taskId].func = Task_LoadTextboxBg;
    }
        

}


static void Task_LoadTextboxBg(u8 taskId)
{
    if (gTasks[taskId].tOpacityLevel < 6)
    {   
        gTasks[taskId].tOpacityLevel += 1;
        SetGpuReg(REG_OFFSET_BLDALPHA, BLDALPHA_BLEND(gTasks[taskId].tOpacityLevel, 16));
    }
    else
    {
        gTasks[taskId].tFrameCounter = 0;
        gTasks[taskId].tOpacityLevel = 0;
        /* Texto */
        FillWindowPixelBuffer(CONTROLS, PIXEL_FILL(TEXT_COLOR_TRANSPARENT));
        PutWindowTilemap(CONTROLS);
        AddTextPrinterParameterized3(CONTROLS, 2, 0, 0, sFontColor_White, 0, gText_MainControlsPokeNav);
        CopyWindowToVram(CONTROLS, 3);
        CreateSelectionCursor(FROM_START_MENU);
        gTasks[taskId].func = Task_LoadIcons;
    }
}

static void PrintSpecificIcon(u8 iconId, u8 frame)
{
    struct Sprite *sprite = &gSprites[iconId];
    StartSpriteAnim(sprite, frame);
}

static void Task_LoadIcons(u8 taskId)
{
    u32 i;
    for (i = 0; i < MAX_ICONS; i++)
    {
        u8 spriteId, x_spawn_coord, y_spawn_coord;
        struct CompressedSpriteSheet spriteSheet;
        spriteSheet.data = appIconSprite;
        spriteSheet.size = 0x2400; /* (x_px * y_px) / 2 */
        spriteSheet.tag = ICON_MIN_TAG;
        if ( (gTasks[taskId].tFrameCounter % 3) == 0 )
            sNewPokenavViewPtr->iterator += 1;
        x_spawn_coord = 50 + (70 * (gTasks[taskId].tFrameCounter % 3));
        y_spawn_coord = 50 + (30 * (sNewPokenavViewPtr->iterator - 1));
        if (sNewPokenavViewPtr->tempFlag == FALSE)
            x_spawn_coord += 200 - (70 * (gTasks[taskId].tFrameCounter % 3));
        

        LoadCompressedSpriteSheet(&spriteSheet);
        LoadPalette(appIconPal, (16 * sAppIconOam.paletteNum) + 0x100, 32);
        spriteId = CreateSprite(&sAppIconSpriteTemplate, x_spawn_coord, y_spawn_coord, 0);
        sNewPokenavViewPtr->iconIds[i] = spriteId;
        PrintSpecificIcon(spriteId, gTasks[taskId].tFrameCounter);
        gTasks[taskId].tFrameCounter += 1;
        
        //sNewPokenavViewPtr->iconIds[gTasks[taskId].tFrameCounter] = spriteId;
    }
    if (sNewPokenavViewPtr->tempFlag == FALSE)
    {
        sNewPokenavViewPtr->rowIndex = 1;
        sNewPokenavViewPtr->columnIndex = 1;
        gTasks[taskId].func = Task_MoveIcons;
    }
    else
    {
        gTasks[taskId].func = Task_FromAppWaitFadeIn;
    }
    
    
    //if (scenario == FROM_APP)
    //{
    //    gSprites[spriteId].x2 = 50 + (sNewPokenavViewPtr->columnIndex * 70) - 120;
    //    gSprites[spriteId].y2 = (sNewPokenavViewPtr->rowIndex * 30) - 30;
    //}
    //UpdateCursorPosition();
}

//if (   gSprites[sNewPokenavViewPtr->iconIds[0]].x2 < 235)
//        gSprites[sNewPokenavViewPtr->iconIds[0]].x2-= 10;

static void Task_MoveIcons(u8 taskId)
{
    if (gSprites[sNewPokenavViewPtr->iconIds[0]].x2 >= -200)
    {
        gSprites[sNewPokenavViewPtr->iconIds[0]].x2-= 10;
        gSprites[sNewPokenavViewPtr->iconIds[3]].x2-= 10;
        gSprites[sNewPokenavViewPtr->iconIds[6]].x2-= 10;
    }
    if (gSprites[sNewPokenavViewPtr->iconIds[1]].x2 >= -120)
    {
        gSprites[sNewPokenavViewPtr->iconIds[1]].x2-= 10;
        gSprites[sNewPokenavViewPtr->iconIds[4]].x2-= 10;
        gSprites[sNewPokenavViewPtr->iconIds[7]].x2-= 10;
    }
    if (gSprites[sNewPokenavViewPtr->iconIds[2]].x2 >= -50)
    {
        gSprites[sNewPokenavViewPtr->iconIds[2]].x2-= 10;
        gSprites[sNewPokenavViewPtr->iconIds[5]].x2-= 10;
        gSprites[sNewPokenavViewPtr->iconIds[8]].x2-= 10;
    }

    if (gSprites[sNewPokenavViewPtr->iconIds[0]].x2 < -190)
    {
        gTasks[taskId].func = Task_LoadAppName;    
    }    
}



static void Task_LoadAppName(u8 taskId)
{
    u8 transformedIndex = sNewPokenavViewPtr->rowIndex + (sNewPokenavViewPtr->columnIndex) * 3;
    FillWindowPixelBuffer(APP_TITLE, PIXEL_FILL(TEXT_COLOR_TRANSPARENT));
    PutWindowTilemap(APP_TITLE);
    StringExpandPlaceholders(gStringVar4, appNames[transformedIndex]);
    AddTextPrinterParameterized3(APP_TITLE, 2, 0, 0, sFontColor_White, 0, gStringVar4);
    CopyWindowToVram(APP_TITLE, 3);
    //CreateSelectionCursor(FROM_START_MENU);
    gTasks[taskId].func = Task_NewPokenavMain;
}
/*
static void Task_MoveIcons(u8 taskId)
{

    gSprites[sNewPokenavViewPtr->iconIds[0]].x2-= 200;
    gSprites[sNewPokenavViewPtr->iconIds[1]].x2-= 130;
    gSprites[sNewPokenavViewPtr->iconIds[2]].x2-= 60;
    
    gTasks[taskId].func = Task_NewPokenavMain;
    
}
*/

static void Task_NewPokenavMain(u8 taskId)
{
    u8 normalizedIndex;
    u8 text[16];
    u16 windowId;
    struct Task *task = &gTasks[taskId];

    /*
    if(IsSEPlaying())
        return;
    */

    if (JOY_NEW(B_BUTTON))
    {
        PlaySE(SE_POKENAV_OFF);
        BeginNormalPaletteFade(0xFFFFFFFF, 0, 0, 16, RGB_BLACK);
        task->func = Task_NewPokenavFadeAndExit;
    }

    if (JOY_NEW(A_BUTTON))
    {
        gTasks[taskId].func = Task_FadeOutAndBeginApp;
        BeginNormalPaletteFade(0xFFFFFFFF, 0, 0, 16, RGB_BLACK);
        /* Start APP */
    }
    if (JOY_NEW(DPAD_LEFT))
    {
        sNewPokenavViewPtr->columnIndex = DecreaseIndex(sNewPokenavViewPtr->columnIndex);
        sNewPokenavViewPtr->cursorIsMoving = TRUE;
    }
    if (JOY_NEW(DPAD_RIGHT))
    {
        sNewPokenavViewPtr->columnIndex = IncreaseIndex(sNewPokenavViewPtr->columnIndex);
        sNewPokenavViewPtr->cursorIsMoving = TRUE;
        /* Start APP */
    }
    if (JOY_NEW(DPAD_UP))
    {
        sNewPokenavViewPtr->rowIndex = DecreaseIndex(sNewPokenavViewPtr->rowIndex);
        sNewPokenavViewPtr->cursorIsMoving = TRUE;
    }
    if (JOY_NEW(DPAD_DOWN))
    {
        sNewPokenavViewPtr->rowIndex = IncreaseIndex(sNewPokenavViewPtr->rowIndex);
        sNewPokenavViewPtr->cursorIsMoving = TRUE;
    }
    if (sNewPokenavViewPtr->cursorIsMoving)
    {
        normalizedIndex = sNewPokenavViewPtr->rowIndex + (sNewPokenavViewPtr->columnIndex) * 3;
        FillWindowPixelBuffer(APP_TITLE, PIXEL_FILL(TEXT_COLOR_TRANSPARENT));
        AddTextPrinterParameterized3(APP_TITLE, 2, 0, 0, sFontColor_White, 0, appNames[normalizedIndex]);
        CopyWindowToVram(APP_TITLE, 3);
        PutWindowTilemap(APP_TITLE);
    }
}


static void Task_FadeOutAndBeginApp(u8 taskId)
{
    u8 normalizedIndex;
    if (!gPaletteFade.active)
    {
        // Reset GPU Registers
        normalizedIndex = sNewPokenavViewPtr->rowIndex + (3 * sNewPokenavViewPtr->columnIndex);
        SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_TGT1_BG1 | BLDCNT_EFFECT_NONE | BLDCNT_TGT2_ALL);
        VarSet(VAR_TEMP_0, sNewPokenavViewPtr->rowIndex);
        VarSet(VAR_TEMP_1, sNewPokenavViewPtr->columnIndex);
        NewPokenavGuiFreeResources();
        SetMainCallback2(menuIcons[normalizedIndex].callback);
        DestroyTask(taskId);
        
        
        
    }
}


static void UpdatePalette(u8 frameNum)
{
    if ((frameNum % 4) == 0) // Change color every 4th frame
    {
        s32 intensity = Sin(frameNum, 128) + 128;
        s32 r = 31 - ((intensity * 32 - intensity) / 269);
        s32 g = 31 - ((intensity * 32 - intensity) / 256);
        s32 b = 0;

        u16 color = RGB(r, g, b);
        LoadPalette(&color, 0x2F, sizeof(color));
    }
}


bool8 StartMenuFromAppCallback(u8 row, u8 column)
{
    rowPos = row;
    columnPos = column;
    CreateTask(Task_OpenNewPokenavFromApp, 0);
    return TRUE;
}


static void Task_OpenNewPokenavFromApp(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        CleanupOverworldWindowsAndTilemaps();
        NewPokenavGuiInitFromApp(CB2_ReturnToFieldWithOpenMenu);
        DestroyTask(taskId);
    }
}


static void NewPokenavGuiInitFromApp(MainCallback callback)
{
    if ((sNewPokenavViewPtr = AllocZeroed(sizeof(struct NewPokenav))) == NULL)
    {
        SetMainCallback2(callback);
        return;
    }
    /*
       El cursor parte en el ícono central, así que debemos establecer 
       las caracteristicas iniciales
    */
    sNewPokenavViewPtr->state = 0;
    sNewPokenavViewPtr->rowIndex = rowPos;
    sNewPokenavViewPtr->columnIndex = columnPos;
    sNewPokenavViewPtr->tempFlag = TRUE;
    sNewPokenavViewPtr->savedCallback = callback;

    SetMainCallback2(NewPokenav_RunSetupFromApp);
}


static void NewPokenav_RunSetupFromApp(void)
{
    while (!NewPokenav_DoGfxSetupFromApp())
    {
    }
}


static bool8 NewPokenav_DoGfxSetupFromApp(void)
{
    u8 taskId;
    switch (gMain.state)
    {
    case 0:
        SetVBlankHBlankCallbacksToNull();
        ClearScheduledBgCopiesToVram();
        gMain.state++;
        break;
    case 1:
        ScanlineEffect_Stop();
        gMain.state++;
        break;
    case 2:
        FreeAllSpritePalettes();
        gMain.state++;
        break;
    case 3:
        ResetPaletteFade();
        ResetSpriteData();
        ResetTasks();
        gMain.state++;
        break;
    case 4:
        if (NewPokenav_InitBgs())
        {
            sNewPokenavViewPtr->state = 0;
            gMain.state++;
        }
        else
        {
            NewPokenavFadeAndExit();
            return TRUE;
        }
        break;
    case 5:
        if (NewPokenav_LoadGraphicsFromApp() == TRUE)
            gMain.state++;
        break;
    case 6:
        NewPokenav_InitWindows();
        sNewPokenavViewPtr->iterator = 0; /* OJO */
        sNewPokenavViewPtr->cursorPosX = 133;
        sNewPokenavViewPtr->cursorPosY = 69;
        sNewPokenavViewPtr->columnIsMoving = FALSE;
        sNewPokenavViewPtr->textHorizontalPos = 0;
        gMain.state++;
        break;
    case 7:
        gMain.state++;
        break;
    case 8:
        taskId = CreateTask(Task_LoadIcons, 0);
        gMain.state++;
        break;
    case 9:
        PrintControlsAndAppName();
        gMain.state++;
        break;
    case 10:
        BlendPalettes(0xFFFFFFFF, 16, RGB_BLACK);
        gMain.state++;
        break;
    case 11:
        BeginNormalPaletteFade(0xFFFFFFFF, 0, 16, 0, RGB_BLACK);
        gMain.state++;
        break;
    default:
        SetVBlankCallback(NewPokenav_VBlankCB);
        SetMainCallback2(NewPokenav_MainCB);
        return TRUE;
    }
    return FALSE;
}

static bool8 NewPokenav_LoadGraphicsFromApp(void)
{
    switch (sNewPokenavViewPtr->state)
    {
    case 0:
        ResetTempTileDataBuffers();
        DecompressAndCopyTileDataToVram(1, margin_tiles, 0, 0, 0);
        DecompressAndCopyTileDataToVram(2, text_bg_tiles, 0, 0, 0);
        DecompressAndCopyTileDataToVram(3, background_tiles, 0, 0, 0);
        sNewPokenavViewPtr->state++;
        break;
    case 1:
        if (FreeTempTileDataBuffersIfPossible() != TRUE)
        {
            LZDecompressWram(margin_map, sBg1TilemapBuffer);
            LZDecompressWram(text_bg_map, sBg2TilemapBuffer);
            LZDecompressWram(background_map, sBg3TilemapBuffer);
            sNewPokenavViewPtr->state++;
        }
        break;
    case 2:
        LoadPalette(margin_pal, 0, 32);
        LoadPalette(background_pal, 0x10, 32);
        LoadPalette(text_bg_pal, 0x20, 32);
        SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_TGT1_BG2 | BLDCNT_EFFECT_BLEND | BLDCNT_TGT2_ALL); /* OJO */
        SetGpuReg(REG_OFFSET_BLDALPHA, BLDALPHA_BLEND(6, 16));
        sNewPokenavViewPtr->state++;
        break;
    default:
        sNewPokenavViewPtr->state = 0;
        return TRUE;
    }
    return FALSE;
}


static void PrintControlsAndAppName(void)
{
    u8 normalizedIndex = sNewPokenavViewPtr->rowIndex + (sNewPokenavViewPtr->columnIndex) * 3;
    FillWindowPixelBuffer(CONTROLS, PIXEL_FILL(TEXT_COLOR_TRANSPARENT));
    PutWindowTilemap(CONTROLS);
    AddTextPrinterParameterized3(CONTROLS, 2, 0, 0, sFontColor_White, 0, gText_MainControlsPokeNav);
    CopyWindowToVram(CONTROLS, 3);
    /* OJO */
    CreateSelectionCursor(FROM_APP);
    /*     */
    FillWindowPixelBuffer(APP_TITLE, PIXEL_FILL(TEXT_COLOR_TRANSPARENT));
    AddTextPrinterParameterized3(APP_TITLE, 2, 0, 0, sFontColor_White, 0, appNames[normalizedIndex]);
    CopyWindowToVram(APP_TITLE, 3);
    PutWindowTilemap(APP_TITLE);
}

static void Task_FromAppWaitFadeIn(u8 taskId)
{
    if (!gPaletteFade.active)
        gTasks[taskId].func = Task_NewPokenavMain;
}

#undef tFrameCounter
#undef tOpacityLevel
#undef tCursorXPos
#undef tCursorYPos