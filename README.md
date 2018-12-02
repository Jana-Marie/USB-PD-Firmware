## USB-PD Firmware
#### forked from https://git.clayhobbs.com/pd-buddy/pd-buddy-firmware

Edited and customized to work with [Niklas]{https://github.com/NiklasFauth} USB-C Otter and [JanHenrik's]{https://github.com/Jan--Henrik} [TS100 USB-PD replacement PCB]https://github.com/Jan--Henrik/Otter-Iron.

Many thanks to [clay]{https://git.clayhobbs.com/clay} for the great work!

## Prerequisites

To compile the firmware, you must first install the [GNU ARM Embedded
Toolchain][toolchain].  Details of its installation is beyond the scope of this
README.  Once the toolchain is installed, clone this repository with:

    $ git clone --recursive http://git.clayhobbs.com/pd-buddy/pd-buddy-firmware.git

This will give you a complete copy of the repository, including the ChibiOS
submodule.

You will also need to install some program to flash the firmware.  The simplest
option is [dfu-util][], as it requires no extra hardware (though if upgrading
from versions older than 1.2.0, either the Boot switch must be installed or two
pads must be bridged).  If you prefer to use SWD, you could also use
[stlink][], [OpenOCD][], or a [Black Magic Probe][].

[toolchain]: https://launchpad.net/gcc-arm-embedded
[dfu-util]: http://dfu-util.sourceforge.net/
[stlink]: https://github.com/texane/stlink
[OpenOCD]: http://openocd.org/
[Black Magic Probe]: https://github.com/blacksphere/blackmagic/wiki

## Compiling

With all the dependencies installed, the firmware can be compiled as follows:

    $ cd pd-buddy-firmware
    $ make

This compiles the firmware to `build/pd-buddy-firmware.{bin,elf}`.

## Flashing

The firmware can be flashed in any number of ways, including but not limited to
the following:

### dfu-util

To flash via DfuSe (ST Microelectronics's extended version of the standard USB
DFU protocol), the Sink must first be put into DFU mode.  If upgrading from a
version older than 1.2.0, or if no firmware is installed, set the Boot switch
(SW1) on the PD Buddy Sink to the position not marked on the silkscreen to set
the device to DFU mode.  If your Sink doesn't have a Boot switch installed, you
can simply bridge the two pads of SW1 circled in [this image][dfu pads] with a
blob of solder to achieve the same effect.  Once this Sink is set to DFU mode,
plug it into your computer.

If upgrading from firmware version 1.2.0 or later, you can easily enter DFU
mode by running the following command from the Sink's configuration shell:

    PDBS) boot

This immediately puts the Sink in DFU mode until it is reset or unplugged.

When the Sink is in DFU mode, the Status LED should be glowing dimly.  The
firmware can then be flashed with:

    $ dfu-util -a 0 -s 0x08000000:leave -D build/pd-buddy-firmware.bin

If this fails with a message like "dfu-util: Cannot open DFU device 0483:df11",
it's likely that you don't have permissions to write to the device.  Try
running dfu-util as root if this is the case.

If applicable, don't forget to set the switch back to normal mode (or remove
the solder blob) after unplugging the device.

[dfu pads]: docs/dfu_pads.jpg

### stlink

If you have an ST-LINK/V2, you can use it to flash the firmware via SWD as
follows:

    $ st-flash write build/pd-buddy-firmware.bin 0x8000000

### OpenOCD

OpenOCD can also be used to flash the firmware.  If your debug probe is an
ST-LINK/V2, you can easily do this as follows:

    $ make flash-openocd-stlink

### Black Magic Probe

Black Magic Probe debuggers can be used to flash the firmware as well.  This
can be easily done as follows:

    $ make flash-bmp

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

For example, to configure the PD Buddy Sink to request 2.25 A at 20 V, run the
following commands:

    PDBS) set_v 20000
    PDBS) set_i 2250
    PDBS) write

When `write` is run, the chosen settings are written to flash.  You can then
simply disconnect the Sink from your computer.

For more information about the serial console configuration interface, refer to
[the full documentation][shell docs].

[GNU Screen]: https://www.gnu.org/software/screen/
[Minicom]: https://alioth.debian.org/projects/minicom
[PuTTY]: http://www.chiark.greenend.org.uk/~sgtatham/putty/
[shell docs]: docs/console_config.md

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

### Status LED descriptions

LED State                  | Mode  | Meaning
---------------------------|-------|---------------------------------------------------
Fast blink (4 blink/sec)   | Sink  | Negotiating USB Power Delivery contract
Medium blink (2 blink/sec) | Sink  | USB Power Delivery contract established, output on
Off                        | Sink  | Output on
On                         | Sink  | Unable to get configured power from source
Long blink (1 blink/2 sec) | Sink  | Only default USB power is available (e.g. powered with USB A to C cable)
Slow blink (1 blink/sec)   | Setup | Running in setup mode
Fast blink (4 blink/sec)   | Setup | `identify` command run
