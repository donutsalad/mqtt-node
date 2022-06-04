#include <commons.h>

#include <nvs.h>
#include <nvs_flash.h>
#include <esp_err.h>

#define ATTEMPT_RESTORE_OK      0
#define ATTEMPT_RESTORE_NONE    1
#define ATTEMPT_RESTORE_NULL    2
#define ATTEMPT_RESTORE_NVSERR  3
#define ATTEMPT_RESTORE_UNKNOWN 4

int attempt_restore(void);