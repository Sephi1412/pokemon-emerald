#include "global.h"
#include "battle.h"
#include "battle_message.h"
#include "main.h"
#include "event_data.h"
#include "menu.h"
#include "menu_helpers.h"
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
#include "sound.h"
#include "strings.h"
#include "list_menu.h"
#include "malloc.h"
#include "string_util.h"
#include "util.h"
#include "reset_rtc_screen.h"
#include "reshow_battle_screen.h"
#include "constants/abilities.h"
#include "constants/moves.h"
#include "constants/items.h"
#include "constants/rgb.h"
#include "constants/vars.h"
#include "text_util.h"
#include "new_pokenav.h"

#define NUM_PAGES 3
#define BATTLE_THEMES 9
#define MISC_THEMES 5
#define IncreaseIndex(n) ((n + 1) % 3)
#define DecreaseIndex(n) ((n + 2) % 3)
#define GetIndex() (sMusicSelectorView->mainMenuIndex + 3 * (sMusicSelectorView->actualPage))

static const u32 sBg3_Tiles[] = INCBIN_U32("graphics/music_selector/bg3_map.4bpp.lz");
static const u32 sBg3_Map[] = INCBIN_U32("graphics/music_selector/bg3_map.bin.lz");
static const u16 sBg3Pal[] = INCBIN_U16("graphics/music_selector/bg3_map.gbapal");
static const u32 sBg2_Tiles[] = INCBIN_U32("graphics/music_selector/bg2_map.4bpp.lz");
static const u32 sBg2_Map[] = INCBIN_U32("graphics/music_selector/bg2_map.bin.lz");
static const u16 sBg2Pal[] = INCBIN_U16("graphics/music_selector/bg2_map.gbapal");
static const u32 sBg1_Tiles[] = INCBIN_U32("graphics/music_selector/bg1_map.4bpp.lz");
static const u32 sBg1_Map[] = INCBIN_U32("graphics/music_selector/bg1_map.bin.lz");
static const u16 sBg1Pal[] = INCBIN_U16("graphics/music_selector/bg1_map.gbapal");
static const u16 pokeballPal[] = INCBIN_U16("graphics/music_selector/icons/poke_ball.gbapal");
static const u32 pokeballSprite[] = INCBIN_U32("graphics/music_selector/icons/poke_ball.4bpp");

struct MusicMenu
{
    u8 prevMainMenuIndex;
    u8 mainMenuIndex;
    u8 firstRowIndex;
    u8 secondRowIndex;
    u8 thirdRowIndex;
    u8 songsIndex[3];
    u8 actualPage;
    u8 actualPlaylistLength;
    u8 rowNavIndex;
    u8 columnNavIndex;
    u16 deselectColor;
    u16 selectColor;
    bool8 songIsActive;
    bool8 pokeballIsSpinning;
    u8 pokeballSpriteId;
};

struct MusicSelectorSong
{
    u8 songName[24];
    u8 gameOrigin[24];
};

enum
{
    LIST_ITEM_WILD,
    LIST_ITEM_TRAINER,
    LIST_ITEM_GYM_LEADER,
    LIST_ITEM_COUNT
};

enum
{
    ACTIVE_WIN_MAIN,
    ACTIVE_WIN_SECONDARY,
    ACTIVE_WIN_MODIFY
};

enum
{
    BATTLING,
    CYCLING,
    SURFING,
    DIVING
};

static const u8 gText_MainControlsDpad[] = _("{DPAD_NONE}Select");
static const u8 gText_MainControlsAB[] = _("{A_BUTTON}Play Song  {B_BUTTON}Close");
static const u8 gText_ActualPage[] = _("{L_BUTTON}Page ");
static const u8 gText_RTriggerButton[] = _("{R_BUTTON}");
static const u8 gText_TitleMusicSelector[] = _("Music  Selector");
static const u8 gText_WildBattle[] = _("Wild Battle");
static const u8 gText_BikeTheme[] = _("Bike Theme");
static const u8 gText_TrainerBattle[] = _("Trainer Battle");
static const u8 gText_EliteFourBattle[] = _("Elite Four Battle");
static const u8 gText_ChampionBattle[] = _("Champion Battle");
static const u8 gText_FrontierBrainBattle[] = _("Frontier Battle");
static const u8 gText_SurfTheme[] = _("Surf Theme");
static const u8 gText_GymLeaderBattle[] = _("Gym Leader Battle");
static const u8 gText_DiveTheme[] = _("Dive Theme");

static EWRAM_DATA struct MusicMenu *sMusicSelectorView = NULL;

