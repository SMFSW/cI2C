/*!\file ci2c.c
** \author SMFSW
** \version 1.2
** \copyright MIT SMFSW (2017)
** \brief arduino master i2c in plain c code
** \warning Don't access (r/w) last 16b internal address byte alone right after init, this would lead to hazardous result (in such case, make a dummy read of addr 0 before)
**/

// TODO: add interrupt vector / callback for it operations (if not too messy)
// TODO: consider interrupts at least for RX when slave (and TX when master)

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

//#define isSetRegBit(r, b)		((r & (1 << b)) != 0)
//#define isClrRegBit(r, b)		((r & (1 << b)) == 0)

#define setRegBit(r, b)			r |= (1 << b)				//!< set bit \b b in register \b r
#define clrRegBit(r, b)			r &= (uint8_t) (~(1 << b))	//!< clear bit \b b in register \b r
#define invRegBit(r, b)			r ^= (1 << b)				//!< invert bit \b b in register \b r

/*!\struct i2c
** \brief static ci2c bus config and control parameters
**/
static struct {
	/*!\struct cfg
	** \brief ci2c bus parameters
	**/
	struct {
		I2C_SPEED	speed;			//!< i2c bus speed
		uint8_t		retries;		//!< i2c message retries when fail
		uint16_t	timeout;		//!< i2c timeout (ms)
	} cfg;
	uint16_t		start_wait;		//!< time start waiting for acknowledge
	bool			busy;			//!< true if already busy (in case of interrupts implementation)
} i2c = { { (I2C_SPEED) 0, DEF_CI2C_NB_RETRIES, DEF_CI2C_TIMEOUT }, 0, false };


// Needed prototypes
static bool I2C_wr(I2C_SLAVE * slave, const uint16_t reg_addr, uint8_t * data, const uint16_t bytes);
static bool I2C_rd(I2C_SLAVE * slave, const uint16_t reg_addr, uint8_t * data, const uint16_t bytes);


/*!\brief Init an I2C slave structure for cMI2C communication
** \param [in] slave - pointer to the I2C slave structure to init
** \param [in] sl_addr - I2C slave address
** \param [in] reg_sz - internal register map size
** \return nothing
**/
void I2C_slave_init(I2C_SLAVE * slave, const uint8_t sl_addr, const I2C_INT_SIZE reg_sz)
{
	(void) I2C_slave_set_addr(slave, sl_addr);
	(void) I2C_slave_set_reg_size(slave, reg_sz);
	I2C_slave_set_rw_func(slave, (ci2c_fct_ptr) I2C_wr, I2C_WRITE);
	I2C_slave_set_rw_func(slave, (ci2c_fct_ptr) I2C_rd, I2C_READ);
	slave->reg_addr = (uint16_t) -1;	// To be sure to send address on first access (warning: unless last 16b byte address is accessed alone)
	slave->status = I2C_OK;
}

/*!\brief Redirect slave I2C read/write function (if needed for advanced use)
** \param [in] slave - pointer to the I2C slave structure to init
** \param [in] func - pointer to read/write function to affect
** \param [in] rw - 0 = write function, 1 = read function
** \return nothing
**/
void I2C_slave_set_rw_func(I2C_SLAVE * slave, const ci2c_fct_ptr func, const I2C_RW rw)
{
	ci2c_fct_ptr * pfc = (ci2c_fct_ptr*) (rw ? &slave->cfg.rd : &slave->cfg.wr);
	*pfc = func;
}

/*!\brief Change I2C slave address
** \param [in, out] slave - pointer to the I2C slave structure to init
** \param [in] sl_addr - I2C slave address
** \return true if new address set (false if address is >7Fh)
**/
bool I2C_slave_set_addr(I2C_SLAVE * slave, const uint8_t sl_addr)
{
	if (sl_addr > 0x7F)		{ return false; }
	slave->cfg.addr = sl_addr;
	return true;
}

