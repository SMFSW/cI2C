/*!\file ci2c.c
** \author SMFSW
** \version 0.2
** \copyright MIT SMFSW (2017)
** \brief arduino master i2c in plain c code
**/

// TODO: add interrupt vector / callback for it operations (if not too messy)
// TODO: consider interrupts at least for RX when slave (and TX when master)
// TODO: change contigous r/w operations so it doesn't send internal address again

// TODO: split functions & headers


#include "ci2c.h"

#define START					0x08
#define REPEATED_START			0x10
#define MT_SLA_ACK				0x18
#define MT_SLA_NACK				0x20
#define MT_DATA_ACK				0x28
#define MT_DATA_NACK			0x30
#define MR_SLA_ACK				0x40
#define MR_SLA_NACK				0x48
#define MR_DATA_ACK				0x50
#define MR_DATA_NACK			0x58
#define LOST_ARBTRTN			0x38
#define TWI_STATUS				(TWSR & 0xF8)

//#define isSetBitReg(v, b)		((v & (1 << b)) != 0)
//#define isClrBitReg(v, b)		((v & (1 << b)) == 0)

#define setBitReg(v, b)			v |= (1 << b)
#define clrBitReg(v, b)			v &= (uint8_t) (~(1 << b))
#define invBitReg(v, b)			v ^= (1 << b)

static struct {
	struct {
		I2C_SPEED	speed;			//!< i2c bus speed
		uint8_t		retries;		//!< i2c message retries when fail
		uint16_t	timeout;		//!< i2c timeout (ms)
	} cfg;
	uint16_t		start_wait;
	bool			busy;
} i2c = { {0, DEF_CI2C_NB_RETRIES, DEF_CI2C_TIMEOUT }, 0, false };


// Needed prototypes
static bool I2C_wr(I2C_SLAVE * slave, uint16_t reg_addr, uint8_t * data, uint16_t nb_bytes);
static bool I2C_rd(I2C_SLAVE * slave, uint16_t reg_addr, uint8_t * data, uint16_t nb_bytes);


/*! \brief Init an I2C slave structure for cMI2C communication
 *! \param [in] slave - pointer to the I2C slave structure to init
 *! \param [in] sl_addr - I2C slave address
 *! \param [in] reg_sz - internal register map size
 *! \return nothing
 */
void I2C_slave_init(I2C_SLAVE * slave, uint8_t sl_addr, I2C_INT_SIZE reg_sz)
{
	(void) I2C_slave_set_addr(slave, sl_addr);
	(void) I2C_slave_set_reg_size(slave, reg_sz);
	I2C_slave_set_rw_func(slave, I2C_rd, 0);
	I2C_slave_set_rw_func(slave, I2C_wr, 1);
	slave->reg_addr = 0;
	slave->status = I2C_OK;
}

/*! \brief Redirect slave I2C read/write function (if needed for advanced use)
 *! \param [in] slave - pointer to the I2C slave structure to init
 *! \param [in] func - pointer to read/write function to affect
 *! \param [in] rw - 0 = read function, 1 = write function
 *! \return nothing
 */
void I2C_slave_set_rw_func(I2C_SLAVE * slave, void * func, bool rw)
{
	ci2c_fct_ptr * pfc = (ci2c_fct_ptr*) (rw ? &slave->cfg.wr : &slave->cfg.rd);
	*pfc = func;
}

/*! \brief Change I2C slave address
 *! \param [in, out] slave - pointer to the I2C slave structure to init
 *! \param [in] sl_addr - I2C slave address
 *! \return true if new address set (false if address is >7Fh)
 */
inline bool I2C_slave_set_addr(I2C_SLAVE * slave, uint8_t sl_addr)
{
	if (sl_addr > 0x7F)		{ return false; }
	slave->cfg.addr = sl_addr;
	return true;
}

/*! \brief Change I2C registers map size (for access)
 *! \param [in, out] slave - pointer to the I2C slave structure
 *! \param [in] reg_sz - internal register map size
 *! \return true if new size is correct (false otherwise and set to 16bit by default)
 */
