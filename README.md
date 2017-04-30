# PD Buddy firmware

This is the firmware for the PD Buddy project.  Currently, this specifically
means the [PD Buddy Sink][].  The firmware is currently under heavy development,
but is largely functional.

[PD Buddy Sink]: https://git.clayhobbs.com/pd-buddy/pd-buddy-sink

## Features

* Two boot modes, Setup and Sink, selected by the Setup button's state at
  startup.
* Sink mode implements a simple USB Power Delivery sink, aiming for full
  compliance with the USB Power Delivery Specification, Revision 2.0,
  Version 1.3.
* Requests the configured voltage and current if available, or a safe, low
  power request otherwise.
* Provides power on the output connector only when an explicit contract is in
  place for the configured voltage and current.
* Setup mode implements a USB CDC-ACM command-line interface allowing
  configuration to be loaded from and stored in flash.

## Prerequisites

To compile the firmware, you must first install the [GNU ARM Embedded
Toolchain][toolchain].  Details of its installation is beyond the scope of this
README.  Once the toolchain is installed, clone this repository with:

    $ git clone --recursive http://git.clayhobbs.com/pd-buddy/pd-buddy-firmware.git

This will give you a complete copy of the repository, including the ChibiOS
submodule.

You will also need to install some program to flash the firmware.  The simplest
option is [dfu-util][], as it requires no extra hardware.  If you prefer to use
SWD, you could also use [stlink][] or [OpenOCD][].

[toolchain]: https://launchpad.net/gcc-arm-embedded
[dfu-util]: http://dfu-util.sourceforge.net/
[stlink]: https://github.com/texane/stlink
[OpenOCD]: http://openocd.org/

## Compiling

With all the dependencies installed, the firmware can be compiled as follows:

    $ cd pd-buddy-firmware
    $ make

This compiles the firmware to `build/pd-buddy-firmware.{bin,elf}`.

## Flashing

The firmware can be flashed in any number of ways, including but not limited to
the following:

### dfu-util

Set the boot mode switch on the PD Buddy Sink to DFU mode and plug it into your
computer.  Flash the firmware with:

    $ dfu-util -a 0 -s 0x08000000:leave -D build/pd-buddy-firmware.bin

Don't forget to set the switch back to normal mode after unplugging the device.

### stlink

If you have an ST-LINK/V2, you can use it to flash the firmware via SWD as
follows:

    $ st-flash write build/pd-buddy-firmware.bin 0x8000000

### OpenOCD

OpenOCD can also be used to flash the firmware.  For example:

    $ openocd -f interface/stlink-v2.cfg -f target/stm32f0x.cfg -c "program build/pd-buddy-firmware.elf verify reset exit"

## Usage

After first flashing the PD Buddy Sink, the device has no configuration.  To
configure it, plug it into your computer while holding the "Setup" button.  The
LED should blink once per second to indicate that the device is in
configuration mode.  There are then two ways to configure the Sink: a serial
terminal, or the configuration GUI.

### Configuration with the Serial Terminal

Connect to the PD Buddy Sink with your favorite serial console program, such as
[GNU Screen][], [Minicom][], or [PuTTY][].  Press Enter, and you should be
greeted with a `PDBS)` prompt.  The `help` command gives brief summaries of
each of the available commands.

To configure the PD Buddy Sink to request 2.25 A at 20 V, run the following
commands:

    PDBS) set_v 20000
    PDBS) set_i 2250
    PDBS) write

When `write` is run, the chosen settings are written to flash.  You can then
simply disconnect the Sink from your computer.

For more information about the serial console configuration interface, see
`docs/console_config.md`.

[GNU Screen]: https://www.gnu.org/software/screen/
[Minicom]: https://alioth.debian.org/projects/minicom
[PuTTY]: http://www.chiark.greenend.org.uk/~sgtatham/putty/

### Configuration with the GUI

The Sink can also be configured by the [PD Buddy Configuration][pd-buddy-gtk]
GUI.  For more information, see that repository's README.

[pd-buddy-gtk]: https://git.clayhobbs.com/pd-buddy/pd-buddy-gtk

### Using the configured PD Buddy Sink

Once the Sink has been configured, just plug it into your USB PD power supply.
If the supply is capable of putting out the configured current at the
configured voltage, the Sink will negotiate it, then turn on its output and
blink the LED three times to indicate success.  If the supply cannot output
enough power, the Sink will turn its LED on solid to indicate failure, and
leave its output turned off.