/*!\brief Change I2C registers map size (for access)
** \param [in, out] slave - pointer to the I2C slave structure
** \param [in] reg_sz - internal register map size
** \return true if new size is correct (false otherwise and set to 16bit by default)
**/
bool I2C_slave_set_reg_size(I2C_SLAVE * slave, const I2C_INT_SIZE reg_sz)
{
	slave->cfg.reg_size = reg_sz > I2C_16B_REG ? I2C_16B_REG : reg_sz;
	return !(reg_sz > I2C_16B_REG);
}

/*!\brief Set I2C current register address
** \attribute inline
** \param [in, out] slave - pointer to the I2C slave structure
** \param [in] reg_addr - register address
** \return nothing
**/
static inline void __attribute__((__always_inline__)) I2C_slave_set_reg_addr(I2C_SLAVE * slave, const uint16_t reg_addr) {
	slave->reg_addr = reg_addr; }



/*!\brief Enable I2c module on arduino board (including pull-ups,
 *        enabling of ACK, and setting clock frequency)
** \param [in] speed - I2C bus speed in KHz
** \return nothing
**/
void I2C_init(const uint16_t speed)
{
	// Set SDA and SCL to ports with pull-ups
	#if defined(__AVR_ATmega168__) || defined(__AVR_ATmega8__) || defined(__AVR_ATmega328P__)
		setRegBit(PORTC, 4);
		setRegBit(PORTC, 5);
	#else
		setRegBit(PORTD, 0);
		setRegBit(PORTD, 1);
	#endif

	(void) I2C_set_speed(speed);
}

/*!\brief Disable I2c module on arduino board (releasing pull-ups, and TWI control)
** \return nothing
**/
void I2C_uninit()
{
	// Release SDA and SCL ports pull-ups
	#if defined(__AVR_ATmega168__) || defined(__AVR_ATmega8__) || defined(__AVR_ATmega328P__)
		clrRegBit(PORTC, 4);
		clrRegBit(PORTC, 5);
	#else
		clrRegBit(PORTD, 0);
		clrRegBit(PORTD, 1);
	#endif

	TWCR = 0;
}


/*!\brief I2C bus reset (Release SCL and SDA lines and re-enable module)
** \return nothing
**/
void I2C_reset(void)
{
	TWCR = 0;
	setRegBit(TWCR, TWEA);
	setRegBit(TWCR, TWEN);
}

/*!\brief Change I2C frequency
** \param [in] speed - I2C speed in KHz (max 400KHz on avr)
** \return Configured bus speed
**/
uint16_t I2C_set_speed(const uint16_t speed)
{
	i2c.cfg.speed = (I2C_SPEED) ((speed == 0) ? (uint16_t) I2C_STD : ((speed > (uint16_t) I2C_FM) ? (uint16_t) I2C_FM : speed));

	clrRegBit(TWCR, TWEN);	// Ensure i2c module is disabled

	// Set prescaler and clock frequency
	clrRegBit(TWSR, TWPS0);
	clrRegBit(TWSR, TWPS1);
	TWBR = (((F_CPU / 1000) / i2c.cfg.speed) - 16) / 2;

	I2C_reset();			// re-enable module

	return i2c.cfg.speed;
}

/*!\brief Change I2C ack timeout
** \param [in] timeout - I2C ack timeout (500 ms max)
** \return Configured timeout
**/
uint16_t I2C_set_timeout(const uint16_t timeout)
{
	static const uint16_t max_timeout = 500;
	i2c.cfg.timeout = (timeout > max_timeout) ? max_timeout : timeout;
	return i2c.cfg.timeout;
}

/*!\brief Change I2C message retries (in case of failure)
** \param [in] retries - I2C number of retries (max of 8)
** \return Configured number of retries
**/
uint8_t I2C_set_retries(const uint8_t retries)
{
	static const uint16_t max_retries = 8;
	i2c.cfg.retries = (retries > max_retries) ? max_retries : retries;
	return i2c.cfg.retries;
}

