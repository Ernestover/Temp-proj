/*
 *   Ernest Stover
 *   I2C protocol for 20x4 LCD display with PCF8574 backpack
 *   I2C address -> 0x27
 */

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

/*
* Hardware Connections:
* -------------------------------------------------------------------------
* Pico Pin      |  Component      |  Pin Type  |  Description
* -------------------------------------------------------------------------
* VBUS (40)     |  LCD VCC        |  5V Power  |  Power for LCD Backlight
* 3V3 (36)      |  DHT11 VCC      |  3.3V Out  |  Power for Sensor
* GND (38)      |  GND Rail       |  Ground    |  Common Ground
* GPIO 22 (29)  |  DHT11 Data     |  I/O       |  One-Wire Data (S-Pin)
* GPIO 26 (31)  |  LCD I2C SDA    |  I2C1 SDA  |  Data line for LCD
* GPIO 27 (32)  |  LCD I2C SCL    |  I2C1 SCL  |  Clock line for LCD
* -------------------------------------------------------------------------
*
* DHT11 Module (3-pin):           LCD I2C Backpack:
* [ S ] -> GPIO 22                [ GND ] -> GND
* [ + ] -> 3.3V                   [ VCC ] -> 5V (VBUS)
* [ - ] -> GND                    [ SDA ] -> GPIO 26
*                                 [ SCL ] -> GPIO 27
*/

// I2C port and pins
#define I2C_PORT i2c1
#define I2C_SDA  26
#define I2C_SCL  27

// PCF8574 bit mapping (most common 0x27 backpack):
// P7 → D7, P6 → D6, P5 → D5, P4 → D4
// P3 → Backlight, P2 → EN, P1 → RW, P0 → RS
#define LCD_BACKLIGHT 0x08
#define LCD_ENABLE    0x04
#define LCD_RW        0x02
#define LCD_RS        0x01

// DHT11 pins 
#define DHT_PIN 22

// LCD I2C address 
static const uint8_t LCD_ADDR = 0x27;

// Time 
const uint MAX_TIMINGS = 85;

// Readings from sensor 
typedef struct {
    float humidity;
    float temp_celsius;
} dht_reading;

// I2C Function prototypes
void i2c_init_pico(void);
void i2c_send_byte(uint8_t data);
void lcd_send_nibble(uint8_t nibble, uint8_t mode);
void lcd_send_command(uint8_t cmd);
void lcd_send_data(uint8_t data);
void lcd_init(void);
void lcd_printf(const char *fmt, ...);
void lcd_print(const char *text);
void lcd_set_cursor(uint8_t row, uint8_t col);


// DHT11 Functionprototypes 
void read_dht(dht_reading *result);

// MAIN LOOP 
int main() 
{
    // SETUP
    stdio_init_all();
    i2c_init_pico();
    gpio_init(DHT_PIN);
    // test GPIO 22 is readable
    gpio_set_dir(DHT_PIN, GPIO_IN);
    gpio_pull_up(DHT_PIN);
    sleep_ms(100);
    if(gpio_get(DHT_PIN) ==1)
    {
        lcd_set_cursor(3,0);
        lcd_print("GPIO 22 OK       ");
    }
    else
    {
        lcd_set_cursor(3,0);
        lcd_print("GPIO 22 FAIL     ");
    }
    sleep_ms(2000);
    lcd_init();
    sleep_ms(100);

    // Title
    lcd_set_cursor(0,0);
    lcd_print("Temp. Monitor:");

    tight_loop_contents();
    dht_reading reading;

    while(1)
    {
        read_dht(&reading);
        if (reading.humidity >= 0)
        {
            float fahrenheit = (reading.temp_celsius * 9 / 5) + 32;
            
            // Line 1: Clear and Update Humidity 
            lcd_set_cursor(1,0);
            lcd_printf("Humidity = %3d%% ", (int)reading.humidity);

            // Line 2: Clear and Update Temperature
            lcd_set_cursor(2,0);
            lcd_printf("Temp: = %3dC | %3.1fF", (int)reading.temp_celsius, fahrenheit);

            // Line 3: clear the error/status line if a good reading happens
            lcd_set_cursor(3,0);
            lcd_print("Status: Ok       ");
        }
       else 
       {
        lcd_set_cursor(3,0);
        lcd_print("Status: SENSOR ERR  ");
       }
       sleep_ms(2000);
    }
    
    return 0;
}


/*----------------------------------------------- I2C FUNCTIONS -----------------------------------------------*/
/**
 * @brief Initializes the I2C1 peripheral and configures the SDA/SCL pins.
 * Sets the baud rate to 100kHz and enables internal pull-up resistors for 
 * * communication with the LCD backpack.
 */
void i2c_init_pico(void) 
{
    i2c_init(I2C_PORT, 50000); // 100 kHz
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
}

/**
 * @brief Sends a single byte of data over the I2C bus to the LCD address.
 * @param data The 8-bit value to be sent ot the PCF8574 backpack
 */
void i2c_send_byte(uint8_t data) 
{
    uint8_t buf[1] = { data };
    i2c_write_blocking(I2C_PORT, LCD_ADDR, buf, 1, false);
}

