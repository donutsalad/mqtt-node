#include "commons.h"
#include "mqttinterface.h"

#define APP_SINGLET     0
#define APP_DUPLICABLE  1

//NEEDS TO BE THE SAME AS stem[X] <- Please enforce!!! (mqttinterface.h lengths should be abstracted anyway)
#define APP_CHANNEL_MAX_LENGTH  16

#define APP_STATUS_NULL         0
#define APP_STATUS_READY        1
#define APP_STATUS_PAUSED       2
#define APP_STATUS_STOPPING     3

typedef int     (*app_init_t)       (unsigned char* id, hash_t data_tag, char *data, size_t data_len);
//typedef void    (*app_handle_t)     (unsigned char id, char* stem, size_t stem_len, hash_t data_tag, char *data, size_t data_len);
typedef void    (*app_handle_t)     (unsigned char id, mqtt_request_t* buffer);
typedef void    (*app_function_t)   (unsigned char id);

typedef unsigned char app_status_t;
typedef unsigned char app_type_t;

#ifndef APP_TYPES_H
#define APP_TYPES_H

typedef struct APP_Interface {
    app_init_t init;
    app_handle_t msg_handle;

    app_function_t pause;
    app_function_t resume;
    app_function_t destroy; //Free mem & tasks for only this ID.

    void (*cleanup)(void);  //Free all mem & tasks used by the app.

    app_status_t    (*get_status)       (unsigned char id);
    size_t          (*get_valid_ids)    (unsigned char *ids, size_t max_ids);

    const char* app_name;
    hash_t      app_name_h;
    app_type_t  app_type; //SINGLET / DUPLICABLE
} app_interface_t;

typedef struct APP_Instance {
    unsigned char id;
    const app_interface_t* app_interface;

    //Checked against incoming.path hash during routing.
    hash_t chash;
    char channel[APP_CHANNEL_MAX_LENGTH];   //Unhashed string.
    //Copy of app_interface->app_name when type APP_SINGLET
} app_instance_t;

//Utility shorteners for instances where id is superfluous in context.

//Deliver message to app instance
static inline void _deliver_msg(app_instance_t* app, mqtt_request_t* buffer) {
    app->app_interface->msg_handle(app->id, buffer);
}
//Pause app instance
static inline void _pause_app(app_instance_t* app) {
    app->app_interface->pause(app->id);
}
//Resume app instance
static inline void _resume_app(app_instance_t* app) {
    app->app_interface->resume(app->id);
}
//Destroy app instance
static inline void _destroy_app(app_instance_t* app) {
    app->app_interface->destroy(app->id);
}
//Get app status
static inline app_status_t _get_app_status(app_instance_t* app) {
    if(app->app_interface == NULL)
        return APP_STATUS_NULL;

    return app->app_interface->get_status(app->id);
}

#endif