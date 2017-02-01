# cI2C
Arduino Hardware I2C for AVR (plain c)

Hardware I2C library for AVR MCUs (lib intended for I2C protocols development in c, for easier ports to other MCUs)

## Library choice:
* cI2C library implements I2C bus for AVR tagets (Uno, Nano, Mega...)
  * you may prefer this one when:
    * working on AVR targets
    * interrupts are not needed
* WireWrapper implements I2C bus for every platform that includes Wire library
  * you would have to use this one when:
    * working on non-AVR targets
    * portability is needed (using Wire library)

No refactoring is required when switching between **cI2C** & **WireWrapper** libs;
Both libs share same Typedefs, Functions & Parameters.

## Notes:
* cI2C is written in plain c (intentionally)
* cI2C does not use any interrupt (yet, but soon will have to)
* cI2C is designed to act as bus Master (Slave mode will be considered in future releases)
* cI2C is set to work on AVR targets only
  * for other targets, you may use **WireWrapper** instead (will be using Wire)

## Usage: 
This library is intended to be able to work with multiple slaves connected on the same I2C bus.
Thus, the I2C bus and Slaves are defined separately.

* On one hand, I2C bus has to be initialised with appropriate speed:
  * use I2C_init(speed): speed can be choosen from I2C_SPEED enum for convenience, or passing an integer as parameter
* On the other hand, Slave(s) have to be defined and initialised too:
  * use I2C_SLAVE typedef to declare slaves structs
  * use I2C_slave_init(pSlave, addr, regsize)
    * **pSlave** is a pointer to the slave struct to initialise
    * **addr** is the slave I2C address (don't shift addr, lib takes care of that)
    * **regsize** is the width of internal slave registers (to be choosen from I2C_INT_SIZE)
  * in case you need to use custom R/W procedures for a particular slave:
    * use I2C_slave_set_rw_func(pSlave, pFunc, rw)
      * **pSlave** is a pointer to the slave declaration to initialise
      * **pFunc** is a pointer to the Read or Write bypass function
      * **rw** can be choosen from I2C_RW enum (wr=0, rd=1)

After all inits are done, the lib can basically be used this way:
* I2C_read(pSlave, regaddr, pData, bytes)
  * **pSlave** is a pointer to the slave struct to read from
  * **regaddr** is the start address to read from
  * **pData** is a pointer to the place where datas read will be stored
  * **bytes** number of bytes to read from slave
  * returns true if read is ok, false otherwise
* I2C_write(pSlave, regaddr, pData, bytes)
  * **pSlave** is a pointer to the slave struct to write to
  * **regaddr** is the start address to write to
  * **pData** is a pointer to the block of datas to write to slave
  * **bytes** number of bytes to write to slave
  * returns true if write is ok, false otherwise

## Examples included:
following examples should work with any I2C EEPROM/FRAM with address 0x50
(yet function to get Chip ID are device dependant (and will probably only work on FUJITSU devices))
* ci2c_master_write.ino: Write some bytes to FRAM and compare them with what's read afterwards
* ci2c_master_read.ino: Read some bytes in FRAM
* ci2c_advanced.ino: Redirecting slave write & read functions (to custom functions following typedef)

Doxygen doc can be generated for the library using doxyfile

## Links:
Feel free to share your thoughts @ xgarmanboziax@gmail.com about:
* issues encountered
* optimisations
* improvements & new functionalities

**cI2C**
- https://github.com/SMFSW/cI2C
- https://bitbucket.org/SMFSW/ci2c

**WireWrapper**
- https://github.com/SMFSW/WireWrapper
- https://bitbucket.org/SMFSW/wirewrapper
