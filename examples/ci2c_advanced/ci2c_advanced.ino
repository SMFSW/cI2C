/*
	Master i2c (advanced)
	Redirecting slave write & read functions in setup (to custom functions following typedef)
	Read and Write operations are then called using the same functions
	Function to get Chip ID are device dependant (and will probably only work on FUJITSU devices)

	This example code is in the public domain.

	created Jan 12 2017
	latest mod Jan 31 2017
	by SMFSW
*/

#include <ci2c.h>

const uint8_t blank = 0xEE;		// blank tab filling value for test

I2C_SLAVE FRAM;					// slave declaration

void setup() {
	uint8_t str[3];
	memset(&str, blank, sizeof(str));

	Serial.begin(115200);	// start serial for output
	I2C_init(I2C_FM);		// init with Fast Mode (400KHz)
	I2C_slave_init(&FRAM, 0x50, I2C_16B_REG);
	I2C_slave_set_rw_func(&FRAM, I2C_wr_advanced, I2C_WRITE);
	I2C_slave_set_rw_func(&FRAM, I2C_rd_advanced, I2C_READ);

	I2C_get_chip_id(&FRAM, &str[0]);

	Serial.println();
	//for (uint8_t i = 0; i < sizeof(str); i++)	{ Serial.print(str[i], HEX); } // print hex values
	Serial.print("\nManufacturer ID: ");
	Serial.print((str[0] << 4) + (str[1]  >> 4), HEX);
	Serial.print("\nProduct ID: ");
	Serial.print(((str[1] & 0x0F) << 8) + str[2], HEX);
}

void loop() {
	const uint16_t reg_addr = 0;
	uint8_t str[7];
	memset(&str, blank, sizeof(str));

	I2C_read(&FRAM, reg_addr, &str[0], sizeof(str));	// FRAM, Addr 0, str, read chars for size of str

	Serial.println();
	for (uint8_t i = 0; i < sizeof(str); i++)
	{
		Serial.print(str[i], HEX); // print hex values
		Serial.print(" ");
	}

	delay(5000);
}


/*! \brief This procedure calls appropriate functions to perform a proper send transaction on I2C bus.
 *  \param [in, out] slave - pointer to the I2C slave structure
 *  \param [in] reg_addr - register address in register map
 *  \param [in] data - pointer to the first byte of a block of data to write
 *  \param [in] bytes - indicates how many bytes of data to write
 *  \return Boolean indicating success/fail of write attempt
 */
bool I2C_wr_advanced(I2C_SLAVE * slave, uint16_t reg_addr, uint8_t * data, uint16_t bytes)
{
	slave->reg_addr = reg_addr;

	if (I2C_start() == false)									{ return false; }
	if (I2C_sndAddr(slave, I2C_WRITE) == false)					{ return false; }
	if (slave->cfg.reg_size)
	{
		if (slave->cfg.reg_size >= I2C_16B_REG)	// if size >2, 16bit address is used
		{
			if (I2C_wr8((uint8_t) (reg_addr >> 8)) == false)	{ return false; }
		}
		if (I2C_wr8((uint8_t) reg_addr) == false)				{ return false; }
	}

	for (uint16_t cnt = 0; cnt < bytes; cnt++)
	{
		if (I2C_wr8(*(data++)) == false)						{ return false; }
		slave->reg_addr++;
	}

	if (I2C_stop() == false)									{ return false; }

	return true;
}


/*! \brief This procedure calls appropriate functions to perform a proper receive transaction on I2C bus.
 *  \param [in, out] slave - pointer to the I2C slave structure
 *  \param [in] reg_addr - register address in register map
 *  \param [in, out] data - pointer to the first byte of a block of data to read
 *  \param [in] bytes - indicates how many bytes of data to read
 *  \return Boolean indicating success/fail of read attempt
 */
bool I2C_rd_advanced(I2C_SLAVE * slave, uint16_t reg_addr, uint8_t * data, uint16_t bytes)
{
	slave->reg_addr = reg_addr;

	if (bytes == 0)	{ bytes = 1; }

	if (slave->cfg.reg_size)
	{
		if (I2C_start() == false)									{ return false; }
		if (I2C_sndAddr(slave, I2C_WRITE) == false)					{ return false; }
		if (slave->cfg.reg_size >= I2C_16B_REG)	// if size >2, 16bit address is used
		{
			if (I2C_wr8((uint8_t) (reg_addr >> 8)) == false)		{ return false; }
		}
		if (I2C_wr8((uint8_t) reg_addr) == false)					{ return false; }
	}
	if (I2C_start() == false)										{ return false; }
	if (I2C_sndAddr(slave, I2C_READ) == false)						{ return false; }

	for (uint16_t cnt = 0; cnt < bytes; cnt++)
	{
		if (I2C_rd8((cnt == (bytes - 1)) ? false : true) == false)	{ return false; }
		*data++ = TWDR;
		slave->reg_addr++;
	}

	if (I2C_stop() == false)										{ return false; }

	return true;
}


/*! \brief This procedure calls appropriate functions to get chip ID of FUJITSU devices.
 *  \param [in, out] slave - pointer to the I2C slave structure
 *  \param [in, out] data - pointer to the first byte of a block of data to read
 *  \return Boolean indicating success/fail of read attempt
 */
bool I2C_get_chip_id(I2C_SLAVE * slave, uint8_t * data)
{
	const uint16_t bytes = 3;
	I2C_SLAVE FRAM_ID;

	I2C_slave_init(&FRAM_ID, 0xF8 >> 1, I2C_16B_REG);	// Dummy slave init for I2C_sndAddr

	if (I2C_start() == false)										{ return false; }
	if (I2C_sndAddr(&FRAM_ID, I2C_WRITE) == false)					{ return false; }
	if (I2C_wr8(slave->cfg.addr << 1) == false)						{ return false; }
	if (I2C_start() == false)										{ return false; }
	if (I2C_sndAddr(&FRAM_ID, I2C_READ) == false)					{ return false; }

	for (uint16_t cnt = 0; cnt < bytes; cnt++)
	{
		if (I2C_rd8((cnt == (bytes - 1)) ? false : true) == false)	{ return false; }
		*data++ = TWDR;
	}

	if (I2C_stop() == false)										{ return false; }

	return true;
}
