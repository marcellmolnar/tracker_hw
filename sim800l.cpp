#include "sim800l.h"

#include "secrets.h"
#include "ssd1306_i2c.h"

#include <algorithm>

void SIM800L::init()
{
    at_send_and_await_response("AT+CPIN?\r", 5000);
    std::string cpinSetCmd = std::string("AT+CPIN=") + SIM_PIN_CODE + "\r";
    at_send_and_await_response(cpinSetCmd.c_str(), 5000);
    at_send_and_await_response("AT+CPIN?\r", 5000);
    return;


    // disable echo mode to save bits, command is stored anyway
    at_send_and_await_response("ATE0\r", 100);

    init_sim_pin();
}

void SIM800L::init_sim_pin()
{
    at_send_and_await_response("AT+CPIN?\r", 5000);
    sleep_ms(1000);

    if (simCardState == SIM_CARD_STATE::INVALID || simCardState == SIM_CARD_STATE::ERROR)
    {
        state = SIM_STATE::ERROR;
        return;
    }

    if (simCardState == SIM_CARD_STATE::READY)
    {
        state = SIM_STATE::READY;
        return;
    }

    if (simCardState == SIM_CARD_STATE::WAITING_FOR_PIN)
    {
        std::string cpinSetCmd = std::string("AT+CPIN=") + SIM_PIN_CODE + "\r";
        at_send_and_await_response(cpinSetCmd.c_str(), 5000);

        at_send_and_await_response("AT+CPIN?\r", 5000);
        if (simCardState == SIM_CARD_STATE::WAITING_FOR_PIN)
        {
            // send again and hope
            at_send_and_await_response(cpinSetCmd.c_str(), 5000);

            at_send_and_await_response("AT+CPIN?\r", 5000);
            if (simCardState == SIM_CARD_STATE::READY)
            {
                state = SIM_STATE::READY;
                return;
            }

            // no chance
            state = SIM_STATE::FLAG;
            return;
            if (simCardState == SIM_CARD_STATE::INVALID || simCardState == SIM_CARD_STATE::ERROR)
            {
                state = SIM_STATE::ERROR;
                return;
            }
        }

        if (simCardState == SIM_CARD_STATE::READY)
        {
            state = SIM_STATE::READY;
            return;
        }
    }
}

void SIM800L::handleStateChange()
{
    if (lastCommandSent.find("AT+CPIN?") != std::string::npos)
    {
        if (response.find("SIM PIN") != std::string::npos)
        {
            simCardState = SIM_CARD_STATE::WAITING_FOR_PIN;
        }
        else if (response.find("READY") != std::string::npos)
        {
            simCardState = SIM_CARD_STATE::READY;
        }
        else if (response.find("ERROR") != std::string::npos)
        {
            simCardState = SIM_CARD_STATE::ERROR;
        }
        else
        {
            simCardState = SIM_CARD_STATE::INVALID;
        }
    }

    const bool isBatteryStatus = lastCommandSent.find("AT+CBC") != std::string::npos;
    if (isBatteryStatus || lastCommandSent.find("AT+CSQ") != std::string::npos)
    {
        response.erase(std::remove_if(response.begin(), response.end(), [](char c) -> bool
            {
                return ('A' < c && c < 'Z') || ('a' < c && c < 'z') || c == '\r' || c == '\n';
            }), response.end());
        if (isBatteryStatus)
            batteryStatus = response;
        else
            connectionStatus = response;
    }
}

void SIM800L::at_send_and_await_response(const char* cmd, size_t timeout)
{
    response.clear();
    uart_puts(SIM800L_UART_ID, cmd);
    lastCommandSent = cmd;
    constexpr size_t sleepms = 50;
    const size_t timeoutCnt = timeout / sleepms;
    for (int i = 0; i < timeoutCnt; i++)
    {
        sleep_ms(sleepms);
        if (!processResponse().empty())
            break;
    }
}

void SIM800L::processChar(char c)
{
    response += c;
}

std::string SIM800L::processResponse()
{
    if (response.back() == '\n' && (response.find("OK\r\n") != std::string::npos
                  || response.find("CME ERROR") != std::string::npos
                  || response.find("CMS ERROR") != std::string::npos))
    {
        while (response.back() == '\r' || response.back() == '\n')
            response.pop_back();
        while (response.front() == '\r' || response.front() == '\n')
            response.erase(0, 1);

        handleStateChange();
        showString(response.c_str());
        return response;
    }
    return "";
}

void SIM800L::info()
{
    /*
    AT+CSQ
    Response
    +CSQ: <rssi>,<ber>
    OK
    If error is related to ME functionality:
    +CME ERROR: <err>
    Execution Command returns received signal strength indication <rssi>
    and channel bit error rate <ber> from the ME. Test Command returns
    values supported by the TA.

    Parameters
    <rssi>
    0 -115 dBm or less
    1 -111 dBm
    2...30 -110... -54 dBm
    31 -52 dBm or greater
    99 not known or not detectable
    <ber> (in percent):
    0...7 As RXQUAL values in the table in GSM 05.08 [20] subclause 7.2.4
    99 Not known or not detectable
    */
    at_send_and_await_response("AT+CSQ\r", 1000);

    //sleep_ms(1000);
    /*
    AT+CBC
    Response
    +CBC: <bcs>, <bcl>,<voltage>
    OK
    If error is related to ME functionality:
    +CME ERROR: <err>

    Parameters
    <bcs> Charge status
    0 ME is not charging
    1 ME is charging
    2 Charging has finished
    <bcl> Battery connection level: 1...100 battery has 1-100 percent of capacity remaining
    <voltage> Battery voltage(mV)
    */
    //at_send_and_await_response("AT+CBC\r", 500);
}

void SIM800L::sleep()
{
    /*
        AT+CSCLK=<n>
        Response
        OK
        ERROR

        Parameter
        <n> 0 Disable slow clock, module will not enter sleep mode.
        1 Enable slow clock, it is controlled by DTR. When DTR is
        high, module can enter sleep mode. When DTR changes to low
        level, module can quit sleep mode.
        2 Enable slow clock automatically. When there is no interrupt
        (on air and hardware such as GPIO interrupt or data in serial
        port), module can enter sleep mode. Otherwise, it will quit sleep
        mode. 
    */
}