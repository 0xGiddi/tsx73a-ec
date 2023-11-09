#define DRVNAME "tsx73a-ec"

#undef pr_fmt
#define pr_fmt(fmt) "%s @ %s: " fmt "\n", DRVNAME, __FUNCTION__

#define EC_MAX_RETRY    	1000	// Times to check EC status 
#define EC_CMD_PORT     	0x6c	// EC "Third Host Interface" command port
#define EC_DAT_PORT     	0x68	// EC "Third Host Interface" data port
#define EC_UDELAY       	300		// Delay between EC status read/clear 
#define EC_CHIP_ID      	0x8528	// ITE8528 chip id
#define EC_VPD_TABLE_SIZE	0x200	// Max VPD table size in bytes

#define EC_VPD_TABLE0_REG_A	0x56
#define EC_VPD_TABLE0_REG_B	0x57
#define EC_VPD_TABLE0_REG_C	0x58

#define EC_VPD_TABLE1_REG_A	0x59
#define EC_VPD_TABLE1_REG_B	0x5a
#define EC_VPD_TABLE1_REG_C	0x5b

#define EC_VPD_TABLE2_REG_A	0x5c
#define EC_VPD_TABLE2_REG_B	0x5d
#define EC_VPD_TABLE2_REG_C	0x5e

#define EC_VPD_TABLE3_REG_A	0x60
#define EC_VPD_TABLE3_REG_B	0x61
#define EC_VPD_TABLE3_REG_C	0x62

#define EC_MAX_TEMP_CHANNELS	64
#define EC_MAX_PWM_CHANNELS	64
#define EC_MAX_FAN_CHANNELS	64

#define EC_FW_VER_REGISTER	0x308
#define EC_FW_VER_LEN		8
#define EC_AC_RECOVER_REG	0x16
#define EC_EUP_SUPPRT_REG	0x101
#define EC_EUP_MODE_REG		0x121
#define EC_CPLD_VER_REG		0x320

/** 
 * EC VPD entries
 * 
 * There are 4 VPD tables, each table 512 byte in length. Each table
 * seems to hold information about a specific component of the system.
 * Only tables 0 and 1 seem to be used on the TS-x73A, being main board and
 * backplane VPD respetivly. The only outliers are the case serial and system
 * nickname which are located in thre backplane VPD. 
 * 
 * A VPD entry is located using the table number,offset withing the table,
 * length of the data and then parsed according to its type. 
 *
 * These entry values are extracted from libLinux_hal, most are unknown.
 * Using values on stickers, PCB silkscreen and common sense
 * the following have been figured out and  assigned an attribute.
 * 
 * The values bits are parsed as such:
 *  MSB																 LSB
 * 	1111    	11   			11  		11111111	1111111111111111
 *	Unkknown	table			type		length  	offset
 * 				0: Main PCB  	0 - str
 *	 			1: Backplane	1 - num
 * 				2/3: Not used	2 - date
 *  Example:
 *  VPD entry 0x0c1000cb = table 3, offset 203, length 16 and type 0
 *  0000 		11 				00 			00010000 	0000000011001011
 
 * 
 * Notes:
 * 	Tables 2 and 3 may be used for network and redundent power VPD
 *  as there are hints to it in hal_daemon/libLinux_hal.
 */

