/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/i2c.h"

#include "mpu6050_i2c.h"

/* Example code to talk to a MPU6050 MEMS accelerometer and gyroscope

   This is taking to simple approach of simply reading registers. It's perfectly
   possible to link up an interrupt line and set things up to read from the
   inbuilt FIFO to make it more useful.

   NOTE: Ensure the device is capable of being driven at 3.3v NOT 5v. The Pico
   GPIO (and therefor I2C) cannot be used at 5v.

   You will need to use a level shifter on the I2C lines if you want to run the
   board at 5v.

   Connections on Raspberry Pi Pico board, other boards may vary.

   GPIO PICO_DEFAULT_I2C_SDA_PIN (On Pico this is GP4 (pin 6)) -> SDA on MPU6050 board
   GPIO PICO_DEFAULT_I2C_SCL_PIN (On Pico this is GP5 (pin 7)) -> SCL on MPU6050 board
   3.3v (pin 36) -> VCC on MPU6050 board
   GND (pin 38)  -> GND on MPU6050 board
*/

i2c_inst_t* i2c_MPU = &i2c0_inst;

// By default these devices  are on bus address 0x68
static int addr = 0x68;

static float GYRO_LSB[4] = {131.0, 65.5, 32.8, 16.4};
static float ACCEL_LSB[4] = {16384.0, 8192.0, 4096.0, 2048.0};

static void mpu6050_reset_normal() {
    // Two byte reset. First byte register, second byte data
    // There are a load more options to set up the device in different ways that could be added here
    uint8_t buf[] = {MPU_PWR_MGMT_1_ADDR, 0x80};
    i2c_write_blocking(i2c_MPU, addr, buf, 2, false);
    sleep_ms(100);

    buf[0] = MPU_PWR_MGMT_1_ADDR;
    buf[1] = 0x00;
    i2c_write_blocking(i2c_MPU, addr, buf, 2, false);

    buf[0] = MPU_GYRO_CONFIG_ADDR; buf[1] = MPU_GYRO_FS_SEL << 3;
    i2c_write_blocking(i2c_MPU, addr, buf, 2, false);
    
    buf[0] = MPU_ACCEL_CONFIG_ADDR; buf[1] = MPU_ACCEL_FS_SEL << 3;
    i2c_write_blocking(i2c_MPU, addr, buf, 2, false);

    buf[0] = MPU_DLPF_CFG_ADDR; buf[1] = 4;
    i2c_write_blocking(i2c_MPU, addr, buf, 2, false);
}
static void mpu6050_reset_motion_detection() {
    // Two byte reset. First byte register, second byte data
    // There are a load more options to set up the device in different ways that could be added here
    uint8_t buf[] = {MPU_PWR_MGMT_1_ADDR, 0x80};
    i2c_write_blocking(i2c_MPU, addr, buf, 2, false);
    sleep_ms(100);

    buf[0] = MPU_PWR_MGMT_1_ADDR;
    buf[1] = 0x00;
    i2c_write_blocking(i2c_MPU, addr, buf, 2, false);

    //Reset all internal signal paths in the MPU-6050 by writing 0x07 to register 0x68;
    buf[0] = MPU_SIGNAL_PATH_RESET;
    buf[1] = 0x07;
    i2c_write_blocking(i2c_MPU, addr, buf, 2, false);
    //write register 0x37 to select how to use the interrupt pin. For an active high, push-pull signal that stays until register (decimal) 58 is read, write 0x20.
    // writeByte( MPU6050_ADDRESS, I2C_SLV0_ADDR, 0x20);
    //Write register 28 (==0x1C) to set the Digital High Pass Filter, bits 3:0. For example set it to 0x01 for 5Hz. (These 3 bits are grey in the data sheet, but they are used! Leaving them 0 means the filter always outputs 0.)
    buf[0] = MPU_ACCEL_CONFIG;
    buf[1] = 0x01;
    i2c_write_blocking(i2c_MPU, addr, buf, 2, false);
    //Write the desired Motion threshold to register 0x1F (For example, write decimal 20).
    buf[0] = MPU_MOT_THR;
    buf[1] = 1;
    i2c_write_blocking(i2c_MPU, addr, buf, 2, false);
    //Set motion detect duration to 1  ms; LSB is 1 ms @ 1 kHz rate
    buf[0] = MPU_MOT_DUR;
    buf[1] = 40;
    i2c_write_blocking(i2c_MPU, addr, buf, 2, false);
    //to register 0x69, write the motion detection decrement and a few other settings (for example write 0x15 to set both free-fall and motion decrements to 1 and accelerometer start-up delay to 5ms total by adding 1ms. )
    buf[0] = MPU_MOT_DETECT_CTRL;
    buf[1] = 0x15;
    i2c_write_blocking(i2c_MPU, addr, buf, 2, false);
    // now INT pin is active low
    buf[0] = 0x37;
    buf[1] = 140;
    i2c_write_blocking(i2c_MPU, addr, buf, 2, false);
    //write register 0x38, bit 6 (0x40), to enable motion detection interrupt.
    buf[0] = MPU_INT_ENABLE;
    buf[1] = 0x40;
    i2c_write_blocking(i2c_MPU, addr, buf, 2, false);
    // 101000 - Cycle & disable TEMP SENSOR
    buf[0] = MPU_PWR_MGMT;
    buf[1] = 8;
    i2c_write_blocking(i2c_MPU, addr, buf, 2, false);
    // Disable Gyros
    buf[0] = 0x6C;
    buf[1] = 7;
    i2c_write_blocking(i2c_MPU, addr, buf, 2, false);
}