inline bool I2C_slave_set_reg_size(I2C_SLAVE * slave, I2C_INT_SIZE reg_sz)
{
	slave->cfg.reg_size = reg_sz > I2C_16B_REG ? I2C_16B_REG : reg_sz;
	if (reg_sz > I2C_16B_REG)	{ return false; }
	else						{ return true; }
}

/*! \brief Set I2C current register address
 *! \param [in, out] slave - pointer to the I2C slave structure
 *! \param [in] reg_addr - register address
 *! \return nothing
 */
static inline void I2C_slave_set_reg_addr(I2C_SLAVE * slave, uint16_t reg_addr)
{
	slave->reg_addr = reg_addr;
}

/*! \brief Get I2C slave address
 *! \param [in] slave - pointer to the I2C slave structure
 *! \return I2C slave address
 */
inline uint8_t I2C_slave_get_addr(I2C_SLAVE * slave)
{
	return slave->cfg.addr;
}

/*! \brief Get I2C register map size (for access)
 *! \param [in] slave - pointer to the I2C slave structure
 *! \return register map using 16bits if true (1Byte otherwise)
 */
inline bool I2C_slave_get_reg_size(I2C_SLAVE * slave)
{
	return slave->cfg.reg_size;
}

/*! \brief Get I2C current register address (addr may passed this way in procedures if contigous accesses)
 *! \param [in] slave - pointer to the I2C slave structure
 *! \return current register map address
 */
inline uint16_t I2C_slave_get_reg_addr(I2C_SLAVE * slave)
{
	return slave->reg_addr;
}


/*! \brief Enable I2c module on arduino board (including pull-ups,
 *!        enabling of ACK, and setting clock frequency)
 *! \param [in] speed - I2C bus speed in KHz
 *! \return nothing
 */
void I2C_init(uint16_t speed)
{
	// Set SDA and SCL to ports
	#if defined(__AVR_ATmega168__) || defined(__AVR_ATmega8__) || defined(__AVR_ATmega328P__)
		setBitReg(PORTC, 4);
		setBitReg(PORTC, 5);
	#else
		setBitReg(PORTD, 0);
		setBitReg(PORTD, 1);
	#endif

	(void) I2C_set_speed(speed);
}

/*! \brief I2C bus reset (Release SCL and SDA lines and re-enable module)
 *! \return nothing
 */
void I2C_reset(void)
{
	TWCR = 0;
	setBitReg(TWCR, TWEA);
	setBitReg(TWCR, TWEN);
}

/*! \brief Change I2C frequency
 *! \param [in] speed - I2C speed in kHz (max 1MHz)
 *! \return true if change is successful (false otherwise)
 */
bool I2C_set_speed(uint16_t speed)
{
	i2c.cfg.speed = (I2C_SPEED) (speed == 0 ? I2C_SLOW : (speed > I2C_FAST ? I2C_SLOW : speed));

	// Ensure i2c module is disabled
	clrBitReg(TWCR, TWEN);

	// Set prescaler and clock frequency
	clrBitReg(TWSR, TWPS0);
	clrBitReg(TWSR, TWPS1);
	TWBR = ((F_CPU / (i2c.cfg.speed * 1000)) - 16) / 2;		// TODO: check speed and make it a param

	// re-enable module
	I2C_reset();

	return i2c.cfg.speed == speed ? true : false;
}

/*! \brief Change I2C ack timeout
 *! \param [in] timeout - I2C ack timeout (500 ms max)
 *! \return true if change is successful (false otherwise)
 */
inline bool I2C_set_timeout(uint16_t timeout)
{
	i2c.cfg.timeout = timeout > 500 ? 500 : timeout;
	return i2c.cfg.timeout == timeout ? true : false;
}

/*! \brief Change I2C message retries (in case of failure)
 *! \param [in] retries - I2C number of retries (max of 8)
 *! \return true if change is successful (false otherwise)
 */
inline bool I2C_set_retries(uint8_t retries)
{
	i2c.cfg.retries = retries > 8 ? 8 : retries;
	return i2c.cfg.retries == retries ? true : false;
}

/*! \brief Get I2C busy status
 *! \return true if busy
 */
inline bool I2C_is_busy(void)
{
	return i2c.busy;
}