/*!\brief Get I2C busy status
** \return true if busy
**/
bool I2C_is_busy(void) {
	return i2c.busy; }


/*!\brief This function reads or writes the provided data to/from the address specified.
 *        If anything in the write process is not successful, then it will be repeated
 *        up till 3 more times (default). If still not successful, returns NACK
** \param [in, out] slave - pointer to the I2C slave structure to init
** \param [in] reg_addr - register address in register map
** \param [in] data - pointer to the first byte of a block of data to write
** \param [in] bytes - indicates how many bytes of data to write
** \param [in] rw - 0 = write, 1 = read operation
** \return I2C_STATUS status of write attempt
**/
static I2C_STATUS I2C_comm(I2C_SLAVE * slave, const uint16_t reg_addr, uint8_t * data, const uint16_t bytes, const I2C_RW rw)
{
	uint8_t			retry = i2c.cfg.retries;
	bool			ack = false;
	ci2c_fct_ptr	fc = (ci2c_fct_ptr) (rw ? slave->cfg.rd : slave->cfg.wr);

	if (I2C_is_busy())	{ return slave->status = I2C_BUSY; }
	i2c.busy = true;

	ack = fc(slave, reg_addr, data, bytes);
	while ((!ack) && (retry != 0))	// If com not successful, retry some more times
	{
		delay(5);
		ack = fc(slave, reg_addr, data, bytes);
		retry--;
	}

	i2c.busy = false;
	return slave->status = ack ? I2C_OK : I2C_NACK;
}

/*!\brief This function writes the provided data to the address specified.
** \param [in, out] slave - pointer to the I2C slave structure
** \param [in] reg_addr - register address in register map
** \param [in] data - pointer to the first byte of a block of data to write
** \param [in] bytes - indicates how many bytes of data to write
** \return I2C_STATUS status of write attempt
**/
I2C_STATUS I2C_write(I2C_SLAVE * slave, const uint16_t reg_addr, uint8_t * data, const uint16_t bytes) {
	return I2C_comm(slave, reg_addr, data, bytes, I2C_WRITE); }

/*!\brief This function reads data from the address specified and stores this
 *        data in the area provided by the pointer.
** \param [in, out] slave - pointer to the I2C slave structure
** \param [in] reg_addr - register address in register map
** \param [in, out] data - pointer to the first byte of a block of data to read
** \param [in] bytes - indicates how many bytes of data to read
** \return I2C_STATUS status of read attempt
**/
I2C_STATUS I2C_read(I2C_SLAVE * slave, const uint16_t reg_addr, uint8_t * data, const uint16_t bytes) {
	return I2C_comm(slave, reg_addr, data, bytes, I2C_READ); }


/*!\brief Start i2c_timeout timer
** \attribute inline
** \return nothing
**/
static inline void __attribute__((__always_inline__)) I2C_start_timeout(void) {
	i2c.start_wait = (uint16_t) millis(); }

/*!\brief Test i2c_timeout
** \attribute inline
** \return true if i2c_timeout occured (false otherwise)
**/
static inline uint8_t __attribute__((__always_inline__)) I2C_timeout(void) {
	return (((uint16_t) millis() - i2c.start_wait) >= i2c.cfg.timeout); }

/*!\brief Send start condition
** \return true if start condition acknowledged (false otherwise)
**/
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

/*!\brief Send stop condition
** \return true if stop condition acknowledged (false otherwise)
**/
bool I2C_stop(void)
{
	I2C_start_timeout();

	TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTO);

	while ((TWCR & (1 << TWSTO)))
	{ if (I2C_timeout())	{ I2C_reset(); return false; } }

	return true;
}

/*!\brief Send byte on bus
** \param [in] dat - data to be sent
** \return true if data sent acknowledged (false otherwise)
**/
bool I2C_wr8(const uint8_t dat)
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

