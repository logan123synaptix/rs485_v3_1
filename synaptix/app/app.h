#ifndef APP_H
#define APP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cmsis_os2.h"
#include "app_config.h"

void app_init(void);

void app_task(void *arg);

#ifdef __cplusplus
}
#endif

#endif