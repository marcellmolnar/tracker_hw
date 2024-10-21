#include "sleep_control.h"

#include "pico/sleep.h"


void rpi_sleep_init()
{
    sleep_run_from_xosc();

    // Start the RTC
    rtc_init();
}


void rpi_sleep(rtc_callback_t callback)
{
    datetime_t t = {
            .year  = 2024,
            .month = 10,
            .day   = 21,
            .dotw  = 1, // 0 is Sunday, so 5 is Friday
            .hour  = 20,
            .min   = 00,
            .sec   = 00
    };
    rtc_set_datetime(&t);

    datetime_t t_alarm = {
            .year  = 2024,
            .month = 10,
            .day   = 21,
            .dotw  = 1, // 0 is Sunday, so 5 is Friday
            .hour  = 20,
            .min   = 00,
            .sec   = 03
    };

    sleep_goto_sleep_until(&t_alarm, callback);
}