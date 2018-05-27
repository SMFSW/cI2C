Arduino Hardware I2C for AVR (plain c)
2017-2018 SMFSW

- cI2C is set to work on AVR targets only
	-> for other targets, you may use WireWrapper instead (will be using Wire)
	-> cI2C & WireWrapper libs declare same structures & functions as seen from the outside
		(switch between libs without changing anyhting but the include)


Feel free to share your thoughts @ xgarmanboziax@gmail.com about:
	- issues encountered
	- optimisations
	- improvements & new functionalities

------------

** Actual:
v1.3	13 May 2018:
- Delay between retries is now 1ms
- Adding support for unit tests and doxygen documentation generation with Travis CI
- Updated README.md

v1.2	30 Nov 2017:
- No internal address transmission when reading/writing to next internal address (make sure not to r/w last 16 address right just after init, otherwise make a dummy of address 0 just before)

v1.1	29 Nov 2017:
- Frequency calculation fix (thanks to TonyWilk)
- Set Frequency higher than Fast Mode (400KHz) will set bus to Fast Mode (frequency is up to 400KHz on AVR)
- I2C_set_xxx now returns values applied, not bool

v1.0	21 Nov 2017:
- Added const qualifier for function parameters
- Return from comm functions if bytes to R/W set to 0

v0.6	12 Jul 2017:
- compliance with Arduino v1.5+ IDE source located in src subfolder

v0.5	31 Jan 2017:
- refactored I2C_SPEED enum names for coherence with I2C specifications
- High Speed mode added in I2C_SPEED enum

v0.4	23 Jan 2017:
- less inlines (less warnings)
- inlines put into header (compatibility with WireWrapper)
- other common code between cI2C and WireWrapper changes
- README.md updated to tell about WireWrapper library

v0.3	22 Jan 2017:
- used function pointer in function parameters for convenience
- fixed read bug for devices without register address
- refactored rw booleans with enum instead (implied logic change)
- I2C_sndAddr function parameters changed
- added I2C_uninit function to release i2c bus
- refactoring & optimisations
- doxygen pass without warnings/errors now
- examples updated to test more of the library

v0.2	16 Jan 2017:
- First release