const struct MusicSelectorSong sBattleThemesInfo[] =
    {
        [0] =
            {
                .songName = _("Wild Pokémon"),
                .gameOrigin = _("RSE"),
            },
        [1] =
            {
                .songName = _("Vs. Trainer"),
                .gameOrigin = _("RSE"),
            },
        [2] =
            {
                .songName = _("Vs. Gym Leader"),
                .gameOrigin = _("RSE"),
            },
        [3] =
            {
                .songName = _("Vs. Elite 4"),
                .gameOrigin = _("RSE"),
            },
        [4] =
            {
                .songName = _("Vs. Champion"),
                .gameOrigin = _("RSE"),
            },
        [5] =
            {
                .songName = _("Wild Pokémon"),
                .gameOrigin = _("FRLG"),
            },
        [6] =
            {
                .songName = _("Vs. Trainer"),
                .gameOrigin = _("FRLG"),
            },
        [7] =
            {
                .songName = _("Vs. Gym Leader"),
                .gameOrigin = _("FRLG"),
            },
        [8] =
            {
                .songName = _("Vs. Elite 4"),
                .gameOrigin = _("BW"),
            },
        [9] =
            {
                .songName = _("Kanto Wild!"),
                .gameOrigin = _("HGSS"),
            },
        [10] =
            {
                .songName = _("Vs. Champion"),
                .gameOrigin = _("FRLG"),
            },
        [11] =
            {
                .songName = _("Prime Cup 4-6"),
                .gameOrigin = _("STADIUM"),
            },
        [12] =
            {
                .songName = _("Frontier Brain"),
                .gameOrigin = _("RSE"),
            },
};

const struct MusicSelectorSong sMiscThemesInfo[] =
    {
        [0] =
            {
                .songName = _("Surfing!"),
                .gameOrigin = _("RSE"),
            },
        [1] =
            {
                .songName = _("Bicycle"),
                .gameOrigin = _("RSE"),
            },
        [2] =
            {
                .songName = _("Sevii Route"),
                .gameOrigin = _("FRLG"),
            },
        [3] =
            {
                .songName = _("Casino"),
                .gameOrigin = _("RSE"),
            },
        [4] =
            {
                .songName = _("Surfing!"),
                .gameOrigin = _("FRLG"),
            },
        [5] =
            {
                .songName = _("Dive"),
                .gameOrigin = _("RSE"),
            },
        [6] =
            {
                .songName = _("Battle Pyramid"),
                .gameOrigin = _("RSE"),
            },
};

const u16 gBattleThemes[] = {
    1, // MUS_BATTLE27,
    1, // MUS_BATTLE20,
    1, // MUS_BATTLE32,
    1, // MUS_BATTLE38,
    1, // MUS_BATTLE33,
    1, // MUS_RG_VS_YASEI,
    1, // MUS_RG_VS_TORE,
    1, // MUS_RG_VS_GYM,
    1, // MUS_BW_ELITE4,
    1, // MUS_HGSS_KANTO_WILD,
    1, // MUS_RG_VS_LAST,
    1, // MUS_PRIME_CUP_4_6_STADIUM,
    1, // MUS_VS_FRONT
};

const u16 gMiscThemes[] = {
    1, // MUS_NAMINORI, // Surf
    1, // MUS_CYCLING, // Bike
    1, // MUS_RG_NANASHIMA, // Lake of Rage
    1, // MUS_CASINO,
    1, // MUS_RG_NAMINORI,
    1, // MUS_DEEPDEEP,
    1, // MUS_PYRAMID
};

const u8 *const gCategoriesString[] =
    {
        gText_WildBattle, gText_TrainerBattle, gText_GymLeaderBattle,
        gText_EliteFourBattle, gText_ChampionBattle, gText_FrontierBrainBattle,
        gText_BikeTheme, gText_SurfTheme, gText_DiveTheme};

const u16 MusicVARs[] =
    {
        1, 1, 1, // VAR_WILD_MUSIC, VAR_TRAINER_MUSIC, VAR_GYM_LEADER_MUSIC,
        1, 1, 1, // VAR_ELITE_FOUR_MUSIC, VAR_CHAMPION_MUSIC, VAR_FRONTIER_BRAIN,
        1, 1, 1  // VAR_CYCLING, VAR_SURFING, VAR_DIVING
};

const u8 playlistLengths[] =
    {
        ARRAY_COUNT(sBattleThemesInfo), ARRAY_COUNT(sBattleThemesInfo), ARRAY_COUNT(sBattleThemesInfo),
        ARRAY_COUNT(sBattleThemesInfo), ARRAY_COUNT(sBattleThemesInfo), ARRAY_COUNT(sBattleThemesInfo),
        ARRAY_COUNT(sMiscThemesInfo), ARRAY_COUNT(sMiscThemesInfo), ARRAY_COUNT(sMiscThemesInfo)};