/*! \brief This function reads or writes the provided data to/from the address specified.
 *!        If anything in the write process is not successful, then it will be
 *!        repeated up till 3 more times (default). If still not successful, returns NACK
 *!
 *! \param [in, out] slave - pointer to the I2C slave structure to init
 *! \param [in] reg_addr - register address in register map
 *! \param [in] data - pointer to the first byte of a block of data to write
 *! \param [in] nb_bytes - indicates how many bytes of data to write
 *! \param [in] wr - 0 = read, 1 = write operation
 *! \return I2C_STATUS status of write attempt
 */
static I2C_STATUS I2C_comm(I2C_SLAVE * slave, uint16_t reg_addr, uint8_t * data, uint16_t nb_bytes, bool wr)
{
	uint8_t	retry = i2c.cfg.retries;
	bool	ack = false;
	ci2c_fct_ptr fc = (ci2c_fct_ptr) (wr ? slave->cfg.wr : slave->cfg.rd);

	if (I2C_is_busy())	{ return slave->status = I2C_BUSY; }
	i2c.busy = true;

	ack = fc(slave, reg_addr, data, nb_bytes);
	while ((!ack) && (retry != 0))	// If com not successful, retry some more times
	{
		delay(5);
		ack = fc(slave, reg_addr, data, nb_bytes);
		retry--;
	}

	i2c.busy = false;
	return slave->status = ack ? I2C_OK : I2C_NACK;
}

/*! \brief This function writes the provided data to the address specified.
 *!
 *! \param [in, out] slave - pointer to the I2C slave structure to init
 *! \param [in] reg_addr - register address in register map
 *! \param [in] data - pointer to the first byte of a block of data to write
 *! \param [in] nb_bytes - indicates how many bytes of data to write
 *! \return I2C_STATUS status of write attempt
 */
inline I2C_STATUS I2C_write(I2C_SLAVE * slave, uint16_t reg_addr, uint8_t * data, uint16_t nb_bytes)
{
	return I2C_comm(slave, reg_addr, data, nb_bytes, 1);
}

/*! \brief This inline is a wrapper to I2C_write in case of contigous operations
 *!
 *! \param [in, out] slave - pointer to the I2C slave structure to init
 *! \param [in] data - pointer to the first byte of a block of data to write
 *! \param [in] nb_bytes - indicates how many bytes of data to write
 *! \return I2C_STATUS status of write attempt
 */
inline I2C_STATUS I2C_write_next(I2C_SLAVE * slave, uint8_t * data, uint16_t nb_bytes)
{
	// TODO: implement read next so that it doesn't have to send start register address again
	return I2C_write(slave, slave->reg_addr, data, nb_bytes);
}

/*! \brief This function reads data from the address specified and stores this
 *!        data in the area provided by the pointer.
 *!
 *! \param [in, out] slave - pointer to the I2C slave structure to init
 *! \param [in] reg_addr - register address in register map
 *! \param [in, out] data - pointer to the first byte of a block of data to read
 *! \param [in] nb_bytes - indicates how many bytes of data to read
 *! \return I2C_STATUS status of read attempt
 */
inline I2C_STATUS I2C_read(I2C_SLAVE * slave, uint16_t reg_addr, uint8_t * data, uint16_t nb_bytes)
{
	return I2C_comm(slave, reg_addr, data, nb_bytes, 0);
}

/*! \brief This inline is a wrapper to I2C_read in case of contigous operations
 *!
 *! \param [in, out] slave - pointer to the I2C slave structure to init
 *! \param [in] data - pointer to the first byte of a block of data to read
 *! \param [in] nb_bytes - indicates how many bytes of data to read
 *! \return I2C_STATUS status of read attempt
 */
inline I2C_STATUS I2C_read_next(I2C_SLAVE * slave, uint8_t * data, uint16_t nb_bytes)
{
	// TODO: implement read next so that it doesn't have to send start register address again
	return I2C_read(slave, slave->reg_addr, data, nb_bytes);
}


/*! \brief Start i2c_timeout timer
 *! \return nothing
 */
static inline void I2C_start_timeout(void)
{
	i2c.start_wait = (uint16_t) millis();
}

/*! \brief Test i2c_timeout
 *! \return true if i2c_timeout occured (false otherwise)
 */
