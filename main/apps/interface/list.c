#include "applist.h"
#include "app_manifest.h"

#include <string.h>

static app_instance_t app_list[APP_LIST_MAXIMUM_CONCURRENT_INSTANCES];

//TODO: Make this thread safe.
//TOOD: Abstract creation process for a future sync-all-ID's to pick up orphaned app instances.
static int Create_New_Instance(
    hash_t stem_hash, char *stem,  size_t stem_len, 
    hash_t data_tag, char *data, size_t data_len
) {
    int fridx = -1;
    for(int i = 0; i < APP_LIST_MAXIMUM_CONCURRENT_INSTANCES; i++)
        if(app_list[i].app_interface == NULL) {
            fridx = i; break; //Lazy search for first free index
        }

    if(fridx == -1) return LIST_NO_SPACE;

    const app_interface_t* interface = App_Interface_From_Name(stem_hash);
    if(interface == NULL) return LIST_BAD_APP_NAME;

    if(interface->app_type == APP_SINGLET)
        for(int i = 0; i < APP_LIST_MAXIMUM_CONCURRENT_INSTANCES; i++)
            if(app_list[i].app_interface == interface) return LIST_SINGLET_DUPLICANT;

    unsigned char id;
    int result = interface->init(&id, data_tag, data, data_len);
    //TODO: Check result

    app_list[fridx].id              = id;
    app_list[fridx].app_interface   = interface;
    switch(interface->app_type)
    {
        case APP_SINGLET:
            strncpy(app_list[fridx].channel, stem, (stem_len > APP_CHANNEL_MAX_LENGTH) ? APP_CHANNEL_MAX_LENGTH : stem_len);
            app_list[fridx].channel[APP_CHANNEL_MAX_LENGTH - 1] = '\0';
            break;

        case APP_DUPLICABLE:
            snprintf(app_list[fridx].channel, APP_CHANNEL_MAX_LENGTH, "%d-%s", id, stem); //Could log trunctation degree here if needed.
            app_list[fridx].channel[APP_CHANNEL_MAX_LENGTH - 1] = '\0';
            break;

        //TODO: Default case error throw.
    }

    app_list[fridx].chash = djb_hash(app_list[fridx].channel);

    return LIST_INSTANTIATED;
}

static app_instance_t* GetAppInstance(hash_t chash)
{
    for(int i = 0; i < APP_LIST_MAXIMUM_CONCURRENT_INSTANCES; i++)
        if(app_list[i].app_interface != NULL && app_list[i].chash == chash)
            return &app_list[i];

    return NULL;
}

static int Destroy_Instance(hash_t chash)
{
    app_instance_t* instance = GetAppInstance(chash);
    if(instance == NULL) return LIST_BAD_APP_NAME;
    
    _destroy_app(instance);
    instance->app_interface = NULL;

    return LIST_DESTROYED;
}

static int Pause_Instance(hash_t chash)
{
    app_instance_t* instance = GetAppInstance(chash);
    if(instance == NULL) return LIST_BAD_APP_NAME;
    
    _pause_app(instance);
    return LIST_PAUSED;
}

static int Resume_Instance(hash_t chash)
{
    app_instance_t* instance = GetAppInstance(chash);
    if(instance == NULL) return LIST_BAD_APP_NAME;
    
    _resume_app(instance);
    return LIST_RESUMED;
}

int Deliver_Message(mqtt_request_t* request)
{
    app_instance_t* instance = GetAppInstance(request->path);
    if(instance == NULL) return LIST_BAD_APP_NAME;
    
    _deliver_msg(
        instance,          request->stem, sizeof(request->stem), 
        request->data_tag, request->data, sizeof(request->data)
    );

    return MSG_DELIVERED;
}