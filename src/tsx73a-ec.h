/* SPDX-License-Identifier: GPL-2.0-or-later */
#define DRVNAME "tsx73a-ec"

#undef pr_fmt
#define pr_fmt(fmt) "%s @ %s: " fmt "\n", DRVNAME, __FUNCTION__

#define EC_MAX_RETRY    	1000	// Times to check EC status
#define EC_CMD_PORT     	0x6c	// EC "Third Host Interface" command port
#define EC_DAT_PORT     	0x68	// EC "Third Host Interface" data port
#define EC_UDELAY       	300		// Delay between EC status read/clear WARN: Under 100 results are unstable
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

#define EC_BTN_RESET		4
#define EC_BTN_COPY		2

#define TSX73A_MAX_HWMON_CHANNELS 64


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

struct qnap_led_classdev {
	struct led_classdev cdev;
	u8 ec_index;
};


struct qnap_code_match {
	char *code;
	unsigned int offset;
	unsigned int length;
};

struct qnap_slot_config {
	char *name;
	u8 ec_index;
};

struct qnap_model_config {
	char *model_name;					// Model name
	struct qnap_code_match bp_code;	// Model matching BP code
	struct qnap_code_match mb_code;	// Model matching MB code

	u64 temp_mask;						// Bitmask of supported temperature channels
	u64 fan_mask;						// Bitmask of supported fan channels
	u64 pwm_mask; 						// Bitmask of supported PWM channels (multiple fan can be on a single PWM driver)

	struct qnap_slot_config disk_slots[33];
};



static struct qnap_model_config tsx73a_configs[] = {
	{
		.model_name = "TS-473A",
		.mb_code = {
			.code = "Q07D0",
			.offset = 4,
			.length = 5,
			},
		.bp_code = {
			.code = "Q07N0",
			.offset = 4,
			.length = 5
			},
		/*.slots = {
			{.index = 1, .ec_bit = 9, .type = 1},
			{.index = 2, .ec_bit = 10, .type = 1},
			{.index = 1, .ec_bit = 1, .type = 0},
			{.index = 2, .ec_bit = 2, .type = 0},
			{.index = 3, .ec_bit = 3, .type = 0},
			{.index = 4, .ec_bit = 4, .type = 0},
		},*/
		.temp_mask = 0x0000000000000e1,
		.fan_mask = 0x0000000000000041,
		.pwm_mask = 0x0000000000000041,
		.disk_slots = { {.name = "m2ssd1", .ec_index = 9}, {.name = "m2ssd2", .ec_index = 10}, {.name = "disk1", .ec_index = 1}, {.name = "disk2", .ec_index = 2}, {.name = "disk3", .ec_index = 3}, {.name = "disk4", .ec_index = 4}, {NULL} }
	},
	{
		.model_name = "TS-673A",
		.mb_code = {
			.code = "Q07D0",
			.offset = 4,
			.length = 5,
			},
		.bp_code = {
			.code = "Q07M0",
			.offset = 4,
			.length = 5
			},
		.temp_mask = 0x000000000000001e,
		.fan_mask = 0x0000000000000041,
		.pwm_mask = 0x0000000000000041,
		.disk_slots = { {.name = "m2ssd1", .ec_index = 9}, {.name = "m2ssd2", .ec_index = 10}, {.name = "disk1", .ec_index = 1}, {.name = "disk2", .ec_index = 2}, {.name = "disk3", .ec_index = 3}, {.name = "disk4", .ec_index = 4}, {.name = "disk5", .ec_index = 5}, {.name = "disk6", .ec_index = 6}, {NULL} }
	},
	{NULL}
};



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
//static ssize_t ec_vpd_entry_store(struct device *dev, struct ec_vpd_attribute *attr, const char *buf, size_t count);
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

static int ec_get_fan_status(unsigned int fan);
static int ec_get_fan_rpm(unsigned int fan);
static int ec_get_fan_pwm(unsigned int fan);
static int ec_set_fan_pwm(unsigned int fan, u8 value);
static int ec_get_temperature(unsigned int sensor);
static int ec_button_get_state(void);
static int ec_led_set_brightness(u8 brightness);
static int ec_led_set_status(u8 mode);
static int ec_led_set_disk(u8 mode);


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
//
#define QNAP8528_MAX_SLOTS  30

struct qnap8528_features {
    // Misc
    u32 ac_recovery:1;
    u32 eup_mode:1;
    // Buttons
    u32 btn_copy:1;
    u32 btn_reset:1;
    u32 btn_chassis:1;
    // LEDs
    u32 led_brightness:1;
    u32 led_status:1;
    u32 led_10g:1;
    u32 led_usb:1;
    u32 led_jbod:1;
    u32 led_locate:1;
    // TODO: Board serial location (VPD, VPD_MB, VPD_BP)
};

struct qnap8528_disk_slot {
    char *name;
    u8 ec_index;
    u8 has_present:1;
    u8 has_active:1;
    u8 has_error:1;
    u8 has_locate:1;
};

struct qnap8528_model_config {
    char *name;
    char *mb_code;
    char *bp_code;
    struct qnap8528_features features;
    struct qnap8528_disk_slot slots[QNAP8528_MAX_SLOTS + 1];
};

static struct qnap8528_model_config model_configs[] = {
    {
        .name = "TDS-2489FU", .mb_code = "Q0530", .bp_code = "Q0590",
        .features = {
            .ac_recovery    = 1, 
            .eup_mode       = 0,
            .btn_copy       = 0,
            .btn_reset      = 1,
            .btn_chassis    = 0,
            .led_brightness = 0,
            .led_status     = 1,
            .led_10g        = 0,
            .led_usb        = 0,
            .led_jbod       = 1,
            .led_locate     = 1,
        },
        .slots = {
			{ .name = "u2ssd1", .ec_index = 1, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd2", .ec_index = 2, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd3", .ec_index = 3, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd4", .ec_index = 4, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd5", .ec_index = 5, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd6", .ec_index = 6, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd7", .ec_index = 7, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd8", .ec_index = 8, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd9", .ec_index = 9, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd10", .ec_index = 10, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd11", .ec_index = 11, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd12", .ec_index = 12, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd13", .ec_index = 13, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd14", .ec_index = 14, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd15", .ec_index = 15, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd16", .ec_index = 16, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd17", .ec_index = 17, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd18", .ec_index = 18, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd19", .ec_index = 19, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd20", .ec_index = 20, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd21", .ec_index = 21, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd22", .ec_index = 22, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd23", .ec_index = 23, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd24", .ec_index = 24, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
            { NULL }
        }
    },


