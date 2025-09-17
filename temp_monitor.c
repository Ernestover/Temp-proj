#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"




// I2C defines
// This example will use I2C0 on GPIO8 (SDA) and GPIO9 (SCL) running at 400KHz.
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
#define I2C_PORT i2c0
#define I2C_SDA 8
#define I2C_SCL 9



int main()
{
    stdio_init_all();

    // I2C Initialisation. Using it at 400Khz.
    i2c_init(I2C_PORT, 400*1000);
    
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    // For more examples of I2C use see https://github.com/raspberrypi/pico-examples/tree/master/i2c

    sleep_ms(200); // give the LCD time to power up
    lcd_init(I2C_PORT, LCD_ADDRESS, 20, 4); // 20 columns, 4 rows

    lcd_clear();
    lcd_setCursor(0, 0);

    lcd_writeString("Hello World!");
    lcd_setCursor(0, 1);
    lcd_writeString("from my RPi Pico");
    while (1) {
       
        sleep_ms(1000);
    }

    return 0;
}
