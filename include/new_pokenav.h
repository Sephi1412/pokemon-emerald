#ifndef GUARD_NEW_POKENAV_H
#define GUARD_NEW_POKENAV_H



/* GUI TAGS */
//#define ICON_PAL_TAG            56000
//#define ICON_GFX_TAG            55130
#define CURSOR_TAG              0x4005
#define CAPTURED_ALL_TAG        0x4002

/* FUNCTIONS */
//void StartNewPokenav_CB2();
void Task_OpenPokeNavFromStartMenu(u8 taskId);
bool8 StartMenuFromAppCallback(u8 row, u8 column);

#endif // GUARD_NEW_POKENAV_H