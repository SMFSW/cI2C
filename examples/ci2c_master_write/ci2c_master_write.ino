/*
	Master i2c write
	Write some bytes to FRAM and compare them
	with what's read afterwards

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
	const uint16_t reg_addr = 0xF000;
	uint8_t str[7] = { 0x55, 20, 30, 40, 50, 60, 0xAA };
	bool match = true;
	uint8_t str_out[7];
	memset(&str_out, blank, sizeof(str));

	I2C_write(&FRAM, reg_addr, &str[0], sizeof(str));	// FRAM, Addr 0, str, read chars for size of str

	Serial.println();

	I2C_read(&FRAM, reg_addr, &str_out[0], sizeof(str));
	for (uint8_t i = 0; i < sizeof(str_out); i++)
	{
		if (str[i] != str_out[i])	{ match = false; }
		Serial.print(str_out[i], HEX); // print hex values
		Serial.print(" ");
	}
	Serial.print("WRITE ");
	Serial.print(match ? "OK" : "NOK");

	while(1);
}