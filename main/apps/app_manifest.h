//App Type and Code Declaration Array

#include "app.h"
#include "applist.h"

/*----------------------------------------------------------------------------
 * App Declarations
 * 
 * The following public declarations are required for each app.
 * app_init_t       _App_Main_NAME          - Initialize an instance of the app (initialize entire app for the first call of duplicables.)
 * app_handle_t     _App_Handle_NAME        - Handle an incoming message for an instance of the app
 * app_function_t   _App_Pause_NAME         - Pause an instance of the app
 * app_function_t   _App_Resume_NAME        - Resume an instance of the app
 * app_function_t   _App_Destroy_NAME       - Destroy an instance of the app
 * 
 * The following instance independent functions are also required.
 * 
 * Clear all memory used by an app and destroy all tasks and queues used by the app.
 * void         (*_Cleanup_NAME)    (void)
 * 
 * Report the status for a specified instance of an app, reporting APP_STATUS_NULL if non-existent
 * app_status_t (*_Status_NAME)     (unsigned char id)
 * 
 * Report all valid instances of an app, reporting 0 if none exist.
 * size_t       (*_IDScan_NAME)     (unsigned char *ids, size_t max_ids)
 *----------------------------------------------------------------------------*/

//--------------------------------------
//Hello World Interface Declarations
//--------------------------------------

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//App Code Forward Declarations
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int             HelloWorld_Main(unsigned char *id, hash_t data_tag, char *data, size_t data_len);
void            HelloWorld_Incoming(unsigned char id, char *stem,size_t stem_len, hash_t data_tag, char *data, size_t data_len);
void            HelloWorld_Pause(unsigned char id);
void            HelloWorld_Resume(unsigned char id);
void            HelloWorld_Destroy(unsigned char id);
void            HelloWorld_Cleanup();
size_t          HelloWorld_ID_Scan(unsigned char *ids, size_t max_ids);
app_status_t    HelloWorld_Status(unsigned char id);

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//Manifest Variables
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
const app_init_t      _App_Main_HelloWorld      = &HelloWorld_Main;
const app_handle_t    _App_Handle_HelloWorld    = &HelloWorld_Incoming;
const app_function_t  _App_Pause_HelloWorld     = &HelloWorld_Pause;
const app_function_t  _App_Resume_HelloWorld    = &HelloWorld_Resume;
const app_function_t  _App_Destroy_HelloWorld   = &HelloWorld_Destroy;

void            (*const _Cleanup_HelloWorld)  (void) = &HelloWorld_Cleanup;
app_status_t    (*const _Status_HelloWorld)   (unsigned char id) = &HelloWorld_Status;
size_t          (*const _IDScan_HelloWorld)   (unsigned char *ids, size_t max_ids) = &HelloWorld_ID_Scan;
//--------------------------------------


/*----------------------------------------------------------------------------
 * App Manifest
 * 
 * For each app, declare an entry in the following array.
 * Every field is required, details given in the prefered order below:
 * 
 * .app_name            - Human readable name of the app (used as channel base)
 * .app_name_hash       - Hash of the app name (from PCHashes)
 * .app_type            - Type of app; singlet, or duplicable.
 * .init                - Pointer to the app's init function.
 * .msg_handle          - Pointer to the app's message handler function.
 * .pause               - Pointer to the app's pause instance function.
 * .resume              - Pointer to the app's resume instance function.
 * .destroy             - Pointer to the app's destroy instance function.
 * .cleanup             - Pointer to the app's cleanup function.
 * .get_status          - Pointer to the app's status from id function.
 * .get_valid_ids       - Pointer to the app's full id scan function.
 *----------------------------------------------------------------------------*/
const app_interface_t _App_Manifest[1] = 
{
    {
        .app_name   = "HelloWorld",
        .app_name_h = PCHASH_HelloWorld,
        .app_type   = APP_SINGLET,

        .init           = _App_Main_HelloWorld,
        .msg_handle     = _App_Handle_HelloWorld,
        .pause          = _App_Pause_HelloWorld,
        .resume         = _App_Resume_HelloWorld,
        .destroy        = _App_Destroy_HelloWorld,
        .cleanup        = _Cleanup_HelloWorld,
        .get_status     = _Status_HelloWorld,
        .get_valid_ids  = _IDScan_HelloWorld
    }
};
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
 * Select App via Name
 * 
 * Attempts to find the app interface for a given hashed name,
 * if no matching manifests are found the returned pointer will be NULL.
 *----------------------------------------------------------------------------*/
const app_interface_t* App_Interface_From_Name(hash_t name_h)
{
    for(int i = 0; i < sizeof(_App_Manifest)/sizeof(app_interface_t); i++)
    {
        if(_App_Manifest[i].app_name_h == name_h)
        {
            return &_App_Manifest[i];
        }
    }

    return NULL;
}
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
 * Total Cleanup
 * 
 * Cleans up all memory, tasks, queues and other resources used by all apps
 * present in the manifest by sweeping through all cleanup functions.
 * 
 * This function is completely synchronous, so please ensure caller is able
 * to be blocked until all cleanup functions have completed. If not, please
 * create a new task to perform the cleanup. This function is designed to be
 * called only by system cleanup functions so no helper task is available.
 * 
 * WARNING:
 * This function doesn't perform any upstream cleanup, so do not call this
 * unless you are sure that the app list is aware you're about to do so - or
 * is inactive/destroyed.
 *----------------------------------------------------------------------------*/
void Cleanup_All_Apps()
{
    for(int i = 0; i < sizeof(_App_Manifest)/sizeof(app_interface_t); i++)
        _App_Manifest[i].cleanup();
}
/*----------------------------------------------------------------------------*/
