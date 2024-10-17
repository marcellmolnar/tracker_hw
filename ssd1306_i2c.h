#ifndef __ssd1306_i2c_H__
#define __ssd1306_i2c_H__

#ifndef i2c_OLED_SDA
#define i2c_OLED_SDA 6
#endif

#ifndef i2c_OLED_SCL
#define i2c_OLED_SCL 7
#endif

#ifdef __cplusplus
extern "C"{
#endif

    void SSD1306_Init();

    void showString(const char* text);
    void clearDisplay();

#ifdef __cplusplus
}
#endif

#endif