/*!\brief Receive byte from bus
** \param [in] ack - true if wait for ack
** \return true if data reception acknowledged (false otherwise)
**/
uint8_t I2C_rd8(const bool ack)
{
	I2C_start_timeout();

	if (ack)	{ TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWEA); }
	else		{ TWCR = (1 << TWINT) | (1 << TWEN); }

	while (!(TWCR & (1 << TWINT)))
	{ if (I2C_timeout())	{ I2C_reset(); return false; } }

	if (TWI_STATUS == LOST_ARBTRTN)		{ I2C_reset(); return false; }

	return ((((TWI_STATUS == MR_DATA_NACK) && (!ack)) || ((TWI_STATUS == MR_DATA_ACK) && (ack))) ? true : false);
}

/*!\brief Send I2C address
** \param [in] slave - pointer to the I2C slave structure
** \param [in] rw - read/write transaction
** \return true if I2C chip address sent acknowledged (false otherwise)
**/
bool I2C_sndAddr(I2C_SLAVE * slave, const I2C_RW rw)
{
	TWDR = (slave->cfg.addr << 1) | rw;

	I2C_start_timeout();

	TWCR = (1 << TWINT) | (1 << TWEN);

	while (!(TWCR & (1 << TWINT)))
	{ if (I2C_timeout())	{ I2C_reset(); return false; } }

	if ((TWI_STATUS == MT_SLA_ACK) || (TWI_STATUS == MR_SLA_ACK))	{ return true; }

	if ((TWI_STATUS == MT_SLA_NACK) || (TWI_STATUS == MR_SLA_NACK))	{ I2C_stop(); }
	else															{ I2C_reset(); }

	return false;
}


/*!\brief This procedure calls appropriate functions to perform a proper send transaction on I2C bus.
** \param [in, out] slave - pointer to the I2C slave structure
** \param [in] reg_addr - register address in register map
** \param [in] data - pointer to the first byte of a block of data to write
** \param [in] bytes - indicates how many bytes of data to write
** \return Boolean indicating success/fail of write attempt
**/
static bool I2C_wr(I2C_SLAVE * slave, const uint16_t reg_addr, uint8_t * data, const uint16_t bytes)
{
	if (bytes == 0)												{ return false; }

	if (I2C_start() == false)									{ return false; }
	if (I2C_sndAddr(slave, I2C_WRITE) == false)					{ return false; }
	if ((slave->cfg.reg_size) && (reg_addr != slave->reg_addr))	// Don't send address if writing next
	{
		(void) I2C_slave_set_reg_addr(slave, reg_addr);

		if (slave->cfg.reg_size >= I2C_16B_REG)	// if size >2, 16bit address is used
		{
			if (I2C_wr8((uint8_t) (reg_addr >> 8)) == false)	{ return false; }
		}
		if (I2C_wr8((uint8_t) reg_addr) == false)				{ return false; }
	}

	for (uint16_t cnt = 0; cnt < bytes; cnt++)
	{
		if (I2C_wr8(*data++) == false)							{ return false; }
		slave->reg_addr++;
	}

	if (I2C_stop() == false)									{ return false; }

	return true;
}


/*!\brief This procedure calls appropriate functions to perform a proper receive transaction on I2C bus.
** \param [in, out] slave - pointer to the I2C slave structure
** \param [in] reg_addr - register address in register map
** \param [in, out] data - pointer to the first byte of a block of data to read
** \param [in] bytes - indicates how many bytes of data to read
** \return Boolean indicating success/fail of read attempt
**/
static bool I2C_rd(I2C_SLAVE * slave, const uint16_t reg_addr, uint8_t * data, const uint16_t bytes)
{
	if (bytes == 0)													{ return false; }

	if ((slave->cfg.reg_size) && (reg_addr != slave->reg_addr))	// Don't send address if reading next
	{
		(void) I2C_slave_set_reg_addr(slave, reg_addr);

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