void mpu6050_read_raw(float accel[3], float gyro[3], float *temp) {
    // For this particular device, we send the device the register we want to read
    // first, then subsequently read from the device. The register is auto incrementing
    // so we don't need to keep sending the register we want, just the first.

    uint8_t buffer[6];

    // Start reading acceleration registers from register 0x3B for 6 bytes
    uint8_t val = 0x3B;
    i2c_write_blocking(i2c_MPU, addr, &val, 1, true); // true to keep master control of bus
    i2c_read_blocking(i2c_MPU, addr, buffer, 6, false);

    for (int i = 0; i < 3; i++) {
        int16_t acc = (buffer[i * 2] << 8 | buffer[(i * 2) + 1]);
        accel[i] = acc / ACCEL_LSB[MPU_ACCEL_FS_SEL];
    }

    // Now gyro data from reg 0x43 for 6 bytes
    // The register is auto incrementing on each read
    val = 0x43;
    i2c_write_blocking(i2c_MPU, addr, &val, 1, true);
    i2c_read_blocking(i2c_MPU, addr, buffer, 6, false);  // False - finished with bus

    for (int i = 0; i < 3; i++) {
        int16_t gyr = (buffer[i * 2] << 8 | buffer[(i * 2) + 1]);
        gyro[i] = gyr / GYRO_LSB[MPU_GYRO_FS_SEL];
    }

    // Now temperature from reg 0x41 for 2 bytes
    // The register is auto incrementing on each read
    val = 0x41;
    i2c_write_blocking(i2c_MPU, addr, &val, 1, true);
    i2c_read_blocking(i2c_MPU, addr, buffer, 2, false);  // False - finished with bus

    int16_t t = buffer[0] << 8 | buffer[1];
    *temp = (t/ 340.0) + 36.53;
}

void mpu6050_init() {
    // This example will use I2C0 on the default SDA and SCL pins (4, 5 on a Pico)
    i2c_init(i2c_MPU, 400 * 1000);
    gpio_set_function(i2c_MPU_SDA, GPIO_FUNC_I2C);
    gpio_set_function(i2c_MPU_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(i2c_MPU_SDA);
    gpio_pull_up(i2c_MPU_SCL);
    // Make the I2C pins available to picotool
    bi_decl(bi_2pins_with_func(i2c_MPU_SDA, i2c_MPU_SCL, GPIO_FUNC_I2C));

    //mpu6050_reset_normal();
    mpu6050_reset_motion_detection();
}