const u8 playlistCategorizationTags[] =
    {
        BATTLING, BATTLING, BATTLING,
        BATTLING, BATTLING, BATTLING,
        CYCLING, SURFING, DIVING};
/*
    Wild Battle, Trainer Battle, Gym Leader Battle
    Surf, Dive, Cycling
    Battle Frontier, Elite 4, Champion Theme 

    Esto motiva a hacer un switch case de playlists dependiendo de la pagina
*/

static const struct BgTemplate sBgTemplates[] =
    {
        {.bg = 0,
         .charBaseIndex = 0,
         .mapBaseIndex = 31,
         .screenSize = 0,
         .paletteMode = 0,
         .priority = 0,
         .baseTile = 0},
        {
            .bg = 1,
            .charBaseIndex = 1,
            .mapBaseIndex = 10,
            .screenSize = 0,
            .paletteMode = 0,
            .priority = 1,
            .baseTile = 0,
        },
        {
            .bg = 2,
            .charBaseIndex = 2,
            .mapBaseIndex = 18,
            .screenSize = 1,
            .paletteMode = 0,
            .priority = 2,
            .baseTile = 0,
        },
        {
            .bg = 3,
            .charBaseIndex = 3,
            .mapBaseIndex = 26,
            .screenSize = 0,
            .paletteMode = 0,
            .priority = 3,
            .baseTile = 0,
        }};

void StartMusicMenu_CB2();
void Callback2_StartMusicSelector(void);
static void PrintControllerInfo();
static void printSongInfo(u8 row);
static void printAllSongsData();
static void printActualPage();
static void Task_PrintCateogriesStrings(u8 taskId);
static void Task_PrintFirstSongName(u8 taskId);
static void Task_PrintFirstSongGame(u8 taskId);
static void Task_PrintSecondSongName(u8 taskId);
static void Task_PrintSecondSongGame(u8 taskId);
static void Task_PrintThirdSongName(u8 taskId);
static void Task_PrintThirdSongGame(u8 taskId);
static void Task_PrintControlStrings(u8 taskId);
static void Task_PrintMainMenuStrings(u8 taskId);
static void Task_NewPokenavWaitFadeIn(u8 taskId);
static void Task_IdleStateHandleInputs(u8 taskId);
static void Task_RemoveText(u8 taskId);
static void Task_MoveBackgroundToChangeInfo(u8 taskId);
static void Task_UpdateValues(u8 taskId);
static void Task_ChangeSongName(u8 taskId);
static void Task_ChangeSongGame(u8 taskId);
static void Task_PlaySongSelected(u8 taskId);
static void Task_StartPokeballAnim(u8 taskId);
static void Task_MoveCursor(u8 taskId);
static void Task_GoBackToPokeNav(u8 taskId);
static void playSongInMenu(u8 row);

static const struct OamData spriteRotationPokeball =
    {
        .y = 0,
        .affineMode = ST_OAM_AFFINE_DOUBLE,
        .objMode = 0,
        .mosaic = 0,
        .bpp = 0,
        .shape = 0,
        .x = 0,
        .matrixNum = 0,
        .size = SPRITE_SIZE(64x64),
        .tileNum = 0,
        .priority = 3,
        .paletteNum = 0,
        .affineParam = 0,
};

const struct SpriteSheet spriteSheetRotationPokeball =
    {
        .data = pokeballSprite,
        .size = 2048,
        .tag = 12, //Este es lugar en que el sistema dibujar� el sprite en la OAM
};

const struct SpritePalette spritePaletteRotationPokeball =
    {
        .data = pokeballPal,
        .tag = 8,
};

static const union AffineAnimCmd gDummyAffineAnim[] =
    {
        AFFINEANIMCMD_FRAME(256, 256, 0, 0),
        AFFINEANIMCMD_END_ALT(1),
};

static const union AffineAnimCmd gRotationPokeballAffineAnim[] =
    {
        AFFINEANIMCMD_FRAME(256, 256, 0, 0),
        AFFINEANIMCMD_FRAME(0, 0, 2, 60),
        AFFINEANIMCMD_JUMP(0),
};

static const union AffineAnimCmd *const gPokeballAffineAnimTable[] =
    {
        gDummyAffineAnim,
        gRotationPokeballAffineAnim,
};

