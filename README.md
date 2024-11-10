**WARNING:** This module is under development and the master branch may be broken here and there. Please feel free to contribute (code, requests, bugs, ideas) via a opening an issue or mailing me at `tsx73a [AT] giddi.net`.

## Overview
This kernel driver is for exposing the ITE8528 embedded controller functionality on QNAP NAS devices via common kernel APIs. The goal is to get as many QNAP devices that use the ITE8528 EC to be supported, current development is done for support of the TS-x73A family while testing on the TS-473A (which is the only device I have available).

## How to install/build


### Install instructions using DKMS
~~First, check that the driver supports your hardware in the "Supported models" section, even if the driver supports your hadrware, it does not mean its been tested yet (but a configuration exists).~~

1. Download the latest release of the driver from the [releases page](https://github.com/0xGiddi/tsx73a-ec/releases/tag/v0.2-debug)
2. Extract the zip/tarball using `unzip <file>` or `tar xzvf <file>`
3. Enter the project directory `tsx73a-ec`
    
    3.1 *Optional*: Edit the makefile `src/Makefile` to remove debug prints (remove `CFLAGS_tsx73a-ec.o := -DDEBUG`) some prints (especially button reading and rapid LED setting can spam the kernel logs)
4. Compile and install the module using with `make install`

### Autoload module on startup
1. Create a new service file `touch /etc/systemd/system/tsx73a-ec-load-module.service`/
2. Open the file in your favorite editor and add the following:
```ini
[Unit]
Description=Install tsx73a-ec kernel module

[Service]
Type=oneshot
RemainAfterExit=yes
ExecStart=/sbin/modprobe tsx73a-ec
ExecStop=/sbin/modprobe -r tsx73a-ec

[Install]
WantedBy=multi-user.target
```
3. Enable the service `systemctl enable --now tsx73a-ec-load-module.service`
4. Check that module loaded properly `lsmod | grep tsx`

If module is not loaded, try `systemctl restart tsx73a-ec-load-module.service`

## Features NOT included in this driver

The following features are NOT included in this driver and are handled by existing drivers in the Linux kernel on my TS-473A NAS (other models may need different kernel modules):

- CPU package/die temperature sensors (handled by `k10temp`). 
- Hardware watchdog control (handled by `sp5100_tco`). 
- Built in buzzer sound generation (handled by `pcspkr`).


## Supported features:

#### Read EC firmware version
The ECs firmware version can be read from `/sys/devices/platform/tsx73a-ec/fw_version`.

#### Read CPLD version
CPLD version of the EC can be read from `/sys/devices/platform/tsx73a-ec/cpld_version`. 

#### Get/Set AC recovery mode
AC recovery mode can be read or set using `/sys/devices/platform/tsx73a-ec/ac_recovery` valid values are:
|Mode|value|
|-|-|
|Keep power off| 0
|Auto power on| 1
|Enter last power state| 2

#### Get/set EuP mode
EuP (Energy-using Products) mode can be read or set using `/sys/devices/platform/tsx73a-ec/eup_mode`, this effects other features such as WOL, power recovery (and power schedules  which are not implemented). valid values are:
|State|value|
|-|-|
|Disabled | 0
| Enabled | 1

#### Fan control
This driver exposes fan controls via HWMon, allowing `lm-sensors` to monitor the fans and third party tools to control fan speeds via PWM. This does not expose an API any settings associated with the ECs built in auto fan control (`TFAN`, if it even works). 

**NOTE:** Fans are controlled in groups, this driver exposes a PWM channel for each group of fans.

Fan speed in RPM can be read from `/sys/class/hwmon/hwmon<X>/fan<X>_input`

PWM fan speed control can be set and read from `/sys/class/hwmon/hwmon<X>/pwm<X>` (values `0`-`255`) 

#### Expose reset/copy buttons
The quick copy button on the front and reset button (in the back) are exposed as an input device. The button events (`EV_KEY`) are reported by the driver as `BTN_0` for reset and `BTN_1` for copy. 

**NOTE:** The EC is polled to get the state of the buttons at a rate of 10Hz (every 100ms), a button press less than 100ms might not register. Polling time might be configurable via a module parameter in the future. 

#### System LEDs
System LED such as the *Status*, *USB*, ~~*JBOD* and *10G*~~ and ~~*IDENT*~~ (depending on the NAS model) currently have basic support for turning on and off, hardware blink is not yet supported and software timers (using `ledtrig_timer`) are buggy at this time.

|LED|Color|Values|
|-|-|-|
|Status|Green/Red| 0/1/2 - OFF/GREEN/RED|
|USB|Blue| 0/1 - OFF/ON
|~~JBOD~~|~~Blue~~|~~0/1 - OFF/ON~~
|~~10G~~|~~?~~|~~0/1 - ON/OFF~~
|~~IDENT~~|~~Red~~|~~0/1 - ON/OFF~~

**NOTE:** Features with a strike are not implemented and maybe model specific. 


#### Disk LEDs
Disk LEDs also have some basic support like the system LEDs. SInce disk numbers is variable between NAS models, currently only the TS-473A configuration is implemented, more to come later. 

For static brightness, the same logic of the *Status* LED is used `0`/`1`/`2` for OFF/GREEN/RED.

**NOTE:** Some of the green disk LEDs blink on disk activity out-of-the-box (but not all of them), they seem to be controlled internally by the EC, this cannot be disabled (maybe some internal register?), even if the LED is set to static GREEN, it will blink with activity if that LED is strobes out-of-the-box. If the LED is set to RED or OFF, it will not blink with activity.

As with system LEDs, hardware blink is not supported yet and software blink has some bugs. 


#### Reading VPD entries
VPD data from the disk backplane and motherboard can be read, only a handful of VPD entries are available.

The following VPD entries are avaiable under `/sys/devices/platform/tsx73a-ec/vpd/`:

|Name| *sysfs* attribute | Example Data | Notes
|-|-|-|-|
|Motherboard name|`mb_name`| `SATA-6G-MB` |
|Motherboard model|`mb_model`| `70-0Q07D0250` | Used for configuration selection
|Motherboard vendor|`mb_vendor`| `QNAP Systems` |
|Motherboard manufacturer|`mb_manufacturer`| ` BTC Systems`
|Motherboard date|`mb_date`| | empty? 
|Motherboard serial|`mb_serial`| `223XXXXXXX` | 
|Backplane name|`mb_name`| `LF-SATA-BP` |
|Backplane model|`mb_model`| `70-1Q07N0200` | Used for configuration selection
|Backplane vendor|`mb_vendor`| `QNAP Systems` | 
|Backplane manufacturer|`mb_manufacturer`| ` BTC Systems` | 
|Backplane date|`mb_date`| `2023-05-22 16:14:00` | Manufacturing date?
|Backplane serial|`mb_serial`| `223XXXXXXXX` | 
|Enclosure Serial | `enc_serial` | `Q2XXXXXXXXX` | Device serial number (on sticker), located in backplane VPD table
|Enclosure nickname|`enc_nickname`|  | Can be written using QNAP software (empty by default?)

In addition, `dbg_tableX` (`X`=0-4) exist for debug purposes to print the entire VPD table for future development (may be changed/removed).

**NOTE:** VPD entries may change between models, especially since there are multiple VPD tables and the TS-473A only uses 2 of them. More tests on other devices are needed to figure out if this needs to be handled differently for each model.


#### SATA Power control
Allows power control over the HDD SATA ports, might be nice to have to power down a disk before a hotswap, currently not implemented. 

## Supported models (coming soon)

## What else
I want to try and make this driver be able to enumerate all it's supported hardware in the driver probe state and avoid using configurations, but this is probably not possible due to the non deterministic behavior of the devices and sensors (fan speed/pwm, disk led bit mapping and more).

Once the driver is in working order I'll publish more details about the device and how I got everything working, what can be learned from the reverse engineering process and how this can be expanded.