static inline uint8_t I2C_timeout(void)
{
	return (((uint16_t) millis() - i2c.start_wait) >= i2c.cfg.timeout);
}

/*! \brief Send start condition
 *! \return true if start condition acknowledged (false otherwise)
 */
bool I2C_start(void)
{
	I2C_start_timeout();

	TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);

	while (!(TWCR & (1 << TWINT)))
	{ if (I2C_timeout())	{ I2C_reset(); return false; } }

	if ((TWI_STATUS == START) || (TWI_STATUS == REPEATED_START))	{ return true; }
	if (TWI_STATUS == LOST_ARBTRTN)									{ I2C_reset(); }

	return false;
}

/*! \brief Send stop condition
 *! \return true if stop condition acknowledged (false otherwise)
 */
bool I2C_stop(void)
{
	I2C_start_timeout();

	TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTO);

	while ((TWCR & (1 << TWSTO)))
	{ if (I2C_timeout())	{ I2C_reset(); return false; } }

	return true;
}

/*! \brief Send I2C address
 *! \param [in] sl_addr - I2C slave address
 *! \return true if I2C slave address sent acknowledged (false otherwise)
 */
bool I2C_sndAddr(uint8_t sl_addr)
{
	TWDR = sl_addr;

	I2C_start_timeout();

	TWCR = (1 << TWINT) | (1 << TWEN);

	while (!(TWCR & (1 << TWINT)))
	{ if (I2C_timeout())	{ I2C_reset(); return false; } }

	if ((TWI_STATUS == MT_SLA_ACK) || (TWI_STATUS == MR_SLA_ACK))	{ return true; }

	if ((TWI_STATUS == MT_SLA_NACK) || (TWI_STATUS == MR_SLA_NACK))	{ I2C_stop(); }
	else															{ I2C_reset(); }

	return false;
}

/*! \brief Send byte on bus
 *! \param [in] dat - data to be sent
 *! \return true if data sent acknowledged (false otherwise)
 */
bool I2C_snd8(uint8_t dat)
{
	TWDR = dat;

	I2C_start_timeout();

	TWCR = (1 << TWINT) | (1 << TWEN);

	while (!(TWCR & (1 << TWINT)))
	{ if (I2C_timeout())	{ I2C_reset(); return false; } }

	if (TWI_STATUS == MT_DATA_ACK)		{ return true; }

	if (TWI_STATUS == MT_DATA_NACK)		{ I2C_stop(); }
	else								{ I2C_reset(); }

	return false;
}

/*! \brief Receive byte from bus
 *! \param [in] ack - true if wait for ack
 *! \return true if data reception acknowledged (false otherwise)
 */
uint8_t I2C_rcv8(bool ack)
{
	I2C_start_timeout();

	if (ack)	{ TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWEA); }
	else		{ TWCR = (1 << TWINT) | (1 << TWEN); }

	while (!(TWCR & (1 << TWINT)))
	{ if (I2C_timeout())	{ I2C_reset(); return false; } }

	if (TWI_STATUS == LOST_ARBTRTN)		{ I2C_reset(); return false; }

	if (((TWI_STATUS == MR_DATA_NACK) && (!ack)) || ((TWI_STATUS == MR_DATA_ACK) && (ack)))		{ return true; }
	else																						{ return false; }
}


/*! \brief This procedure calls appropriate functions to perform a proper send transaction on I2C bus.
 *!
 *! \param [in, out] slave - pointer to the I2C slave structure to init
 *! \param [in] reg_addr - register address in register map
 *! \param [in] data - pointer to the first byte of a block of data to write
 *! \param [in] nb_bytes - indicates how many bytes of data to write
 *! \return Boolean indicating success/fail of write attempt
 */
static bool I2C_wr(I2C_SLAVE * slave, uint16_t reg_addr, uint8_t * data, uint16_t nb_bytes)
{
	uint16_t ct_w;
	(void) I2C_slave_set_reg_addr(slave, reg_addr);

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
static bool I2C_rd(I2C_SLAVE * slave, uint16_t reg_addr, uint8_t * data, uint16_t nb_bytes)
{
	uint16_t ct_r;
	(void) I2C_slave_set_reg_addr(slave, reg_addr);

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

