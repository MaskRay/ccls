#include "config.h"

std::unique_ptr<Config> g_config;
thread_local int g_thread_id;
