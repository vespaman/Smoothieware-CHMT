

### This fork is about DMA on the serial/RS232 hardware with handshaking and increased + more relaible throughput. 

With this branch, DMA is implemented on rx as well as tx with hardware flow control. 
In order to benefit from higher thoughput and hardware flow control, you will need to modify your control board.
The changes needed can be defined in two; one for the actual hardware flow control, and one for increased bitrate.
The latter needs the former to be useful.

If you want achieve >1Mbit, you will also need to remove the rs232 signalling level, and connect the USB-serial uart directly to the isolators.


Both 36 and 48 models share the same control board, with a little difference; the 48 has a native rs422 interface populated, whereas the 36 has rs232.
#### For the 48 models, the following needs to be done;
* Add a 1+1 channel isolator chip to unpopulated position U31 e.g. ADUM121NOBRZ-RL7. Note; put a piece of kapton tape on pads 2 & 3, since they shall not be soldered to the board pads, but instead be connected to the wires described below.
* Position U33 is made for a rs232 level shifter. The rx/tx/rts/cts path needs to be connected by wires, since we don't want 232 levels, in order to achieve higher speeds.
* Add a 4 pole through hole mount JST connector (board uses China brand Yeonho Electronics SMW250/SMH250 throughout, so if you have those at hand that would be nicer, but the JST are very similar in all aspects but the locking).
* Add ESD protection components to the 'new' connector. Or simply make sure the signals comes out like I did in the picture.. :) 
    Update: If you aim for highter speeds, I would recommend removing the ESD protection resistors, and possibly also the tranzil, since the signal integrity will be slightly better without, and anyway, the USB interface will anyway be between the surrounding world and the rs232 interface on this board.
* Pull two wires from an unpopulated U18 pin 4 & 1 (SO8) (rs485 transciever) to pin 2 & 3 of the new isosolator chip. These are the RTS/CTS signals.
* Move the 0R resistor from position R132 to position R131. This will connect the rx signal from the rs232 input instead of the original rs422.

With this change, you will no longer have the extra rs232 for debug available, if this is important for you. If if is, the debug could instead be moved to the rs422 interface. 


#### For the 36 models, which already have the rs232 you need to;
* Remove U33 (see above)
* disconnect pin 2 & 3 on U31 from board pads.
* Pull the two wires as described.
* Remove (or keep) the ESD protection network close to the connectors. (See benefits above).

#### How to configure the serial port 
In config.default there are two relevant lines; one for specifying the baudrate you wish to run, and one setting for enabling RTS/CTS hardware flow control.

In OpenPnP you will need to select RTS/CTS flow control, and uncheck the "Confirmation Flow Control" since we will not need it any more.

#### A note of warning 
Chmt is rather noisy, and chasing higer bitrates might mean that the serial lines gets disturbed. Keep the wires between the USB-Serial board and the the mainboard as short as possible.


A picture of the patch
![rts_cts_patch](https://user-images.githubusercontent.com/18227864/158996475-5d222994-015a-4fb8-b81a-a45bb956cf9d.jpg)


#### Things that can be improved in code 
* The buffer management in this implementation is not very optimized, since it uses the original buffer as a secondary rx buffer. It would be more efficient to keep everything in the DMA buffer, with the main loop retreiving data directly from it. Or at least some more optimized secondary buffer. Not sure if this extra complexity is needed though.
* The tx routing is waiting for DMA to finish, in order to mimic the original code. This might not be necessary. If so, the check that DMA and UART Tx is done could be checked before doing a tx.
* Rx DMA is not using DMA FIFO, so DMA will use unnecessary much of the memory bus bandwidth. (tx DMA is already using DMA FIFO)
* The UART & DMA interrupt level is currently being shared with ADC, which may not be good if ADC does not get serviced in time. UART/DMA should then be moved below ADC (or ADC raised above).




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