/**
 * @brief Sends a 4-bit nibble to the LCD in 4-bit mode.
 * Packages the nibble with backlight and control bits, then toggles the
 * * Enable (EN) pin to latch the data. 
 * @param nibble The upper 4 bits containing the command or data. 
 * @param mode The register select (RS) bit (0 for command, 1 for data).
 */
void lcd_send_nibble(uint8_t nibble, uint8_t mode) 
{
    uint8_t data = nibble | LCD_BACKLIGHT | mode;

    // EN high
    i2c_send_byte(data | LCD_ENABLE);
    sleep_us(1);
    // EN low
    i2c_send_byte(data & ~LCD_ENABLE);
    sleep_us(50);
}

/**
 * @brief Sends a full 8-bit command to the LCD controller.
 * @param cmd The command byte (e.g., clear display, set cursor).
 */
void lcd_send_command(uint8_t cmd) 
{
    lcd_send_nibble(cmd & 0xF0, 0);            // high nibble, RS=0
    lcd_send_nibble((cmd << 4) & 0xF0, 0);     // low nibble, RS=0
}

/**
 * @brief Sends a full 8-bit data byte (character) to be displayed to the LCD.
 * @param data the ASCII character to display. 
 */
void lcd_send_data(uint8_t data) 
{
    lcd_send_nibble(data & 0xF0, LCD_RS);        // high nibble, RS=1
    lcd_send_nibble((data << 4) & 0xF0, LCD_RS); // low nibble, RS=1
}
/**
 * @brief Executes the initialization sequence for 20x4 LCD.
 * Sets the display to 4-bit mode, enables the display, clears existing 
 * * content, and sets the entry mode. 
 */
void lcd_init(void) 
{
    sleep_ms(50); // wait for LCD power up

    // 4-bit mode init sequence
    lcd_send_nibble(0x30, 0);
    sleep_ms(5);
    lcd_send_nibble(0x30, 0);
    sleep_us(150);
    lcd_send_nibble(0x20, 0); // set to 4-bit mode

    // function set: 4-bit, 2-line, 5x8 dots
    lcd_send_command(0x28);
    // display on, cursor off, blink off
    lcd_send_command(0x0C);
    // clear display
    lcd_send_command(0x01);
    sleep_ms(2);
    // entry mode set: increment, no shift
    lcd_send_command(0x06);
}

/**
 * @brief Displays a string of text on the LCD at the current cursor position.
 * @param text Pointer to a null_terminated string.
 */
void lcd_print(const char *text) 
{
    while (*text) 
    {
        lcd_send_data(*text++);
    }
}

/**
 * @brief Formats and prints a string to the LCD, similar to standard printf.
 * @param fmt Format string followed by variable arguments.
 */
void lcd_printf(const char *fmt, ...) 
{
    char buffer[64];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    lcd_print(buffer);
}

/**
 * @brief Moves the LCD cursor to a specific row and column.
 * @param row The target row (0 to 3).
 * @param col The target column (0 to 19).
 */
void lcd_set_cursor(uint8_t row, uint8_t col) 
{
    uint8_t row_offsets[] = {0x00, 0x40, 0x14, 0x54};
    if (row > 3) row = 3;
    lcd_send_command(0x80 | (col + row_offsets[row]));
}


/**
 * @brief Reads temperature and humidity data from the DHT11 sensor.
 * Manages the high-precision "One-Wire" timing protocol by sending a 
 * * start signal and timing the response pulses from the sensor.
 * Includes checksum verification to ensure data integrity.
 * @param result Pointer to a dht_reading struct where the parsed values 
 * * will be stored. 
 */
void read_dht(dht_reading *result) 
{
    int data[5] = {0, 0, 0, 0, 0};
    uint j = 0;

    // 1. Start signal
    gpio_set_dir(DHT_PIN, GPIO_OUT);
    gpio_put(DHT_PIN, 0);
    sleep_ms(20);           // DHT11 needs at least 18ms
    gpio_put(DHT_PIN, 1);
    sleep_us(30);

    // 2. Prepare to Read
    gpio_set_dir(DHT_PIN, GPIO_IN);
    gpio_pull_up(DHT_PIN);          // Ensure line stays high

    uint last_state = 1;
    for (uint i = 0; i < MAX_TIMINGS; i++) {
        uint count = 0;
        // wait for state change
        while (gpio_get(DHT_PIN) == last_state) {
            count++;
            sleep_us(1);
            if (count == 255) break;
        }

        last_state = gpio_get(DHT_PIN);
        if (count == 255) break;

        // Ignore first 3 transitions (Sensor response pulses)
        if ((i >= 4) && (i % 2 == 0)) {
            data[j / 8] <<=1;
            // dht11: 26-28us is '0', 7-US IS '1'.
            // If count (roughly us) is > 40, it's a 1.
            if (count > 40) {
                data[j / 8] |= 1;
            }
            j++;
        }
    }
    
    // 3. Verify Checksum
    if ((j >= 40) && (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF))) {
        result -> humidity = (float)data[0];
        result -> temp_celsius = (float)data[2];
    }
    else 
    {
        result -> humidity = -1;
        result -> temp_celsius = -100;
    }

}