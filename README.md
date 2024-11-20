- [Overview](#overview)
  - [Supported features](#supported-features)
- [Installation Instructions](#installation-instructions)
  - [Install instructions using DKMS](#install-instructions-using-dkms)
  - [Autoload module on startup with systemd](#autoload-module-on-startup-with-systemd)
  - [Removing the module](#removing-the-module)
- [Supported models](#supported-models)
- [Question and Answers](#question-and-answers)


## Overview


The qnap8528 project is a kernel module for exposing the ITE8528 embedded controller functionality on QNAP NAS devices via common kernel APIs and subsystems. The goal is to get as many QNAP devices that use the ITE8528 EC to be supported. This project has no affiliation with *QNAP Systems Inc.*.

### Supported features
✅ Reading EC firmware version and CPLD version\
✅ Changing AC power recovery mode\
✅ Changing EuP mode\
✅ Fan reporting and control via hwmon\
✅ Temperature reporting via hwmon\
✅ Copy/Reset/Chassis button inputs\
✅ System LED control (e.g. *status*, *usb*, *ident*, *jbod*)\
✅ Disk slot LED control\
✅ Reading VPD entries (e.g device serial no.)

Sounds amazing right? below are instruction to install and use this module for yourself! If you install this module, please be kind and run a few tests to verify that all the features are working properly and report back either by creating a new issue or emailing me at `qnap8528 [AT] giddi.net`.

## Installation Instructions
Before installing, please check the *Supported Models* table and see that your device is supported by this module, if your device is not yet supported, please check seethe Q&A for more information. The following instructions have been tested on *Debian 12 x64*.

**Disclaimer:** This kernel module is provided as-is, without any warranty of functionality or fitness for a specific purpose. The developers of this kernel module accept no liability for any damage, data loss, or system instability resulting from its use, Use at your own risk .
### Install instructions using DKMS

1. Download the latest release of the module from the [releases page](https://github.com/0xGiddi/tsx73a-ec/releases/latest) or clone the repository locally using `git clone https://github.com/0xgiddi/qnap8528.git`
2. Extract the zip/tarball using `unzip <file>`, `tar xzf <file>`
3. Enter the project directory `qnap8528`
4. Compile and install the module using with `make install`
5. Ensure the module is installed using `dkms status`

### Autoload module on startup with systemd
1. Create a new unit file `touch /etc/systemd/system/qnap8528-load-module.service`
2. Open the file in your favorite editor and add the following:
```ini
[Unit]
Description=Load qnap8528 EC kernel module

[Service]
Type=oneshot
RemainAfterExit=yes
ExecStart=/sbin/modprobe qnap8528
ExecStop=/sbin/modprobe -r qnap8528

[Install]
WantedBy=multi-user.target
```
3. Enable the service `systemctl enable --now qnap8528-load-module.service`
4. Check that module loaded properly `lsmod | grep qnap8528`

If module is not loaded, try `systemctl daemon-reload` and `systemctl start qnap8528-load-module.service`

### Removing the module
1. Stop and disable the service with `systemctl disable --now qnap8528-load-module.service`
2. Delete the service unit file `/etc/systemd/system/qnap8528-load-module.service`
3. use `make uninstall` to uninstall the kernel module
4. Verify with `dkms status` that the module is no longer installed

## Supported models

The following table lists all devices that have a valid configuration in the module and should 
work out-of-the-box. If you cannot find your model here, it either does not yet have a configuration entry
or does not use the IT8528 chip. Please check the Q&A for more information. 

|Model Name|MB Code|BP Code|Disk LEDs|Notes
|-|-|-|-|-|
|TVS-874|B6490|Q0AA0|10/10 |
|TVS-674|B6490|Q0BK0|8/8 |
|TVS-H874T|B6491|Q0AA0|10/10 |
|TVS-H674T|B6491|Q0BK0|8/8 |
|TVS-H874X|B6492|Q0AA0|10/10 |
|TVS-H674X|B6492|Q0BK0|8/8 |
|TS-983XU|Q00I1|Q00X0|9/9 |
|TS-2888X|Q00Q0|Q00S0|28/28 | ❗3rd code `Q00R0`, See *3
|TS-2483XU|Q00V1|Q00W0|24/24 |
|TVS-872XT|Q0120|Q0160|10/10 |
|TVS-672XT|Q0120|Q0170|8/8 |
|TVS-472XT|Q0120|Q0180|6/6 |
|TVS-872X|Q0121|Q0160|10/10 |
|TVS-672X|Q0121|Q0170|8/8 |
|TS-2490FU|Q03X0|Q04K0|24/24 |
|TNS-1083X|Q0410|Q0490|10/12 | ⚠️ [^1] ❗3rd code `Q04U0`, See *3
|TNS-C1083X|Q0411|Q0490|10/12 | ⚠️ [^1] ❗3rd code `Q04U0`, See *3
|TVS-872N|Q0420|Q0160|10/10 |
|TVS-672N|Q0420|Q0170|8/8 |
|TVS-472X|Q0420|Q0180|6/6 |
|TS-1886XU|Q0470|Q04L0|18/18 |
|TS-1673AU-RP|Q0520|Q0580|16/16 |
|TS-1273AU|Q0520|Q05G0|12/12 |
|TS-873AU|Q0520|Q05G1|8/8 |
|TS-1273AU-RP|Q0520|Q0670|12/12 |
|TS-873AU-RP|Q0520|Q0671|8/8 |
|TDS-2489FU|Q0530|Q0590|24/26 | ⚠️ See *1
|TDS-2489FU R2|Q0531|Q0590|24/26 | ⚠️ See *1
|TS-886|Q05S0|Q0650|10/10 |
|TS-686|Q05S0|Q0660|8/8 |
|TVS-1688X|Q05T0|Q0630|18/18 |
|TVS-1288X|Q05W0|Q05K0|14/14 |
|TS-3088XU|Q06X0|Q06Y0|30/30 |
|TS-973AX|Q0711|Q0760|9/9 |
|TS-873A|Q07D0|Q07L0|10/10 |🟩 Similar to TS-473A
|TS-673A|Q07D0|Q07M0|8/8 |🟩 Similar to TS-473A
|TS-473A|Q07D0|Q07N0|6/6 |  ✅ Tested 
|TS-1655|Q07Z1|Q08G0|18/18 |
|TS-2287XU|Q0840|Q08A0|22/22 |
|TS-1887XU|Q0840|Q0950|18/18 |
|TVS-675|Q08B0|Q0890|8/8 |
|TS-3087XU|Q08H0|Q08Z0|30/30 |
|TS-1290FX|Q09A0|Q09C0|12/12 |
|TS-1090FU|Q09B0|Q09I0|10/10 |
|TS-873AEU|Q0AK0|Q0AO0|8/10 | ⚠️ See *1, See *2
|TS-873AEU-RP|Q0AK0|Q0AO1|8/10 | ⚠️ See *1, See *2
|TS-1886XU R2|Q0B50|Q0950|18/18 |
|TVS-474|Q0BB0|Q0BL0|6/6 |
|TS-855EU|Q0BT0|Q0BU0|8/10 | ⚠️ See *1
|TS-655X|Q0CH0|Q0CI0|8/8 |
|TS-855X|Q0CH0|Q0CJ0|10/10 |
|TES-1885U|QX540|QY270|6/18 | ⚠️ See *1
|TES-3085U|QX541|QY510|6/30 | ⚠️ See *1
|TS-1685|QY380|QY390|22/22 |
|TES-1685-SAS|QY380|QY390|22/22 |
|TS-977XU|QZ480|Q0060|9/9 |
|TS-1277XU|QZ490|QZ550|12/12 |
|TS-877XU|QZ490|QZ551|8/8 |
|TS-1677XU|QZ491|QZ540|16/16 |
|TS-2477XU|QZ500|Q0070|24/24 |
|TS-1683XU|QZ601|Q0040|16/16 |
|TS-1283XU|QZ601|Q00M0|12/12 |
|TS-883XU|QZ601|Q00M1|8/8 |
|TVS-875U|SAP00|SBO60|8/10 | ⚠️ See *1
|TVS-1275U|SAP00|SBO70|12/14 | ⚠️ See *1
|TVS-1675U|SAP00|SBO80|16/18 | ⚠️ See *1

*1 Some or all disks LEDs are managed by other hardware (not the EC).\
*2 Some or all of the disks do not have a present or error (green/red) LED.\
*3 This device config file contains a 3rd code number which is not checked or tested. Might hint at use of VPD table 3 and 4?

## Question and Answers

**Q.** I can't find my model in the supported model list, can I use this module?\
**A.** *If your device uses the IT8528 chip, then probably yes, but a configuration will need to be added to the module. If you are not sure, please create an issue requesting support and include your device model*

**Q.**\ 
**A.**

**Q.**\
**A.**

**Q.**\
**A.**

**Q.**\
**A.**

**Q.**\
**A.**

**Q.**\
**A.**