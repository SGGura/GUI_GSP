#ifndef GSP_SESSION_H
#define GSP_SESSION_H

#include "GUI_GSP.h"

#define GSP_SESSION_MAGIC           0xA5
#define GSP_SESSION_MAX_SCREENS     16
#define GSP_SESSION_USER_DATA_SIZE  32

typedef struct
{
    unsigned char magic;
    int           screen_id;
    unsigned char user_data[GSP_SESSION_USER_DATA_SIZE];
    unsigned char checksum;
} __attribute__((packed)) GSP_SessionData;

typedef struct
{
    int (*read)(unsigned int address, void *buffer, unsigned int size);
    int (*write)(unsigned int address, const void *buffer, unsigned int size);
    unsigned int base_address;
} GSP_SessionStorage;

#ifdef USED_SESSION

void GSP_Session_Init(GSP_SessionStorage *storage);

int  GSP_Session_RegisterScreen(int screen_id, ScreenCreateFunc create_func);

int  GSP_Session_Save(void);

int  GSP_Session_SaveWithData(const void *data, unsigned int size);

int  GSP_Session_Restore(void);

int  GSP_Session_GetUserData(void *buffer, unsigned int size);

int  GSP_Session_Clear(void);

int  GSP_Session_GetSavedScreenID(void);

#endif /* USED_SESSION */

#endif /* GSP_SESSION_H */
