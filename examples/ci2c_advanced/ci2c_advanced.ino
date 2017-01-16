/*
	Master i2c (advanced)
	Redirecting write & read slave functions in setup (to custom functions following template)
	Read and Write operations are then called using the same functions

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
	I2C_slave_set_rw_func(&FRAM, I2C_rd_advanced, 0);
	I2C_slave_set_rw_func(&FRAM, I2C_wr_advanced, 1);
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


/*! \brief This procedure calls appropriate functions to perform a proper send transaction on I2C bus.
 *!
 *! \param [in, out] slave - pointer to the I2C slave structure to init
 *! \param [in] reg_addr - register address in register map
 *! \param [in] data - pointer to the first byte of a block of data to write
 *! \param [in] nb_bytes - indicates how many bytes of data to write
 *! \return Boolean indicating success/fail of write attempt
 */
static bool I2C_wr_advanced(I2C_SLAVE * slave, uint16_t reg_addr, uint8_t * data, uint16_t nb_bytes)
{
	uint16_t ct_w;

	slave->reg_addr = reg_addr;

	if (I2C_start() == false)									{ return false; }
	if (I2C_sndAddr(slave->cfg.addr << 1) == false)				{ return false; }
	if (slave->cfg.reg_size)
	{
		if (slave->cfg.reg_size >= I2C_16B_REG)	// if size >2, 16bit address is used
		{
			if (I2C_snd8((uint8_t) (reg_addr >> 8)) == false)	{ return false; }
		}
		if (I2C_snd8((uint8_t) reg_addr) == false)				{ return false; }
	}

	for (ct_w = 0; ct_w < nb_bytes; ct_w++)
	{
		if (I2C_snd8(*(data++)) == false)						{ return false; }
		slave->reg_addr++;
	}

	if (I2C_stop() == false)									{ return false; }

	return true;
}


/*! \brief This procedure calls appropriate functions to perform a proper receive transaction on I2C bus.
 *!
 *! \param [in, out] slave - pointer to the I2C slave structure to init
 *! \param [in] reg_addr - register address in register map
 *! \param [in, out] data - pointer to the first byte of a block of data to read
 *! \param [in] nb_bytes - indicates how many bytes of data to read
 *! \return Boolean indicating success/fail of read attempt
 */
static bool I2C_rd_advanced(I2C_SLAVE * slave, uint16_t reg_addr, uint8_t * data, uint16_t nb_bytes)
{
	uint16_t ct_r;

	slave->reg_addr = reg_addr;

	if (nb_bytes == 0)	{ nb_bytes++; }

	if (I2C_start() == false)										{ return false; }
	if (I2C_sndAddr(slave->cfg.addr << 1) == false)					{ return false; }
	if (slave->cfg.reg_size)
	{
		if (slave->cfg.reg_size >= I2C_16B_REG)	// if size >2, 16bit address is used
		{
			if (I2C_snd8((uint8_t) (reg_addr >> 8)) == false)		{ return false; }
		}
		if (I2C_snd8((uint8_t) reg_addr) == false)					{ return false; }
		if (I2C_start() == false)									{ return false; }
		if (I2C_sndAddr((slave->cfg.addr << 1) | 0x01) == false)	{ return false; }
	}

	for (ct_r = 0; ct_r < nb_bytes; ct_r++)
	{
		if (ct_r == (nb_bytes - 1))
		{
			if (I2C_rcv8(false) == false)							{ return false; }
		}
		else
		{
			if (I2C_rcv8(true) == false)							{ return false; }
		}
		*data++ = TWDR;
		slave->reg_addr++;
	}

	if (I2C_stop() == false)										{ return false; }

	return true;
}

