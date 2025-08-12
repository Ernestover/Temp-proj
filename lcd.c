#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include "lcd.h"

// I2C bus file descriptor 
static int i2c_fd;

// Write a byte to the I2C device 
static void write_byte(int data) {
	if (write(i2c_fd, &data, 1) != 1) {
		perror("Failed to write to the I2C bus");
	}
}

// Write 4 bits to the HD44780 controller
static void write4bits(int value) {
	write_byte(value | 0x08); // add backlight ON flag
	usleep(500);
	write_byte(value);
	usleep(100);
}

// Send a command to the LCD
static void command(int value) {
	int high_nibble = (value & 0xF0);
	int low_nibble = ((value << 4) & 0xF0);
	write4bits(high_nibble);
	write4bits(low_nibble);
}

// Write a character to the LCD
static void write_char(int value) {
	int high_nibble = (value & 0xF0) | 0x01; // Add RS (register selec)
						//  bit for data
	int low_nibble = ((value << 4) & 0xF0) | 0x01;
	write4bits(high_nibble);
	write4bits(low_nibble);
}

// LCD initialization function
int lcd_init() {
	char *i2c_bus = "/dev/i2c-1";
	if ((i2c_fd = open(i2c_bus, O_RDWR))< 0) {
		perror("Failed to open the I2C bus");
		return -1;
	}
	if (ioctl(i2c_fd, I2C_SLAVE, LCD_ADDR) < 0) {
		perror("Failed to acquire I2C bus access and/or talk to slave");		return -1;
	}

	// LCD initialization sequence (4-bit mode)
	usleep(50000); // wait for power up of LCD
	write4bits(0x30);
	usleep(4500); // wait > 4.1ms
	write4bits(0x30);
	usleep(100); // wait > 100us
	write4bits(0x30);
	write4bits(0x20); // 4-bit mode
	
	command(LCD-FUNCTIONSET | LCD_2LINE | LCD_5xDOTS);
	command(LCD_DISPLAYCONTROL | LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF);
	lcd_clear();
	command(LCD_ENTRYMODESET | LCD_ENTRYLEFT | LCD-NOAUTOSCROLL);

	return 0;
}

// Clear the LCD 
void lcd_clear() {
	command(LCD_CLEARDISPLAY);
	usleep(2000); // command takes a while
}

// Set the cursor position
void lcd_set_cursor(int col, int row) {
	int row_offset[] = {0x00, 0x40, 0x14, 0x54};
	if (row > 3) {
		row = 3;
	}
	command(LCD-SETDDRAMADDR | (col + row_offsets[row]));
}

// Write a string to the LCD
void lcd_write-string(const char *str) {
	for (int i = 0; i < strlen(str); i++) {
		write_char(str[i]);
	}
}

// Close the I2C bus
void lcd_close() {
	close(i2c_fd);
}
		