/* PVD entries exttracted from libLinux_hal that are known */
#define EC_VPD_MB_DATE		0x00203000		// Ta:00 Of:0b Ty:02 Le:03
#define EC_VPD_MB_MANUF 	0x0010000f      // Ta:00 Of:0f Ty:00 Le:10
#define EC_VPD_MB_NAME		0x00100020      // Ta:00 Of:20 Ty:00 Le:10
#define EC_VPD_MB_SERIAL	0x00100031      // Ta:00 Of:31 Ty:00 Le:10
#define EC_VPD_MB_MODEL		0x00200042      // Ta:00 Of:42 Ty:00 Le:20
#define EC_VPD_MB_VENDOR	0x0010007c      // Ta:00 Of:7c Ty:00 Le:10
#define EC_VPD_ENC_NICKNAME	0x001000d6      // Ta:00 Of:d6 Ty:00 Le:10
#define EC_VPD_ENC_SERIAL 	0x0410001d      // Ta:01 Of:1d Ty:00 Le:10
#define EC_VPD_BP_DATE	 	0x06030033      // Ta:01 Of:33 Ty:02 Le:03
#define EC_VPD_BP_MANUF 	0x04100037      // Ta:01 Of:37 Ty:00 Le:10
#define EC_VPD_BP_NAME 		0x04100048      // Ta:01 Of:48 Ty:00 Le:10
#define EC_VPD_BP_SERIAL 	0x04100059      // Ta:01 Of:59 Ty:00 Le:10
#define EC_VPD_BP_MODEL 	0x0420006a      // Ta:01 Of:6a Ty:00 Le:20
#define EC_VPD_BP_VENDOR 	0x04100094      // Ta:01 Of:94 Ty:00 Le:10
/* More VPD entries from libLinux_hal that are unknown at this time */
//#define EC_VPD_ 			0x01100064      // Ta:00 Of:64 Ty:01 Le:10
//#define EC_VPD_			0x0020008d      // Ta:00 Of:8d Ty:00 Le:20
//#define EC_VPD_			0x001000ae      // Ta:00 Of:ae Ty:00 Le:10
//#define EC_VPD_			0x000300bf      // Ta:00 Of:bf Ty:00 Le:03
//#define EC_VPD_			0x001000c3      // Ta:00 Of:c3 Ty:00 Le:10
//#define EC_VPD_			0x010400e7      // Ta:00 Of:e7 Ty:01 Le:04
//#define EC_VPD_			0x0501000a      // Ta:01 Of:0a Ty:01 Le:01
//#define EC_VPD_ 			0x0410000c      // Ta:01 Of:0c Ty:00 Le:10
//#define EC_VPD_			0x042000a5      // Ta:01 Of:a5 Ty:00 Le:20
//#define EC_VPD_			0x040100c6      // Ta:01 Of:c6 Ty:00 Le:01
//#define EC_VPD_			0x040200c8      // Ta:01 Of:c8 Ty:00 Le:02
//#define EC_VPD_			0x041000cb      // Ta:01 Of:cb Ty:00 Le:10
//#define EC_VPD_ 			0x0901000a      // Ta:02 Of:0a Ty:01 Le:01
//#define EC_VPD_ 			0x0810000c      // Ta:02 Of:0c Ty:00 Le:10
//#define EC_VPD_ 			0x0810001d      // Ta:02 Of:1d Ty:00 Le:10
//#define EC_VPD_ 			0x0a030033      // Ta:02 Of:33 Ty:02 Le:03
//#define EC_VPD_ 			0x08100037      // Ta:02 Of:37 Ty:00 Le:10
//#define EC_VPD_ 			0x08100048      // Ta:02 Of:48 Ty:00 Le:10
//#define EC_VPD_ 			0x08100059      // Ta:02 Of:59 Ty:00 Le:10
//#define EC_VPD_ 			0x0820006a      // Ta:02 Of:6a Ty:00 Le:20
//#define EC_VPD_ 			0x08100094      // Ta:02 Of:94 Ty:00 Le:10
//#define EC_VPD_ 			0x081000a5      // Ta:02 Of:a5 Ty:00 Le:10
//#define EC_VPD_ 			0x081000b6      // Ta:02 Of:b6 Ty:00 Le:10
//#define EC_VPD_ 			0x08030077      // Ta:02 Of:77 Ty:00 Le:03
//#define EC_VPD_ 			0x081000cb      // Ta:02 Of:cb Ty:00 Le:10
//#define EC_VPD_ 			0x0d01000a      // Ta:03 Of:0a Ty:01 Le:01
//#define EC_VPD_ 			0x0c10000c      // Ta:03 Of:0c Ty:00 Le:10
//#define EC_VPD_ 			0x0c10001d      // Ta:03 Of:1d Ty:00 Le:10
//#define EC_VPD_ 			0x0e030033      // Ta:03 Of:33 Ty:02 Le:03
//#define EC_VPD_ 			0x0c100037      // Ta:03 Of:37 Ty:00 Le:10
//#define EC_VPD_ 			0x0c100048      // Ta:03 Of:48 Ty:00 Le:10
//#define EC_VPD_ 			0x0c100059      // Ta:03 Of:59 Ty:00 Le:10
//#define EC_VPD_ 			0x0c20006a      // Ta:03 Of:6a Ty:00 Le:20
//#define EC_VPD_ 			0x0c100094      // Ta:03 Of:94 Ty:00 Le:10
//#define EC_VPD_ 			0x0c1000a5      // Ta:03 Of:a5 Ty:00 Le:10
//#define EC_VPD_ 			0x0c1000b6      // Ta:03 Of:b6 Ty:00 Le:10
//#define EC_VPD_ 			0x0c030077      // Ta:03 Of:77 Ty:00 Le:03
//#define EC_VPD_ 			0x0c1000cb      // Ta:03 Of:cb Ty:00 Le:10

