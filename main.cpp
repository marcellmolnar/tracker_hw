#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "pico/stdlib.h"
#include "pico/sleep.h"

#include "hardware/rtc.h"
#include "hardware/pll.h"
#include "hardware/clocks.h"
#include "hardware/xosc.h"
#include "hardware/rosc.h"
#include "hardware/regs/io_bank0.h"
// For __wfi
#include "hardware/sync.h"
// For scb_hw so we can enable deep sleep
#include "hardware/structs/scb.h"


#include "ssd1306_i2c.h"
#include "mpu6050_i2c.h"
#include "neo6m.h"
#include "sim800l.h"
#include "sleep_control.h"

#define LED_PIN 29

struct circ_bbuf_t {
    const static int maxlen = 240;
    char buffer[maxlen];
    int head;
    int tail;
};
int circ_bbuf_push(circ_bbuf_t *c, char data)
{
    int next;

    next = c->head + 1;  // next is where head will point to after this write.
    if (next >= c->maxlen)
        next = 0;

    if (next == c->tail)  // if the head + 1 == tail, circular buffer is full
        return -1;

    c->buffer[c->head] = data;  // Load data and then move
    c->head = next;             // head to next data offset.
    return 0;  // return success to indicate successful push.
}
int circ_bbuf_pop(circ_bbuf_t *c, char *data)
{
    int next;

    if (c->head == c->tail)  // if the head == tail, we don't have any data
        return -1;

    next = c->tail + 1;  // next is where tail will point to after this read.
    if(next >= c->maxlen)
        next = 0;

    *data = c->buffer[c->tail];  // Read data and then move
    c->tail = next;              // tail to next offset.
    return 0;  // return success to indicate successful push.
}

circ_bbuf_t circ_buff_gps;
uint32_t chrs_gps = 0;
// RX interrupt handler
void on_gps_rx() {
    chrs_gps++;
    while (uart_is_readable(GPS_UART_ID)) {
        char c = uart_getc(GPS_UART_ID);
        circ_bbuf_push(&circ_buff_gps, c);
    }
}

circ_bbuf_t circ_buff_sim800;
uint32_t chrs_sim800 = 0;
// RX interrupt handler
void on_sim800_rx() {
    chrs_sim800++;
    while (uart_is_readable(SIM800L_UART_ID)) {
        char c = uart_getc(SIM800L_UART_ID);
        circ_bbuf_push(&circ_buff_sim800, c);
    }
}

void uart_init(uart_inst_t* uart_id, uint32_t uart_tx_pin, uint32_t uart_rx_pin, uint32_t baud_rate, uint32_t data_bits, uint32_t stop_bits, uart_parity_t parity, void(*irq_handler)(void))
{
     // Set up our UART with a basic baud rate.
    uart_init(uart_id, 2400);

    // Set the TX and RX pins by using the function select on the GPIO
    // Set datasheet for more information on function select
    gpio_set_function(uart_tx_pin, GPIO_FUNC_UART);
    gpio_set_function(uart_rx_pin, GPIO_FUNC_UART);

    // Actually, we want a different speed
    // The call will return the actual baud rate selected, which will be as close as
    // possible to that requested
    int __unused actual = uart_set_baudrate(uart_id, baud_rate);

    // Set UART flow control CTS/RTS, we don't want these, so turn them off
    uart_set_hw_flow(uart_id, false, false);

    // Set our data format
    uart_set_format(uart_id, data_bits, stop_bits, parity);

    // Turn off FIFO's - we want to do this character by character
    uart_set_fifo_enabled(uart_id, false);

    // Set up a RX interrupt
    // We need to set up the handler first
    // Select correct interrupt for the UART we are using
    int UART_IRQ = uart_id == uart0 ? UART0_IRQ : UART1_IRQ;

    // And set up and enable the interrupt handlers
    irq_set_exclusive_handler(UART_IRQ, irq_handler);
    irq_set_enabled(UART_IRQ, true);

    // Now enable the UART to send interrupts - RX only
    uart_set_irq_enables(uart_id, true, false);
}

void init() {
    stdio_init_all();
    xosc_init();

    rpi_sleep_init();

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    uart_init(GPS_UART_ID, GPS_UART_TX_PIN, GPS_UART_RX_PIN, GPS_BAUD_RATE, GPS_DATA_BITS, GPS_STOP_BITS, GPS_PARITY, &on_gps_rx);
    uart_init(SIM800L_UART_ID, SIM800L_UART_TX_PIN, SIM800L_UART_RX_PIN, SIM800L_BAUD_RATE, SIM800L_DATA_BITS, SIM800L_STOP_BITS, SIM800L_PARITY, &on_sim800_rx);

    SSD1306_Init();
    //mpu6050_init();

    sim800l_init();
}

