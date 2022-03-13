#include "global.h"
#include "menu.h"
#include "palette.h"
#include "string_util.h"
#include "text.h"
#include "text_window.h"
#include "window.h"

/* File's functions*/

void InitTextWindowsFromTemplate(const struct WindowTemplate *TextWindow)
{
    InitWindows(TextWindow);                                  //Inicializa las ventanas de texto
    DeactivateAllTextPrinters();                              //Desactiva cualquier posible Text printer que haya quedado abierto
    LoadPalette(GetOverworldTextboxPalettePtr(), 0xf0, 0x20); //Carga la paleta del texto por defecto del juego en el slot 15 de las paletas para backgrounds
                                                              //LoadUserWindowBorderGfx(YES_NO_WINDOW, 211, 0xe0); //Opcional, necesario si se van a usar ventanas Yes/no
}

void AddTextToTextBox(const u8 *text)
{
    StringExpandPlaceholders(gStringVar4, text); //Coloca el texto en el buffer de strings
}

u8 CreateTextBox(u8 windowId, u8 font, const u8 *colourTable, u8 left, u8 top, u8 fillColor)
{
    FillWindowPixelBuffer(windowId, fillColor);                                            //Llena todos los pixeles del Buffer con el color indicado (0-> transparencia)
    PutWindowTilemap(windowId);                                                            //Coloca el tilemap de los graficos
    AddTextPrinterParameterized3(windowId, font, left, top, colourTable, -1, gStringVar4); //Asigna un textPrinter
    CopyWindowToVram(windowId, 3);                                                         //Copia la ventana del buffer a la VRam
    return windowId;
}

u8 CreateTextBoxWithStringBuffer(u8 windowId, u8 font, const u8 *colourTable, u8 left, u8 top, u8 fillColor, u8 *stringBuffer)
{
    FillWindowPixelBuffer(windowId, fillColor);                                             //Llena todos los pixeles del Buffer con el color indicado (0-> transparencia)
    PutWindowTilemap(windowId);                                                             //Coloca el tilemap de los graficos
    AddTextPrinterParameterized3(windowId, font, left, top, colourTable, -1, stringBuffer); //Asigna un textPrinter
    CopyWindowToVram(windowId, 3);                                                          //Copia la ventana del buffer a la VRam
    return windowId;
}

static void AddTextPrinterForMessage_Custom(bool8 allowSkippingDelayWithButtonPress)
{
    void (*callback)(struct TextPrinterTemplate *, u16) = NULL;
    gTextFlags.canABSpeedUpPrint = allowSkippingDelayWithButtonPress;
    AddTextPrinterParameterized2(0, 1, gStringVar4, GetPlayerTextSpeedDelay(), callback, 2, 1, 3);
}

/*

void GenerateText(const u8 *text) // Si falla anteponer un const al u8
{
    StringExpandPlaceholders(gStringVar4, text);
    AddTextPrinterForMessage_Custom(0);
}

void GenerateTextBox(u8 windowId)
{
    //Estas 3 funciones son predefinidas de pokeemerald (muy importante para los que están empezando diferenciar cuáles creamos y cúales no)
    FillWindowPixelBuffer(windowId, 0); //color 0 para los píxeles(transparente), de no hacerlo veríamos el texto acompañado de unas líneas verticales, muy bug.
    PutWindowTilemap(windowId);         // Colocamos el tilemap, OJO sus gráficos no los estamos colocando ni lo vamos a hacer. Sin el tilemap no se pueden mostrar textos.
    CopyWindowToVram(windowId, 3);      // Evidentemente, si no mandamos el window a la ram de video tampoco podemos mostrar texto. Generalmente se usa el modo 3.
}

void InitTextWindows(const struct WindowTemplate *TextWindow)
{
    //Estas 3 funciones son predefinidas de pokeemerald
    InitWindows(TextWindow);                                //Inicializa los window, para que podamos colocar nuestro texto.
    DeactivateAllTextPrinters();                              //Desactiva printers anteriores que hubiese, para que no haya contradicciones.
    LoadPalette(GetOverworldTextboxPalettePtr(), 0xf0, 0x20); /*Carga la paleta del texto por defecto del juego en el slot 15 de las paletas para backgrounds,
                                esto claramente afecta si quieres cargar un bg de 224 colores, porque necesitas 16 colores para el
                                texto y otros 16 para el textbox(si lo tuviera, que en este caso no lo cargamos, por tanto podrías usar
                                un fondo de 240 colores). Así que si indexas tu tileset en 255 colores, con CMP debes eliminar
                                en la paleta los 16 colores de la fila 15 y los 16 colores de la última fila. CMP adaptará el tileset perfectamente.
}

void InitText(u8 windowId, const u8 *text)
{
    GenerateTextBox(windowId); // ó CreateTextBox(0);
    GenerateText(text);
}

void EliminateText(u8 windowId)
{
    if (!RunTextPrintersAndIsPrinter0Active())
    {
        ClearDialogWindowAndFrame(windowId, 1);
        FreeAllWindowBuffers();
    }
}

*/