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

circ_bbuf_t circ_buff;
uint32_t chrs = 0;
// RX interrupt handler
void on_uart_rx() {
        chrs++;
    while (uart_is_readable(GPS_UART_ID)) {
        char c = uart_getc(GPS_UART_ID);
        circ_bbuf_push(&circ_buff, c);
    }
}

void uart_gps_init()
{
     // Set up our UART with a basic baud rate.
    uart_init(GPS_UART_ID, 2400);

    // Set the TX and RX pins by using the function select on the GPIO
    // Set datasheet for more information on function select
    gpio_set_function(GPS_UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(GPS_UART_RX_PIN, GPIO_FUNC_UART);

    // Actually, we want a different speed
    // The call will return the actual baud rate selected, which will be as close as
    // possible to that requested
    int __unused actual = uart_set_baudrate(GPS_UART_ID, GPS_BAUD_RATE);

    // Set UART flow control CTS/RTS, we don't want these, so turn them off
    uart_set_hw_flow(GPS_UART_ID, false, false);

    // Set our data format
    uart_set_format(GPS_UART_ID, GPS_DATA_BITS, GPS_STOP_BITS, GPS_PARITY);

    // Turn off FIFO's - we want to do this character by character
    uart_set_fifo_enabled(GPS_UART_ID, false);

    // Set up a RX interrupt
    // We need to set up the handler first
    // Select correct interrupt for the UART we are using
    int UART_IRQ = GPS_UART_ID == uart0 ? UART0_IRQ : UART1_IRQ;

    // And set up and enable the interrupt handlers
    irq_set_exclusive_handler(UART_IRQ, on_uart_rx);
    irq_set_enabled(UART_IRQ, true);

    // Now enable the UART to send interrupts - RX only
    uart_set_irq_enables(GPS_UART_ID, true, false);
}

void init() {
    stdio_init_all();
    xosc_init();

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    uart_gps_init();

    SSD1306_Init();
    mpu6050_init();
}

int main() {

    init();
    gpio_put(LED_PIN, 0);

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
        while(0 == circ_bbuf_pop(&circ_buff, &c))
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

        mpu6050_read_raw(acceleration, gyro, &temp);
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
