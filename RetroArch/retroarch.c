#include "RetroArch/retroarch.c"
#include "retroarch.h"

bool my_config_init(void) {
    configuration_settings = (settings_t*)calloc(1, sizeof(settings_t));
    if (!configuration_settings)
        return false;
    return true;
}

void my_config_free(void) {
    free(configuration_settings);
    configuration_settings = NULL;
}
