# cI2C
Arduino Hardware I2C for AVR (plain c)
Hardware I2C library for AVR µcontrollers (lib intended for I2C protocols development in c, for easier ports to other µcontrollers)

notes:
- cI2C is written in plain c (intentionally)
- cI2C does not use any interrupt (yet, but soon will have to)
- cI2C is designed to act as bus Master (Slave mode will be considered in future releases)
- cI2C is set to work on AVR targets only
	-> port to SAM &(|) ESP8266 targets would have to be considered, yet the lib is not multi-cores approach designed

Usage: 
refer to Doxygen generated documentation & example sketches

examples included:
following examples should work with any I2C EEPROM/FRAM with address 0x50
(yet function to get Chip ID are device dependant (and will probably only work on FUJITSU devices))
ci2c_master_write.ino: Write some bytes to FRAM and compare them with what's read afterwards
ci2c_master_read.ino: Read some bytes in FRAM
ci2c_advanced.ino: Redirecting slave write & read functions (to custom functions following typedef)


Doxygen doc can be generated for the library using doxyfile

Feel free to share your thoughts @ xgarmanboziax@gmail.com about:
	- issues encountered
	- optimisations
	- improvements & new functionalities