struct ec_platform_data {
    
};

struct ec_vpd_entry {
	int table;
	int offset;
	int length;
	int type;
};

struct ec_vpd_attribute {
    struct attribute attr; 
    ssize_t (*show)(struct device *dev, struct ec_vpd_attribute *attr,
			char *buf);
    ssize_t (*store)(struct device *dev, struct ec_vpd_attribute *attr,
			 const char *buf, size_t count);
	struct ec_vpd_entry vpd;
};

#define __VPD_ATTR(_name, _mode, _show, _store, _vpd) {		\
	.attr = {.name = __stringify(_name),				    \
		 .mode = VERIFY_OCTAL_PERMISSIONS(_mode) },		    \
	.show	= _show,						                \
	.store	= _store,						                \
    .vpd = {.table = ((_vpd >> 0x1a) & 3), 		\
		.offset = (_vpd & 0xffff), 			\
		.length = ((_vpd >> 0x10) & 0xff), 	\
		.type = ((_vpd >> 0x18) & 3), },                                      		\
}

#define VPD_DEV_ATTR(_name, _mode, _show, _store, _vpd) \
	struct ec_vpd_attribute dev_attr_##_name = __VPD_ATTR(_name, _mode, _show, _store, _vpd)

static int ec_check_exists(void);
static int ec_wait_obf_set(void);
static int ec_wait_ibf_clear(void);
static int ec_clear_obf(void);
static int ec_send_command(u16 command);
static int ec_read_byte(u16 command, u8 *data);
static int ec_write_byte(u16 command, u8 data);
static int ec_driver_probe(struct platform_device *pdev);
static int ec_driver_remove(struct platform_device *pdev);
static ssize_t ec_vpd_table_show(struct device *dev, struct ec_vpd_attribute *attr, char *buf);
static ssize_t ec_vpd_entry_show(struct device *dev, struct ec_vpd_attribute *attr, char *buf);
static ssize_t ec_vpd_entry_store(struct device *dev, struct ec_vpd_attribute *attr, const char *buf, size_t count);
static umode_t ec_hwmon_is_visible(const void* const_data, enum hwmon_sensor_types type, u32 attribute, int channel);
static int ec_hwmon_read(struct device *, enum hwmon_sensor_types type, u32 attr, int, long *);
static int ec_hwmon_write(struct device *, enum hwmon_sensor_types type, u32 attr, int, long);
static int ec_hwmon_read_string(struct device *dev, enum hwmon_sensor_types type, u32 attr, int channel, const char **str);
static ssize_t ec_ac_recovery_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t ec_ac_recovery_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
static ssize_t ec_fw_version_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t ec_eup_mode_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t ec_eup_mode_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
static ssize_t ec_cpld_version_show(struct device *dev, struct device_attribute *attr, char *buf);

static int __init tsx73a_init(void);
static void __exit tsx73a_exit(void);

/*
[0 ... 4]   ->CPU 
[5 ... 9]   ->SYS 
[10 ... 14] ->POWER 
[15 ... 38] ->ENV
*/


// SYS 5  6  7
//     27 27 35
