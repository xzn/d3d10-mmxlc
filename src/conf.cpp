#include "conf.h"

Config::Config() {
    InitializeCriticalSection(&cs);
}

Config::~Config() {
    DeleteCriticalSection(&cs);
}

Config *default_config;
