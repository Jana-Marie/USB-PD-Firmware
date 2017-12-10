# PD Buddy Sink Serial Console Configuration Interface

Version 1.2.0-dev, 2017-12-09

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

### Voltage Ranges

The preferred voltage may be set to any value for programmable power supplies.
This means uncommon voltages may be set, e.g. 13.8 V.  Few non-programmable
power supplies offer such a voltage.  To ensure a Sink configured this way can
still work with as many power supplies as possible, it is possible to configure
a closed range of acceptable voltages:

    PDBS) set_v 13800
    PDBS) set_vrange 11000 16000
    PDBS) get_cfg
    status: valid
    flags: (none)
    v: 13.80 V
    vmin: 11.00 V
    vmax: 16.00 V
    i: 2.25 A

If 12 V and 15 V are available from a power supply, the Sink would request 12 V
given this configuration.  If higher voltages from the range are preferred over
lower ones, it is possible to set this as well:

    PDBS) toggle_hv_preferred
    PDBS) get_cfg
    status: valid
    flags: HV_Preferred
    v: 13.80 V
    vmin: 11.00 V
    vmax: 16.00 V
    i: 2.25 A

To remove a configured voltage range, returning to a single desired voltage,
simply set the top and bottom of the range to 0 V.

### Alternate Configuration Types

While configuring a current to be requested at any voltage works well for some
use cases (e.g. linear regulators), for others, it may make more sense to set
the power required, or even the resistance of a resistive load.  As of firmware
version 1.2.0, the PD Buddy Sink supports setting these directly:

    PDBS) set_p 45000
    PDBS) get_tmpcfg
    status: valid
    flags: (none)
    v: 20.00 V
    p: 45.00 W
    PDBS) set_r 8889
    PDBS) get_tmpcfg
    status: valid
    flags: (none)
    v: 20.00 V
    r: 8.88 Ω

In either case, the device will set the current it requests according to the
configured value.  The value is kept constant across the entire configured
voltage range, allowing the current to vary accordingly.

## Commands

Commands are echoed on the terminal as characters are received.  Lines are
separated by `\r\n` and a command's output ends with the `PDBS) ` prompt.  The
character encoding used for text is UTF-8.

The command buffer can be cleared by sending ^D (a `\x04` character).  It is
recommended to do this at the start of programmatic communications to ensure
that the first command sent will be correctly processed.

If a received command is not recognized, the PD Buddy Sink responds with a line
repeating the unknown command with any arguments removed, followed by a space
and a question mark.  For example:

    PDBS) foo bar
    foo ?

### Miscellaneous

#### help

Usage: `help`

Prints short help messages about all available commands.

#### license

Usage: `license`

Prints licensing information for the firmware.

#### identify

Usage: `identify`

Blinks the LED quickly.  Useful for identifying which device you're connected
to if several are plugged in to your computer at once.

### Storage

#### get_cfg

Usage: `get_cfg [index]`

If no index is provided, prints the current configuration from flash.  If there
is no configuration, `No configuration` is printed instead.

For developers: if an index is provided, prints a particular location in the
configuration flash sector.  If the index lies outside the configuration flash
sector, `Invalid index` is printed instead.

#### load

Usage: `load`

Loads the current configuration from flash into the buffer.  Useful if you want
to change some settings while leaving others alone.  If there is no
configuration, `No configuration` is printed instead.

#### write

Usage: `write`

Synchronously writes the contents of the configuration buffer to flash.  Wear
leveling is done to ensure long flash life, and the flash sector is
automatically erased if necessary.

If the output is enabled, the newly written configuration is automatically
negotiated.  The newly configured power is then made available on the output
connector if it is available from the source.

#### erase

Usage: `erase`

Synchronously erases all stored configuration from flash.  This can be used to
restore a device to its default state.

Note: The `erase` command is mainly intended for development and testing.
Stored configuration is automatically erased if necessary when `write` is run,
and wear leveling is performed as well.  Unless you really know what you're
doing, there should be no reason to ever run `erase`.

### Configuration

#### get_tmpcfg

Usage: `get_tmpcfg`

Prints the contents of the configuration buffer.

#### clear_flags

Usage: `clear_flags`

Clears the configuration buffer flags that can be toggled with `toggle_*`
commands.

#### toggle_giveback

Usage: `toggle_giveback`

Toggles the GiveBack flag in the configuration buffer.  GiveBack allows the
power supply to temporarily remove power from the Sink's output if another
device needs more power.  Recommended if the Sink is being used to charge a
battery.

#### toggle_hv_preferred

Usage: `toggle_hv_preferred`

Toggles the HV_Preferred flag in the configuration buffer.  When enabled,
preference is given to higher voltages in the range.  When disabled, preference
is given to lower voltages.

#### set_v

Usage: `set_v voltage_in_mV`

Sets the voltage of the configuration buffer, in millivolts.  Prints no output
on success, an error message on failure.

Note: values are rounded down to the nearest 20 mV, 50 mV, or 100 mV for
various parts of the USB Power Delivery protocol.

#### set_vrange

Usage: `set_vrange min_voltage_in_mV max_voltage_in_mV`

Sets the minimum and maximum voltage of the configuration buffer, in
millivolts.  Prints no message on success, an error message on failure.

Note: values are rounded down to the nearest 20 mV, 50 mV, or 100 mV for
various parts of the USB Power Delivery protocol.

#### set_i

Usage: `set_i current_in_mA`

Sets the current of the configuration buffer, in milliamperes, overriding any
power or resistance configured.  Prints no output on success, an error message
on failure.

Note: values are rounded down to the nearest 10 mA.

#### set_p

Usage: `set_p power_in_mW`

