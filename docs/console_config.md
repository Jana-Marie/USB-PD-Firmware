# PD Buddy Sink Serial Console Configuration Interface

Version 1.1.0-dev, 2017-07-27

The PD Buddy Sink can be put into setup mode by holding the Setup button while
plugging it into a computer.  In this mode, the device runs a configuration
interface over a USB CDC-ACM virtual serial port in addition to the usual USB
Power Delivery communications.  This allows the user to change the voltage
and current the Sink requests, as well as other settings related to the
device's operation.

## Quick Start

### Connecting to the Configuration Console

Connect to the PD Buddy Sink with your favorite serial console program, such as
[GNU Screen][], [Minicom][], or [PuTTY][].  On Linux, the device file will
probably be something like `/dev/ttyACM0`.  Any baud rate will work, as USB
CDC-ACM doesn't care what it's set to.  After connecting, press Enter and you
should be greeted with a `PDBS)` prompt.

[GNU Screen]: https://www.gnu.org/software/screen/
[Minicom]: https://alioth.debian.org/projects/minicom
[PuTTY]: http://www.chiark.greenend.org.uk/~sgtatham/putty/

### View the Saved Configuration

To see the configuration the device already has, run `get_cfg`:

    PDBS) get_cfg
    status: valid
    flags: (none)
    v: 9.00 V
    i: 3.00 A

If the Sink has no configuration, this will simply print `No configuration`.

### Setting Voltage and Current

The `set_v` and `set_i` commands allow you to set the voltage and current the
Sink will request.  The units used are millivolts and milliamperes.  For
example, to configure the device to request 2.25 A at 20 V, run the following
commands:

    PDBS) set_v 20000
    PDBS) set_i 2250

### Reviewing Configuration Changes

The changes made so far are held temporarily in RAM.  To review the temporary
configuration buffer, run `get_tmpcfg`:

    PDBS) get_tmpcfg
    status: valid
    flags: (none)
    v: 20.00 V
    i: 2.25 A

### Saving Configuration

The configuration buffer must be written to flash for the device to actually
request the selected voltage and current.  To do this, run:

    PDBS) write

As soon as the prompt reappears after running `write`, the changes have been
stored to flash, which can be verified with `get_cfg`.  The Sink may be safely
unplugged at any time.

## Commands

Commands are echoed on the terminal as characters are received.  Lines are
separated by `\r\n` and a command's output ends with the `PDBS) ` prompt.

The command buffer can be cleared by sending ^D (a `\x04` character).  It is
recommended to do this at the start of programmatic communications to ensure
that the first command sent will be correctly processed.

### help

Usage: `help`

Prints short help messages about all available commands.

### license

Usage: `license`

Prints licensing information for the firmware.

### erase

Usage: `erase`

Synchronously erases all stored configuration from flash.  This can be used to
restore a device to its default state.

Note: The `erase` command is mainly intended for development and testing.
Stored configuration is automatically erased if necessary when `write` is run,
and wear leveling is performed as well.  Unless you really know what you're
doing, there should be no reason to ever run `erase`.

### write

Usage: `write`

Synchronously writes the contents of the configuration buffer to flash.  Wear
leveling is done to ensure long flash life, and the flash sector is
automatically erased if necessary.

If the output is enabled, the newly written configuration is automatically
negotiated.  The newly configured power is then made available on the output
connector if it is available from the source.

### load

Usage: `load`

Loads the current configuration from flash into the buffer.  Useful if you want
to change some settings while leaving others alone.  If there is no
configuration, `No configuration` is printed instead.

### get_cfg

Usage: `get_cfg [index]`

If no index is provided, prints the current configuration from flash.  If there
is no configuration, `No configuration` is printed instead.

For developers: if an index is provided, prints a particular location in the
configuration flash sector.  If the index lies outside the configuration flash
sector, `Invalid index` is printed instead.

### get_tmpcfg

Usage: `get_tmpcfg`

Prints the contents of the configuration buffer.

### clear_flags

Usage: `clear_flags`

Clears all the flags in the configuration buffer.

### toggle_giveback

Usage: `toggle_giveback`

Toggles the GiveBack flag in the configuration buffer.  GiveBack allows the
power supply to temporarily remove power from the Sink's output if another
device needs more power.  Recommended if the Sink is being used to charge a
battery.

### set_v

Usage: `set_v voltage_in_mV`

Sets the voltage of the configuration buffer, in millivolts.  Prints no output
on success, an error message on failure.

Note: values are rounded down to the nearest Power Delivery voltage unit
(50 mV).

### set_i

Usage: `set_i current_in_mA`

Sets the current of the configuration buffer, in milliamperes.  Prints no
output on success, an error message on failure.

Note: values are rounded down to the nearest Power Delivery current unit
(10 mA).

### identify

Usage: `identify`

Blinks the LED quickly.  Useful for identifying which device you're connected
to if several are plugged in to your computer at once.

### output

Usage: `output [enable|disable]`

If no argument is provided, prints the state of the power output (`enabled` or
`disabled`).

If an argument is provided, sets the output to the specified state.

This command only affects whether the output can be turned on during the
ongoing run of Setup mode.  The output is disabled in Setup mode by default,
and is always enabled in Sink mode.

## Configuration Format

Wherever a configuration object is printed, the following format is used.

The configuration consists of a number of fields, one per line.  Each field is
of the format:

    name: value

Only the `status` field is mandatory.  Any or all other fields may be absent if
their values are not valid or relevant.

### status

The `status` field holds the name of the status of the printed configuration
object.  The possible names are:

* `empty`: A configuration object left empty after the last erase.
* `valid`: The configuration object that holds the current device settings.
* `invalid`: A configuration object that once held settings, but has been
  superseded.

### flags

The `flags` field holds zero or more flags.  If no flags are enabled, the
field's value is `(none)`.  Otherwise, the field's value is some combination of
the following words, separated by spaces, representing the flags enabled in
this configuration object:

* `GiveBack`: allows the power supply to temporarily reduce power to the device
  if necessary.

### v

The `v` field holds the fixed voltage of the configuration object, in volts.
The field's value is a floating-point decimal number, followed by a space and a
capital V.  For example: `20.00 V`

### i

The `i` field holds the fixed current of the configuration object, in amperes.
The field's value is a floating-point decimal number, followed by a space and a
capital A.  For example: `2.25 A`

## USB Descriptors

The PD Buddy Sink can be identified by the following USB device descriptors:

* idVendor: 0x1209 (InterBiometrics, or [pid.codes][])
* idProduct: 0x9DB5 (PD Buddy Sink)

The device's firmware version number is given in the iSerial descriptor.  The
version number follows [Semantic Versioning][].  The serial console
configuration interface is the API that the version number describes.

[pid.codes]: http://pid.codes/
[Semantic Versioning]: http://semver.org/
