

### This fork is about RS232 hardware handshaking and increased + more relaible throughput. 

In order to benefit from higher thoughput and hardware flow control, you will need to modify your control board.
The changes needed can be defined in two; one for the actual hardware flow control, and one for increased bitrate.
The latter needs the former to be useful.

This fork is meant to also work without the hardware board changes, but it has not been tested much by me. 


Both 36 and 48 models share the same control board, with a little difference; the 48 has a native rs422 interface populated, whereas the 36 has rs232.
#### For the 48 models, the following needs to be done;
Since we are going to change the 48 into rs232, we need to add the missing components to do so;
* Add a 1+1 channel isolator chip to unpopulated position U31 e.g. ADUM121NOBRZ-RL7. Note; put a piece of kapton tape on pads 2 & 3, since they shall not be soldered to the board pads, but instead be connected to the wires described below.
* Add a rs232 level shifter capable of 1mbit/s e.g. SN65C3232EDR to unpopulated position U33.
* Add a few SMD 100nF caps around U31 and U33 on the unpopulated posistions.
* Add a 4 pole through hole mount JST connector (board uses China brand Yeonho Electronics SMW250/SMH250 throughout, so if you have those at hand that would be nicer, but the JST are very similar in all aspects but the locking).
* Add ESD protection components to the 'new' connector. Or simply make sure the signals comes out like I did in the picture.. :) 
    Update: If you aim for highter speeds, I would recommend removing the ESD protection resistors, and possibly also the tranzil, since the signal integrity will be slightly better without, and anyway, the USB interface will anyway be between the surrounding world and the rs232 interface on this board.
* Pull two wires from an unpopulated U18 pin 4 & 1 (SO8) (rs485 transciever) to pin 2 & 3 of the new isosolator chip. These are the RTS/CTS signals.
* Move the 0R resistor from position R132 to position R131. This will connect the rx signal from the rs232 input instead of the original rs422.

With this change, you will no longer have the extra rs232 for debug available, if this is important for you. If if is, the debug could instead be moved to the rs422 interface. 

Note that the only thing you need to do, if you want to revert back to rs422, is to move the resistor back to position R132. 

#### For the 36 models, which already have the rs232 you need to;
* Change position U33 to something faster. The on board U33 is specified for max 115200 baud.
* disconnect pin 2 & 3 on U31 from board pads.
* Pull the two wires as described.
* Remove (or keep) the ESD protection network close to the connectors. (See benefits above).

#### How to configure the serial port 
In config.default there are two relevant lines; one for specifying the baudrate you wish to run, and one setting for enabling RTS/CTS hardware flow control.

In OpenPnP you will need to select RTS/CTS flow control, and uncheck the "Confirmation Flow Control" since we will not need it any more.

#### A note of warning 
Not all USB-RS232 adapters are good (and even the good ones can be configured differenctly) so it is strongly suggested to test everything once you are done, by connecting a serial comm's program and issue some tests.
The author of this fork are using gtkcomm under linux, adding a 'macro' sending out a string "M115\nM114\nM115\nM114\nM115\nM114\n" (which will work on the orignial fw as well), or "M444\nM444\nM444\nM444\nM444\nM444\nM444\nM444\n" (not in other firmwares), which will result in a respons of 'U' and 'z' which are binary 0x55 and 0x5a.
By putting the M-commands in a macro a good comms's program will send them rather rapidly, during also receiving the first results. 
Check the result carefully for missed out response, or wrong respons characters. Repeat a few times.


