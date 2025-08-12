#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "lcd.h"

int main() {
	while(1) {
		lcd_clear();
		lcd_set_cursor(0,0);
		lcd_write_string("Le Potato Test");

		sleep(5);
	}
	lcd_close();
	return 0;
}
