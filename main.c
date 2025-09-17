/*
*   Ernest Stover
*   I2C protocol for 20x4 LED display
*   I2C address -> 0x3F (commonly 0x27 or 0x3F)
*/

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

// I2C port
#define I2C_PORT i2c0

// I2C Address
static const uint8_t LCD_ADDR = 0x3F;   // change to 0x27 if needed

// GPIO Pins
static const short I2C_SDA = 26;
static const short I2C_SCL = 27;

/*
*   Backpack pin mapping (PCF8574T, common)
*   P0 -> RS
*   P1 -> RW
*   P2 -> EN
*   P3 -> Backlight
*   P4..P7 -> D4..D7
*/
#define LCD_BACKLIGHT 0x08
#define LCD_ENABLE    0x04
#define LCD_READWRITE 0x02
#define LCD_RS        0x01

// Function declarations
void i2c_send_byte(uint8_t data);
void lcd_send_nibble(uint8_t nibble, uint8_t mode);
void lcd_send_command(uint8_t cmd);
void lcd_send_data(uint8_t data);
void i2c_init_pico();
void lcd_init();
void lcd_print(const char *text);

/// Main loop
int main() {
    stdio_init_all();
    i2c_init_pico();
    lcd_init();

    lcd_send_command(0x80);   // set cursor to line 1, position 0
    lcd_print("Hello, Pico!");

    while (1) {
        tight_loop_contents();
    }
    return 0;
}

void i2c_send_byte(uint8_t data) {
    uint8_t buf[1];
    buf[0] = data | LCD_BACKLIGHT; // keep backlight on
    i2c_write_blocking(I2C_PORT, LCD_ADDR, buf, 1, false);
}

void lcd_send_nibble(uint8_t nibble, uint8_t mode) {
    uint8_t data = nibble & 0xF0;  // upper 4 bits
    if (mode) data |= LCD_RS;      // RS=1 for data
    // RW always 0

    // Pulse EN
    i2c_send_byte(data | LCD_ENABLE);
    sleep_us(1);
    i2c_send_byte(data & ~LCD_ENABLE);
    sleep_us(50);
}

void lcd_send_command(uint8_t cmd) {
    lcd_send_nibble(cmd & 0xF0, 0);
    lcd_send_nibble((cmd << 4) & 0xF0, 0);
    sleep_us(50);
}

void lcd_send_data(uint8_t data) {
    lcd_send_nibble(data & 0xF0, 1);
    lcd_send_nibble((data << 4) & 0xF0, 1);
    sleep_us(50);
}

void i2c_init_pico() {
    i2c_init(I2C_PORT, 100000); // 100 kHz
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
}

void lcd_init() {
    sleep_ms(50); // wait for LCD to power up

    // initialization sequence
    lcd_send_command(0x30);
    sleep_ms(5);
    lcd_send_command(0x30);
    sleep_us(150);
    lcd_send_command(0x30);

    lcd_send_command(0x20); // switch to 4-bit mode

    lcd_send_command(0x28); // function set: 4-bit, 2 line, 5x8 font
    lcd_send_command(0x0C); // display on, cursor off
    lcd_send_command(0x06); // entry mode: increment cursor
    lcd_send_command(0x01); // clear display
    sleep_ms(2);
}

void lcd_print(const char *text) {
    while (*text) {
        lcd_send_data(*text++);
    }
}
