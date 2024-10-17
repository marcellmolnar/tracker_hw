#ifndef __mpu6050_i2c_H__
#define __mpu6050_i2c_H__

#define i2c_MPU_SDA 4
#define i2c_MPU_SCL 5

#define MPU_PWR_MGMT_1_ADDR 0x6B

#define MPU_DLPF_CFG_ADDR 0x1A

#define MPU_GYRO_CONFIG_ADDR 0x1B
#define MPU_ACCEL_CONFIG_ADDR 0x1C

#define MPU_GYRO_FS_SEL 0x01
#define MPU_ACCEL_FS_SEL 0x01

#define MPU_SIGNAL_PATH_RESET  0x68
#define MPU_I2C_SLV0_ADDR      0x37
#define MPU_ACCEL_CONFIG       0x1C
#define MPU_MOT_THR            0x1F  // Motion detection threshold bits [7:0]
#define MPU_MOT_DUR            0x20  // This seems wrong // Duration counter threshold for motion interrupt generation, 1 kHz rate, LSB = 1 ms
#define MPU_MOT_DETECT_CTRL    0x69
#define MPU_INT_ENABLE         0x38
#define MPU_PWR_MGMT           0x6B //SLEEPY TIME
#define MPU_INT_STATUS 0x3A

#ifdef __cplusplus
extern "C"{
#endif

    void mpu6050_init();
    void mpu6050_read_raw(float accel[3], float gyro[3], float* temp);

#ifdef __cplusplus
}
#endif

#endif