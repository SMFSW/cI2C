/*
	Master i2c read
	Read some bytes in FRAM

	This example code is in the public domain.

	created Jan 12 2017
	latest mod Jan 31 2017
	by SMFSW
*/

#include <ci2c.h>

const uint8_t blank = 0xEE;		// blank tab filling value for test

I2C_SLAVE FRAM;					// slave declaration

void setup() {
	Serial.begin(115200);	// start serial for output
	I2C_init(I2C_FM);		// init with Fast Mode (400KHz)
	I2C_slave_init(&FRAM, 0x50, I2C_16B_REG);
}

void loop() {
	const uint16_t reg_addr = 0;
	uint8_t str[8];
	const uint8_t half_sz = sizeof(str) / 2;
	memset(&str, blank, sizeof(str));
	
	I2C_read(&FRAM, reg_addr, &str[0], half_sz);	// FRAM, Addr 0, str, read chars for size of half str
	I2C_read_next(&FRAM, &str[half_sz], half_sz);

	Serial.println();
	for (uint8_t i = 0; i < sizeof(str); i++)
	{
		Serial.print(str[i], HEX); // print hex values
		Serial.print(" ");
	}

	delay(5000);
}