const struct SpriteTemplate spriteTemplateRotationPokeball =
    {
        .tileTag = 12,
        .paletteTag = 8,
        .oam = &spriteRotationPokeball,
        .anims = gDummySpriteAnimTable,
        .images = NULL,
        .affineAnims = gPokeballAffineAnimTable,
        .callback = SpriteCallbackDummy,
};

static const u8 sFontColourTableType[] = {0, 1, 2};
static const u8 sFontColourTableTypeBlack[] = {0, 2, 3};

enum
{
    MUSIC_SELECTOR,
    SELECT_DPAD,
    AB_BUTTONS,
    CATEGORY_1,
    SONG_1,
    GAME_1,
    CATEGORY_2,
    SONG_2,
    GAME_2,
    CATEGORY_3,
    SONG_3,
    GAME_3,
    PAGE_ID
};

static const struct WindowTemplate sWindowTemplate_MenuStrings[] =
    {
        [MUSIC_SELECTOR] = {
            .bg = 0,
            .tilemapLeft = 2,
            .tilemapTop = 0,
            .width = 12,
            .height = 2,
            .paletteNum = 15,
            .baseBlock = 1,
        },
        [SELECT_DPAD] = {
            .bg = 0,
            .tilemapLeft = 1,
            .tilemapTop = 18,
            .height = 2,
            .width = 6,
            .paletteNum = 15,
            .baseBlock = 25,
        },
        [AB_BUTTONS] = {
            .bg = 0,
            .tilemapLeft = 16,
            .tilemapTop = 18,
            .width = 13,
            .height = 2,
            .paletteNum = 15,
            .baseBlock = 37,
        },
        [CATEGORY_1] = {
            .bg = 0,
            .tilemapLeft = 1,
            .tilemapTop = 4,
            .width = 12,
            .height = 2,
            .paletteNum = 15,
            .baseBlock = 63,
        },
        [SONG_1] = {
            .bg = 0,
            .tilemapLeft = 4,
            .tilemapTop = 6,
            .width = 14,
            .height = 2,
            .paletteNum = 15,
            .baseBlock = 87,
        },
        [CATEGORY_2] = {
            .bg = 0,
            .tilemapLeft = 1,
            .tilemapTop = 8,
            .width = 12,
            .height = 2,
            .paletteNum = 15,
            .baseBlock = 115,
        },
        [SONG_2] = {
            .bg = 0,
            .tilemapLeft = 4,
            .tilemapTop = 10,
            .width = 14,
            .height = 2,
            .paletteNum = 15,
            .baseBlock = 139,
        },
        [CATEGORY_3] = {
            .bg = 0,
            .tilemapLeft = 1,
            .tilemapTop = 12,
            .width = 12,
            .height = 2,
            .paletteNum = 15,
            .baseBlock = 167,
        },
        [SONG_3] = {
            .bg = 0,
            .tilemapLeft = 4,
            .tilemapTop = 14,
            .width = 14,
            .height = 2,
            .paletteNum = 15,
            .baseBlock = 191,
        },
        [GAME_1] = {
            .bg = 0,
            .tilemapLeft = 14,
            .tilemapTop = 4,
            .width = 7,
            .height = 2,
            .paletteNum = 15,
            .baseBlock = 220,
        },
        [GAME_2] = {
            .bg = 0,
            .tilemapLeft = 14,
            .tilemapTop = 8,
            .width = 7,
            .height = 2,
            .paletteNum = 15,
            .baseBlock = 235,
        },
        [GAME_3] = {
            .bg = 0,
            .tilemapLeft = 14,
            .tilemapTop = 12,
            .width = 7,
            .height = 2,
            .paletteNum = 15,
            .baseBlock = 250,
        },
        [PAGE_ID] = {
            .bg = 0,
            .tilemapLeft = 21,
            .tilemapTop = 0,
            .width = 8,
            .height = 2,
            .paletteNum = 15,
            .baseBlock = 264,
        },
};

#define tFrameCounter data[0]
#define tPosition data[1]

static void dummyFunc()
{
    return;
}

