#ifndef __sim800l_uart_H__
#define __sim800l_uart_H__

#include "pico/time.h"
#include "hardware/uart.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#define SIM800L_UART_TX_PIN 4
#define SIM800L_UART_RX_PIN 5

#define SIM800L_UART_ID   uart1
#define SIM800L_BAUD_RATE 9600
#define SIM800L_DATA_BITS 8
#define SIM800L_STOP_BITS 1
#define SIM800L_PARITY    UART_PARITY_NONE


#ifdef __cplusplus
extern "C"{
#endif

    void sim800l_init();
    void sim800l_at_send();

#ifdef __cplusplus
}
#endif

#endif