A picture of the patch
![rts_cts_patch](https://user-images.githubusercontent.com/18227864/158996475-5d222994-015a-4fb8-b81a-a45bb956cf9d.jpg)



Currently the serial is interrupt driven, so the actual throughput will be less than the selected baud rate. e.d. for 576000 baud the actual throughput is around 500 kbps.
The idea is to enable DMA on the rx, but probably not on tx, since the tx is pacing the main loop, which we need to keep in sync with anyways.




## Old STM32/CHMT Notes from upstream

To build, follow normal smoothie build process to get setup.  Then checkout chmt branch and rebuild.

### Port Status:
* mbed hooks - Added, compiles, tested
* stm32f4xx libs - Added, compiles, tested
* timers - Ported, compiles, tested
* wdt - Ported, compiles, tested
* gpio - Ported, compiles, tested
* adc - Ported, compiles, tested
* pwm - Ported, compiles, tested
* build scripts - Added, project builds successfully

### CHMT Status:
* config file - 48VB Complete
* pin map - 48VB/36VA Complete
* operation/verfication - All System Functions Operational (excl. axis encoders)
* machine testing - 48VB All Systems Operational

### TODO:
* DONE: Target initialization and board bringup (clocks, mpu, etc)
* DONE: Verification of ported peripherals (step generation, watchdog, gpios)
* DONE: Debug/Comm uart setup
* DONE: CHMT controller specific configuration

### Notes/Caveats/Gotchas:
* smoothie mbed was ancient, so the oldest stm32 mbed available was integrated to reduce friction -- incompatibilities, and bugs from dated mbed may have been introduced
* MRI (gdb over serial) is not supported on stm32, use SWD/JTAG
* config file must be hardcoded into firmware build

### Next Steps/Priority
* CHMT Pinout Reversing -- Complete
* CHMT Config File Development -- 48VB Complete
* CHMT Machine Testing -- All Base Functions Operational, Long term and stability testing required.
* Synchronize System GCODEs to OpenPNP standards
* Stability Testing Required.
* WDT rewrite for longer timeout

# Smoothie

## Overview
Smoothie is a free, opensource, high performance G-code interpreter and CNC controller written in Object-Oriented C++ for the LPC17xx micro-controller ( ARM Cortex M3 architecture ). It will run on a mBed, a LPCXpresso, a SmoothieBoard, R2C2 or any other LPC17xx-based board. The motion control part is a port of the awesome grbl.

Documentation can be found here : [[http://smoothieware.org/]]

NOTE it is not necessary to build Smoothie yourself unless you want to. prebuilt binaries are available here: [[http://triffid-hunter.no-ip.info/Smoothie.html|Nightly builds]] and here: [[https://github.com/Smoothieware/Smoothieware/blob/edge/FirmwareBin/firmware.bin?raw=true|recent stable build]]

## Quick Start
These are the quick steps to get Smoothie dependencies installed on your computer:
* Pull down a clone of the Smoothie github project to your local machine.
* In the root subdirectory of the cloned Smoothie project, there are install scripts for the supported platforms.  Run the install script appropriate for your platform:
** Windows: win_install.cmd
** OS X: mac_install
** Linux: linux_install
* You can then run the BuildShell script which will be created during the install to properly configure the PATH environment variable to point to the required version of GCC for ARM which was just installed on your machine.  You may want to edit this script to further customize your development environment.

## Building Smoothie
Follow this guide... [[http://smoothieware.org/compiling-smoothie]]

In short...
From a shell, switch into the root Smoothie project directory and run:
{{{
make clean
make all
}}}

To upload you can do

{{{
make upload
}}}

if you have dfu-util installed.

Alternatively copy the file LPC1768/main.bin to the sdcard calling it firmware.bin and reset.

## Filing issues (for bugs ONLY)
Please follow this guide [[https://github.com/Smoothieware/Smoothieware/blob/edge/ISSUE_TEMPLATE.md]]

## Contributing

Please take a look at : 

* http://smoothieware.org/coding-standards
* http://smoothieware.org/developers-guide
* http://smoothieware.org/contribution-guidlines

Contributions very welcome !

## Donate
The Smoothie firmware is free software developed by volunteers. If you find this software useful, want to say thanks and encourage development, please consider a 
[[https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=9QDYFXXBPM6Y6&lc=US&item_name=Smoothieware%20development&currency_code=USD&bn=PP%2dDonationsBF%3abtn_donate_SM%2egif%3aNonHosted|Donation]]

## License

Smoothieware is released under the GNU GPL v3, which you can find at http://www.gnu.org/licenses/gpl-3.0.en.html