static void InitAndShowBgsFromTemplate()
{
    InitBgsFromTemplates(0, sBgTemplates, ARRAY_COUNT(sBgTemplates));
    LZ77UnCompVram(sBg3_Tiles, (void *)VRAM + 0x4000 * 3);
    LZ77UnCompVram(sBg3_Map, (u16 *)BG_SCREEN_ADDR(26));
    LZ77UnCompVram(sBg2_Tiles, (void *)VRAM + 0x4000 * 2);
    LZ77UnCompVram(sBg2_Map, (u16 *)BG_SCREEN_ADDR(18));
    SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_TGT1_BG1 | BLDCNT_EFFECT_BLEND | BLDCNT_TGT2_ALL);
    SetGpuReg(REG_OFFSET_BLDALPHA, BLDALPHA_BLEND(0, 3));
    LZ77UnCompVram(sBg1_Tiles, (void *)VRAM + 0x4000 * 1);
    LZ77UnCompVram(sBg1_Map, (u16 *)BG_SCREEN_ADDR(10));
    LoadPalette(sBg2Pal, 0x00, 0x20);
    LoadPalette(sBg2Pal, 0x10, 0x20);
    LoadPalette(sBg2Pal, 0x20, 0x20);
    LoadPalette(sBg3Pal, 0x30, 0x20);
    LoadPalette(sBg1Pal, 0x40, 0x20);
    LoadPalette(&sMusicSelectorView->selectColor, 0x08, sizeof(sMusicSelectorView->selectColor));
    LoadPalette(&sMusicSelectorView->deselectColor, 0x18, sizeof(sMusicSelectorView->deselectColor));
    LoadPalette(&sMusicSelectorView->deselectColor, 0x28, sizeof(sMusicSelectorView->deselectColor));
    ResetAllBgsCoordinates();
    ShowBg(0);
    ShowBg(1);
    ShowBg(2);
    ShowBg(3);
}

static void MainCallBack2(void)
{
    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    UpdatePaletteFade();
}

static void drawPokeball()
{
    u8 spriteId;
    LoadSpriteSheet(&spriteSheetRotationPokeball);
    LoadSpritePalette(&spritePaletteRotationPokeball);
    spriteId = CreateSprite(&spriteTemplateRotationPokeball, 210, 100, 0);
    sMusicSelectorView->pokeballSpriteId = spriteId;
}

static void VBlankCallBack(void)
{
    LoadOam();
    ChangeBgX(3, -30, 2);
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

static void printSongInfo(u8 row)
{
    u8 CATEGORY_WINDOW_POS = 3 + (3 * row);
    u8 SONG_WINDOW_POS = 4 + (3 * row);
    u8 GAME_WINDOW_POS = 5 + (3 * row);
    u8 playlistCategory = playlistCategorizationTags[row + 3 * (sMusicSelectorView->actualPage)];
    AddTextToTextBox(gCategoriesString[row + 3 * (sMusicSelectorView->actualPage)]);
    CreateTextBox(CATEGORY_WINDOW_POS, 2, sFontColourTableType, 0, 0, 0);
    switch (playlistCategory)
    {
    default:
    case BATTLING:
        AddTextToTextBox(sBattleThemesInfo[sMusicSelectorView->songsIndex[row]].gameOrigin);
        CreateTextBox(GAME_WINDOW_POS, 0, sFontColourTableType, 0, 0, 0);
        AddTextToTextBox(sBattleThemesInfo[sMusicSelectorView->songsIndex[row]].songName);
        CreateTextBox(SONG_WINDOW_POS, 2, sFontColourTableTypeBlack, 0, 0, 0);
        break;
    case SURFING:
    case DIVING:
    case CYCLING:
        AddTextToTextBox(sMiscThemesInfo[sMusicSelectorView->songsIndex[row]].gameOrigin);
        CreateTextBox(GAME_WINDOW_POS, 0, sFontColourTableType, 0, 0, 0);
        AddTextToTextBox(sMiscThemesInfo[sMusicSelectorView->songsIndex[row]].songName);
        CreateTextBox(SONG_WINDOW_POS, 2, sFontColourTableTypeBlack, 0, 0, 0);
        break;
    }
}

void OpenMusicSelector()
{
    if (!gPaletteFade.active)
    {
        ResetTasks();
        gMain.state = 0;
        sMusicSelectorView = AllocZeroed(sizeof(struct MusicMenu));
        sMusicSelectorView->rowNavIndex = VarGet(VAR_TEMP_0);
        sMusicSelectorView->columnNavIndex = VarGet(VAR_TEMP_1);
        sMusicSelectorView->songIsActive = FALSE;
        sMusicSelectorView->mainMenuIndex = 0;
        sMusicSelectorView->actualPage = 0;
        sMusicSelectorView->songsIndex[0] = VarGet(MusicVARs[0]);
        sMusicSelectorView->songsIndex[1] = VarGet(MusicVARs[1]);
        sMusicSelectorView->songsIndex[2] = VarGet(MusicVARs[2]);
        sMusicSelectorView->selectColor = RGB(31, 31, 31);
        sMusicSelectorView->deselectColor = RGB(20, 20, 20);
        CleanupOverworldWindowsAndTilemaps();
        SetMainCallback2(Callback2_StartMusicSelector);

        return;
    }
    return;
}

void StartMusicSelector_CB2()
{
    if (!gPaletteFade.active)
    {
        gMain.state = 0;
        CleanupOverworldWindowsAndTilemaps();
        SetMainCallback2(Callback2_StartMusicSelector);
        return;
    }
    return;
}

void Callback2_StartMusicSelector(void)
{
    u8 taskId;
    switch (gMain.state)
    {
    default:
    case 0:
        SetVBlankCallback(NULL);
        gMain.state++;
        break;
    case 1:
        ResetVramOamAndBgCntRegs();
        SetGpuReg(REG_OFFSET_DISPCNT, 0);
        ResetBgsAndClearDma3BusyFlags(0);
        InitBgsFromTemplates(0, sBgTemplates, ARRAY_COUNT(sBgTemplates));
        ResetAllBgsCoordinates();
        FreeAllWindowBuffers();
        FreeAllSpritePalettes();
        DeactivateAllTextPrinters();
        SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_ON | DISPCNT_OBJ_1D_MAP);
        gMain.state++;
        break;
    case 2:
        ResetPaletteFade();
        ScanlineEffect_Stop();
        ResetTasks();
        ResetSpriteData();
        gMain.state++;
        break;
    case 3:
        InitAndShowBgsFromTemplate();
        drawPokeball();
        gMain.state++;
        break;
    case 4:
        InitTextWindowsFromTemplate(sWindowTemplate_MenuStrings);
        gMain.state++;
        break;
    case 5:
        PrintControllerInfo();
        gMain.state++;
        break;
    case 6:
        printAllSongsData();
        gMain.state++;
        break;
    case 7:
        printActualPage();
        gMain.state++;
        break;
    case 8:
        sMusicSelectorView->actualPlaylistLength = ARRAY_COUNT(sBattleThemesInfo);
        taskId = CreateTask(Task_NewPokenavWaitFadeIn, 0);
        gMain.state++;
        break;
    case 9:
        BeginNormalPaletteFade(-1, 0, 0x10, 0, 0);
        SetVBlankCallback(VBlankCallBack);
        SetMainCallback2(MainCallBack2);
        break;
    }
}

