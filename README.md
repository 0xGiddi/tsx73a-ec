# UNDER ACTIVE DEVELOPMENT
Since this is in the early PoC stages, please do not use this if you are not trying to develop the driver. Main branch may not compile, crash the system or ruin the device.

Contributions are welcome.

# QNAP TS-x73A EC Driver

## Overview
This kernel driver is for exposing the ITE8528 embedded controller functionality on QNAP NAS devices via common kernel APIs. The goal is to get as many QNAP devices that use the ITE8528 EC to be supported, current development is done  for support of the TS-x73A family while testing on the TS-473A (which is the only device I have available).


## Features in development
 Task | Status | Description  |
-|-|-|
 Read VPD entries | :white_check_mark: PoC complete | Read vital product information entries from the VPD tables (Device serial, backplane/motherboard serial, product code and so on)
Read EC FW version| :white_check_mark: PoC complete | Get EC firmware version
Read CPLD version| :white_check_mark: PoC complete | Get CPLD version (not sure if is in use, but added anyway)
Get/Set AC recover| :white_check_mark: PoC complete | Get and set the AC power recovery mode in case of power failure(remain off, turn on, last state)
Get/Set EuP mode| :white_check_mark: PoC complete | Get and set the EuP (Energy using Product) mode. (affects Wake-on-Lan, AC Power recovery, power schedules)
Expose fans via HWMON | :white_check_mark: | Expose the fan PWM/Speed via HWMON. Used for monitoring with `lm-sensors` for example.
Expose temperature sensors via HWMON | :white_check_mark: | Expose the system temperature sensors via HWMON. Used for monitoring with `lm-sensors` for example [1] [2].
Expose fan control via PWM | WIP | Expose PWM interface for setting fan speeds manually. [3] [4] [5]
Read RESET/COPY buttons |  :white_check_mark: PoC complete | Expose the state of the front COPY and rear RESET buttons as an input device. From my research it seems there is not interrupt for these buttons, so they are polled at 100ms intervals [6]
LED global brightness |  :white_check_mark: PoC complete | A virtual LED exposed via the LED subsystem to control the bightness of all front panel LEDs.
Status LED | WIP | Expose the status LED (red/green/blink/alternate) via the LED subsystem [7]
USB LED | WIP |  Expose the blue USB LED (solid/blink/off)
Disk LEDs | WIP | Expose the disk LEDs (present/error/blink) [7]. Fix N.2 NVME activity?[8]
Enclosure IDENT LED | WIP | Expose the enclosure identification LED. The TS-x73A blinks all disk LEDs red for ident, so maybe not needed? But other models might have dedicated LED.
|----|----|----
SATA power control | Unknown | There seems to be sata power control to power up/down SATA disks slots, this is not officially supported (according to the missing entries in `/etc/model.con`) but it works. Maybe add it for replacing disks on the fly?


[1] The AMD CPU temperature is exposed via the `k10temp`` kernel driver (Tctl/Tdie).

[2] When testing, it seems that there are 3 temperature readings (indexes 5, 6 and 7) but I only located two physical temperature sensors on the main board. it might be two of the readings are identical. Requires more research.

[3] Currently the method for setting the fan speed works, however, I have not yet found a way to set the fans back to automatic control (based on sensor reading) other that reooting the system

[4] From the research, it seems there is a way to set the EC to run the fans based on a LOW/MED/HIGH temperature reading (TFAN settings), but I have not yet figured this out.

[5] There also seems to be a function for calibrating the temperature sensor, calibration procedure is still unknown.

[6] Note to self, maybe disable polling when input device is ot open to save on resources.

[7] For all multicolor LEDs, I have not decided how to properly expose them, since the status and disk LED colors are mutually exclusive (red OR green). I might expose them as two colored LEDs, my only problem is how to expose the alternate color (geed/red) flashing status LED. If you have ideas, let me know.

[8] The LEDs connected to SATA devices blink when disk is accessed, however the M.2 NVME leds stay constant and do not respond to read/write activity. Maybe create a custom trigger/hood the NVME block devices to blink the correct LEDs?, This will require matching the correct LED per NVME disk via PCI BDF, does not seem like a problem since each NVME is on its own PCI BDF, but can be a hassle, maybe add an interface to the user can pass the path to the device he wants to monitor with LED?

## Nice to have features for other models
- Get thunderbolt chip count / status
- Battery backup support/exists (if it's referring to builtin and not external UPS)
- OLED functions (FW version, buttons, status texts) - if done via EC
- Fan status LEDs
- Cover status  (open/closed)
- Buzzer mute (at boot - a BIOS setting)

## Things that this driver does not do:
- CPU temperature (no SYS) (use `k10temp` AMD cpu kernel driver)
- Making sounds with the buzzer (use the `pcspkr` interface, `beep` command)
- Handle the hardware watchdog (SP5100 TCO, use `sp5100_tco` kernel module)

## What else
I want to try and make this driver be able to enumerate all it's supported hardware in the driver probe state and avoid using configurations, but this is probably not possible due to the non deterministic behavior of the devices and sensors (fan speed/pwm, disk led bit mapping and more).

Once the driver is in working order I'll publish more details about the device and how I got everything working, what can be learned from the reverse engineering process and how this can be expanded.