void main_loop_all()
{
    char text[240];
    sprintf(text, "A long time ago\n"
    "  on an OLED \n"
    "   display\n"
    " far far away");

    GPSPlus gps;
    char c;
    uint16_t ledCntr = 0;

    float acceleration[3], gyro[3], temp;
    acceleration[0] = 1; acceleration[1] = 2; acceleration[2] = 3;
    gyro[0] = 4; gyro[1] = 5; gyro[2] = 6; temp = 40;

    double lat = 0, lng = 0; bool locValid = false;
    uint16_t year = 2000; uint8_t month = 1, day = 1; bool dateValid = false;
    uint8_t hour = 0, min = 0, sec = 0; bool timeValid = false;
    int32_t sats = -1;

    while (true) {
        while(0 == circ_bbuf_pop(&circ_buff_gps, &c))
        {
            if (gps.encode(c))
            {
                if (gps.location.isValid())
                {
                    locValid = true;
                    lat = gps.location.lat();
                    lng = gps.location.lng();
                }
                if (gps.date.isValid())
                {
                    dateValid = true;
                    year = gps.date.year();
                    month = gps.date.month();
                    day = gps.date.day();
                }
                if (gps.time.isValid())
                {
                    timeValid = true;
                    hour = gps.time.hour();
                    min = gps.time.minute();
                    sec = gps.time.second();
                }
                if (gps.satellites.isValid())
                {
                    sats = gps.satellites.value();
                }
            }
        }
        sprintf(text, "l %5.2f %5.2f %d\nd %4u %2u %2u %d\nt %2u %2u %2u %d\nS %d", 
        lat, lng, locValid, year, month, day, dateValid, hour, min, sec, timeValid, sats);

        //mpu6050_read_raw(acceleration, gyro, &temp);
        /*sprintf(text, "a %+4.2f %+4.2f %+4.2f\ng %+4.2f %+4.2f %+4.2f\nT %4.1f\n ", 
        acceleration[0], acceleration[1], acceleration[2], gyro[0], gyro[1], gyro[2], temp);*/

        showString(text);

        ledCntr++;
        if (ledCntr == 5)
        {
            gpio_put(LED_PIN, 1);
            ledCntr = 0;
        }
        sleep_ms(200);
        gpio_put(LED_PIN, 0);
    }
}

static void sleep_callback(void)
{
}

void main_loop_sleep()
{
    bool pinState = false;
    while (true)
    {
        rpi_sleep(&sleep_callback);

        pinState = !pinState;
        gpio_put(LED_PIN, pinState);
    }
}

void main_loop_sim800()
{
    char resp[256];
    int respSize = 0;
    bool newResp = false;
    int lastNewLine = 0;
    char cc;
    bool pinState = false;

    //uart_puts(SIM800L_UART_ID, "AT CPIN=7859\r");
    sleep_ms(200);

    uart_puts(SIM800L_UART_ID, "AT&F0\r");
    sleep_ms(400);
    //sleep_ms(4000);
    //uart_puts(SIM800L_UART_ID, "AT+CMINS=1\r");

    int pingCntr = 0;
    while (true)
    {
        while(0 == circ_bbuf_pop(&circ_buff_sim800, &cc))
        {
            resp[respSize] = cc;
            if (respSize < 255)
                respSize++;
            if (respSize == 255)
            {
                resp[respSize] = '\0';
                newResp = true;
            }
            if (cc=='\n' && respSize > 4 && resp[respSize-2] == '\r')
            {
                newResp = true;
                resp[respSize-2] = '\0';
                break;
            }
        }
        if (newResp)
        {
            int offset = 0;
            for (int i = 0; i < 2; i++)
            if (resp[i] == '\r' || resp[i] == '\n')
                offset++;
            showString(&resp[offset]);
            respSize = 0;
            newResp = false;
        }

        pingCntr++;
        if (pingCntr == 3)
        {
            uart_puts(SIM800L_UART_ID, "AT+CPIN?\r");
            pingCntr = 0;
        }

        pinState = !pinState;
        gpio_put(LED_PIN, pinState);
        sleep_ms(500);
    }
}

int main() {

    init();
    gpio_put(LED_PIN, 0);

    //main_loop_sleep();
    //main_loop_all();
    main_loop_sim800();

    return 0;
}
