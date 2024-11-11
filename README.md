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

## ~~Supported models~~ Models that will be supported
**Scroll left and right to view full table**
|Image|Model Name|MB/BP Code|Tested|AC Recovery|EuP Mode|Copy Btn|Reset Btn|Chassis Btn|LED Brightness|Status LED|10G LED|USB LED|JBOD LED|Locate LED|Disk LEDs
|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-
![]()|TVS-874|B6490/Q0AA0||❌|❌|✅|✅|❌|✅|✅|❌|✅|❌|✅|10/10
![]()|TVS-674|B6490/Q0BK0||❌|❌|✅|✅|❌|✅|✅|❌|✅|❌|✅|8/8
![]()|TVS-H874T|B6491/Q0AA0||❌|❌|✅|✅|❌|✅|✅|❌|✅|❌|✅|10/10
![]()|TVS-H674T|B6491/Q0BK0||❌|❌|✅|✅|❌|✅|✅|❌|✅|❌|✅|8/8
![]()|TVS-H874X|B6492/Q0AA0||❌|❌|✅|✅|❌|✅|✅|❌|✅|❌|✅|10/10
![]()|TVS-H674X|B6492/Q0BK0||❌|❌|✅|✅|❌|✅|✅|❌|✅|❌|✅|8/8
![]()|TS-983XU|Q00I1/Q00X0||✅|❌|❌|✅|❌|❌|✅|❌|❌|❌|✅|9/9
![]()|TS-2888X|Q00Q0/Q00S0||✅|✅|✅|✅|❌|✅|✅|❌|✅|❌|✅|28/28
![]()|TS-2483XU|Q00V1/Q00W0||✅|❌|❌|✅|❌|❌|✅|❌|❌|✅|✅|24/24
![]()|TVS-872XT|Q0120/Q0160||✅|❌|✅|✅|❌|✅|✅|❌|✅|❌|✅|10/10
![]()|TVS-672XT|Q0120/Q0170||✅|❌|✅|✅|❌|✅|✅|❌|✅|❌|✅|8/8
![]()|TVS-472XT|Q0120/Q0180||✅|❌|✅|✅|❌|✅|✅|❌|✅|❌|✅|6/6
![]()|TVS-872X|Q0121/Q0160||✅|❌|✅|✅|❌|✅|✅|❌|✅|❌|✅|10/10
![]()|TVS-672X|Q0121/Q0170||✅|❌|✅|✅|❌|✅|✅|❌|✅|❌|✅|8/8
![]()|TS-2490FU|Q03X0/Q04K0||✅|❌|❌|✅|❌|❌|✅|✅|❌|✅|✅|0/24
![]()|TNS-1083X|Q0410/Q0490||✅|❌|❌|✅|❌|❌|✅|❌|❌|✅|✅|10/12
![]()|TNS-C1083X|Q0411/Q0490||✅|❌|❌|✅|❌|❌|✅|❌|❌|✅|✅|10/12
![]()|TVS-872N|Q0420/Q0160||✅|❌|✅|✅|❌|✅|✅|❌|✅|❌|✅|10/10
![]()|TVS-672N|Q0420/Q0170||✅|❌|✅|✅|❌|✅|✅|❌|✅|❌|✅|8/8
![]()|TVS-472X|Q0420/Q0180||✅|❌|✅|✅|❌|✅|✅|❌|✅|❌|✅|6/6
![]()|TS-1886XU|Q0470/Q04L0||✅|❌|❌|✅|✅|❌|✅|✅|❌|✅|✅|18/18
![]()|TS-1673AU-RP|Q0520/Q0580||✅|❌|❌|✅|❌|❌|✅|❌|❌|✅|✅|0/16
![]()|TS-1273AU|Q0520/Q05G0||✅|❌|❌|✅|❌|❌|✅|❌|❌|✅|✅|0/12
![]()|TS-873AU|Q0520/Q05G1||✅|❌|❌|✅|❌|❌|✅|❌|❌|✅|✅|0/8
![]()|TS-1273AU-RP|Q0520/Q0670||✅|❌|❌|✅|❌|❌|✅|❌|❌|✅|✅|0/12
![]()|TS-873AU-RP|Q0520/Q0671||✅|❌|❌|✅|❌|❌|✅|❌|❌|✅|✅|0/8
![]()|TDS-2489FU|Q0530/Q0590||✅|❌|❌|✅|❌|❌|✅|❌|❌|✅|✅|24/26
![]()|TDS-2489FU R2|Q0531/Q0590||✅|❌|❌|✅|❌|❌|✅|❌|❌|✅|✅|24/26
![]()|TS-886|Q05S0/Q0650||✅|✅|✅|✅|❌|✅|✅|❌|✅|❌|✅|10/10
![]()|TS-686|Q05S0/Q0660||✅|✅|✅|✅|❌|✅|✅|❌|✅|❌|✅|8/8
![]()|TVS-1688X|Q05T0/Q0630||✅|✅|✅|✅|❌|✅|✅|❌|✅|❌|✅|18/18
![]()|TVS-1288X|Q05W0/Q05K0||✅|✅|✅|✅|❌|✅|✅|❌|✅|❌|✅|14/14
![]()|TS-3088XU|Q06X0/Q06Y0||✅|❌|❌|✅|✅|❌|✅|✅|❌|✅|✅|30/30
![]()|TS-973AX|Q0711/Q0760||✅|✅|✅|✅|❌|✅|✅|❌|✅|❌|✅|0/9
![]()|TS-873A|Q07D0/Q07L0||✅|✅|✅|✅|❌|✅|✅|❌|✅|❌|✅|10/10
![]()|TS-673A|Q07D0/Q07M0||✅|✅|✅|✅|❌|✅|✅|❌|✅|❌|✅|8/8
![]()|TS-473A|Q07D0/Q07N0|✅|✅|✅|✅|✅|❌|✅|✅|❌|✅|❌|✅|6/6
![]()|TS-1655|Q07Z1/Q08G0||✅|✅|✅|✅|❌|✅|✅|❌|✅|❌|✅|18/18
![]()|TS-2287XU|Q0840/Q08A0||❌|✅|❌|✅|❌|❌|✅|✅|❌|✅|✅|22/22
![]()|TS-1887XU|Q0840/Q0950||❌|✅|❌|✅|❌|❌|✅|✅|❌|✅|✅|18/18
![]()|TVS-675|Q08B0/Q0890||✅|✅|✅|✅|❌|✅|✅|❌|✅|❌|✅|8/8
![]()|TS-3087XU|Q08H0/Q08Z0||❌|✅|❌|✅|❌|❌|✅|✅|❌|✅|✅|30/30
![]()|TS-1290FX|Q09A0/Q09C0||✅|✅|✅|✅|❌|✅|✅|✅|✅|✅|✅|0/12
![]()|TS-1090FU|Q09B0/Q09I0||✅|❌|❌|✅|❌|❌|✅|❌|❌|✅|✅|0/10
![]()|TS-873AEU|Q0AK0/Q0AO0||✅|❌|❌|✅|❌|❌|✅|❌|❌|✅|✅|8/10
![]()|TS-873AEU-RP|Q0AK0/Q0AO1||✅|❌|❌|✅|❌|❌|✅|❌|❌|✅|✅|8/10
![]()|TS-1886XU R2|Q0B50/Q0950||✅|❌|❌|✅|❌|❌|✅|❌|❌|✅|✅|18/18
![]()|TVS-474|Q0BB0/Q0BL0||❌|❌|✅|✅|❌|✅|✅|❌|✅|❌|✅|6/6
![]()|TS-855EU|Q0BT0/Q0BU0||✅|✅|❌|✅|❌|❌|✅|❌|❌|✅|✅|8/10
![]()|TS-655X|Q0CH0/Q0CI0||✅|✅|✅|✅|❌|✅|✅|❌|✅|❌|✅|8/8
![]()|TS-855X|Q0CH0/Q0CJ0||✅|✅|✅|✅|❌|✅|✅|❌|✅|❌|✅|10/10
![]()|TES-1885U|QX540/QY270||✅|❌|❌|✅|✅|❌|✅|✅|❌|✅|❌|6/18
![]()|TES-3085U|QX541/QY510||✅|❌|❌|✅|✅|❌|✅|✅|❌|✅|❌|6/30
![]()|TS-1685|QY380/QY390||✅|✅|✅|✅|❌|✅|✅|❌|✅|❌|✅|22/22
![]()|TES-1685-SAS|QY380/QY390||✅|✅|✅|✅|❌|✅|✅|❌|✅|❌|✅|22/22
![]()|TS-977XU|QZ480/Q0060||✅|✅|✅|✅|❌|❌|✅|❌|❌|✅|✅|0/9
![]()|TS-1277XU|QZ490/QZ550||✅|✅|✅|✅|❌|❌|✅|❌|❌|✅|✅|0/12
![]()|TS-877XU|QZ490/QZ551||✅|✅|✅|✅|❌|❌|✅|❌|❌|✅|✅|0/8
![]()|TS-1677XU|QZ491/QZ540||✅|✅|✅|✅|❌|❌|✅|❌|❌|✅|✅|0/16
![]()|TS-2477XU|QZ500/Q0070||✅|✅|✅|✅|✅|❌|✅|❌|❌|✅|✅|0/24
![]()|TS-1683XU|QZ601/Q0040||✅|❌|❌|✅|❌|❌|✅|❌|❌|✅|✅|16/16
![]()|TS-1283XU|QZ601/Q00M0||✅|❌|❌|✅|❌|❌|✅|❌|❌|✅|✅|12/12
![]()|TS-883XU|QZ601/Q00M1||✅|❌|❌|✅|❌|❌|✅|❌|❌|✅|✅|8/8
![]()|TVS-875U|SAP00/SBO60||✅|❌|❌|✅|✅|❌|✅|❌|❌|✅|✅|8/10
![]()|TVS-1275U|SAP00/SBO70||✅|❌|❌|✅|✅|❌|✅|❌|❌|✅|✅|12/14
![]()|TVS-1675U|SAP00/SBO80||✅|❌|❌|✅|✅|❌|✅|❌|❌|✅|✅|16/18

## What else
I want to try and make this driver be able to enumerate all it's supported hardware in the driver probe state and avoid using configurations, but this is probably not possible due to the non deterministic behavior of the devices and sensors (fan speed/pwm, disk led bit mapping and more).

Once the driver is in working order I'll publish more details about the device and how I got everything working, what can be learned from the reverse engineering process and how this can be expanded.
