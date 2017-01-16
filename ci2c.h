/*!\file ci2c.h
** \author SMFSW
** \version 0.2
** \copyright MIT SMFSW (2017)
** \brief arduino i2c in plain c declarations
**/
/****************************************************************/
#ifndef __CI2C_H__
	#define __CI2C_H__		"v0.2"
/****************************************************************/

#if (ARDUINO >= 100)
	#include <Arduino.h>
#else
	#include <WProgram.h>
#endif

#include <inttypes.h>
#include <stdbool.h>


#ifdef __cplusplus
extern "C"{
#endif

#define DEF_CI2C_SPEED			100000
#define DEF_CI2C_NB_RETRIES		3
#define DEF_CI2C_TIMEOUT		100

typedef enum __attribute__((__packed__)) enI2C_STATUS {
	I2C_OK = 0x00,
	I2C_BUSY,
	I2C_NACK
} I2C_STATUS;

typedef enum __attribute__((__packed__)) enI2C_SPEED {
	I2C_SLOW = 100,		//!< I2C Slow speed (100KHz)
	I2C_LOW = 400,		//!< I2C Low speed (400KHz)
	I2C_FAST = 1000		//!< I2C Fast mode (1MHz)
} I2C_SPEED;


typedef enum __attribute__((__packed__)) enI2C_INT_SIZE {
	I2C_NO_REG = 0x00,	//!< Internal adress registers not applicable for slave
	I2C_8B_REG,			//!< Slave internal adress registers space is 8bits wide
	I2C_16B_REG			//!< Slave internal adress registers space is 16bits wide
} I2C_INT_SIZE;


typedef bool (*ci2c_fct_ptr) (const void*, uint16_t, uint8_t*, uint16_t);	//!< i2c read/write function typedef

typedef struct __attribute__((__packed__)) StructI2CSlave {
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

/*! \brief Init an I2C slave structure for cMI2C communication
 *! \param [in] slave - pointer to the I2C slave structure to init
 *! \param [in] sl_addr - I2C slave address
 *! \param [in] reg_sz - internal register map size
 *! \return nothing
 */
extern void I2C_slave_init(I2C_SLAVE * slave, uint8_t sl_addr, I2C_INT_SIZE reg_sz);

/*! \brief Redirect slave I2C read/write function (if needed for advanced use)
 *! \param [in] slave - pointer to the I2C slave structure to init
 *! \param [in] func - pointer to read/write function to affect
 *! \param [in] rw - 0 = read function, 1 = write function
 *! \return nothing
 */
extern void I2C_slave_set_rw_func(I2C_SLAVE * slave, void * func, bool rw);

/*! \brief Change I2C slave address
 *! \param [in, out] slave - pointer to the I2C slave structure to init
 *! \param [in] sl_addr - I2C slave address
 *! \return true if new address set (false if address is >7Fh)
 */
extern inline bool I2C_slave_set_addr(I2C_SLAVE * slave, uint8_t sl_addr) __attribute__((__always_inline__));

/*! \brief Change I2C registers map size (for access)
 *! \param [in, out] slave - pointer to the I2C slave structure
 *! \param [in] reg_sz - internal register map size
 *! \return true if new size is correct (false otherwise and set to 16bit by default)
 */
extern inline bool I2C_slave_set_reg_size(I2C_SLAVE * slave, I2C_INT_SIZE reg_sz) __attribute__((__always_inline__));

/*! \brief Get I2C slave address
 *! \param [in] slave - pointer to the I2C slave structure
 *! \return I2C slave address
 */
extern inline uint8_t I2C_slave_get_addr(I2C_SLAVE * slave) __attribute__((__always_inline__));

/*! \brief Get I2C register map size (for access)
 *! \param [in] slave - pointer to the I2C slave structure
 *! \return register map using 16bits if true (1Byte otherwise)
 */
extern inline bool I2C_slave_get_reg_size(I2C_SLAVE * slave) __attribute__((__always_inline__));

/*! \brief Get I2C current register address (addr may passed this way in procedures if contigous accesses)
 *! \param [in] slave - pointer to the I2C slave structure
 *! \return current register map address
 */
extern inline uint16_t I2C_slave_get_reg_addr(I2C_SLAVE * slave) __attribute__((__always_inline__));


/*************************/
/*** I2C BUS FUNCTIONS ***/
/*************************/

/*! \brief Enable I2c module on arduino board (including pull-ups,
 *!        enabling of ACK, and setting clock frequency)
 *! \param [in] speed - I2C bus speed in KHz
 *! \return nothing
 */
extern void I2C_init(uint16_t speed);

/*! \brief Change I2C frequency
 *! \param [in] speed - I2C bus speed in KHz
 *! \return true if change is successful (false otherwise)
 */
extern bool I2C_set_speed(uint16_t speed);

/*! \brief Change I2C ack timeout
 *! \param [in] timeout - I2C ack timeout (500 ms max)
 *! \return true if change is successful (false otherwise)
 */
extern inline bool I2C_set_timeout(uint16_t timeout);

/*! \brief Change I2C message retries (in case of failure)
 *! \param [in] retries - I2C number of retries (max of 8)
 *! \return true if change is successful (false otherwise)
 */
extern inline bool I2C_set_retries(uint8_t retries);

/*! \brief Get I2C busy status
 *! \return true if busy
 */
extern inline bool I2C_is_busy(void);

/*! \brief This function writes the provided data to the address specified.
 *!
 *! \param [in, out] slave - pointer to the I2C slave structure to init
 *! \param [in] reg_addr - register address in register map
 *! \param [in] data - pointer to the first byte of a block of data to write
 *! \param [in] nb_bytes - indicates how many bytes of data to write
 *! \return I2C_STATUS status of write attempt
 */
extern inline I2C_STATUS I2C_write(I2C_SLAVE * slave, uint16_t reg_addr, uint8_t * data, uint16_t nb_bytes);

/*! \brief This inline is a wrapper to I2C_write in case of contigous operations
 *!
 *! \param [in, out] slave - pointer to the I2C slave structure to init
 *! \param [in] data - pointer to the first byte of a block of data to write
 *! \param [in] nb_bytes - indicates how many bytes of data to write
 *! \return I2C_STATUS status of write attempt
 */
extern inline I2C_STATUS I2C_write_next(I2C_SLAVE * slave, uint8_t * data, uint16_t nb_bytes);


/*! \brief This function reads data from the address specified and stores this
 *!        data in the area provided by the pointer.
 *!
 *! \param [in, out] slave - pointer to the I2C slave structure to init
 *! \param [in] reg_addr - register address in register map
 *! \param [in, out] data - pointer to the first byte of a block of data to read
 *! \param [in] nb_bytes - indicates how many bytes of data to read
 *! \return I2C_STATUS status of read attempt
 */
extern inline I2C_STATUS I2C_read(I2C_SLAVE * slave, uint16_t reg_addr, uint8_t * data, uint16_t nb_bytes);

/*! \brief This inline is a wrapper to I2C_read in case of contigous operations
 *!
 *! \param [in, out] slave - pointer to the I2C slave structure to init
 *! \param [in] data - pointer to the first byte of a block of data to read
 *! \param [in] nb_bytes - indicates how many bytes of data to read
 *! \return I2C_STATUS status of read attempt
 */
extern inline I2C_STATUS I2C_read_next(I2C_SLAVE * slave, uint8_t * data, uint16_t nb_bytes);


/***********************************/
/***  cI2C LOW LEVEL FUNCTIONS   ***/
/*** THAT MAY BE USEFUL FOR DVPT ***/
/***********************************/
/*! \brief I2C bus reset (Release SCL and SDA lines and re-enable module)
 *! \return nothing
 */
extern void I2C_reset(void);
/*! \brief Send start condition
 *! \return true if start condition acknowledged (false otherwise)
 */
extern bool I2C_start(void);
/*! \brief Send stop condition
 *! \return true if stop condition acknowledged (false otherwise)
 */
extern bool I2C_stop(void);
/*! \brief Send I2C address
 *! \param [in] sl_addr - I2C slave address
 *! \return true if I2C chip address sent acknowledged (false otherwise)
 */
extern bool I2C_sndAddr(uint8_t sl_addr);
/*! \brief Send byte on bus
 *! \param [in] dat - data to be sent
 *! \return true if data sent acknowledged (false otherwise)
 */
extern bool I2C_snd8(uint8_t dat);
/*! \brief Receive byte from bus
 *! \param [in] ack - true if wait for ack
 *! \return true if data reception acknowledged (false otherwise)
 */
extern uint8_t I2C_rcv8(bool ack);


#ifdef __cplusplus
}
#endif

#endif
