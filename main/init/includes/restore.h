#include <commons.h>

#include <nvs.h>
#include <nvs_flash.h>
#include <esp_err.h>

#define RESTORATION_LACKING         0
#define RESTORATION_USING           1
#define RESTORATION_UNDEFINED       2

#define ATTEMPT_RESTORE_OK          0
#define ATTEMPT_RESTORE_NONE        1
#define ATTEMPT_RESTORE_NULL        2
#define ATTEMPT_RESTORE_NVSERR      3
#define ATTEMPT_RESTORE_UNKNOWN     4
#define ATTEMPT_RESTORE_NEW_WIFI    5
#define ATTEMPT_RESTORE_BAD_WIFI    6

#define FLAGS_SET_SINGLE            0
#define FLAGS_SET_UNCHANGED         1
#define FLAGS_SET_TWEAKED           2
#define FLAGS_SET_UNHANDLED         3

int attempt_restore(void);
int using_restoration_data(void);
int get_boot_config_status(void);
int validate_restore_data(void);
int failed_with_config(void);
int bad_auth_from_config(void);