static void printActualPage()
{
    ConvertIntToDecimalStringN(gStringVar3, sMusicSelectorView->actualPage + 1, 2, 1);
    StringCopy(gStringVar1, &gText_RTriggerButton[0]);
    StringCopy(gStringVar2, &gText_ActualPage[0]);
    StringAppend(gStringVar3, gStringVar1);
    StringAppend(gStringVar2, gStringVar3);
    AddTextToTextBox(gStringVar2);
    CreateTextBox(PAGE_ID, 0, sFontColourTableType, 0, 0, 0);
}

static void printAllSongsData()
{
    u8 row;
    for (row = 0; row < 3; row++)
    {
        printSongInfo(row);
    }
}

static void PrintControllerInfo()
{
    AddTextToTextBox(gText_TitleMusicSelector);
    CreateTextBox(MUSIC_SELECTOR, 2, sFontColourTableType, 0, 0, 0);
    AddTextToTextBox(gText_MainControlsDpad);
    CreateTextBox(SELECT_DPAD, 2, sFontColourTableType, 0, 0, 0);
    AddTextToTextBox(gText_MainControlsAB);
    CreateTextBox(AB_BUTTONS, 2, sFontColourTableType, 0, 0, 0);
}



static void Task_NewPokenavWaitFadeIn(u8 taskId)
{
    if (!gPaletteFade.active)
        gTasks[taskId].func = Task_IdleStateHandleInputs;
}

