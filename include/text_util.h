#ifndef GUARD_TEXT_UTIL_H
#define GUARD_TEXT_UTIL_H

void InitTextWindowsFromTemplate(const struct WindowTemplate *TextWindow);
u8 CreateTextBox(u8 windowId, u8 font, const u8* colourTable, u8 left, u8 top, u8 fillColor);
void AddTextToTextBox(const u8* text);
u8 CreateTextBoxWithStringBuffer(u8 windowId, u8 font, const u8* colourTable, u8 left, u8 top, u8 fillColor, u8* stringBuffer);
//void GenerateText(const u8 *text); 
//void GenerateTextBox(u8 windowId);
//void InitTextWindows(const struct WindowTemplate *TextWindow);
//void InitText(u8 windowId, const u8 *text);
//void EliminateText(u8 windowId);
#endif // GUARD_TEXT_UTIL_H