    {
        .name = "TDS-2489FU R2", .mb_code = "Q0531", .bp_code = "Q0590",
        .features = {
            .ac_recovery    = 1, 
            .eup_mode       = 0,
            .btn_copy       = 0,
            .btn_reset      = 1,
            .btn_chassis    = 0,
            .led_brightness = 0,
            .led_status     = 1,
            .led_10g        = 0,
            .led_usb        = 0,
            .led_jbod       = 1,
            .led_locate     = 1,
        },
        .slots = {
			{ .name = "u2ssd1", .ec_index = 1, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd2", .ec_index = 2, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd3", .ec_index = 3, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd4", .ec_index = 4, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd5", .ec_index = 5, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd6", .ec_index = 6, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd7", .ec_index = 7, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd8", .ec_index = 8, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd9", .ec_index = 9, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd10", .ec_index = 10, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd11", .ec_index = 11, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd12", .ec_index = 12, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd13", .ec_index = 13, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd14", .ec_index = 14, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd15", .ec_index = 15, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd16", .ec_index = 16, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd17", .ec_index = 17, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd18", .ec_index = 18, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd19", .ec_index = 19, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd20", .ec_index = 20, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd21", .ec_index = 21, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd22", .ec_index = 22, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd23", .ec_index = 23, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd24", .ec_index = 24, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
            { NULL }
        }
    },


    {
        .name = "TES-1685-SAS", .mb_code = "QY380", .bp_code = "QY390",
        .features = {
            .ac_recovery    = 1, 
            .eup_mode       = 1,
            .btn_copy       = 1,
            .btn_reset      = 1,
            .btn_chassis    = 0,
            .led_brightness = 1,
            .led_status     = 1,
            .led_10g        = 0,
            .led_usb        = 1,
            .led_jbod       = 0,
            .led_locate     = 1,
        },
        .slots = {
			{ .name = "m2ssd1", .ec_index = 17, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "m2ssd2", .ec_index = 18, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "m2ssd3", .ec_index = 19, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "m2ssd4", .ec_index = 20, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "m2ssd5", .ec_index = 21, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "m2ssd6", .ec_index = 22, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd1", .ec_index = 13, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd2", .ec_index = 14, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd3", .ec_index = 15, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd4", .ec_index = 16, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd1", .ec_index = 1, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd2", .ec_index = 2, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd3", .ec_index = 3, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd4", .ec_index = 4, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd5", .ec_index = 5, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd6", .ec_index = 6, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd7", .ec_index = 7, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd8", .ec_index = 8, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd9", .ec_index = 9, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd10", .ec_index = 10, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd11", .ec_index = 11, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd12", .ec_index = 12, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
            { NULL }
        }
    },


    {
        .name = "TES-1885U", .mb_code = "QX540", .bp_code = "QY270",
        .features = {
            .ac_recovery    = 1, 
            .eup_mode       = 0,
            .btn_copy       = 0,
            .btn_reset      = 1,
            .btn_chassis    = 1,
            .led_brightness = 0,
            .led_status     = 1,
            .led_10g        = 1,
            .led_usb        = 0,
            .led_jbod       = 1,
            .led_locate     = 0,
        },
        .slots = {
			{ .name = "ssd1", .ec_index = 4, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd2", .ec_index = 5, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd3", .ec_index = 6, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd4", .ec_index = 1, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd5", .ec_index = 2, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd6", .ec_index = 3, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
            { NULL }
        }
    },


    {
        .name = "TES-3085U", .mb_code = "QX541", .bp_code = "QY510",
        .features = {
            .ac_recovery    = 1, 
            .eup_mode       = 0,
            .btn_copy       = 0,
            .btn_reset      = 1,
            .btn_chassis    = 1,
            .led_brightness = 0,
            .led_status     = 1,
            .led_10g        = 1,
            .led_usb        = 0,
            .led_jbod       = 1,
            .led_locate     = 0,
        },
        .slots = {
			{ .name = "ssd1", .ec_index = 4, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd2", .ec_index = 5, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd3", .ec_index = 6, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd4", .ec_index = 1, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd5", .ec_index = 2, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd6", .ec_index = 3, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
            { NULL }
        }
    },


    {
        .name = "TNS-1083X", .mb_code = "Q0410", .bp_code = "Q0490",
        .features = {
            .ac_recovery    = 1, 
            .eup_mode       = 0,
            .btn_copy       = 0,
            .btn_reset      = 1,
            .btn_chassis    = 0,
            .led_brightness = 0,
            .led_status     = 1,
            .led_10g        = 0,
            .led_usb        = 0,
            .led_jbod       = 1,
            .led_locate     = 1,
        },
        .slots = {
			{ .name = "hdd1", .ec_index = 1, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd2", .ec_index = 2, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd3", .ec_index = 3, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd4", .ec_index = 4, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd5", .ec_index = 5, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd6", .ec_index = 6, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd7", .ec_index = 7, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd8", .ec_index = 8, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd1", .ec_index = 9, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd2", .ec_index = 10, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
            { NULL }
        }
    },


    {
        .name = "TNS-C1083X", .mb_code = "Q0411", .bp_code = "Q0490",
        .features = {
            .ac_recovery    = 1, 
            .eup_mode       = 0,
            .btn_copy       = 0,
            .btn_reset      = 1,
            .btn_chassis    = 0,
            .led_brightness = 0,
            .led_status     = 1,
            .led_10g        = 0,
            .led_usb        = 0,
            .led_jbod       = 1,
            .led_locate     = 1,
        },
        .slots = {
			{ .name = "hdd1", .ec_index = 1, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd2", .ec_index = 2, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd3", .ec_index = 3, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd4", .ec_index = 4, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd5", .ec_index = 5, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd6", .ec_index = 6, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd7", .ec_index = 7, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd8", .ec_index = 8, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd1", .ec_index = 9, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd2", .ec_index = 10, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
            { NULL }
        }
    },


    {
        .name = "TS-1090FU", .mb_code = "Q09B0", .bp_code = "Q09I0",
        .features = {
            .ac_recovery    = 1, 
            .eup_mode       = 0,
            .btn_copy       = 0,
            .btn_reset      = 1,
            .btn_chassis    = 0,
            .led_brightness = 0,
            .led_status     = 1,
            .led_10g        = 0,
            .led_usb        = 0,
            .led_jbod       = 1,
            .led_locate     = 1,
        },
        .slots = {
			{ .name = "u2ssd1", .ec_index = 1, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd2", .ec_index = 2, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd3", .ec_index = 3, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd4", .ec_index = 4, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd5", .ec_index = 5, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd6", .ec_index = 6, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd7", .ec_index = 7, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd8", .ec_index = 8, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd9", .ec_index = 9, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd10", .ec_index = 10, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
            { NULL }
        }
    },


    {
        .name = "TS-1273AU", .mb_code = "Q0520", .bp_code = "Q05G0",
        .features = {
            .ac_recovery    = 1, 
            .eup_mode       = 0,
            .btn_copy       = 0,
            .btn_reset      = 1,
            .btn_chassis    = 0,
            .led_brightness = 0,
            .led_status     = 1,
            .led_10g        = 0,
            .led_usb        = 0,
            .led_jbod       = 1,
            .led_locate     = 1,
        },
        .slots = {
			{ .name = "", .ec_index = 1, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "", .ec_index = 2, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "", .ec_index = 3, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "", .ec_index = 4, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "", .ec_index = 5, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "", .ec_index = 6, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "", .ec_index = 7, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "", .ec_index = 8, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "", .ec_index = 9, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "", .ec_index = 10, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "", .ec_index = 11, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "", .ec_index = 12, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
            { NULL }
        }
    },


    {
        .name = "TS-1273AU-RP", .mb_code = "Q0520", .bp_code = "Q0670",
        .features = {
            .ac_recovery    = 1, 
            .eup_mode       = 0,
            .btn_copy       = 0,
            .btn_reset      = 1,
            .btn_chassis    = 0,
            .led_brightness = 0,
            .led_status     = 1,
            .led_10g        = 0,
            .led_usb        = 0,
            .led_jbod       = 1,
            .led_locate     = 1,
        },
        .slots = {
			{ .name = "", .ec_index = 1, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "", .ec_index = 2, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "", .ec_index = 3, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "", .ec_index = 4, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "", .ec_index = 5, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "", .ec_index = 6, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "", .ec_index = 7, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "", .ec_index = 8, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "", .ec_index = 9, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "", .ec_index = 10, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "", .ec_index = 11, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "", .ec_index = 12, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
            { NULL }
        }
    },


    {
        .name = "TS-1277XU", .mb_code = "QZ494", .bp_code = "QZ550",
        .features = {
            .ac_recovery    = 1, 
            .eup_mode       = 1,
            .btn_copy       = 1,
            .btn_reset      = 1,
            .btn_chassis    = 0,
            .led_brightness = 0,
            .led_status     = 1,
            .led_10g        = 0,
            .led_usb        = 0,
            .led_jbod       = 1,
            .led_locate     = 1,
        },
        .slots = {
			{ .name = "hdd1", .ec_index = 1, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd2", .ec_index = 2, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd3", .ec_index = 3, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd4", .ec_index = 4, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd5", .ec_index = 5, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd6", .ec_index = 6, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd7", .ec_index = 7, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd8", .ec_index = 8, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd9", .ec_index = 9, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd10", .ec_index = 10, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd11", .ec_index = 11, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd12", .ec_index = 12, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
            { NULL }
        }
    },


    {
        .name = "TS-1283XU", .mb_code = "QZ602", .bp_code = "Q00M0",
        .features = {
            .ac_recovery    = 1, 
            .eup_mode       = 0,
            .btn_copy       = 0,
            .btn_reset      = 1,
            .btn_chassis    = 0,
            .led_brightness = 0,
            .led_status     = 1,
            .led_10g        = 0,
            .led_usb        = 0,
            .led_jbod       = 1,
            .led_locate     = 1,
        },
        .slots = {
			{ .name = "hdd1", .ec_index = 1, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd2", .ec_index = 2, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd3", .ec_index = 3, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd4", .ec_index = 4, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd5", .ec_index = 5, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd6", .ec_index = 6, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd7", .ec_index = 7, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd8", .ec_index = 8, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd9", .ec_index = 9, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd10", .ec_index = 10, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd11", .ec_index = 11, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd12", .ec_index = 12, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
            { NULL }
        }
    },


    {
        .name = "TS-1290FX", .mb_code = "Q09A0", .bp_code = "Q09C0",
        .features = {
            .ac_recovery    = 1, 
            .eup_mode       = 1,
            .btn_copy       = 1,
            .btn_reset      = 1,
            .btn_chassis    = 0,
            .led_brightness = 1,
            .led_status     = 1,
            .led_10g        = 1,
            .led_usb        = 1,
            .led_jbod       = 1,
            .led_locate     = 1,
        },
        .slots = {
			{ .name = "u2ssd1", .ec_index = 1, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd2", .ec_index = 2, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd3", .ec_index = 3, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd4", .ec_index = 4, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd5", .ec_index = 5, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd6", .ec_index = 6, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd7", .ec_index = 7, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd8", .ec_index = 8, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd9", .ec_index = 9, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd10", .ec_index = 10, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd11", .ec_index = 11, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd12", .ec_index = 12, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
            { NULL }
        }
    },


    {
        .name = "TS-1655", .mb_code = "Q07Z1", .bp_code = "Q08G0",
        .features = {
            .ac_recovery    = 1, 
            .eup_mode       = 1,
            .btn_copy       = 1,
            .btn_reset      = 1,
            .btn_chassis    = 0,
            .led_brightness = 1,
            .led_status     = 1,
            .led_10g        = 0,
            .led_usb        = 1,
            .led_jbod       = 0,
            .led_locate     = 1,
        },
        .slots = {
			{ .name = "m2ssd1", .ec_index = 17, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "m2ssd2", .ec_index = 18, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd1", .ec_index = 13, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd2", .ec_index = 14, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd3", .ec_index = 15, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd4", .ec_index = 16, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd1", .ec_index = 1, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd2", .ec_index = 2, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd3", .ec_index = 3, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd4", .ec_index = 4, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd5", .ec_index = 5, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd6", .ec_index = 6, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd7", .ec_index = 7, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd8", .ec_index = 8, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd9", .ec_index = 9, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd10", .ec_index = 10, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd11", .ec_index = 11, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd12", .ec_index = 12, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
            { NULL }
        }
    },


    {
        .name = "TS-1673AU-RP", .mb_code = "Q0580", .bp_code = "Q0671",
        .features = {
            .ac_recovery    = 1, 
            .eup_mode       = 0,
            .btn_copy       = 0,
            .btn_reset      = 1,
            .btn_chassis    = 0,
            .led_brightness = 0,
            .led_status     = 1,
            .led_10g        = 0,
            .led_usb        = 0,
            .led_jbod       = 1,
            .led_locate     = 1,
        },
        .slots = {
			{ .name = "", .ec_index = 1, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "", .ec_index = 2, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "", .ec_index = 3, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "", .ec_index = 4, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "", .ec_index = 5, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "", .ec_index = 6, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "", .ec_index = 7, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "", .ec_index = 8, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "", .ec_index = 9, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "", .ec_index = 10, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "", .ec_index = 11, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "", .ec_index = 12, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "", .ec_index = 13, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "", .ec_index = 14, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "", .ec_index = 15, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "", .ec_index = 16, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
            { NULL }
        }
    },


    {
        .name = "TS-1677XU", .mb_code = "QZ494", .bp_code = "QZ540",
        .features = {
            .ac_recovery    = 1, 
            .eup_mode       = 1,
            .btn_copy       = 1,
            .btn_reset      = 1,
            .btn_chassis    = 0,
            .led_brightness = 0,
            .led_status     = 1,
            .led_10g        = 0,
            .led_usb        = 0,
            .led_jbod       = 1,
            .led_locate     = 1,
        },
        .slots = {
			{ .name = "hdd1", .ec_index = 1, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd2", .ec_index = 2, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd3", .ec_index = 3, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd4", .ec_index = 4, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd5", .ec_index = 5, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd6", .ec_index = 6, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd7", .ec_index = 7, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd8", .ec_index = 8, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd9", .ec_index = 9, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd10", .ec_index = 10, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd11", .ec_index = 11, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd12", .ec_index = 12, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd13", .ec_index = 13, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd14", .ec_index = 14, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd15", .ec_index = 15, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd16", .ec_index = 16, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
            { NULL }
        }
    },


    {
        .name = "TS-1683XU", .mb_code = "QZ602", .bp_code = "Q0040",
        .features = {
            .ac_recovery    = 1, 
            .eup_mode       = 0,
            .btn_copy       = 0,
            .btn_reset      = 1,
            .btn_chassis    = 0,
            .led_brightness = 0,
            .led_status     = 1,
            .led_10g        = 0,
            .led_usb        = 0,
            .led_jbod       = 1,
            .led_locate     = 1,
        },
        .slots = {
			{ .name = "hdd1", .ec_index = 1, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd2", .ec_index = 2, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd3", .ec_index = 3, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd4", .ec_index = 4, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd5", .ec_index = 5, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd6", .ec_index = 6, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd7", .ec_index = 7, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd8", .ec_index = 8, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd9", .ec_index = 9, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd10", .ec_index = 10, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd11", .ec_index = 11, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd12", .ec_index = 12, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd13", .ec_index = 13, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd14", .ec_index = 14, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd15", .ec_index = 15, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd16", .ec_index = 16, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
            { NULL }
        }
    },


    {
        .name = "TS-1685", .mb_code = "QY380", .bp_code = "QY390",
        .features = {
            .ac_recovery    = 1, 
            .eup_mode       = 1,
            .btn_copy       = 1,
            .btn_reset      = 1,
            .btn_chassis    = 0,
            .led_brightness = 1,
            .led_status     = 1,
            .led_10g        = 0,
            .led_usb        = 1,
            .led_jbod       = 0,
            .led_locate     = 1,
        },
        .slots = {
			{ .name = "m2ssd1", .ec_index = 17, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "m2ssd2", .ec_index = 18, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "m2ssd3", .ec_index = 19, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "m2ssd4", .ec_index = 20, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "m2ssd5", .ec_index = 21, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "m2ssd6", .ec_index = 22, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd1", .ec_index = 13, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd2", .ec_index = 14, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd3", .ec_index = 15, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd4", .ec_index = 16, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd1", .ec_index = 1, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd2", .ec_index = 2, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd3", .ec_index = 3, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd4", .ec_index = 4, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd5", .ec_index = 5, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd6", .ec_index = 6, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd7", .ec_index = 7, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd8", .ec_index = 8, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd9", .ec_index = 9, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd10", .ec_index = 10, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd11", .ec_index = 11, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd12", .ec_index = 12, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
            { NULL }
        }
    },


    {
        .name = "TS-1886XU", .mb_code = "Q0471", .bp_code = "Q04L0",
        .features = {
            .ac_recovery    = 1, 
            .eup_mode       = 0,
            .btn_copy       = 0,
            .btn_reset      = 1,
            .btn_chassis    = 1,
            .led_brightness = 0,
            .led_status     = 1,
            .led_10g        = 1,
            .led_usb        = 0,
            .led_jbod       = 1,
            .led_locate     = 1,
        },
        .slots = {
			{ .name = "ssd1", .ec_index = 13, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd2", .ec_index = 14, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd3", .ec_index = 15, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd4", .ec_index = 16, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd5", .ec_index = 17, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd6", .ec_index = 18, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd1", .ec_index = 1, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd2", .ec_index = 2, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd3", .ec_index = 3, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd4", .ec_index = 4, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd5", .ec_index = 5, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd6", .ec_index = 6, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd7", .ec_index = 7, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd8", .ec_index = 8, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd9", .ec_index = 9, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd10", .ec_index = 10, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd11", .ec_index = 11, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd12", .ec_index = 12, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
            { NULL }
        }
    },


    {
        .name = "TS-1886XU R2", .mb_code = "Q0B50", .bp_code = "Q0950",
        .features = {
            .ac_recovery    = 1, 
            .eup_mode       = 0,
            .btn_copy       = 0,
            .btn_reset      = 1,
            .btn_chassis    = 0,
            .led_brightness = 0,
            .led_status     = 1,
            .led_10g        = 0,
            .led_usb        = 0,
            .led_jbod       = 1,
            .led_locate     = 1,
        },
        .slots = {
			{ .name = "ssd1", .ec_index = 13, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd2", .ec_index = 14, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd3", .ec_index = 15, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd4", .ec_index = 16, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd5", .ec_index = 17, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd6", .ec_index = 18, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd1", .ec_index = 1, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd2", .ec_index = 2, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd3", .ec_index = 3, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd4", .ec_index = 4, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd5", .ec_index = 5, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd6", .ec_index = 6, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd7", .ec_index = 7, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd8", .ec_index = 8, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd9", .ec_index = 9, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd10", .ec_index = 10, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd11", .ec_index = 11, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd12", .ec_index = 12, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
            { NULL }
        }
    },


    {
        .name = "TS-1887XU", .mb_code = "Q0840", .bp_code = "Q0950",
        .features = {
            .ac_recovery    = 0, 
            .eup_mode       = 1,
            .btn_copy       = 0,
            .btn_reset      = 1,
            .btn_chassis    = 0,
            .led_brightness = 0,
            .led_status     = 1,
            .led_10g        = 1,
            .led_usb        = 0,
            .led_jbod       = 1,
            .led_locate     = 1,
        },
        .slots = {
			{ .name = "ssd1", .ec_index = 13, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd2", .ec_index = 14, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd3", .ec_index = 15, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd4", .ec_index = 16, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd5", .ec_index = 17, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd6", .ec_index = 18, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd1", .ec_index = 1, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd2", .ec_index = 2, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd3", .ec_index = 3, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd4", .ec_index = 4, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd5", .ec_index = 5, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd6", .ec_index = 6, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd7", .ec_index = 7, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd8", .ec_index = 8, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd9", .ec_index = 9, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd10", .ec_index = 10, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd11", .ec_index = 11, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd12", .ec_index = 12, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
            { NULL }
        }
    },


    {
        .name = "TS-2287XU", .mb_code = "Q0840", .bp_code = "Q08A0",
        .features = {
            .ac_recovery    = 0, 
            .eup_mode       = 1,
            .btn_copy       = 0,
            .btn_reset      = 1,
            .btn_chassis    = 0,
            .led_brightness = 0,
            .led_status     = 1,
            .led_10g        = 1,
            .led_usb        = 0,
            .led_jbod       = 1,
            .led_locate     = 1,
        },
        .slots = {
			{ .name = "ssd1", .ec_index = 17, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd2", .ec_index = 18, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd3", .ec_index = 19, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd4", .ec_index = 20, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd5", .ec_index = 21, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd6", .ec_index = 22, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd1", .ec_index = 1, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd2", .ec_index = 2, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd3", .ec_index = 3, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd4", .ec_index = 4, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd5", .ec_index = 5, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd6", .ec_index = 6, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd7", .ec_index = 7, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd8", .ec_index = 8, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd9", .ec_index = 9, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd10", .ec_index = 10, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd11", .ec_index = 11, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd12", .ec_index = 12, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd13", .ec_index = 13, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd14", .ec_index = 14, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd15", .ec_index = 15, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd16", .ec_index = 16, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
            { NULL }
        }
    },


    {
        .name = "TS-2477XU", .mb_code = "QZ504", .bp_code = "Q0070",
        .features = {
            .ac_recovery    = 1, 
            .eup_mode       = 1,
            .btn_copy       = 1,
            .btn_reset      = 1,
            .btn_chassis    = 1,
            .led_brightness = 0,
            .led_status     = 1,
            .led_10g        = 0,
            .led_usb        = 0,
            .led_jbod       = 1,
            .led_locate     = 1,
        },
        .slots = {
			{ .name = "hdd1", .ec_index = 1, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd2", .ec_index = 2, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd3", .ec_index = 3, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd4", .ec_index = 4, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd5", .ec_index = 5, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd6", .ec_index = 6, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd7", .ec_index = 7, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd8", .ec_index = 8, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd9", .ec_index = 9, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd10", .ec_index = 10, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd11", .ec_index = 11, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd12", .ec_index = 12, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd13", .ec_index = 13, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd14", .ec_index = 14, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd15", .ec_index = 15, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd16", .ec_index = 16, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd17", .ec_index = 17, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd18", .ec_index = 18, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd19", .ec_index = 19, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd20", .ec_index = 20, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd21", .ec_index = 21, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd22", .ec_index = 22, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd23", .ec_index = 23, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd24", .ec_index = 24, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
            { NULL }
        }
    },


    {
        .name = "TS-2483XU", .mb_code = "Q00V2", .bp_code = "Q00W0",
        .features = {
            .ac_recovery    = 1, 
            .eup_mode       = 0,
            .btn_copy       = 0,
            .btn_reset      = 1,
            .btn_chassis    = 0,
            .led_brightness = 0,
            .led_status     = 1,
            .led_10g        = 0,
            .led_usb        = 0,
            .led_jbod       = 1,
            .led_locate     = 1,
        },
        .slots = {
			{ .name = "hdd1", .ec_index = 1, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd2", .ec_index = 2, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd3", .ec_index = 3, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd4", .ec_index = 4, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd5", .ec_index = 5, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd6", .ec_index = 6, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd7", .ec_index = 7, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd8", .ec_index = 8, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd9", .ec_index = 9, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd10", .ec_index = 10, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd11", .ec_index = 11, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd12", .ec_index = 12, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd13", .ec_index = 13, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd14", .ec_index = 14, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd15", .ec_index = 15, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd16", .ec_index = 16, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd17", .ec_index = 17, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd18", .ec_index = 18, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd19", .ec_index = 19, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd20", .ec_index = 20, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd21", .ec_index = 21, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd22", .ec_index = 22, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd23", .ec_index = 23, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd24", .ec_index = 24, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
            { NULL }
        }
    },


    {
        .name = "TS-2490FU", .mb_code = "Q03X0", .bp_code = "Q04K0",
        .features = {
            .ac_recovery    = 1, 
            .eup_mode       = 0,
            .btn_copy       = 0,
            .btn_reset      = 1,
            .btn_chassis    = 0,
            .led_brightness = 0,
            .led_status     = 1,
            .led_10g        = 1,
            .led_usb        = 0,
            .led_jbod       = 1,
            .led_locate     = 1,
        },
        .slots = {
			{ .name = "u2ssd1", .ec_index = 1, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd2", .ec_index = 2, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd3", .ec_index = 3, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd4", .ec_index = 4, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd5", .ec_index = 5, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd6", .ec_index = 6, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd7", .ec_index = 7, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd8", .ec_index = 8, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd9", .ec_index = 9, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd10", .ec_index = 10, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd11", .ec_index = 11, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd12", .ec_index = 12, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd13", .ec_index = 13, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd14", .ec_index = 14, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd15", .ec_index = 15, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd16", .ec_index = 16, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd17", .ec_index = 17, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd18", .ec_index = 18, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd19", .ec_index = 19, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd20", .ec_index = 20, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd21", .ec_index = 21, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd22", .ec_index = 22, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd23", .ec_index = 23, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd24", .ec_index = 24, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
            { NULL }
        }
    },


    {
        .name = "TS-2888X", .mb_code = "Q00Q0", .bp_code = "Q00S0",
        .features = {
            .ac_recovery    = 1, 
            .eup_mode       = 1,
            .btn_copy       = 1,
            .btn_reset      = 1,
            .btn_chassis    = 0,
            .led_brightness = 1,
            .led_status     = 1,
            .led_10g        = 0,
            .led_usb        = 1,
            .led_jbod       = 0,
            .led_locate     = 1,
        },
        .slots = {
			{ .name = "u2ssd1", .ec_index = 25, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd2", .ec_index = 26, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd3", .ec_index = 27, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "u2ssd4", .ec_index = 28, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd1", .ec_index = 9, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd2", .ec_index = 10, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd3", .ec_index = 11, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd4", .ec_index = 12, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd5", .ec_index = 13, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd6", .ec_index = 14, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd7", .ec_index = 15, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd8", .ec_index = 16, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd9", .ec_index = 17, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd10", .ec_index = 18, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd11", .ec_index = 19, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd12", .ec_index = 20, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd13", .ec_index = 21, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd14", .ec_index = 22, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd15", .ec_index = 23, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd16", .ec_index = 24, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd1", .ec_index = 1, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd2", .ec_index = 2, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd3", .ec_index = 3, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd4", .ec_index = 4, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd5", .ec_index = 5, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd6", .ec_index = 6, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd7", .ec_index = 7, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd8", .ec_index = 8, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
            { NULL }
        }
    },


    {
        .name = "TS-3087XU", .mb_code = "Q08H0", .bp_code = "Q08Z0",
        .features = {
            .ac_recovery    = 0, 
            .eup_mode       = 1,
            .btn_copy       = 0,
            .btn_reset      = 1,
            .btn_chassis    = 0,
            .led_brightness = 0,
            .led_status     = 1,
            .led_10g        = 1,
            .led_usb        = 0,
            .led_jbod       = 1,
            .led_locate     = 1,
        },
        .slots = {
			{ .name = "ssd1", .ec_index = 30, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd2", .ec_index = 29, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd3", .ec_index = 28, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd4", .ec_index = 27, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd5", .ec_index = 26, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd6", .ec_index = 25, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd1", .ec_index = 1, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd2", .ec_index = 2, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd3", .ec_index = 3, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd4", .ec_index = 4, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd5", .ec_index = 5, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd6", .ec_index = 6, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd7", .ec_index = 7, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd8", .ec_index = 8, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd9", .ec_index = 9, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd10", .ec_index = 10, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd11", .ec_index = 11, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd12", .ec_index = 12, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd13", .ec_index = 13, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd14", .ec_index = 14, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd15", .ec_index = 15, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd16", .ec_index = 16, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd17", .ec_index = 17, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd18", .ec_index = 18, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd19", .ec_index = 19, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd20", .ec_index = 20, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd21", .ec_index = 21, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd22", .ec_index = 22, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd23", .ec_index = 23, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd24", .ec_index = 24, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
            { NULL }
        }
    },


    {
        .name = "TS-3088XU", .mb_code = "Q06X0", .bp_code = "Q06Y0",
        .features = {
            .ac_recovery    = 1, 
            .eup_mode       = 0,
            .btn_copy       = 0,
            .btn_reset      = 1,
            .btn_chassis    = 1,
            .led_brightness = 0,
            .led_status     = 1,
            .led_10g        = 1,
            .led_usb        = 0,
            .led_jbod       = 1,
            .led_locate     = 1,
        },
        .slots = {
			{ .name = "ssd1", .ec_index = 1, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd2", .ec_index = 2, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd3", .ec_index = 3, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd4", .ec_index = 4, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd5", .ec_index = 5, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd6", .ec_index = 6, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd7", .ec_index = 7, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd8", .ec_index = 8, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd9", .ec_index = 9, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd10", .ec_index = 10, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd11", .ec_index = 11, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd12", .ec_index = 12, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd13", .ec_index = 13, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd14", .ec_index = 14, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd15", .ec_index = 15, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd16", .ec_index = 16, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd17", .ec_index = 17, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd18", .ec_index = 18, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd19", .ec_index = 19, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd20", .ec_index = 20, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd21", .ec_index = 21, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd22", .ec_index = 22, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd23", .ec_index = 23, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd24", .ec_index = 24, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd25", .ec_index = 25, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd26", .ec_index = 26, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd27", .ec_index = 27, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd28", .ec_index = 28, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd29", .ec_index = 29, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd30", .ec_index = 30, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
            { NULL }
        }
    },


    {
        .name = "TS-473A", .mb_code = "Q07D0", .bp_code = "Q07N0",
        .features = {
            .ac_recovery    = 1, 
            .eup_mode       = 1,
            .btn_copy       = 1,
            .btn_reset      = 1,
            .btn_chassis    = 0,
            .led_brightness = 1,
            .led_status     = 1,
            .led_10g        = 0,
            .led_usb        = 1,
            .led_jbod       = 0,
            .led_locate     = 1,
        },
        .slots = {
			{ .name = "m2ssd1", .ec_index = 9, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "m2ssd2", .ec_index = 10, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd1", .ec_index = 1, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd2", .ec_index = 2, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd3", .ec_index = 3, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd4", .ec_index = 4, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
            { NULL }
        }
    },


    {
        .name = "TS-655X", .mb_code = "Q0CH0", .bp_code = "Q0CI0",
        .features = {
            .ac_recovery    = 1, 
            .eup_mode       = 1,
            .btn_copy       = 1,
            .btn_reset      = 1,
            .btn_chassis    = 0,
            .led_brightness = 1,
            .led_status     = 1,
            .led_10g        = 0,
            .led_usb        = 1,
            .led_jbod       = 0,
            .led_locate     = 1,
        },
        .slots = {
			{ .name = "m2ssd1", .ec_index = 9, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "m2ssd2", .ec_index = 10, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd1", .ec_index = 7, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd2", .ec_index = 8, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd1", .ec_index = 2, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd2", .ec_index = 1, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd3", .ec_index = 3, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd4", .ec_index = 4, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
            { NULL }
        }
    },


    {
        .name = "TS-673A", .mb_code = "Q07D0", .bp_code = "Q07M0",
        .features = {
            .ac_recovery    = 1, 
            .eup_mode       = 1,
            .btn_copy       = 1,
            .btn_reset      = 1,
            .btn_chassis    = 0,
            .led_brightness = 1,
            .led_status     = 1,
            .led_10g        = 0,
            .led_usb        = 1,
            .led_jbod       = 0,
            .led_locate     = 1,
        },
        .slots = {
			{ .name = "m2ssd1", .ec_index = 9, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "m2ssd2", .ec_index = 10, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd1", .ec_index = 1, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd2", .ec_index = 2, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd3", .ec_index = 3, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd4", .ec_index = 4, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd5", .ec_index = 5, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd6", .ec_index = 6, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
            { NULL }
        }
    },


    {
        .name = "TS-686", .mb_code = "Q05S1", .bp_code = "Q0660",
        .features = {
            .ac_recovery    = 1, 
            .eup_mode       = 1,
            .btn_copy       = 1,
            .btn_reset      = 1,
            .btn_chassis    = 0,
            .led_brightness = 1,
            .led_status     = 1,
            .led_10g        = 0,
            .led_usb        = 1,
            .led_jbod       = 0,
            .led_locate     = 1,
        },
        .slots = {
			{ .name = "m2ssd1", .ec_index = 9, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "m2ssd2", .ec_index = 10, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd1", .ec_index = 7, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd2", .ec_index = 8, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd1", .ec_index = 1, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd2", .ec_index = 2, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd3", .ec_index = 3, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd4", .ec_index = 4, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
            { NULL }
        }
    },


    {
        .name = "TS-855EU", .mb_code = "Q0BT0", .bp_code = "Q0BU1",
        .features = {
            .ac_recovery    = 1, 
            .eup_mode       = 1,
            .btn_copy       = 0,
            .btn_reset      = 1,
            .btn_chassis    = 0,
            .led_brightness = 0,
            .led_status     = 1,
            .led_10g        = 0,
            .led_usb        = 0,
            .led_jbod       = 1,
            .led_locate     = 1,
        },
        .slots = {
			{ .name = "hdd1", .ec_index = 1, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd2", .ec_index = 2, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd3", .ec_index = 3, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd4", .ec_index = 4, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd5", .ec_index = 5, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd6", .ec_index = 6, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd7", .ec_index = 7, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd8", .ec_index = 8, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
            { NULL }
        }
    },


    {
        .name = "TS-855X", .mb_code = "Q0CH0", .bp_code = "Q0CJ0",
        .features = {
            .ac_recovery    = 1, 
            .eup_mode       = 1,
            .btn_copy       = 1,
            .btn_reset      = 1,
            .btn_chassis    = 0,
            .led_brightness = 1,
            .led_status     = 1,
            .led_10g        = 0,
            .led_usb        = 1,
            .led_jbod       = 0,
            .led_locate     = 1,
        },
        .slots = {
			{ .name = "m2ssd1", .ec_index = 9, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "m2ssd2", .ec_index = 10, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd1", .ec_index = 7, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd2", .ec_index = 8, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd1", .ec_index = 1, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd2", .ec_index = 2, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd3", .ec_index = 3, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd4", .ec_index = 4, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd5", .ec_index = 5, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd6", .ec_index = 6, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
            { NULL }
        }
    },


    {
        .name = "TS-873A", .mb_code = "Q07D0", .bp_code = "Q07L0",
        .features = {
            .ac_recovery    = 1, 
            .eup_mode       = 1,
            .btn_copy       = 1,
            .btn_reset      = 1,
            .btn_chassis    = 0,
            .led_brightness = 1,
            .led_status     = 1,
            .led_10g        = 0,
            .led_usb        = 1,
            .led_jbod       = 0,
            .led_locate     = 1,
        },
        .slots = {
			{ .name = "m2ssd1", .ec_index = 9, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "m2ssd2", .ec_index = 10, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd1", .ec_index = 1, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd2", .ec_index = 2, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd3", .ec_index = 3, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd4", .ec_index = 4, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd5", .ec_index = 5, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd6", .ec_index = 6, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd7", .ec_index = 7, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd8", .ec_index = 8, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
            { NULL }
        }
    },


    {
        .name = "TS-873AEU", .mb_code = "Q0AK0", .bp_code = "Q0AO0",
        .features = {
            .ac_recovery    = 1, 
            .eup_mode       = 0,
            .btn_copy       = 0,
            .btn_reset      = 1,
            .btn_chassis    = 0,
            .led_brightness = 0,
            .led_status     = 1,
            .led_10g        = 0,
            .led_usb        = 0,
            .led_jbod       = 1,
            .led_locate     = 1,
        },
        .slots = {
			{ .name = "hdd1", .ec_index = 1, .has_present = 0, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd2", .ec_index = 2, .has_present = 0, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd3", .ec_index = 3, .has_present = 0, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd4", .ec_index = 4, .has_present = 0, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd5", .ec_index = 5, .has_present = 0, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd6", .ec_index = 6, .has_present = 0, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd7", .ec_index = 7, .has_present = 0, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd8", .ec_index = 8, .has_present = 0, .has_active = 0, .has_error = 1, .has_locate = 1 },
            { NULL }
        }
    },


    {
        .name = "TS-873AEU-RP", .mb_code = "Q0AK0", .bp_code = "Q0AO1",
        .features = {
            .ac_recovery    = 1, 
            .eup_mode       = 0,
            .btn_copy       = 0,
            .btn_reset      = 1,
            .btn_chassis    = 0,
            .led_brightness = 0,
            .led_status     = 1,
            .led_10g        = 0,
            .led_usb        = 0,
            .led_jbod       = 1,
            .led_locate     = 1,
        },
        .slots = {
			{ .name = "hdd1", .ec_index = 1, .has_present = 0, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd2", .ec_index = 2, .has_present = 0, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd3", .ec_index = 3, .has_present = 0, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd4", .ec_index = 4, .has_present = 0, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd5", .ec_index = 5, .has_present = 0, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd6", .ec_index = 6, .has_present = 0, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd7", .ec_index = 7, .has_present = 0, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd8", .ec_index = 8, .has_present = 0, .has_active = 0, .has_error = 1, .has_locate = 1 },
            { NULL }
        }
    },


    {
        .name = "TS-873AU", .mb_code = "Q0520", .bp_code = "Q05G1",
        .features = {
            .ac_recovery    = 1, 
            .eup_mode       = 0,
            .btn_copy       = 0,
            .btn_reset      = 1,
            .btn_chassis    = 0,
            .led_brightness = 0,
            .led_status     = 1,
            .led_10g        = 0,
            .led_usb        = 0,
            .led_jbod       = 1,
            .led_locate     = 1,
        },
        .slots = {
			{ .name = "", .ec_index = 1, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "", .ec_index = 2, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "", .ec_index = 3, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "", .ec_index = 4, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "", .ec_index = 5, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "", .ec_index = 6, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "", .ec_index = 7, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "", .ec_index = 8, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
            { NULL }
        }
    },


    {
        .name = "TS-873AU-RP", .mb_code = "Q0520", .bp_code = "Q0671",
        .features = {
            .ac_recovery    = 1, 
            .eup_mode       = 0,
            .btn_copy       = 0,
            .btn_reset      = 1,
            .btn_chassis    = 0,
            .led_brightness = 0,
            .led_status     = 1,
            .led_10g        = 0,
            .led_usb        = 0,
            .led_jbod       = 1,
            .led_locate     = 1,
        },
        .slots = {
			{ .name = "", .ec_index = 1, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "", .ec_index = 2, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "", .ec_index = 3, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "", .ec_index = 4, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "", .ec_index = 5, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "", .ec_index = 6, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "", .ec_index = 7, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "", .ec_index = 8, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
            { NULL }
        }
    },


    {
        .name = "TS-877XU", .mb_code = "QZ494", .bp_code = "QZ551",
        .features = {
            .ac_recovery    = 1, 
            .eup_mode       = 1,
            .btn_copy       = 1,
            .btn_reset      = 1,
            .btn_chassis    = 0,
            .led_brightness = 0,
            .led_status     = 1,
            .led_10g        = 0,
            .led_usb        = 0,
            .led_jbod       = 1,
            .led_locate     = 1,
        },
        .slots = {
			{ .name = "hdd1", .ec_index = 1, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd2", .ec_index = 2, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd3", .ec_index = 3, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd4", .ec_index = 4, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd5", .ec_index = 5, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd6", .ec_index = 6, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd7", .ec_index = 7, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd8", .ec_index = 8, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
            { NULL }
        }
    },


    {
        .name = "TS-883XU", .mb_code = "QZ601", .bp_code = "Q00M1",
        .features = {
            .ac_recovery    = 1, 
            .eup_mode       = 0,
            .btn_copy       = 0,
            .btn_reset      = 1,
            .btn_chassis    = 0,
            .led_brightness = 0,
            .led_status     = 1,
            .led_10g        = 0,
            .led_usb        = 0,
            .led_jbod       = 1,
            .led_locate     = 1,
        },
        .slots = {
			{ .name = "hdd1", .ec_index = 1, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd2", .ec_index = 2, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd3", .ec_index = 3, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd4", .ec_index = 4, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd5", .ec_index = 5, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd6", .ec_index = 6, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd7", .ec_index = 7, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd8", .ec_index = 8, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
            { NULL }
        }
    },


    {
        .name = "TS-886", .mb_code = "Q05S1", .bp_code = "Q0650",
        .features = {
            .ac_recovery    = 1, 
            .eup_mode       = 1,
            .btn_copy       = 1,
            .btn_reset      = 1,
            .btn_chassis    = 0,
            .led_brightness = 1,
            .led_status     = 1,
            .led_10g        = 0,
            .led_usb        = 1,
            .led_jbod       = 0,
            .led_locate     = 1,
        },
        .slots = {
			{ .name = "m2ssd1", .ec_index = 9, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "m2ssd2", .ec_index = 10, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd1", .ec_index = 7, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd2", .ec_index = 8, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd1", .ec_index = 1, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd2", .ec_index = 2, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd3", .ec_index = 3, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd4", .ec_index = 4, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd5", .ec_index = 5, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd6", .ec_index = 6, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
            { NULL }
        }
    },


    {
        .name = "TS-973AX", .mb_code = "Q0711", .bp_code = "Q0760",
        .features = {
            .ac_recovery    = 1, 
            .eup_mode       = 1,
            .btn_copy       = 1,
            .btn_reset      = 1,
            .btn_chassis    = 0,
            .led_brightness = 1,
            .led_status     = 1,
            .led_10g        = 0,
            .led_usb        = 1,
            .led_jbod       = 0,
            .led_locate     = 1,
        },
        .slots = {
			{ .name = "hdd1", .ec_index = 1, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd2", .ec_index = 2, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd3", .ec_index = 3, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd4", .ec_index = 4, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd5", .ec_index = 5, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd1", .ec_index = 6, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd2", .ec_index = 7, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd3", .ec_index = 8, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd4", .ec_index = 9, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
            { NULL }
        }
    },


    {
        .name = "TS-977XU", .mb_code = "QZ482", .bp_code = "Q0060",
        .features = {
            .ac_recovery    = 1, 
            .eup_mode       = 1,
            .btn_copy       = 1,
            .btn_reset      = 1,
            .btn_chassis    = 0,
            .led_brightness = 0,
            .led_status     = 1,
            .led_10g        = 0,
            .led_usb        = 0,
            .led_jbod       = 1,
            .led_locate     = 1,
        },
        .slots = {
			{ .name = "hdd1", .ec_index = 1, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd2", .ec_index = 2, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd3", .ec_index = 3, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd4", .ec_index = 4, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd1", .ec_index = 5, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd2", .ec_index = 6, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd3", .ec_index = 7, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd4", .ec_index = 8, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd5", .ec_index = 9, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
            { NULL }
        }
    },


    {
        .name = "TS-983XU", .mb_code = "Q00I1", .bp_code = "Q00X0",
        .features = {
            .ac_recovery    = 1, 
            .eup_mode       = 0,
            .btn_copy       = 0,
            .btn_reset      = 1,
            .btn_chassis    = 0,
            .led_brightness = 0,
            .led_status     = 1,
            .led_10g        = 0,
            .led_usb        = 0,
            .led_jbod       = 0,
            .led_locate     = 1,
        },
        .slots = {
			{ .name = "hdd1", .ec_index = 1, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd2", .ec_index = 2, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd3", .ec_index = 3, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd4", .ec_index = 4, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd1", .ec_index = 5, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd2", .ec_index = 6, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd3", .ec_index = 7, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd4", .ec_index = 8, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd5", .ec_index = 9, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
            { NULL }
        }
    },


    {
        .name = "TVS-1275U", .mb_code = "SAP00", .bp_code = "SBO70",
        .features = {
            .ac_recovery    = 1, 
            .eup_mode       = 0,
            .btn_copy       = 0,
            .btn_reset      = 1,
            .btn_chassis    = 1,
            .led_brightness = 0,
            .led_status     = 1,
            .led_10g        = 0,
            .led_usb        = 0,
            .led_jbod       = 1,
            .led_locate     = 1,
        },
        .slots = {
			{ .name = "hdd1", .ec_index = 1, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd2", .ec_index = 2, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd3", .ec_index = 3, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd4", .ec_index = 4, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd5", .ec_index = 5, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd6", .ec_index = 6, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd7", .ec_index = 7, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd8", .ec_index = 8, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd9", .ec_index = 9, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd10", .ec_index = 10, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd11", .ec_index = 11, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd12", .ec_index = 12, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
            { NULL }
        }
    },


    {
        .name = "TVS-1288X", .mb_code = "Q05W0", .bp_code = "Q05K0",
        .features = {
            .ac_recovery    = 1, 
            .eup_mode       = 1,
            .btn_copy       = 1,
            .btn_reset      = 1,
            .btn_chassis    = 0,
            .led_brightness = 1,
            .led_status     = 1,
            .led_10g        = 0,
            .led_usb        = 1,
            .led_jbod       = 0,
            .led_locate     = 1,
        },
        .slots = {
			{ .name = "m2ssd1", .ec_index = 17, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "m2ssd2", .ec_index = 18, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd1", .ec_index = 13, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd2", .ec_index = 14, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd3", .ec_index = 15, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd4", .ec_index = 16, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd1", .ec_index = 1, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd2", .ec_index = 2, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd3", .ec_index = 3, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd4", .ec_index = 4, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd5", .ec_index = 5, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd6", .ec_index = 6, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd7", .ec_index = 7, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd8", .ec_index = 8, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
            { NULL }
        }
    },


    {
        .name = "TVS-1675U", .mb_code = "SAP00", .bp_code = "SBO80",
        .features = {
            .ac_recovery    = 1, 
            .eup_mode       = 0,
            .btn_copy       = 0,
            .btn_reset      = 1,
            .btn_chassis    = 1,
            .led_brightness = 0,
            .led_status     = 1,
            .led_10g        = 0,
            .led_usb        = 0,
            .led_jbod       = 1,
            .led_locate     = 1,
        },
        .slots = {
			{ .name = "hdd1", .ec_index = 1, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd2", .ec_index = 2, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd3", .ec_index = 3, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd4", .ec_index = 4, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd5", .ec_index = 5, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd6", .ec_index = 6, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd7", .ec_index = 7, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd8", .ec_index = 8, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd9", .ec_index = 9, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd10", .ec_index = 10, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd11", .ec_index = 11, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd12", .ec_index = 12, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd13", .ec_index = 13, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd14", .ec_index = 14, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd15", .ec_index = 15, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd16", .ec_index = 16, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
            { NULL }
        }
    },


    {
        .name = "TVS-1688X", .mb_code = "Q05T0", .bp_code = "Q0630",
        .features = {
            .ac_recovery    = 1, 
            .eup_mode       = 1,
            .btn_copy       = 1,
            .btn_reset      = 1,
            .btn_chassis    = 0,
            .led_brightness = 1,
            .led_status     = 1,
            .led_10g        = 0,
            .led_usb        = 1,
            .led_jbod       = 0,
            .led_locate     = 1,
        },
        .slots = {
			{ .name = "m2ssd1", .ec_index = 17, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "m2ssd2", .ec_index = 18, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd1", .ec_index = 13, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd2", .ec_index = 14, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd3", .ec_index = 15, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "ssd4", .ec_index = 16, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd1", .ec_index = 1, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd2", .ec_index = 2, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd3", .ec_index = 3, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd4", .ec_index = 4, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd5", .ec_index = 5, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd6", .ec_index = 6, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd7", .ec_index = 7, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd8", .ec_index = 8, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd9", .ec_index = 9, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd10", .ec_index = 10, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd11", .ec_index = 11, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd12", .ec_index = 12, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
            { NULL }
        }
    },


    {
        .name = "TVS-472X", .mb_code = "Q0420", .bp_code = "Q0180",
        .features = {
            .ac_recovery    = 1, 
            .eup_mode       = 0,
            .btn_copy       = 1,
            .btn_reset      = 1,
            .btn_chassis    = 0,
            .led_brightness = 1,
            .led_status     = 1,
            .led_10g        = 0,
            .led_usb        = 1,
            .led_jbod       = 0,
            .led_locate     = 1,
        },
        .slots = {
			{ .name = "hdd1", .ec_index = 1, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd2", .ec_index = 2, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd3", .ec_index = 3, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd4", .ec_index = 4, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "m2ssd1", .ec_index = 9, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "m2ssd2", .ec_index = 10, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
            { NULL }
        }
    },


    {
        .name = "TVS-472XT", .mb_code = "Q0120", .bp_code = "Q0180",
        .features = {
            .ac_recovery    = 1, 
            .eup_mode       = 0,
            .btn_copy       = 1,
            .btn_reset      = 1,
            .btn_chassis    = 0,
            .led_brightness = 1,
            .led_status     = 1,
            .led_10g        = 0,
            .led_usb        = 1,
            .led_jbod       = 0,
            .led_locate     = 1,
        },
        .slots = {
			{ .name = "hdd1", .ec_index = 1, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd2", .ec_index = 2, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd3", .ec_index = 3, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd4", .ec_index = 4, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "m2ssd1", .ec_index = 9, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "m2ssd2", .ec_index = 10, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
            { NULL }
        }
    },


    {
        .name = "TVS-474", .mb_code = "Q0BB0", .bp_code = "Q0BL0",
        .features = {
            .ac_recovery    = 0, 
            .eup_mode       = 0,
            .btn_copy       = 1,
            .btn_reset      = 1,
            .btn_chassis    = 0,
            .led_brightness = 1,
            .led_status     = 1,
            .led_10g        = 0,
            .led_usb        = 1,
            .led_jbod       = 0,
            .led_locate     = 1,
        },
        .slots = {
			{ .name = "m2ssd1", .ec_index = 9, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "m2ssd2", .ec_index = 10, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd1", .ec_index = 1, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd2", .ec_index = 2, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd3", .ec_index = 3, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd4", .ec_index = 4, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
            { NULL }
        }
    },


    {
        .name = "TVS-672N", .mb_code = "Q0420", .bp_code = "Q0170",
        .features = {
            .ac_recovery    = 1, 
            .eup_mode       = 0,
            .btn_copy       = 1,
            .btn_reset      = 1,
            .btn_chassis    = 0,
            .led_brightness = 1,
            .led_status     = 1,
            .led_10g        = 0,
            .led_usb        = 1,
            .led_jbod       = 0,
            .led_locate     = 1,
        },
        .slots = {
			{ .name = "hdd1", .ec_index = 1, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd2", .ec_index = 2, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd3", .ec_index = 3, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd4", .ec_index = 4, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd5", .ec_index = 5, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd6", .ec_index = 6, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "m2ssd1", .ec_index = 9, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "m2ssd2", .ec_index = 10, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
            { NULL }
        }
    },


    {
        .name = "TVS-672X", .mb_code = "Q0121", .bp_code = "Q0170",
        .features = {
            .ac_recovery    = 1, 
            .eup_mode       = 0,
            .btn_copy       = 1,
            .btn_reset      = 1,
            .btn_chassis    = 0,
            .led_brightness = 1,
            .led_status     = 1,
            .led_10g        = 0,
            .led_usb        = 1,
            .led_jbod       = 0,
            .led_locate     = 1,
        },
        .slots = {
			{ .name = "hdd1", .ec_index = 1, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd2", .ec_index = 2, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd3", .ec_index = 3, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd4", .ec_index = 4, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd5", .ec_index = 5, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd6", .ec_index = 6, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "m2ssd1", .ec_index = 9, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "m2ssd2", .ec_index = 10, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
            { NULL }
        }
    },


    {
        .name = "TVS-672XT", .mb_code = "Q0120", .bp_code = "Q0170",
        .features = {
            .ac_recovery    = 1, 
            .eup_mode       = 0,
            .btn_copy       = 1,
            .btn_reset      = 1,
            .btn_chassis    = 0,
            .led_brightness = 1,
            .led_status     = 1,
            .led_10g        = 0,
            .led_usb        = 1,
            .led_jbod       = 0,
            .led_locate     = 1,
        },
        .slots = {
			{ .name = "hdd1", .ec_index = 1, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd2", .ec_index = 2, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd3", .ec_index = 3, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd4", .ec_index = 4, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd5", .ec_index = 5, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd6", .ec_index = 6, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "m2ssd1", .ec_index = 9, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "m2ssd2", .ec_index = 10, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
            { NULL }
        }
    },


    {
        .name = "TVS-674", .mb_code = "B6490", .bp_code = "Q0BK0",
        .features = {
            .ac_recovery    = 0, 
            .eup_mode       = 0,
            .btn_copy       = 1,
            .btn_reset      = 1,
            .btn_chassis    = 0,
            .led_brightness = 1,
            .led_status     = 1,
            .led_10g        = 0,
            .led_usb        = 1,
            .led_jbod       = 0,
            .led_locate     = 1,
        },
        .slots = {
			{ .name = "m2ssd1", .ec_index = 9, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "m2ssd2", .ec_index = 10, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd1", .ec_index = 1, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd2", .ec_index = 2, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd3", .ec_index = 3, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd4", .ec_index = 4, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd5", .ec_index = 5, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd6", .ec_index = 6, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
            { NULL }
        }
    },


    {
        .name = "TVS-675", .mb_code = "SAP10", .bp_code = "SBO90",
        .features = {
            .ac_recovery    = 1, 
            .eup_mode       = 0,
            .btn_copy       = 1,
            .btn_reset      = 1,
            .btn_chassis    = 0,
            .led_brightness = 0,
            .led_status     = 1,
            .led_10g        = 0,
            .led_usb        = 1,
            .led_jbod       = 0,
            .led_locate     = 1,
        },
        .slots = {
			{ .name = "m2ssd1", .ec_index = 1, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "m2ssd2", .ec_index = 2, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd1", .ec_index = 3, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd2", .ec_index = 4, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd3", .ec_index = 5, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd4", .ec_index = 6, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd5", .ec_index = 7, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd6", .ec_index = 8, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
            { NULL }
        }
    },


    {
        .name = "TVS-872N", .mb_code = "Q0420", .bp_code = "Q0160",
        .features = {
            .ac_recovery    = 1, 
            .eup_mode       = 0,
            .btn_copy       = 1,
            .btn_reset      = 1,
            .btn_chassis    = 0,
            .led_brightness = 1,
            .led_status     = 1,
            .led_10g        = 0,
            .led_usb        = 1,
            .led_jbod       = 0,
            .led_locate     = 1,
        },
        .slots = {
			{ .name = "hdd1", .ec_index = 1, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd2", .ec_index = 2, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd3", .ec_index = 3, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd4", .ec_index = 4, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd5", .ec_index = 5, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd6", .ec_index = 6, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd7", .ec_index = 7, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd8", .ec_index = 8, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "m2ssd1", .ec_index = 9, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "m2ssd2", .ec_index = 10, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
            { NULL }
        }
    },


    {
        .name = "TVS-872X", .mb_code = "Q0121", .bp_code = "Q0160",
        .features = {
            .ac_recovery    = 1, 
            .eup_mode       = 0,
            .btn_copy       = 1,
            .btn_reset      = 1,
            .btn_chassis    = 0,
            .led_brightness = 1,
            .led_status     = 1,
            .led_10g        = 0,
            .led_usb        = 1,
            .led_jbod       = 0,
            .led_locate     = 1,
        },
        .slots = {
			{ .name = "hdd1", .ec_index = 1, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd2", .ec_index = 2, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd3", .ec_index = 3, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd4", .ec_index = 4, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd5", .ec_index = 5, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd6", .ec_index = 6, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd7", .ec_index = 7, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd8", .ec_index = 8, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "m2ssd1", .ec_index = 9, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "m2ssd2", .ec_index = 10, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
            { NULL }
        }
    },


    {
        .name = "TVS-872XT", .mb_code = "Q0120", .bp_code = "Q0160",
        .features = {
            .ac_recovery    = 1, 
            .eup_mode       = 0,
            .btn_copy       = 1,
            .btn_reset      = 1,
            .btn_chassis    = 0,
            .led_brightness = 1,
            .led_status     = 1,
            .led_10g        = 0,
            .led_usb        = 1,
            .led_jbod       = 0,
            .led_locate     = 1,
        },
        .slots = {
			{ .name = "hdd1", .ec_index = 1, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd2", .ec_index = 2, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd3", .ec_index = 3, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd4", .ec_index = 4, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd5", .ec_index = 5, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd6", .ec_index = 6, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd7", .ec_index = 7, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd8", .ec_index = 8, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "m2ssd1", .ec_index = 9, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "m2ssd2", .ec_index = 10, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
            { NULL }
        }
    },


    {
        .name = "TVS-874", .mb_code = "B6490", .bp_code = "Q0AA0",
        .features = {
            .ac_recovery    = 0, 
            .eup_mode       = 0,
            .btn_copy       = 1,
            .btn_reset      = 1,
            .btn_chassis    = 0,
            .led_brightness = 1,
            .led_status     = 1,
            .led_10g        = 0,
            .led_usb        = 1,
            .led_jbod       = 0,
            .led_locate     = 1,
        },
        .slots = {
			{ .name = "m2ssd1", .ec_index = 9, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "m2ssd2", .ec_index = 10, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd1", .ec_index = 1, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd2", .ec_index = 2, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd3", .ec_index = 3, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd4", .ec_index = 4, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd5", .ec_index = 5, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd6", .ec_index = 6, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd7", .ec_index = 7, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd8", .ec_index = 8, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
            { NULL }
        }
    },


    {
        .name = "TVS-875U", .mb_code = "SAP00", .bp_code = "SBO71",
        .features = {
            .ac_recovery    = 1, 
            .eup_mode       = 0,
            .btn_copy       = 0,
            .btn_reset      = 1,
            .btn_chassis    = 1,
            .led_brightness = 0,
            .led_status     = 1,
            .led_10g        = 0,
            .led_usb        = 0,
            .led_jbod       = 1,
            .led_locate     = 1,
        },
        .slots = {
			{ .name = "hdd1", .ec_index = 1, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd2", .ec_index = 2, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd3", .ec_index = 3, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd4", .ec_index = 4, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd5", .ec_index = 5, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd6", .ec_index = 6, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd7", .ec_index = 7, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd8", .ec_index = 8, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
            { NULL }
        }
    },


    {
        .name = "TVS-H674T", .mb_code = "B6491", .bp_code = "Q0BK0",
        .features = {
            .ac_recovery    = 0, 
            .eup_mode       = 0,
            .btn_copy       = 1,
            .btn_reset      = 1,
            .btn_chassis    = 0,
            .led_brightness = 1,
            .led_status     = 1,
            .led_10g        = 0,
            .led_usb        = 1,
            .led_jbod       = 0,
            .led_locate     = 1,
        },
        .slots = {
			{ .name = "m2ssd1", .ec_index = 9, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "m2ssd2", .ec_index = 10, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd1", .ec_index = 1, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd2", .ec_index = 2, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd3", .ec_index = 3, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd4", .ec_index = 4, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd5", .ec_index = 5, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd6", .ec_index = 6, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
            { NULL }
        }
    },


    {
        .name = "TVS-H674X", .mb_code = "B6492", .bp_code = "Q0BK0",
        .features = {
            .ac_recovery    = 0, 
            .eup_mode       = 0,
            .btn_copy       = 1,
            .btn_reset      = 1,
            .btn_chassis    = 0,
            .led_brightness = 1,
            .led_status     = 1,
            .led_10g        = 0,
            .led_usb        = 1,
            .led_jbod       = 0,
            .led_locate     = 1,
        },
        .slots = {
			{ .name = "m2ssd1", .ec_index = 9, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "m2ssd2", .ec_index = 10, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd1", .ec_index = 1, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd2", .ec_index = 2, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd3", .ec_index = 3, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd4", .ec_index = 4, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd5", .ec_index = 5, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd6", .ec_index = 6, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
            { NULL }
        }
    },


    {
        .name = "TVS-H874T", .mb_code = "B6491", .bp_code = "Q0AA0",
        .features = {
            .ac_recovery    = 0, 
            .eup_mode       = 0,
            .btn_copy       = 1,
            .btn_reset      = 1,
            .btn_chassis    = 0,
            .led_brightness = 1,
            .led_status     = 1,
            .led_10g        = 0,
            .led_usb        = 1,
            .led_jbod       = 0,
            .led_locate     = 1,
        },
        .slots = {
			{ .name = "m2ssd1", .ec_index = 9, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "m2ssd2", .ec_index = 10, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd1", .ec_index = 1, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd2", .ec_index = 2, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd3", .ec_index = 3, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd4", .ec_index = 4, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd5", .ec_index = 5, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd6", .ec_index = 6, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd7", .ec_index = 7, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd8", .ec_index = 8, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
            { NULL }
        }
    },


    {
        .name = "TVS-H874X", .mb_code = "B6492", .bp_code = "Q0AA0",
        .features = {
            .ac_recovery    = 0, 
            .eup_mode       = 0,
            .btn_copy       = 1,
            .btn_reset      = 1,
            .btn_chassis    = 0,
            .led_brightness = 1,
            .led_status     = 1,
            .led_10g        = 0,
            .led_usb        = 1,
            .led_jbod       = 0,
            .led_locate     = 1,
        },
        .slots = {
			{ .name = "m2ssd1", .ec_index = 9, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "m2ssd2", .ec_index = 10, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd1", .ec_index = 1, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd2", .ec_index = 2, .has_present = 1, .has_active = 0, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd3", .ec_index = 3, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd4", .ec_index = 4, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd5", .ec_index = 5, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd6", .ec_index = 6, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd7", .ec_index = 7, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
			{ .name = "hdd8", .ec_index = 8, .has_present = 1, .has_active = 1, .has_error = 1, .has_locate = 1 },
            { NULL }
        }
    },
};
