/**
 * Simple demo application for GUI_GSP.
 * One screen, two labels; strings from language pack (English.json) when available.
 *
 * Integration:
 * - Add GUI_GSP and your display driver to the project (see PORTING_GUIDE).
 * - In main loop call only GUI_Run().
 * - In timer ISR (e.g. 10 ms) call GUI_TimerClock(10).
 * - When you have a language pack built (CrateLangsPack -> PackLang.lpn -> CrateMedia),
 *   pass &PackLang to Init_Langs; otherwise pass NULL and fallback strings are used.
 */

#include "GUI_GSP/GUI_GSP.h"
#include "GSP_Label.h"
#include "Langs.h"
#include <string.h>

/* Optional: when using FLASH language pack from CrateMedia, declare PackLang and pass &PackLang to Init_Langs */
/* extern const IMAGE_EXTERNAL PackLang; */

static _GSP_Screen *DemoScreen;
static _GSP_Label  *LabelTitle;
static _GSP_Label  *LabelBottom;

static const char *fallback_title   = "GUI_GSP Demo";
static const char *fallback_bottom  = "< Back";

static void CreateDemoScreen(void)
{
    /* Full-screen: 0, 0, SCREEN_HOR_SIZE, SCREEN_VER_SIZE */
    DemoScreen = Crate_Screen(0, 0, SCREEN_HOR_SIZE, SCREEN_VER_SIZE, 0);
    if (!DemoScreen) return;

    /* Title label: top center, string from language pack or fallback */
    LabelTitle = Crate_Label(DemoScreen, 0, 2, SCREEN_HOR_SIZE, 12, 1);
    if (LabelTitle) {
        const char *s = Get_String_From_LangPack(0, &AppTitle, 0);
        LabelSetText(LabelTitle, s ? s : fallback_title);
        LabelSetTextAllign(LabelTitle, ALLIGN_TEXT_CENTER);
        LabelSetWidgetAlign(LabelTitle, GSP_WIDGET_ALIGN_TOP_CENTER);
    }

    /* Bottom line: bottom center, from language pack or fallback */
    LabelBottom = Crate_Label(DemoScreen, 0, SCREEN_VER_SIZE - 14, SCREEN_HOR_SIZE, 10, 2);
    if (LabelBottom) {
        const char *s = Get_String_From_LangPack(0, &BottomLine, 0);
        LabelSetText(LabelBottom, s ? s : fallback_bottom);
        LabelSetTextAllign(LabelBottom, ALLIGN_TEXT_CENTER);
        LabelSetWidgetAlign(LabelBottom, GSP_WIDGET_ALIGN_BOTTOM_CENTER);
    }

    Set_Active_Screen(DemoScreen);
}

int main(void)
{
    /* 1. Init display (your Init_LCD) is called from GUI_GSP_Init via GSP_HW */
    GUI_GSP_Init();

    /* 2. Optional: init language pack from FLASH. Pass NULL if no pack linked yet. */
    /* When PackLang is available (from CrateMedia): Init_Langs((void *)&PackLang, 0); */
    if (Init_Langs(NULL, 0) >= 0) {
        /* Pack loaded; Get_String_From_LangPack will return strings from pack */
    }
    /* Else: no pack, use fallback strings in CreateDemoScreen */

    /* 3. Create and show demo screen */
    CreateDemoScreen();

    /* 4. Main loop: only GUI_Run(). Timer ISR must call GUI_TimerClock(period_ms). */
    for (;;) {
        GUI_Run();
    }
}
