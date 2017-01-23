# cI2C
Arduino Hardware I2C for AVR (plain c)
Hardware I2C library for AVR µcontrollers (lib intended for I2C protocols development in c, for easier ports to other µcontrollers)

## Notes:
* cI2C is written in plain c (intentionally)
* cI2C does not use any interrupt (yet, but soon will have to)
* cI2C is designed to act as bus Master (Slave mode will be considered in future releases)
* cI2C is set to work on AVR targets only
  * for other targets, you may use **WireWrapper** instead (will be using Wire)
  * **cI2C** & **WireWrapper** libs declare same structures & functions as seen from the outside
    (switch between libs without changing anyhting but the include)

## Usage: 
refer to Doxygen generated documentation & example sketches

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
