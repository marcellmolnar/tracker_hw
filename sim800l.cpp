#include "sim800l.h"

#include "secrets.h"
#include "ssd1306_i2c.h"

void SIM800L::init()
{
    // disable echo mode to save bits, command is stored anyway
    //at_send("ATE0\r");
    //sleep_ms(100);
    //processResponse();

    init_sim_pin();
}

void SIM800L::init_sim_pin()
{
    at_send("AT+CPIN?\r");
    sleep_ms(1000);
    processResponse();

    if (simCardState == SIM_CARD_STATE::INVALID)
    {
        at_send("AT+CPIN?\r");
        // max response time for cpin command
        sleep_ms(5000);
        processResponse();
    }

    if (simCardState == SIM_CARD_STATE::INVALID)
    {
        state = SIM_STATE::FLAG;
        return;
    }
    
    if(simCardState == SIM_CARD_STATE::ERROR)
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
        at_send(cpinSetCmd.c_str());
        sleep_ms(100);
        processResponse();

        if (simCardState == SIM_CARD_STATE::WAITING_FOR_PIN)
        {
            // max response time for cpin command
            sleep_ms(5000);
            processResponse();
            if (simCardState == SIM_CARD_STATE::WAITING_FOR_PIN)
            {
                at_send(cpinSetCmd.c_str());
                sleep_ms(5000);
                processResponse();

                if (simCardState == SIM_CARD_STATE::READY)
                {
                    state = SIM_STATE::READY;
                    return;
                }

                // no chance
                state = SIM_STATE::ERROR;
                return;
            }

            if (simCardState == SIM_CARD_STATE::READY)
            {
                state = SIM_STATE::READY;
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
}

void SIM800L::at_send(const char* cmd)
{
    response.clear();
    uart_puts(SIM800L_UART_ID, cmd);
    lastCommandSent = cmd;
}

void SIM800L::processChar(char c)
{
    response += c;
}

std::string SIM800L::processResponse()
{
    showString(response.c_str());
    if (response.back() == '\n' && (response.find("OK\r\n") != std::string::npos
                  || response.find("CME ERROR") != std::string::npos
                  || response.find("CMS ERROR") != std::string::npos))
    {
        while (response.back() == '\r' || response.back() == '\n')
            response.pop_back();
        while (response.front() == '\r' || response.front() == '\n')
            response.erase(0, 1);

        handleStateChange();
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