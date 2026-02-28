#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "GUI_GSP.h"
#include "GSP_Session.h"
#include "GSP_mem_monitor.h"

#ifdef USED_SESSION

static GSP_SessionStorage *SessionNVM = NULL;
static GSP_SessionData     SessionCache;
static bool                SessionLoaded = false;

static struct
{
    int             screen_id;
    ScreenCreateFunc create_func;
} ScreenRegistry[GSP_SESSION_MAX_SCREENS];

static int RegistryCount = 0;

static unsigned char CalcChecksum(const GSP_SessionData *data)
{
    unsigned char cs = 0;
    const unsigned char *p = (const unsigned char *)data;
    unsigned int len = sizeof(GSP_SessionData) - 1;
    for(unsigned int i = 0; i < len; i++)
        cs ^= p[i];
    return cs;
}

static bool ValidateSession(const GSP_SessionData *data)
{
    if(data->magic != GSP_SESSION_MAGIC) return false;
    return (CalcChecksum(data) == data->checksum);
}

static int WriteSession(const GSP_SessionData *data)
{
    if(!SessionNVM || !SessionNVM->write) return -1;
    return SessionNVM->write(SessionNVM->base_address, data, sizeof(GSP_SessionData));
}

static int ReadSession(GSP_SessionData *data)
{
    if(!SessionNVM || !SessionNVM->read) return -1;
    return SessionNVM->read(SessionNVM->base_address, data, sizeof(GSP_SessionData));
}
//*****************************************************************
void GSP_Session_Init(GSP_SessionStorage *storage)
{
    SessionNVM = storage;
    RegistryCount = 0;
    SessionLoaded = false;
    memset(&SessionCache, 0, sizeof(SessionCache));
    memset(ScreenRegistry, 0, sizeof(ScreenRegistry));
}
//*****************************************************************
int GSP_Session_RegisterScreen(int screen_id, ScreenCreateFunc create_func)
{
    if(!create_func) return -1;
    if(RegistryCount >= GSP_SESSION_MAX_SCREENS) return -2;

    for(int i = 0; i < RegistryCount; i++)
    {
        if(ScreenRegistry[i].screen_id == screen_id)
        {
            ScreenRegistry[i].create_func = create_func;
            return 0;
        }
    }

    ScreenRegistry[RegistryCount].screen_id   = screen_id;
    ScreenRegistry[RegistryCount].create_func  = create_func;
    RegistryCount++;
    return 0;
}
//*****************************************************************
int GSP_Session_Save(void)
{
    if(!Active_Screen) return -1;

    GSP_SessionData data;
    memset(&data, 0, sizeof(data));
    data.magic     = GSP_SESSION_MAGIC;
    data.screen_id = Active_Screen->ID;
    data.checksum  = CalcChecksum(&data);

    SessionCache = data;
    SessionLoaded = true;

    return WriteSession(&data);
}
//*****************************************************************
int GSP_Session_SaveWithData(const void *user_data, unsigned int size)
{
    if(!Active_Screen) return -1;

    GSP_SessionData data;
    memset(&data, 0, sizeof(data));
    data.magic     = GSP_SESSION_MAGIC;
    data.screen_id = Active_Screen->ID;

    if(user_data && size > 0)
    {
        if(size > GSP_SESSION_USER_DATA_SIZE)
            size = GSP_SESSION_USER_DATA_SIZE;
        memcpy(data.user_data, user_data, size);
    }

    data.checksum = CalcChecksum(&data);

    SessionCache = data;
    SessionLoaded = true;

    return WriteSession(&data);
}
//*****************************************************************
int GSP_Session_Restore(void)
{
    GSP_SessionData data;
    if(ReadSession(&data) != 0) return -1;

    if(!ValidateSession(&data)) return -2;

    SessionCache = data;
    SessionLoaded = true;

    for(int i = 0; i < RegistryCount; i++)
    {
        if(ScreenRegistry[i].screen_id == data.screen_id)
        {
            if(ScreenRegistry[i].create_func)
            {
                ScreenRegistry[i].create_func();
                return 0;
            }
        }
    }

    return -3;
}
//*****************************************************************
int GSP_Session_GetUserData(void *buffer, unsigned int size)
{
    if(!buffer || size == 0) return -1;
    if(!SessionLoaded) return -2;

    if(size > GSP_SESSION_USER_DATA_SIZE)
        size = GSP_SESSION_USER_DATA_SIZE;

    memcpy(buffer, SessionCache.user_data, size);
    return 0;
}
//*****************************************************************
int GSP_Session_Clear(void)
{
    GSP_SessionData data;
    memset(&data, 0, sizeof(data));

    SessionLoaded = false;
    memset(&SessionCache, 0, sizeof(SessionCache));

    return WriteSession(&data);
}
//*****************************************************************
int GSP_Session_GetSavedScreenID(void)
{
    if(SessionLoaded) return SessionCache.screen_id;

    GSP_SessionData data;
    if(ReadSession(&data) != 0) return -1;
    if(!ValidateSession(&data)) return -2;

    return data.screen_id;
}

#endif /* USED_SESSION */
