#ifndef __sim800l_uart_H__
#define __sim800l_uart_H__

#include "pico/time.h"
#include "hardware/uart.h"

#include <cinttypes>
#include <cstdlib>
#include <string>

#define SIM800L_UART_TX_PIN 4
#define SIM800L_UART_RX_PIN 5

#define SIM800L_UART_ID   uart1
#define SIM800L_BAUD_RATE 9600
#define SIM800L_DATA_BITS 8
#define SIM800L_STOP_BITS 1
#define SIM800L_PARITY    UART_PARITY_NONE

enum class SIM_STATE {
    READY,
    ERROR,
    FLAG,
    INVALID
};

class SIM800L {
public:
    SIM800L() {}
    void init();
    void at_send(const char* cmd);

    void processChar(char c);
    std::string processResponse();

    void info();
    void sleep();

private:
    enum class SIM_CARD_STATE {
        WAITING_FOR_PIN,
        READY,
        ERROR,
        INVALID
    };

    void init_sim_pin();

    void handleStateChange();

public:
    volatile SIM_STATE state = SIM_STATE::INVALID;

private:
    std::string lastCommandSent;
    std::string response;

    SIM_CARD_STATE simCardState = SIM_CARD_STATE::INVALID;
};

#endif