#ifndef __sleep_control_H__
#define __sleep_control_H__

#include "hardware/rtc.h"

#ifdef __cplusplus
extern "C"{
#endif

    void rpi_sleep_init();
    void rpi_sleep(rtc_callback_t callback);

#ifdef __cplusplus
}
#endif

#endif