static void Task_IdleStateHandleInputs(u8 taskId)
{
    u8 actualRow = sMusicSelectorView->mainMenuIndex;
    u8 playlistLen = sMusicSelectorView->actualPlaylistLength;
    if (gMain.newKeys & A_BUTTON)
    {
        gTasks[taskId].func = Task_PlaySongSelected;
    }
    if (gMain.newKeys & DPAD_UP)
    {
        sMusicSelectorView->prevMainMenuIndex = sMusicSelectorView->mainMenuIndex;
        sMusicSelectorView->mainMenuIndex = DecreaseIndex(sMusicSelectorView->mainMenuIndex);
        sMusicSelectorView->actualPlaylistLength = playlistLengths[GetIndex()];
        gTasks[taskId].func = Task_MoveCursor;
    }
    if (gMain.newKeys & DPAD_DOWN)
    {
        sMusicSelectorView->prevMainMenuIndex = sMusicSelectorView->mainMenuIndex;
        sMusicSelectorView->mainMenuIndex = IncreaseIndex(sMusicSelectorView->mainMenuIndex);
        sMusicSelectorView->actualPlaylistLength = playlistLengths[GetIndex()];
        gTasks[taskId].func = Task_MoveCursor;
    }
    if (gMain.newKeys & DPAD_RIGHT)
    {

        sMusicSelectorView->songsIndex[actualRow] = (sMusicSelectorView->songsIndex[actualRow] + 1) % playlistLen;
        gTasks[taskId].func = Task_ChangeSongName;
    }
    if (gMain.newKeys & DPAD_LEFT)
    {
        sMusicSelectorView->songsIndex[actualRow] = (sMusicSelectorView->songsIndex[actualRow] + (playlistLen - 1)) % playlistLen;
        gTasks[taskId].func = Task_ChangeSongName;
    }
    if (gMain.newKeys & L_BUTTON)
    {
        sMusicSelectorView->actualPage = (sMusicSelectorView->actualPage + (NUM_PAGES - 1)) % NUM_PAGES;
        sMusicSelectorView->actualPlaylistLength = playlistLengths[GetIndex()];
        gTasks[taskId].tFrameCounter = 0;
        gTasks[taskId].tPosition = 0;
        gTasks[taskId].func = Task_UpdateValues;
    }
    if (gMain.newKeys & R_BUTTON)
    {
        sMusicSelectorView->actualPage = (sMusicSelectorView->actualPage + 1) % NUM_PAGES;
        sMusicSelectorView->actualPlaylistLength = playlistLengths[GetIndex()];
        gTasks[taskId].tFrameCounter = 0;
        gTasks[taskId].tPosition = 0;
        gTasks[taskId].func = Task_UpdateValues;
    }

    if (gMain.newKeys & B_BUTTON)
    {
        BeginNormalPaletteFade(0xFFFFFFFF, 0, 0, 0x10, RGB_BLACK);
        Overworld_ClearSavedMusic();
        gTasks[taskId].func = Task_GoBackToPokeNav;
    };
}

/*
static void Task_RemoveText(u8 taskId)
{
    u8 row, CATEGORY_WINDOW_POS, SONG_WINDOW_POS, GAME_WINDOW_POS;
    for(row = 0; row < 3; row++)
    {
        AddTextToTextBox(gText_Blank);
        CreateTextBox(CATEGORY_WINDOW_POS, 2, sFontColourTableType, 0, 0, 0);
        AddTextToTextBox(gText_Blank);
        CreateTextBox(GAME_WINDOW_POS, 0, sFontColourTableType, 0, 0, 0);
        AddTextToTextBox(gText_Blank);
        CreateTextBox(SONG_WINDOW_POS, 2, sFontColourTableTypeBlack, 0, 0, 0);
    }
    AddTextToTextBox(gText_TitleMusicSelector);
    CreateTextBox(MUSIC_SELECTOR, 2, sFontColourTableType, 0, 0, 0);
    PlaySE(SE_Z_PAGE);
    gTasks[taskId].func = Task_MoveBackgroundToChangeInfo;
}
*/

static void Task_MoveBackgroundToChangeInfo(u8 taskId)
{
    if (gTasks[taskId].tFrameCounter >= 30)
    {
        gTasks[taskId].tPosition = 0;
        gTasks[taskId].tFrameCounter = 0;
        gTasks[taskId].func = Task_UpdateValues;
    }
    else
    {
        if (gTasks[taskId].tFrameCounter < 15)
        {
            gTasks[taskId].tPosition += 10;
            gTasks[taskId].tFrameCounter++;
            SetGpuReg(REG_OFFSET_BG2HOFS, gTasks[taskId].tPosition);
        }
        else
        {
            gTasks[taskId].tPosition -= 10;
            gTasks[taskId].tFrameCounter++;
            SetGpuReg(REG_OFFSET_BG2HOFS, gTasks[taskId].tPosition);
        }
    }
}

static void Task_UpdateValues(u8 taskId)
{
    PlaySE(SE_DEX_PAGE);
    sMusicSelectorView->songsIndex[0] = VarGet(MusicVARs[0 + 3 * sMusicSelectorView->actualPage]);
    sMusicSelectorView->songsIndex[1] = VarGet(MusicVARs[1 + 3 * sMusicSelectorView->actualPage]);
    sMusicSelectorView->songsIndex[2] = VarGet(MusicVARs[2 + 3 * sMusicSelectorView->actualPage]);
    dummyFunc();
    printActualPage();
    printAllSongsData();
    gTasks[taskId].func = Task_IdleStateHandleInputs;
}

