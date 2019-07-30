#include "conf.h"

class Config::Impl {
    cs_wrapper cs;
    friend class Config;
};

void Config::begin_config() {
    impl->cs.begin_cs();
}

void Config::end_config() {
    impl->cs.end_cs();
}

Config::Config() : impl(new Impl()) {}

Config::~Config() { delete impl; }

Config *default_config;
