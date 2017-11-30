/*!\file ci2c.h
** \author SMFSW
** \version 1.2
** \copyright MIT SMFSW (2017)
** \brief arduino i2c in plain c declarations
** \warning Don't access (r/w) last 16b internal address byte alone right after init, this would lead to hazardous result (in such case, make a dummy read of addr 0 before)
**/
/****************************************************************/
#ifndef __CI2C_H__
	#define __CI2C_H__
/****************************************************************/

#if defined(DOXY)
	// Define gcc __attribute__ as void when Doxygen runs
	#define __attribute__(a)	//!< GCC attribute (ignored by Doxygen)
#endif

#if (ARDUINO >= 100)
	#include <Arduino.h>
#else
	#include <WProgram.h>
#endif

#include <inttypes.h>
#include <stdbool.h>


#ifdef __cplusplus
extern "C" {
#endif

#define DEF_CI2C_NB_RETRIES		3		//!< Default cI2C transaction retries
#define DEF_CI2C_TIMEOUT		100		//!< Default cI2C timeout


/*!\enum enI2C_RW
** \brief I2C RW bit enumeration
** \attribute packed enum
**/
typedef enum __attribute__((__packed__)) enI2C_RW {
	I2C_WRITE = 0,	//!< I2C rw bit (write)
	I2C_READ		//!< I2C rw bit (read)
} I2C_RW;


/*!\enum enI2C_SPEED
** \brief I2C bus speed
** \attribute packed enum
**/
typedef enum __attribute__((__packed__)) enI2C_SPEED {
	I2C_STD = 100,		//!< I2C Standard (100KHz)
	I2C_FM = 400,		//!< I2C Fast Mode (400KHz)
	I2C_FMP = 1000,		//!< I2C Fast mode + (1MHz): will set speed to Fast Mode (up to 400KHz on avr)
	I2C_HS = 3400		//!< I2C High Speed (3.4MHz): will set speed to Fast Mode (up to 400KHz on avr)
} I2C_SPEED;


/*!\enum enI2C_STATUS
** \brief I2C slave status
** \attribute packed enum
**/
typedef enum __attribute__((__packed__)) enI2C_STATUS {
	I2C_OK = 0x00,	//!< I2C OK
	I2C_BUSY,		//!< I2C Bus busy
	I2C_NACK		//!< I2C Not Acknowledge
} I2C_STATUS;

/*!\enum enI2C_INT_SIZE
** \brief I2C slave internal address registers size
** \attribute packed enum
**/
typedef enum __attribute__((__packed__)) enI2C_INT_SIZE {
	I2C_NO_REG = 0x00,	//!< Internal address registers not applicable for slave
	I2C_8B_REG,			//!< Slave internal address registers space is 8bits wide
	I2C_16B_REG			//!< Slave internal address registers space is 16bits wide
} I2C_INT_SIZE;


typedef bool (*ci2c_fct_ptr) (void*, const uint16_t, uint8_t*, const uint16_t);	//!< i2c read/write function pointer typedef


/*!\struct StructI2CSlave
** \brief ci2c slave config and control parameters
** \attribute packed struct
**/
typedef struct __attribute__((__packed__)) StructI2CSlave {
	/*!\struct cfg
	** \brief ci2c slave parameters
	**/
	struct {
		uint8_t			addr;		//!< Slave address
		I2C_INT_SIZE	reg_size;	//!< Slave internal registers size
		ci2c_fct_ptr	wr;			//!< Slave write function pointer
		ci2c_fct_ptr	rd;			//!< Slave read function pointer
	} cfg;
	uint16_t			reg_addr;	//!< Internal current register address
	I2C_STATUS			status;		//!< Status of the last communications
} I2C_SLAVE;


/***************************/
/*** I2C SLAVE FUNCTIONS ***/
/***************************/

/*!\brief Init an I2C slave structure for cMI2C communication
** \param [in] slave - pointer to the I2C slave structure to init
** \param [in] sl_addr - I2C slave address
** \param [in] reg_sz - internal register map size
** \return nothing
**/
void I2C_slave_init(I2C_SLAVE * slave, const uint8_t sl_addr, const I2C_INT_SIZE reg_sz);

/*!\brief Redirect slave I2C read/write function (if needed for advanced use)
** \param [in] slave - pointer to the I2C slave structure to init
** \param [in] func - pointer to read/write function to affect
** \param [in] rw - 0 = write function, 1 = read function
** \return nothing
**/
void I2C_slave_set_rw_func(I2C_SLAVE * slave, const ci2c_fct_ptr func, const I2C_RW rw);

/*!\brief Change I2C slave address
** \param [in, out] slave - pointer to the I2C slave structure to init
** \param [in] sl_addr - I2C slave address
** \return true if new address set (false if address is >7Fh)
**/
bool I2C_slave_set_addr(I2C_SLAVE * slave, const uint8_t sl_addr);

/*!\brief Change I2C registers map size (for access)
** \param [in, out] slave - pointer to the I2C slave structure
** \param [in] reg_sz - internal register map size
** \return true if new size is correct (false otherwise and set to 16bit by default)
**/
bool I2C_slave_set_reg_size(I2C_SLAVE * slave, const I2C_INT_SIZE reg_sz);

/*!\brief Get I2C slave address
** \attribute inline
** \param [in] slave - pointer to the I2C slave structure
** \return I2C slave address
**/
inline uint8_t __attribute__((__always_inline__)) I2C_slave_get_addr(const I2C_SLAVE * slave) {
	return slave->cfg.addr; }

/*!\brief Get I2C register map size (for access)
** \attribute inline
** \param [in] slave - pointer to the I2C slave structure
** \return register map using 16bits if true (1Byte otherwise)
**/
inline bool __attribute__((__always_inline__)) I2C_slave_get_reg_size(const I2C_SLAVE * slave) {
	return slave->cfg.reg_size; }

/*!\brief Get I2C current register address (addr may passed this way in procedures if contigous accesses)
** \attribute inline
** \param [in] slave - pointer to the I2C slave structure
** \return current register map address
**/
inline uint16_t __attribute__((__always_inline__)) I2C_slave_get_reg_addr(const I2C_SLAVE * slave) {
	return slave->reg_addr; }


/*************************/
/*** I2C BUS FUNCTIONS ***/
/*************************/

/*!\brief Enable I2c module on arduino board (including pull-ups,
 *        enabling of ACK, and setting clock frequency)
** \param [in] speed - I2C bus speed in KHz
** \return nothing
**/
void I2C_init(const uint16_t speed);

/*!\brief Disable I2c module on arduino board (releasing pull-ups, and TWI control)
** \return nothing
**/
void I2C_uninit();

/*!\brief Change I2C frequency
** \param [in] speed - I2C bus speed in KHz (max 400KHz on AVR)
** \return Configured bus speed
**/
uint16_t I2C_set_speed(const uint16_t speed);

/*!\brief Change I2C ack timeout
** \param [in] timeout - I2C ack timeout (500 ms max)
** \return Configured timeout
**/
uint16_t I2C_set_timeout(const uint16_t timeout);

/*!\brief Change I2C message retries (in case of failure)
** \param [in] retries - I2C number of retries (max of 8)
** \return Configured number of retries
**/
uint8_t I2C_set_retries(const uint8_t retries);

/*!\brief Get I2C busy status
** \return true if busy
**/
bool I2C_is_busy(void);

/*!\brief This function writes the provided data to the address specified.
** \param [in, out] slave - pointer to the I2C slave structure
** \param [in] reg_addr - register address in register map
** \param [in] data - pointer to the first byte of a block of data to write
** \param [in] bytes - indicates how many bytes of data to write
** \return I2C_STATUS status of write attempt
**/
I2C_STATUS I2C_write(I2C_SLAVE * slave, const uint16_t reg_addr, uint8_t * data, const uint16_t bytes);

/*!\brief This inline is a wrapper to I2C_write in case of contigous operations
** \attribute inline
** \param [in, out] slave - pointer to the I2C slave structure
** \param [in] data - pointer to the first byte of a block of data to write
** \param [in] bytes - indicates how many bytes of data to write
** \return I2C_STATUS status of write attempt
**/
inline I2C_STATUS __attribute__((__always_inline__)) I2C_write_next(I2C_SLAVE * slave, uint8_t * data, const uint16_t bytes) {
	return I2C_write(slave, slave->reg_addr, data, bytes); }

/*!\brief This function reads data from the address specified and stores this
 *        data in the area provided by the pointer.
** \param [in, out] slave - pointer to the I2C slave structure
** \param [in] reg_addr - register address in register map
** \param [in, out] data - pointer to the first byte of a block of data to read
** \param [in] bytes - indicates how many bytes of data to read
** \return I2C_STATUS status of read attempt
**/
I2C_STATUS I2C_read(I2C_SLAVE * slave, const uint16_t reg_addr, uint8_t * data, const uint16_t bytes);

/*!\brief This inline is a wrapper to I2C_read in case of contigous operations
** \attribute inline
** \param [in, out] slave - pointer to the I2C slave structure
** \param [in] data - pointer to the first byte of a block of data to read
** \param [in] bytes - indicates how many bytes of data to read
** \return I2C_STATUS status of read attempt
**/
inline I2C_STATUS __attribute__((__always_inline__)) I2C_read_next(I2C_SLAVE * slave, uint8_t * data, const uint16_t bytes) {
	return I2C_read(slave, slave->reg_addr, data, bytes); }


/***********************************/
/***  cI2C LOW LEVEL FUNCTIONS   ***/
/*** THAT MAY BE USEFUL FOR DVPT ***/
/***********************************/
/*!\brief I2C bus reset (Release SCL and SDA lines and re-enable module)
** \return nothing
**/
void I2C_reset(void);

/*!\brief Send start condition
** \return true if start condition acknowledged (false otherwise)
**/
bool I2C_start(void);

/*!\brief Send stop condition
** \return true if stop condition acknowledged (false otherwise)
**/
bool I2C_stop(void);

/*!\brief Send byte on bus
** \param [in] dat - data to be sent
** \return true if data sent acknowledged (false otherwise)
**/
bool I2C_wr8(const uint8_t dat);

/*!\brief Receive byte from bus
** \param [in] ack - true if wait for ack
** \return true if data reception acknowledged (false otherwise)
**/
uint8_t I2C_rd8(const bool ack);

/*!\brief Send I2C address
** \param [in] slave - pointer to the I2C slave structure
** \param [in] rw - read/write transaction
** \return true if I2C chip address sent acknowledged (false otherwise)
**/
bool I2C_sndAddr(I2C_SLAVE * slave, const I2C_RW rw);


#ifdef __cplusplus
}
#endif

#endif