static void Task_ChangeSongName(u8 taskId)
{
    u8 actualRow = sMusicSelectorView->mainMenuIndex;
    u8 songIndex = sMusicSelectorView->songsIndex[actualRow];
    u8 playlistCategory = playlistCategorizationTags[actualRow + 3 * (sMusicSelectorView->actualPage)];
    u8 SONG_WIN_POS = 4 + (actualRow)*3;
    PlaySE(SE_SELECT);
    switch (playlistCategory)
    {
    default:
    case BATTLING:
        AddTextToTextBox(sBattleThemesInfo[songIndex].songName);
        CreateTextBox(SONG_WIN_POS, 2, sFontColourTableTypeBlack, 0, 0, 0);
        break;

    case SURFING:
    case DIVING:
    case CYCLING:
        AddTextToTextBox(sMiscThemesInfo[songIndex].songName);
        CreateTextBox(SONG_WIN_POS, 2, sFontColourTableTypeBlack, 0, 0, 0);
        break;
    }
    VarSet(MusicVARs[actualRow + 3 * (sMusicSelectorView->actualPage)], songIndex);
    gTasks[taskId].func = Task_ChangeSongGame;
}

static void Task_ChangeSongGame(u8 taskId)
{
    u8 actualRow = sMusicSelectorView->mainMenuIndex;
    u8 songIndex = sMusicSelectorView->songsIndex[actualRow];
    u8 playlistCategory = playlistCategorizationTags[actualRow + 3 * (sMusicSelectorView->actualPage)];
    u8 GAME_WIN_POS = 5 + (actualRow)*3;

    switch (playlistCategory)
    {
    default:
    case BATTLING:
        AddTextToTextBox(sBattleThemesInfo[songIndex].gameOrigin);
        CreateTextBox(GAME_WIN_POS, 0, sFontColourTableType, 0, 0, 0);
        break;

    case SURFING:
    case DIVING:
    case CYCLING:
        AddTextToTextBox(sMiscThemesInfo[songIndex].gameOrigin);
        CreateTextBox(GAME_WIN_POS, 0, sFontColourTableType, 0, 0, 0);
        break;
    }
    gTasks[taskId].func = Task_IdleStateHandleInputs;
}

static void Task_PlaySongSelected(u8 taskId)
{
    if (!sMusicSelectorView->pokeballIsSpinning)
        gTasks[taskId].func = Task_StartPokeballAnim;
    else
    {
        if (sMusicSelectorView->songIsActive)
            Overworld_ClearSavedMusic();
        playSongInMenu(sMusicSelectorView->mainMenuIndex);
        sMusicSelectorView->songIsActive = TRUE;
        gTasks[taskId].func = Task_IdleStateHandleInputs;
    }
}

static void Task_StartPokeballAnim(u8 taskId)
{
    StartSpriteAffineAnim(&gSprites[sMusicSelectorView->pokeballSpriteId], 1);
    sMusicSelectorView->pokeballIsSpinning = TRUE;
    gTasks[taskId].func = Task_PlaySongSelected;
}

static void Task_MoveCursor(u8 taskId)
{

    u8 previousRow = sMusicSelectorView->prevMainMenuIndex;
    u8 actualRow = sMusicSelectorView->mainMenuIndex;
    PlaySE(SE_SELECT);
    LoadPalette(&sMusicSelectorView->deselectColor, 0x10 * previousRow + 0x08, sizeof(sMusicSelectorView->deselectColor));
    LoadPalette(&sMusicSelectorView->selectColor, 0x10 * actualRow + 0x08, sizeof(sMusicSelectorView->selectColor));
    gTasks[taskId].func = Task_IdleStateHandleInputs;
}

static void Task_GoBackToPokeNav(u8 taskId)
{
    if (!gPaletteFade.active)
    {

        Free(sMusicSelectorView);
        PlaySE(SE_SAVE);
        DestroyTask(taskId);
        FreeAllWindowBuffers();
        StartMenuFromAppCallback(sMusicSelectorView->rowNavIndex, sMusicSelectorView->columnNavIndex);
    }
}

static void playSongInMenu(u8 row)
{
    u8 playlistCategory = playlistCategorizationTags[row + 3 * (sMusicSelectorView->actualPage)];

    switch (playlistCategory)
    {
    default:
    case BATTLING:
        Overworld_ChangeMusicTo(gBattleThemes[VarGet(MusicVARs[row + 3 * sMusicSelectorView->actualPage])]);
        break;

    case SURFING:
    case DIVING:
    case CYCLING:
        Overworld_ChangeMusicTo(gMiscThemes[VarGet(MusicVARs[row + 3 * sMusicSelectorView->actualPage])]);
        break;
    }
}

#undef tFrameCounter
#undef tPosition
