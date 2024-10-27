#include "sim800l.h"


void sim800l_init()
{
    /*
        AT+CPIN=<pin>[,<new pin>]
        Response
        +CPIN: <code>
        OK
        TA stores a password which is necessary before it can be operated (SIM
        PIN, SIM PUK, PH-SIM PIN, etc.).
        If the PIN required is SIM PUK or SIM PUK2, the second pin is required.
        This second pin, <new pin>, is used to replace the old pin in the SIM.
        OK
        If error is related to ME functionality:
        +CME ERROR: <err>
        Parameters
        <pin> String type; password
        <new pin> String type; If the PIN required is SIM PUK or SIMPUK2: new password
        Parameter Saving Mode: NO_SAVE
        Max Response Time: 5s
    */
    //uart_puts(SIM800L_UART_ID, "AT+CPIN=?\r");
}

void sim800l_send()
{

}

void sim800l_info()
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

void sleep()
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