Sets the power of the configuration buffer, in milliwatts, overriding any
current or resistance configured.  Prints no output on success, an error
message on failure.

Note: values are rounded down to the nearest 10 mW.

#### set_r

Usage: `set_r resistance_in_mΩ`

Sets the resistance of the configuration buffer, in milliohms, overriding any
current or power configured.  Prints no output on success, an error message on
failure.

Note: values are rounded down to the nearest 10 mΩ.

### Power Delivery

#### output

Usage: `output [enable|disable]`

If no argument is provided, prints the state of the power output (`enabled` or
`disabled`).

If an argument is provided, sets the output to the specified state.

This command only affects whether the output can be turned on during the
ongoing run of Setup mode.  The output is disabled in Setup mode by default,
and is always enabled in Sink mode.

#### get_source_cap

Usage: `get_source_cap`

Prints the capabilities advertised by the Power Delivery source.  Each
capability is represented by a Power Data Object (PDO), printed in the format
listed in the PDO Format section below.  If there are no capabilities, e.g.
when the source does not support USB Power Delivery, `No Source_Capabilities`
is printed instead.

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
* `HV_Preferred`: precedence is given to higher voltages when selecting from
  the range (lower voltages take precedence when the flag is disabled).

### v

The `v` field holds the preferred voltage of the configuration object, in
volts.
The field's value is a floating-point decimal number, followed by a space and a
capital V.  For example: `20.00 V`.

### vmin

The `vmin` field holds the lower end of the configuration object's voltage
range, in volts.
The field's value is a floating-point decimal number, followed by a space and a
capital V.  For example: `20.00 V`.

When absent, this field's value may be assumed to be `0 V`.

### vmax

The `vmax` field holds the upper end of the configuration object's voltage
range, in volts.
The field's value is a floating-point decimal number, followed by a space and a
capital V.  For example: `20.00 V`.

When absent, this field's value may be assumed to be `0 V`.

### i

The `i` field holds the current of the configuration object, in amperes.
The field's value is a floating-point decimal number, followed by a space and a
capital A.  For example: `2.25 A`.

### p

The `p` field holds the power of the configuration object, in watts.
The field's value is a floating-point decimal number, followed by a space and a
capital W.  For example: `2.25 W`.

### r

The `r` field holds the resistance of the configuration object, in ohms.
The field's value is a floating-point decimal number, followed by a space and a
capital Ω.  For example: `2.25 Ω`.

## PDO Format

When a list of PDOs is printed, each PDO is numbered with a line as follows:

    PDO n: type

`n` is the index of the PDO.  `type` is one of `fixed`, `typec_virtual`, or the
entire PDO represented as a 32-bit hexadecimal number if the type is unknown.
If `type` is not a hexadecimal number, the rest of the PDO is printed as a list
of fields, one per line, each indented by a single ASCII tab character.  Each
field is of the format:

    name: value

### Source Fixed Supply PDO Fields

This section describes how Source Fixed Supply PDOs (type `fixed`) are printed.
For more information about the meaning of each field, see the USB Power
Delivery Specification, Revision 2.0, Version 1.3, section 6.4.1.2.3.

#### dual_role_pwr

The `dual_role_pwr` field holds the value of the PDO's Dual-Role Power bit
(B29).  If this field is not present, its value shall be assumed 0.

#### usb_suspend

The `usb_suspend` field holds the value of the PDO's USB Suspend Supported bit
(B28).  If this field is not present, its value shall be assumed 0.

#### unconstrained_pwr

The `unconstrained_pwr` field holds the value of the PDO's Unconstrained Power
bit (B27).  If this field is not present, its value shall be assumed 0.

#### usb_comms

The `usb_comms` field holds the value of the PDO's USB Communications Capable
bit (B26).  If this field is not present, its value shall be assumed 0.

#### dual_role_data

The `dual_role_data` field holds the value of the PDO's Dual-Role Data bit
(B25).  If this field is not present, its value shall be assumed 0.

#### peak_i

The `peak_i` field holds the value of the PDO's Peak Current field (B21-20), in
decimal.  If this field is not present, its value shall be assumed 0.

#### v

The `v` field holds the value of the PDO's Voltage field (B19-10), in volts.
The field's value is a floating-point decimal number, followed by a space and a
capital V.  For example: `20.00 V`.

#### i

The `i` field holds the value of the PDO's Maximum Current field (B9-0), in
amperes.  The field's value is a floating-point decimal number, followed by a
space and a capital A.  For example: `2.25 A`.

### Type-C Current Virtual PDO Fields

This section describes how Type-C Current Virtual PDOs (type `typec_virtual`)
are printed.  These are not actually PDOs sent in a PD Source_Capabilities
message, but merely a convenient way of reporting advertised Type-C Current
when USB Power Delivery is not available.

#### i

The `i` field holds the value of the advertised Type-C Current, in amperes.
The field's value is a floating-point decimal number, followed by a space and a
capital A.  For example: `1.50 A`.

## USB Descriptors

The PD Buddy Sink can be identified by the following USB device descriptors:

* idVendor: 0x1209 (InterBiometrics, or [pid.codes][])
* idProduct: 0x9DB5 (PD Buddy Sink)

The device's firmware version number is given in the iSerial descriptor.  The
version number follows [Semantic Versioning][].  The serial console
configuration interface is the API that the version number describes.

Starting with firmware version 1.2.0, the version number can also be read using
the firmware version extension: an interface descriptor with class code `0xFF`,
subclass code `0x46`, and protocol `0x57` points to a string descriptor with
the firmware version.

[pid.codes]: http://pid.codes/
[Semantic Versioning]: http://semver.org/
