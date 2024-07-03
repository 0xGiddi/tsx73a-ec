#!/usr/bin/env python3
import os
import sys
import configparser



def main(config_path):
    with open(config_path, 'r') as config_fp:
        sys.stderr.write(config_path + "\n")
        ini = configparser.ConfigParser()
        try:
            ini.read_string(config_fp.read().lower())
        except Exception as ex:
            print(f"Failed to parse config '{config_path}' - '{ex}'")
            return 

        model = ini["system enclosure"]["model"]
        fm_name = ini["system enclosure"]["display_fixed_model_name"] if "display_fixed_model_name" in ini["system enclosure"] else model
        if "sio_device" not in ini["system enclosure"] or ini["system enclosure"]["sio_device"] != "it8528":
            print(f"QNAP model '{model}' does not seem to use the IT8528 EC ('{config_path}')")
            return

        num_disks = ini["system enclosure"]["max_disk_num"]
        # Need to check for each fan if FAN_UNIT = EC
        num_fans = ini["system enclosure"]["max_fan_num"]
        num_cpu_fans = ini["system enclosure"]["max_cpu_fan_num"] if "max_cpu_fan_num" in ini["system enclosure"] else 0
        # Check only if ENV_TEMP_UNIT or ENV_TEMP_UNIT = EC
        num_temp_sensor = ini["system enclosure"]["max_temp_num"]
        ac_recovery = True if ini["system enclosure"]["pwr_recovery_unit"] == "ec" else False
        # Sone are just VPD - What does that mean?
        serial_no_location = ini["system enclosure"]["board_sn_device"]
        eup_status = True if "eup_status" in ini["system enclosure"] and ini["system enclosure"]["eup_status"] == "ec" else False
        copy_button = True if "usb_copy_button" in ini["system io"] and ini["system io"]["usb_copy_button"] == "ec" else False
        reset_button = True if ini["system io"]["reset_button"] == "ec" else False
        status_green = True if ini["system io"]["status_green_led"] == "ec" else False
        status_red = True if ini["system io"]["status_red_led"] == "ec" else False
        led_bv_interface = True if "led_bv_interface" in ini["system io"] and ini["system io"]["led_bv_interface"] == "ec" else False
        audio_mute = True if "audio_mute" in ini["system io"] and ini["system io"]["audio_mute"] == "ec" else False        

        
        # Parse sys fans
        _fans = [int(ini["system fan"][f"fan_{i}"].replace("i", ""))-1 for i in range(1, int(num_fans) + 1)] + [int(ini["cpu fan"][f"fan_{i}"].replace("i", ""))-1 for i in range(1, int(num_cpu_fans) + 1)]
        fan_mask = 0
        for i in _fans:
            fan_mask |= (1 << i)

        # pwm mask = fan mask? on 873 the second fan may be on the same wm controlle        
        pwm_mas = fan_mask

        # Parse disks

        #for i in range(1, int(num_disks) + 1):
            #print(ini[f"system disk {i}"][f"dev_bus"])
        
        #_disks = [ini[f"system disk {i}"]["slot_name"] for i in range(1, int(num_disks) + 1)]
        #if len(_disks) != num_disks:
        #    print(f"{model} - {len(_disks)}/{num_disks} '{config_path}'")
        _disks = []
        for i in range(1, int(num_disks)+1):
            d = ini[f"system disk {i}"]
            
            _disks.append({
                "disk":i,
                "name":d["slot_name"] if "slot_name" in d else "",
                "error":d["err_led"] if "err_led" in d else "",
                "present": d["present_led"] if "present_led" in d else "",
                "locate": d["locate_led"] if "locate_led" in d else "",
                "blink":d["blink_led"] if "blink_led" in d else "",
                "type":d["bus_type"] if "bus_type" in d else "",
                "bus_type":d["bus_type"] if "bus_type" in d else ""})
        if _disks[0]["error"].startswith("ec:"):
            pass

        #if len(_disks) != int(num_disks):
        #    print(f"{model} - {len(_disks)}/{num_disks} '{config_path}'")

        print(f"\n\nModel: %s\nFixed Name: %s\nSys Fans: %s\nCPU Fans: %s\nTemp sensors: %s\nAC Recovery: %r\nSerial location: %s\nEup Mode: %r\nCopy BTN: %r\nReset BTN: %r\nStatus GREEN: %r\nStatus RED: %r\nBrighness control: %r\nAudio mute: %r" % (model,fm_name,num_fans,num_cpu_fans,num_temp_sensor,ac_recovery,serial_no_location,eup_status,copy_button,reset_button,status_green,status_red,led_bv_interface,audio_mute))
        for z in _disks:    
            for key, value in z.items():
                if key == "disk":
                    print(f"\tDisk {value}")
                else:
                    print(f"\t\t{key}: {value}")
            





if __name__ == "__main__":
    if len(sys.argv) < 2:
        print(f"Usage: {os.path.basename(__file__)} <config file>")
        sys.exit(1)

    main(sys.argv[1])