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
    lcd_init();

    // Title
    lcd_set_cursor(0,0);
    lcd_print("Temp. Monitor:");

    tight_loop_contents();
    dht_reading reading;
        
   // read_dht(&reading);
    //float fahrenheit = (reading.temp_celsius *9/5) + 32;
    //lcd_set_cursor(1,0);
    //lcd_printf("Humidity = %.1f%% ", reading.humidity);

    //lcd_set_cursor(2,0);
    //lcd_printf("Temperature = %.1fC (%.1fF)", reading.temp_celsius, fahrenheit);
    //sleep_ms(2000);

    while(1)
    {
        read_dht(&reading);
        if (reading.humidity >= 0 && reading.temp_celsius > -50)
        {
            float fahrenheit = (reading.temp_celsius * 9 / 5) + 32;
            lcd_set_cursor(1,0);
            lcd_printf("Humidity = %.1d%% ", (int)reading.humidity);
            lcd_set_cursor(2,0);
            lcd_print("                    "); // clear line 2 
            lcd_set_cursor(2,0);
            lcd_printf("Temperature = %.1dC (%.1fF)", (int)reading.temp_celsius, fahrenheit);
        }
       else 
       {
        lcd_set_cursor(1,0);
        lcd_print("Bad DHT11 reading");
       }
       sleep_ms(2000);
    }
    
    return 0;
}


// I2C Functions
void i2c_init_pico(void) 
{
    i2c_init(I2C_PORT, 100000); // 100 kHz
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
}

void i2c_send_byte(uint8_t data) 
{
    uint8_t buf[1] = { data };
    i2c_write_blocking(I2C_PORT, LCD_ADDR, buf, 1, false);
}

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

void lcd_send_command(uint8_t cmd) 
{
    lcd_send_nibble(cmd & 0xF0, 0);            // high nibble, RS=0
    lcd_send_nibble((cmd << 4) & 0xF0, 0);     // low nibble, RS=0
}

void lcd_send_data(uint8_t data) 
{
    lcd_send_nibble(data & 0xF0, LCD_RS);        // high nibble, RS=1
    lcd_send_nibble((data << 4) & 0xF0, LCD_RS); // low nibble, RS=1
}

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

void lcd_print(const char *text) 
{
    while (*text) 
    {
        lcd_send_data(*text++);
    }
}

void lcd_printf(const char *fmt, ...) 
{
    char buffer[64];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    lcd_print(buffer);
}

void lcd_set_cursor(uint8_t row, uint8_t col) 
{
    uint8_t row_offsets[] = {0x00, 0x40, 0x14, 0x54};
    if (row > 3) row = 3;
    lcd_send_command(0x80 | (col + row_offsets[row]));
}


// Dht11 Function
void read_dht(dht_reading *result) 
{
    int data[5] = {0, 0, 0, 0, 0};
    uint last = 1;
    uint j = 0;

    gpio_set_dir(DHT_PIN, GPIO_OUT);
    gpio_put(DHT_PIN, 0);
    sleep_ms(25);
    gpio_put(DHT_PIN, 1);
    sleep_us(100);
   if (gpio_get(DHT_PIN) == 0)
   {
    lcd_set_cursor(3,0);
    lcd_printf("Got response ");
   }
   else 
   {
    lcd_set_cursor(3,0);
    lcd_printf("No response ");
   }
    gpio_set_dir(DHT_PIN, GPIO_IN);

    for (uint i = 0; i < MAX_TIMINGS; i++) 
    {
        uint count = 0;
        while (gpio_get(DHT_PIN) == last) 
        {
            count++;
            sleep_us(1);
            if (count == 255) break;
           // may need to put it here
        }
    
        last = gpio_get(DHT_PIN);
        if (count == 255) break;

        if ((i >= 4) && (i % 2 == 0)) 
        {
            data[j / 8] <<= 1;
            if (count > 50) 
            {
                data[j / 8] |= 1;
            }
            j++;
        }
    }

   // lcd_set_cursor(3,0);
    //lcd_printf("bits read: %d,", j);
   
    if ((j >= 40) && (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF)))
    {
        // DHT11: values are already whole numbers
        result->humidity     = data[0];   // %RH (0–100)
        result->temp_celsius = data[2];   // °C (0–50 typically)
    }
    else
    {
        // mark as invalid reading
        result->humidity = -1;
        result->temp_celsius = -100;
    }
}