# PD Buddy firmware

This is the firmware for the PD Buddy project.  Currently, this specifically
means the PD Buddy Sink.  The firmware is currently under heavy development,
but is partially functional.

## Prerequisites

To compile the firmware, you must first install the [GNU ARM Embedded
Toolchain](https://launchpad.net/gcc-arm-embedded).  Details of its
installation is beyond the scope of this README.  Once the toolchain is
installed, clone this repository with:

    $ git clone --recursive http://git.clayhobbs.com/clay/pd-buddy-firmware.git

This will give you a complete copy of the repository, including the ChibiOS
submodule.

You will also need to install some program to flash the firmware.  The simplest
option is [dfu-util](http://dfu-util.sourceforge.net/), as it requires no extra
hardware.  If you prefer to use SWD, you could also use
[stlink](https://github.com/texane/stlink) or [OpenOCD](http://openocd.org/).

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

Currently, there isn't much to say here.  Plug it into your power supply, and
the PD Buddy Sink will negotiate 2.25 A at 20 V.  This can be configured by
editing `dpm_desired_v` and `dpm_desired_i` in `src/device_policy_manager.c`,
then recompiling and reflashing.
