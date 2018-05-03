# cI2C [![Build Status](https://travis-ci.org/SMFSW/cI2C.svg?branch=master)](https://travis-ci.org/SMFSW/cI2C)

Arduino Hardware I2C for AVR (plain c)

Hardware I2C library for AVR MCUs (lib intended for I2C protocols development in c, for easier ports to other MCUs)

## Library choice

* cI2C library implements I2C bus for AVR targets (Uno, Nano, Mega...)
  * you may prefer this one when:
    * working on AVR targets
    * interrupts are not needed
* WireWrapper implements I2C bus for every platform that includes Wire library
  * you would have to use this one when:
    * working on non-AVR targets
    * portability is needed (using Wire library)

No refactoring is required when switching between **cI2C** & **WireWrapper** libs;
Both libs share same Typedefs, Functions & Parameters.

## Notes

* cI2C is written in plain c (intentionally)
* cI2C does not use any interrupt (yet, but soon will have to)
* cI2C is designed to act as bus Master (Slave mode will be considered in future releases)
* cI2C is set to work on AVR targets only
  * for other targets, you may use **WireWrapper** instead (will be using Wire)

## Usage

This library is intended to be able to work with multiple slaves connected on the same I2C bus.
Thus, the I2C bus and Slaves are defined separately.

* On one hand, I2C bus has to be initialized with appropriate speed:
  * use `I2C_init(speed)`: speed can be chosen from `I2C_SPEED` enum for convenience, or passing an integer as parameter
* On the other hand, Slave(s) have to be defined and initialized too:
  * use `I2C_SLAVE` typedef to declare slaves structs
  * use `I2C_slave_init(pSlave, addr, regsize)`
    * `pSlave`: pointer to the slave struct to initialize
    * `addr`: slave I2C address (don't shift addr, lib takes care of that)
    * `regsize`: width of internal slave registers (to be chosen from `I2C_INT_SIZE`)
  * in case you need to use custom R/W procedures for a particular slave:
    * use `I2C_slave_set_rw_func(pSlave, pFunc, rw)`
      * `pSlave`: pointer to the slave declaration to initialize
      * `pFunc`: pointer to the Read or Write bypass function
      * `rw`: can be chosen from `I2C_RW` enum (wr=0, rd=1)

After all inits are done, the lib can basically be used this way:
* `I2C_read(pSlave, regaddr, pData, bytes)`
  * `pSlave`: pointer to the slave struct to read from
  * `regaddr`: start address to read from
  * `pData`: pointer to the place where datas read will be stored
  * `bytes`: number of bytes to read from slave
  * returns `true` if read is ok, `false` otherwise
* `I2C_write(pSlave, regaddr, pData, bytes)`
  * `pSlave`: pointer to the slave struct to write to
  * `regaddr`: start address to write to
  * `pData`: pointer to the block of datas to write to slave
  * `bytes`: number of bytes to write to slave
  * returns `true` if write is ok, `false` otherwise

## Examples included

following examples should work with any I2C EEPROM/FRAM with address 0x50
(yet function to get Chip ID are device dependent (and will probably only work on FUJITSU devices))
* [ci2c_master_write.ino](examples/ci2c_master_write/ci2c_master_write.ino): Write some bytes to FRAM and compare them with what's read afterwards
* [ci2c_master_read.ino](examples/ci2c_master_read/ci2c_master_read.ino): Read some bytes in FRAM
* [ci2c_advanced.ino](examples/ci2c_advanced/ci2c_advanced.ino): Redirecting slave write & read functions (to custom functions following typedef)

## Documentation

Doxygen doc can be generated using "Doxyfile".

See [generated documentation](https://smfsw.github.io/cI2C/)

## Release Notes

See [release notes](ReleaseNotes.md)

## See also

**cI2C**
* [cI2C github](https://github.com/SMFSW/cI2C) - C implementation of this library

**WireWrapper**
* [WireWrapper github](https://github.com/SMFSW/WireWrapper) - Cpp implementation using Wire Wrapper
