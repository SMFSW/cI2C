/*
	Master i2c read
	Read some bytes in FRAM

	This example code is in the public domain.

	created Jan 12 2017
	latest mod Jan 16 2017
	by SMFSW
*/

#include <ci2c.h>

I2C_SLAVE FRAM;

void setup() {
	Serial.begin(115200);	// start serial for output
	I2C_init(I2C_LOW);		// init with low speed (400KHz)
	I2C_slave_init(&FRAM, 0x50, I2C_16B_REG);
}

void loop() {
	const uint16_t reg_addr = 0;
	uint8_t str[7];
	memset(&str, 0xEE, sizeof(str));
	
	I2C_read(&FRAM, reg_addr, &str[0], sizeof(str));	// Addr 0, 2bytes Addr size, str, read chars for size of str

	Serial.println();
	for (uint8_t i = 0; i < sizeof(str); i++)
	{
		Serial.print(str[i], HEX); // receive a byte as character
		Serial.print(" ");
	}

	delay(5000);
}