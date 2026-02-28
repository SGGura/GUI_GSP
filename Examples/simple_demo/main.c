/**
 * Simple demo application for GUI_GSP.
 * One screen, two labels; strings from language pack (English.json) when available.
 * Demonstrates session save/restore: the last active screen is remembered across reboots.
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

/*
 * Platform-specific NVM read/write — replace with your EEPROM/Flash driver.
 * These stubs show the required function signatures.
 */
#ifdef USED_SESSION
static int NVM_Read(unsigned int address, void *buffer, unsigned int size)
{
    /* TODO: implement for your platform, e.g.:
     * return EEPROM_Read(address, buffer, size); */
    (void)address; (void)buffer; (void)size;
    return -1;
}

static int NVM_Write(unsigned int address, const void *buffer, unsigned int size)
{
    /* TODO: implement for your platform, e.g.:
     * return EEPROM_Write(address, buffer, size); */
    (void)address; (void)buffer; (void)size;
    return -1;
}

static GSP_SessionStorage SessionStorage = {
    .read         = NVM_Read,
    .write        = NVM_Write,
    .base_address = 0x0000
};
#endif

#define SCREEN_ID_DEMO  0
#define SCREEN_ID_SECOND 1

static void CreateDemoScreen(void);
static void CreateSecondScreen(void);

static void CreateDemoScreen(void)
{
    DemoScreen = Crate_Screen(0, 0, SCREEN_HOR_SIZE, SCREEN_VER_SIZE, SCREEN_ID_DEMO);
    if (!DemoScreen) return;

    LabelTitle = Crate_Label(DemoScreen, 0, 2, SCREEN_HOR_SIZE, 12, 1);
    if (LabelTitle) {
        if (Set_Text_Font_Label(0, LabelTitle, AppTitle.key_name, 0) == NULL)
            LabelSetText(LabelTitle, fallback_title);
        LabelSetTextAllign(LabelTitle, ALLIGN_TEXT_CENTER);
        LabelSetWidgetAlign(LabelTitle, GSP_WIDGET_ALIGN_TOP_CENTER);
    }

    LabelBottom = Crate_Label(DemoScreen, 0, SCREEN_VER_SIZE - 14, SCREEN_HOR_SIZE, 10, 2);
    if (LabelBottom) {
        if (Set_Text_Font_Label(0, LabelBottom, BottomLine.key_name, 0) == NULL)
            LabelSetText(LabelBottom, fallback_bottom);
        LabelSetTextAllign(LabelBottom, ALLIGN_TEXT_CENTER);
        LabelSetWidgetAlign(LabelBottom, GSP_WIDGET_ALIGN_BOTTOM_CENTER);
    }

    Set_Active_Screen(DemoScreen);

#ifdef USED_SESSION
    GSP_Session_Save();
#endif
}

static void CreateSecondScreen(void)
{
    _GSP_Screen *scr = Crate_Screen(0, 0, SCREEN_HOR_SIZE, SCREEN_VER_SIZE, SCREEN_ID_SECOND);
    if (!scr) return;

    _GSP_Label *lbl = Crate_Label(scr, 0, 0, SCREEN_HOR_SIZE, 12, 10);
    if (lbl) {
        LabelSetText(lbl, "Screen 2");
        LabelSetTextAllign(lbl, ALLIGN_TEXT_CENTER);
        LabelSetWidgetAlign(lbl, GSP_WIDGET_ALIGN_MIDDLE_CENTER);
    }

    Set_Active_Screen(scr);

#ifdef USED_SESSION
    GSP_Session_Save();
#endif
}

int main(void)
{
    GUI_GSP_Init();

    if (Init_Langs(NULL, 0) >= 0) {
        /* Pack loaded */
    }

#ifdef USED_SESSION
    GSP_Session_Init(&SessionStorage);
    GSP_Session_RegisterScreen(SCREEN_ID_DEMO,   CreateDemoScreen);
    GSP_Session_RegisterScreen(SCREEN_ID_SECOND,  CreateSecondScreen);

    if (GSP_Session_Restore() != 0) {
        CreateDemoScreen();
    }
#else
    CreateDemoScreen();
#endif

    for (;;) {
        GUI_Run();
    }
}
