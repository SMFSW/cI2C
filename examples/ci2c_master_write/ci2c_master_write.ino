/*
	Master i2c write
	Write some bytes to FRAM and compare them
	with what's read afterwards

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
	const uint16_t reg_addr = 0xF000;
	uint8_t str[7] = { 0x55, 20, 30, 40, 50, 60, 0xAA };
	bool match = true;
	uint8_t str_out[7];
	memset(&str_out, 0xEE, sizeof(str));

	I2C_write(&FRAM, reg_addr, &str[0], sizeof(str));	// Addr 0, 2bytes Addr size, str, read chars for size of str

	Serial.println();

	I2C_read(&FRAM, reg_addr, &str_out[0], sizeof(str));
	for (uint8_t i = 0; i < sizeof(str_out); i++)
	{
		if (str[i] != str_out[i])	{ match = false; }
		Serial.print(str_out[i], HEX); // receive a byte as character
		Serial.print(" ");
	}
	Serial.print("WRITE ");
	Serial.print(match ? "OK" : "NOK");

	while(1);
}