/*
*   Ernest Stover
*   I2C protocol for 20x4 LED display 
*   I2C address -> 0x3F
*/

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

// I2C port 
#define I2C_PORT i2c0

// I2C Address
static const uint8_t LCD_ADDR = 0x3F;

// GPIO Pins
static const short I2C_SDA = 8;
static const short I2C_SCL = 9;  

/*
*   my layout for my 20x4 lcd screen
*
*   GPIO 8 (pin 11) -> SDA
*   GPIO 9 (pin 12) -> SCL
*   VBUS (pin 40) -> 5v 
*   GND (pin 38) -> GND 
*/

/// COMMANDS
void i2c_send_byte(uint8_t data);   // send a single byte over I2C
void lcd_send_command(uint8_t cmd); // send a command to the LCD
void lcd_send_data(unsigned char data); // send char to the LCD
void i2c_init_pico();               // initialize I2C on the Pico
void lcd_print(const char *text);   // output text to LCD screen

/// @brief  Main loop 
/// @return 
int main() {
    stdio_init_all();
    i2c_init_pico();

    // LCD initialization sequence
    lcd_send_command(0x02); // Return home
    sleep_ms(2);
    lcd_send_command(0x28); 
    sleep_us(50);
    lcd_send_command(0x0C);
    sleep_us(50);
    lcd_send_command(0x06); 
    sleep_us(50);
    lcd_send_command(0x01);
    sleep_ms(2);

    lcd_print("Hello, Pico!");

    while (1) {
        tight_loop_contents();
    }
    return 0;
}

void i2c_send_byte(uint8_t data)
{
    uint8_t buf[1];
    buf[0] = data;
    i2c_write_blocking(I2C_PORT, LCD_ADDR, buf, 1, false);
}

void lcd_send_command(uint8_t cmd)
{
    uint8_t data_nibble;

    // Send high nibble
    data_nibble = (cmd & 0xF0);
    i2c_send_byte(data_nibble | 0x0C); // En=1, RS=0, D4-D7
    i2c_send_byte(data_nibble | 0x08); // En=0, RS=0

    // Send low nibble
    data_nibble = ((cmd << 4) & 0xF0);
    i2c_send_byte(data_nibble | 0x0C); // En=1, RS=0, D4-D7
    i2c_send_byte(data_nibble | 0x08); // En=0, RS=0

    sleep_us(100); // Wait for execution
}

void lcd_send_data(unsigned char data) 
{
    uint8_t data_nibble;

    // send high nibble
    data_nibble = (data & 0xF0);
    i2c_send_byte(data_nibble | 0x0D); // RS=1, En=1
    i2c_send_byte(data_nibble | 0x09); // RS=1, En=0

    // send low nibble 
    data_nibble = ((data << 4) & 0xF0);
    i2c_send_byte(data_nibble | 0x0D); // RS=1, En=1
    i2c_send_byte(data_nibble | 0x09); // RS=1, En=0

    sleep_us(50);
}

void i2c_init_pico()
{
    i2c_init(I2C_PORT, 100000); // Initialize I2C0 at 100 kHz
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
}

void lcd_print(const char *text)
{
    while (*text)
    {
        lcd_send_data(*text);
        text++;
    }
}