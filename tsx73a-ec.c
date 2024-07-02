#include <linux/module.h>
#include <linux/ioport.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>
#include <linux/hwmon.h>
#include <linux/time.h>
#include <linux/input.h>
#include "tsx73a-ec.h"

/**
 * 'Done' means PoC implemented
 * cd ..
 * TODO:
 *  GENERAL:
 *      - Add parameter to ignore fan ranges?
 *      - Make debug attribute single, add all functions.
 *      - Depending on availability, auto detect fans, temps?
 *  VPD:
 *      - Broken, reads first byte only. Fix.
 *  Fan:#include <linux/rtc.h>
 *      - Get SPEED     - Done
 *      - Get STATUS    - Done, requires rework
 *      - Get PWM       - Done
 *      - Set PWM       - Done
 *      - Set to back to auto?
 *      - TFAN?, change temp ranges?
 *  Temp:
 *      - Get TEMP      - Done
 *  CPLD:
 *      - Get version   - Done
 *  Disk:
 *      - Power up/down SATA
 *  Button:
 *      - Get RESET
 *      - Get USB/COPY
 *  LED:
 *      - STATUS
 *      - USB
 *      - DISK ERROR/PRESENT/ACTIVE
 *      - IDENT
 *      - NVME Activity fix?
 *
 *      front-panel::global_brightness  -
 *          0x243 = Value, 0x245 |= 0x10, 0x246 = val, 0x245 &= 0xef
 *      front-panel:blue:usb            - 0x154 0/Off 1/Blink 2/Solid
 *      front-panel:green:status        -
 *          0x155 0/Off 1/Red 2/Green 3/BlinkRed 4/BlinkGreen 5/BlinkBoth
 *      front-panel:red:status          -
 *          ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
 *      front-panel:red:diskN           -
 *      front-panel:green:disk1N
 *
 *      front-panel:red:locate
 *
 *  General:
 *      - Refactor ec_read_byte to return the value as signed int
 *          w/ positive = raw value, negative = ERRNO and use IS_ERR_VALUE to
 *          make errors branches unlikely
 *      - Add a way to detect the device model (MB and BP VPD model info) to
 *          select correct config
 *      - Add config overrides?
 *
*/
static struct platform_device *ec_pdev;

static struct resource ec_resources[] = {
	DEFINE_RES_IO_NAMED(EC_CMD_PORT, 1, DRVNAME),
	DEFINE_RES_IO_NAMED(EC_DAT_PORT, 1, DRVNAME)
};

static struct platform_driver ec_pdriver = {
	.driver = {
		.owner = THIS_MODULE,
		.name  = DRVNAME,
	},
	.probe	= ec_driver_probe,
	.remove	= ec_driver_remove
};

static struct input_dev *ec_bdev;

static DEVICE_ATTR(fw_version, S_IRUGO, ec_fw_version_show, NULL);
static DEVICE_ATTR(cpld_version, S_IRUGO, ec_cpld_version_show, NULL);
static DEVICE_ATTR(ac_recovery, S_IRUGO | S_IWUSR, ec_ac_recovery_show, ec_ac_recovery_store);
static DEVICE_ATTR(eup_mode, S_IRUGO | S_IWUSR, ec_eup_mode_show, ec_eup_mode_store);

static struct attribute *ec_attrs[] = {
	&dev_attr_fw_version.attr,
	&dev_attr_cpld_version.attr,
	&dev_attr_ac_recovery.attr,
	&dev_attr_eup_mode.attr,
	NULL
};

static const struct attribute_group ec_attr_group = {
	.attrs = ec_attrs
};



/* Attributes to dump an entire VPD table, mostly for development*/
static VPD_DEV_ATTR(dbg_table0, S_IRUGO, ec_vpd_table_show, NULL, 0);
static VPD_DEV_ATTR(dbg_table1, S_IRUGO, ec_vpd_table_show, NULL, 0);
static VPD_DEV_ATTR(dbg_table2, S_IRUGO, ec_vpd_table_show, NULL, 0);
static VPD_DEV_ATTR(dbg_table3, S_IRUGO, ec_vpd_table_show, NULL, 0);
/**
 * Attributes for all known VPD entries (see tsx73a-ec.h)
 * Currently only nickname VPD entry sis writeable, it's not really useful.
 **/
static VPD_DEV_ATTR(mb_date, S_IRUGO, ec_vpd_entry_show, NULL, EC_VPD_MB_DATE);
static VPD_DEV_ATTR(mb_manufacturer, S_IRUGO, ec_vpd_entry_show, NULL, EC_VPD_MB_MANUF);
static VPD_DEV_ATTR(mb_name, S_IRUGO, ec_vpd_entry_show, NULL, EC_VPD_MB_NAME);
static VPD_DEV_ATTR(mb_serial, S_IRUGO, ec_vpd_entry_show, NULL, EC_VPD_MB_SERIAL);
static VPD_DEV_ATTR(mb_model, S_IRUGO, ec_vpd_entry_show, NULL, EC_VPD_MB_MODEL);
static VPD_DEV_ATTR(mb_vendor, S_IRUGO, ec_vpd_entry_show, NULL, EC_VPD_MB_VENDOR);
static VPD_DEV_ATTR(bp_date, S_IRUGO, ec_vpd_entry_show, NULL, EC_VPD_BP_DATE);
static VPD_DEV_ATTR(bp_manufacturer, S_IRUGO, ec_vpd_entry_show, NULL, EC_VPD_BP_MANUF);
static VPD_DEV_ATTR(bp_name, S_IRUGO, ec_vpd_entry_show, NULL, EC_VPD_BP_NAME);
static VPD_DEV_ATTR(bp_serial, S_IRUGO, ec_vpd_entry_show, NULL, EC_VPD_BP_SERIAL);
static VPD_DEV_ATTR(bp_model, S_IRUGO, ec_vpd_entry_show, NULL, EC_VPD_BP_MODEL);
static VPD_DEV_ATTR(bp_vendor, S_IRUGO, ec_vpd_entry_show, NULL, EC_VPD_BP_VENDOR);
static VPD_DEV_ATTR(enc_serial, S_IRUGO, ec_vpd_entry_show, NULL, EC_VPD_ENC_SERIAL);
static VPD_DEV_ATTR(enc_nickname, S_IRUGO | S_IWUSR, ec_vpd_entry_show, NULL, EC_VPD_ENC_NICKNAME); /* Maybe add store here */

static struct attribute *ec_vpd_attrs[] = {
	&dev_attr_dbg_table0.attr,
	&dev_attr_dbg_table1.attr,
	&dev_attr_dbg_table2.attr,
	&dev_attr_dbg_table3.attr,
	&dev_attr_mb_date.attr,
	&dev_attr_mb_manufacturer.attr,
	&dev_attr_mb_name.attr,
	&dev_attr_mb_serial.attr,
	&dev_attr_mb_model.attr,
	&dev_attr_mb_vendor.attr,
	&dev_attr_bp_date.attr,
	&dev_attr_bp_manufacturer.attr,
	&dev_attr_bp_name.attr,
	&dev_attr_bp_serial.attr,
	&dev_attr_bp_model.attr,
	&dev_attr_bp_vendor.attr,
	&dev_attr_enc_serial.attr,
	&dev_attr_enc_nickname.attr,
	NULL
};

static const struct attribute_group ec_vpd_attr_group = {
	.name = "vpd",
	.attrs = ec_vpd_attrs
};

static DEFINE_MUTEX(ec_lock);

static u32 ec_hwmon_temp_config[TSX73A_MAX_HWMON_CHANNELS + 1] = {0};
static struct hwmon_channel_info ec_hwmon_temp_chan_info = {
	.type = hwmon_temp,
	.config = ec_hwmon_temp_config
};

static u32 ec_hwmon_fan_config[TSX73A_MAX_HWMON_CHANNELS + 1] = {0};
static struct hwmon_channel_info ec_hwmon_fan_chan_info = {
	.type = hwmon_fan,
	.config = ec_hwmon_fan_config
};

static u32 ec_hwmon_pwm_config[TSX73A_MAX_HWMON_CHANNELS + 1] = {0};
static struct hwmon_channel_info ec_hwmon_pwm_chan_info = {
	.type = hwmon_pwm,
	.config = ec_hwmon_pwm_config
};

static struct hwmon_channel_info const *hwmon_chan_info[] = {
	&ec_hwmon_temp_chan_info,
	&ec_hwmon_fan_chan_info,
	&ec_hwmon_pwm_chan_info,
	NULL
};

static struct hwmon_ops ec_hwmon_ops = {
	.is_visible = &ec_hwmon_is_visible,
	.read = &ec_hwmon_read,
	.write = &ec_hwmon_write,
	.read_string = &ec_hwmon_read_string
};

static struct hwmon_chip_info ec_hwmon_chip_info = {
	.info = hwmon_chan_info,
	.ops = &ec_hwmon_ops
};
/*
* static struct led_classdev tsx73a_leds[] = {
*	{
*		.name = DRVNAME ":red:status".
*		.brightness = LED_OFF
*		.max_brightness = LED_ON,
*		.brightness_set = NULL,
*		.brightness_get = NULL,
*	},
*	{
*		.name = DRVNAME ":green:status".
*		.brightness = LED_OFF
*		.max_brightness = LED_ON,
*		.brightness_set = NULL,
*		.brightness_get = NULL,
*	},
*	{
*		.name = DRVNAME ":blue:usb".
*		.brightness = LED_OFF
*		.max_brightness = LED_ON,
*		.brightness_set = NULL,
*		.brightness_get = NULL,
*	},
*	 {
*		.name = DRVNAME ":blue:usb".
*		.brightness = LED_OFF
*		.max_brightness = LED_FULL,
*		.brightness_set = NULL,
*		.brightness_get = NULL,
*	},
*	{ NULL }
* };
*/


/**
 * ec_check_exists - Check that EC is ITE8528
 *
 * Check is done by reading idregs from the SIO port,
 * ID bytes should be 0x85 and 0x28 for the ITE8528.
 * Only SIO port combination on  0x2e,0x2f are checked
 * as this seems to be sufficiant for the QNAP system.
 */
static int ec_check_exists(void)
{
	int ret = 0;
	u16 ec_id;

	if (!request_muxed_region(0x2e, 2, DRVNAME)) {
		ret = -EBUSY;
		goto ec_check_exists_ret;
	}

	outb(0x20, 0x2e);
	ec_id = inb(0x2f) << 8;
	outb(0x21, 0x2e);
	ec_id |= inb(0x2f);

	/* HACK HACK HACK - Bypass for testing */
	if (ec_id != EC_CHIP_ID)
		/* ret = -ENODEV; */
		ret = 0;
	release_region(0x2e, 2);

ec_check_exists_ret:
	return ret;
}

/**
 * ec_wait_obf_set - Wait for EC output buffer have data
 */
static int ec_wait_obf_set(void)
{
	int retries = 0;

	do {
		if (inb(EC_CMD_PORT) & BIT(0))
			return 0;
		udelay(EC_UDELAY);
	} while (retries++ < EC_MAX_RETRY);
	pr_debug("Error");
	return -EBUSY;
}

/**
 * ec_wait_obf_set - Wait for EC input buffer to be clear
 */
static int ec_wait_ibf_clear(void)
{
	int retries = 0;

	do {
		if (!(inb(EC_CMD_PORT) & BIT(1)))
			return 0;
		udelay(EC_UDELAY);
	} while (retries++ < EC_MAX_RETRY);
	pr_debug("Error");
	return -EBUSY;
}

/**
 * ec_clear_obf - Clear and old data in EC output buffer
 */
static int ec_clear_obf(void)
{
	int retries = 0;

	do {
		if (!(inb(EC_CMD_PORT) & BIT(0)))
			return 0;
		inb(EC_DAT_PORT);
		udelay(EC_UDELAY);
	} while (retries++ < EC_MAX_RETRY);
	pr_debug("Error");
	return -EBUSY;
}

/**
 * ec_send_command - Send a read/write register command to the EC
 *
 * Start by sending  0x88 to the command port, then send HI and LO bytes.
 * Between writes check that the EC IBF is clear.
 */
static int ec_send_command(u16 command)
{
	int ret;

	ret = ec_wait_ibf_clear();
	if (ret)
		goto ec_send_cmd_out;
	outb(0x88, EC_CMD_PORT);

	ret = ec_wait_ibf_clear();
	if (ret)
		goto ec_send_cmd_out;
	outb((command >> 8) & 0xff, EC_DAT_PORT);

	ret = ec_wait_ibf_clear();
	if (ret)
		goto ec_send_cmd_out;
	outb(command & 0xff, EC_DAT_PORT);

ec_send_cmd_out:
	return ret;
}

/**
 * ec_read_byte - Read a register from the EC
 *
 * Read a register values from the EC data port.
 * Clears EC buffer before read command issued
 * and waits for EC OBF to be set.
 */
static int ec_read_byte(u16 command, u8 *data)
{
	int ret;

	mutex_lock(&ec_lock);
	ret = ec_clear_obf();
	if (ret)
		goto ec_read_byte_out;

	ret = ec_send_command(command);
	if (ret)
		goto ec_read_byte_out;

	ret = ec_wait_obf_set();
	if (ret)
		goto ec_read_byte_out;

	*data = inb(EC_DAT_PORT);


ec_read_byte_out:
	mutex_unlock(&ec_lock);
	return ret;
}

/**
 * ec_write_byte - Write a register on the EC
 *
 * Write a values to a register on EC.
 * Clears EC buffer before read command issued
 * and waits for EC IBF to be set. When writing to the EC
 * bit 7 of the HI command bye should be set.
 */
static int ec_write_byte(u16 command, u8 data)
{
	int ret;

	mutex_lock(&ec_lock);
	ret = ec_send_command(command | 0x8000);
	if (ret)
		goto ec_read_byte_out;

	ret = ec_wait_ibf_clear();
	if (ret)
		goto ec_read_byte_out;

	outb(data, EC_DAT_PORT);

ec_read_byte_out:
	mutex_unlock(&ec_lock);
	return ret;
}

/**
 * ec_get_fan_status - Get the fan status from the EC
 *
 * TODO:
 *  - Check if fan status changes when the fan is turned off (does not seem the case)
 *  - Change function so fan status returns true and not false
 *  - Clean up switch-case with range values in if statements
 * BUGS:
 *  - Fan range of 0x14-0x19 seem to return a TRUE status even though they dont exist?
 *      Possible workaround: No TS-x73A is known to have more than 3 fans, maybe just add option to ignore?
 * NOTES:
 *  - On TS-473A, fan 0 is the disk fan and fan 6 is the CPU fan
 */
static int ec_get_fan_status(unsigned int fan)
{
	u16 reg;
	u8 value;

	switch (fan) {
	case 0:
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
		reg = 0x242;
		break;
	case 6:
	case 7:
		reg = 0x244;
		break;
	case 0x14:
	case 0x15:
	case 0x16:
	case 0x17:
	case 0x18:
	case 0x19:
		reg = 0x259;
		break;
	case 0x1e:
	case 0x1f:
	case 0x20:
	case 0x21:
	case 0x22:
	case 0x23:
		reg = 0x25a;
		break;
	default:
		return -EINVAL;
	}

	if (ec_read_byte(reg, &value))
		return -EBUSY;

	switch (fan) {
	case 0:
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
		return ((value >> (fan & 0x1f)) & 1) == 0;
	case 6:
	case 7:
		return ((value >> ((fan - 0x06) & 0x1f)) & 1) == 0;
	case 0x14:
	case 0x15:
	case 0x16:
	case 0x17:
	case 0x18:
	case 0x19:
		return ((value >> ((fan - 0x14) & 0x1f)) & 1) == 0;
	case 0x1e:
	case 0x1f:
	case 0x20:
	case 0x21:
	case 0x22:
	case 0x23:
		return ((value >> ((fan - 0x1e) & 0x1f)) & 1) == 0;
	default:
			return -EINVAL;
	}

}

static int ec_get_fan_rpm(unsigned int fan)
{
	u8 tmp;
	u16 value, reg_a, reg_b;
	int ret;

	if (fan >= 0 && fan <= 5) {
		reg_a = (fan + 0x312) * 2;
		reg_b = (fan * 2) + 0x625;
	} else if (fan == 6 || fan == 7) {
		reg_a = (fan + 0x30a) * 2;
		reg_b = ((fan - 6) * 2) + 0x621;
	} else if (fan == 10) {
		reg_a = 0x65b;
		reg_b = 0x65a;
	} else if (fan == 0x0b) {
		reg_a = 0x65e;
		reg_b = 0x65d;
	} else if (fan >= 0x14 && fan <= 0x19) {
		reg_a = (fan + 0x30e) * 2;
		reg_b = ((fan - 0x14) * 2) + 0x645;
	} else if (fan >= 0x1e && fan <= 0x23) {
		reg_a = (fan + 0x2f8) * 2;
		reg_b = ((fan - 0x1e) * 2) + 0x62d;
	} else {
		return -EINVAL;
	}

	ret = ec_read_byte(reg_a, &tmp);
	if (ret)
		return ret;
	value = tmp << 8;

	ret = ec_read_byte(reg_b, &tmp);
	if (ret)
		return ret;
	return value | tmp;
}

static int ec_get_fan_pwm(unsigned int fan)
{
	u16 reg;
	u8 value;
	int ret;

	if (fan >= 0 && fan <= 5)
		reg = 0x22e;
	else if (fan == 6 || fan == 7)
		reg = 0x24b;
	else if (fan >= 0x14 && fan <= 0x19)
		reg = 0x22f;
	else if (fan >= 0x1e && fan <= 0x23)
		reg = 0x23b;
	else
		return -EINVAL;

	ret = ec_read_byte(reg, &value);
	if (ret)
		return -1337;
	return (value * 0x100 - value) / 100;
}



static int ec_set_fan_pwm(unsigned int fan, u8 value)
{
	u16 reg_a, reg_b;
	int ret;

	value = (value * 100) / 0xff;

	if (fan >= 0 && fan <= 5) {
		reg_a = 0x220;
		reg_b = 0x22e;
	} else if (fan == 6 || fan == 7) {
		reg_a = 0x223;
		reg_b = 0x24b;
	} else if (fan >= 0x14 && fan <= 0x19) {
		reg_a = 0x221;
		reg_b = 0x22f;
	} else if (fan >= 0x1e && fan <= 0x23) {
		reg_a = 0x222;
		reg_b = 0x23b;
	} else {
		return -EINVAL;
	}

	ret = ec_write_byte(reg_a, 0x10);
	if (ret)
		return ret;

	ret = ec_write_byte(reg_b, value);
	if (ret)
		return ret;

	return 0;
}

static int ec_get_temperature(unsigned int sensor)
{
	u8 value;
	int ret;
	u16 reg;

	switch (sensor) {
	case 0:
	case 1:
		/* CPU */
		reg = 0x600 + sensor;
		break;
	case 5:
	case 6:
	case 7:
		reg = 0x5fd + sensor;
		break;
	case 10:
		reg = 0x659;
		break;
	case 0xb:
		reg = 0x65c;
		break;
	case 0xf:
	case 0x10:
	case 0x11:
	case 0x12:
	case 0x13:
	case 0x14:
	case 0x15:
	case 0x16:
	case 0x17:
	case 0x18:
	case 0x19:
	case 0x1a:
	case 0x1b:
	case 0x1c:
	case 0x1d:
	case 0x1e:
	case 0x1f:
	case 0x20:
	case 0x21:
	case 0x22:
	case 0x23:
	case 0x24:
	case 0x25:
	case 0x26:
		reg = 0x5f7 + sensor;
		break;
	default:
		return -EINVAL;
	}

	ret = ec_read_byte(reg, &value);
	if (ret)
		return ret;
	return value;
}

/**
 * ec_vpd_parse_data - Parses VPD entry according to its type
 */
static ssize_t ec_vpd_parse_data(struct ec_vpd_entry *vpd, char *raw, char *buf)
{
	ssize_t ret = 0;
	int i, offs;
	time64_t ts, time_bytes = 0;

	switch (vpd->type) {
	case 0:
		/* Field is a string */
		strncpy(buf, raw, vpd->length);
		buf[vpd->length + 1] = '\0';
		ret = vpd->length + 1;
		break;
	case 1:
		/* Field is a number, convert endianness */
		ret += scnprintf(buf, PAGE_SIZE, "0x");
		for (i = 0; i < vpd->length; i++) {
			offs = (i + 1) * 2;
			ret += scnprintf(buf + offs, PAGE_SIZE - offs, "%02x", raw[(vpd->length - i) - 1]);
		}
		break;
	case 2:
		/* Field is date */
		memcpy(&time_bytes, raw, vpd->length);
		ts = mktime64(2013, 1, 1, 0, 0, 0) + (time_bytes * 0x3c);
		ret += scnprintf(buf, PAGE_SIZE, "%ptTs", &ts);
		break;
	default:
		ret = -EINVAL;
	}
	return ret;
}

/**
 * ec_vpd_entry_show - Read the VPD entry associated with the vpd attribute.
 */
static ssize_t ec_vpd_entry_show(struct device *dev, struct ec_vpd_attribute *attr, char *buf)
{
	char raw_data[EC_VPD_TABLE_SIZE + 1] = {0};
	ssize_t i, reg_a, reg_b, reg_c;

	pr_debug("reading entry: Ta:%X Of:%x Ty:%x Le:%x", attr->vpd.table, attr->vpd.offset, attr->vpd.type, attr->vpd.length);
	switch (attr->vpd.table) {
	case 0:
		reg_a = EC_VPD_TABLE0_REG_A;
		reg_b = EC_VPD_TABLE0_REG_B;
		reg_c = EC_VPD_TABLE0_REG_C;
		break;
	case 1:
		reg_a = EC_VPD_TABLE1_REG_A;
		reg_b = EC_VPD_TABLE1_REG_B;
		reg_c = EC_VPD_TABLE1_REG_C;
		break;
	case 2:
		reg_a = EC_VPD_TABLE2_REG_A;
		reg_b = EC_VPD_TABLE2_REG_B;
		reg_c = EC_VPD_TABLE2_REG_C;
		break;
	case 3:
		reg_a = EC_VPD_TABLE3_REG_A;
		reg_b = EC_VPD_TABLE3_REG_B;
		reg_c = EC_VPD_TABLE3_REG_C;
		break;
	default:
		return -EINVAL;
	}

	for (i = 0; i < attr->vpd.length; i++) {
		if (ec_write_byte(reg_a, ((attr->vpd.offset + i) >> 8) & 0xff) ||
			ec_write_byte(reg_b, (attr->vpd.offset + i) & 0xff) ||
			ec_read_byte(reg_c, &raw_data[i]))
			return -EBUSY;
	}

	return ec_vpd_parse_data(&attr->vpd, raw_data, buf);
}


/**
 * ec_vpd_entry_show - Read the VPD entry associated with the vpd attribute.
 */
static ssize_t ec_vpd_read_raw(int table, int offset, int length, char *buf)
{
	ssize_t i, reg_a, reg_b, reg_c;

	pr_debug("VPD read raw: Ta:%X Of:%x Le:%x", table, offset, length);
	switch (table) {
	case 0:
		reg_a = EC_VPD_TABLE0_REG_A;
		reg_b = EC_VPD_TABLE0_REG_B;
		reg_c = EC_VPD_TABLE0_REG_C;
		break;
	case 1:
		reg_a = EC_VPD_TABLE1_REG_A;
		reg_b = EC_VPD_TABLE1_REG_B;
		reg_c = EC_VPD_TABLE1_REG_C;
		break;
	case 2:
		reg_a = EC_VPD_TABLE2_REG_A;
		reg_b = EC_VPD_TABLE2_REG_B;
		reg_c = EC_VPD_TABLE2_REG_C;
		break;
	case 3:
		reg_a = EC_VPD_TABLE3_REG_A;
		reg_b = EC_VPD_TABLE3_REG_B;
		reg_c = EC_VPD_TABLE3_REG_C;
		break;
	default:
		return -EINVAL;
	}

	for (i = 0; i <= length; i++) {
		if (ec_write_byte(reg_a, ((offset + i) >> 8) & 0xff) ||
			ec_write_byte(reg_b, (offset + i) & 0xff) ||
			ec_read_byte(reg_c, &buf[i]))
			return -EBUSY;
	}

	return 0;
}

/**
 * ec_vpd_entry_store - Store attribute data to associated VPD entry.
 *
 * Under normal circumstances VPD should not be written to.
 */
/*
*static ssize_t ec_vpd_entry_store(struct device *dev, struct ec_vpd_attribute *attr, const char *buf, size_t count) {
*	ssize_t i, reg_a, reg_b, reg_c;
*
*	if ((count > attr->vpd.length) || ((attr->vpd.offset + attr->vpd.length) > EC_VPD_TABLE_SIZE))
*		return -ENOBUFS;
*
*	switch (attr->vpd.table) {
*	case 0:
*		reg_a = EC_VPD_TABLE0_REG_A;
*		reg_b = EC_VPD_TABLE0_REG_B;
*		reg_c = EC_VPD_TABLE0_REG_C;
*		break;
*	case 1:
*		reg_a = EC_VPD_TABLE1_REG_A;
*		reg_b = EC_VPD_TABLE1_REG_B;
*		reg_c = EC_VPD_TABLE1_REG_C;
*		break;
*	case 2:
*		reg_a = EC_VPD_TABLE2_REG_A;
*		reg_b = EC_VPD_TABLE2_REG_B;
*		reg_c = EC_VPD_TABLE2_REG_C;
*		break;
*	case 3:
*		reg_a = EC_VPD_TABLE3_REG_A;
*		reg_b = EC_VPD_TABLE3_REG_B;
*		reg_c = EC_VPD_TABLE3_REG_C;
*		break;
*	default:
*		return -EINVAL;
*	}
*
*	for (i = 0; i < attr->vpd.length; i++) {
*		if (ec_write_byte(reg_a, (i >> 8) & 0xff) ||
*			ec_write_byte(reg_b, i & 0xff) ||
*			ec_write_byte(reg_c, buf[i]))
*			return -EBUSY;
*	}
*	return 0;
*}
*/

/**
 * ec_vpd_table_show - Dump an entire VPD table as binary.
 *
 * Read the entire 512 long VPD data from the EC, used for debug and development.
 */
static ssize_t ec_vpd_table_show(struct device *dev, struct ec_vpd_attribute *attr, char *buf)
{
	int table_id = 0, ret;
	u16 i, reg_a, reg_b, reg_c;
	ssize_t count = 0;
	u8 tmp;

	if (!(sscanf(attr->attr.name, "dbg_table%d", &table_id) == 1))
		return -EINVAL;

	switch (table_id) {
	case 0:
		reg_a = EC_VPD_TABLE0_REG_A;
		reg_b = EC_VPD_TABLE0_REG_B;
		reg_c = EC_VPD_TABLE0_REG_C;
		break;
	case 1:
		reg_a = EC_VPD_TABLE1_REG_A;
		reg_b = EC_VPD_TABLE1_REG_B;
		reg_c = EC_VPD_TABLE1_REG_C;
		break;
	case 2:
		reg_a = EC_VPD_TABLE2_REG_A;
		reg_b = EC_VPD_TABLE2_REG_B;
		reg_c = EC_VPD_TABLE2_REG_C;
		break;
	case 3:
		reg_a = EC_VPD_TABLE3_REG_A;
		reg_b = EC_VPD_TABLE3_REG_B;
		reg_c = EC_VPD_TABLE3_REG_C;
		break;
	default:
		return -EINVAL;
	}

	for (i = 0; i < 64; i++) {
		/* pr_debug("VPD Table read: rega=%04x, regb=%04x, regc=%04x, table=%x, i=%x, last='%c', count=%d, bufp=%d", reg_a & 0xffff, reg_b & 0xffff, reg_c & 0xffff, table_id, i, tmp,(int)count, &buf[i]); */
		if (ec_write_byte(reg_a, (i >> 8) & 0xff) || ec_write_byte(reg_b, i & 0xff))
			return -EBUSY;
		ret = ec_read_byte(reg_c, &tmp);
		if (ret)
			return -EBUSY;
		/* count += scnprintf(&buf[i], PAGE_SIZE-i, "%c", tmp); */
	}
	return count;
}

/**
 * ec_ac_recovery_show - Show AC recovery mode
 *
 * See ec_ac_recovery_store for valid values returned
 */
static ssize_t ec_ac_recovery_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	u8 value, ret;

	ret = ec_read_byte(EC_AC_RECOVER_REG, &value);
	if (ret)
		return ret;
	return scnprintf(buf, PAGE_SIZE, "%d", value);
}

/**
 * ec_ac_recovery_show - Store AC recovery mode
 *
 * Valid values:
 *  0: Remain off
 *  1: Power on
 *  2: Resume last power state
 */
static ssize_t ec_ac_recovery_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	u8 value;
	int ret;

	ret = kstrtou8(buf, 10, &value);
	if (ret)
		return ret;

	if ((value < 0) || (value > 2))
		return -ERANGE;

	ret = ec_write_byte(EC_AC_RECOVER_REG, value);
	if (ret)
		return ret;

	return count;
}

/**
 * ec_fw_version_show - Show EC firmware version
 */
static ssize_t ec_fw_version_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	u8 value;
	int i;
	int ret;
	int read = 0;

	for (i = 0; i <  EC_FW_VER_LEN; i++) {
		ret = ec_read_byte(i, &value);
		if (ret)
			return ret;
		read += scnprintf(buf + i, PAGE_SIZE - i, "%c", value);
	}

	return read;
}

/**
 * ec_cpld_version_show - Show CPLD firmware version
 */
static ssize_t ec_cpld_version_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	u8 value;
	int ret;

	ret = ec_read_byte(EC_CPLD_VER_REG, &value);
	if (ret)
		return ret;
	return scnprintf(buf, PAGE_SIZE, "0x%x", value);
}

/**
 * ec_eup_mode_show - Show EuP mode
 */
static ssize_t ec_eup_mode_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	u8 ret, value;

	ret = ec_read_byte(EC_EUP_SUPPRT_REG, &value);
	if (ret)
		return ret;

	if (!(value & 0x08))
		return -ENOTSUPP;

	ret = ec_read_byte(EC_EUP_MODE_REG, &value);
	if (ret)
		return ret;

	return scnprintf(buf, PAGE_SIZE, "%d", (value & 0x08) ? 1 : 0);
}

/**
 * ec_eup_mode_store - Store EuP mode
 *
 * Valid values:
 *  0: Off
 *  1: On
 */
static ssize_t ec_eup_mode_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	u8 value, tmp;
	int ret;

	ret = ec_read_byte(EC_EUP_SUPPRT_REG, &value);
	if (ret)
		return ret;

	if (!(value & 0x08))
		return -ENOTSUPP;

	ret = kstrtou8(buf, 10, &value);
	if (ret)
		return ret;

	if ((value < 0) || (value > 1))
		return -ERANGE;

	ret = ec_read_byte(EC_EUP_MODE_REG, &tmp);
	if (ret)
		return ret;

	tmp &= 0xf7;
	tmp |= value ? 0x08 : 0x00;

	ret = ec_write_byte(EC_EUP_MODE_REG, tmp);
	if (ret)
		return ret;

	return count;
}

static int ec_button_get_state(void)
{
	u8 value;
	int ret;

	ret = ec_read_byte(0x143, &value);
	if (ret)
		return ret;
	pr_debug("Buttons: %x", value);
	return value;
}

static int ec_led_set_brightness(u8 brightness)
{
	int ret;
	u8 value;

	ret = ec_write_byte(0x243, brightness);
	if (ret)
		return ret;

	ret = ec_read_byte(0x245, &value);
	if (ret)
		return ret;
	value |= 0x10;

	ret = ec_write_byte(0x245, value);
	if (ret)
		return ret;

	ret = ec_write_byte(0x246, brightness);
	if (ret)
		return ret;

	ret = ec_read_byte(0x245, &value);
	if (ret)
		return ret;
	value |= 0xef;

	ret = ec_write_byte(0x245, value);
	if (ret)
		return ret;

	return 0;
}

static int ec_led_set_status(u8 mode)
{
	int ret;

	ret = ec_write_byte(0x155, mode);
	if (ret)
		return ret;
	return 0;
}

static int ec_led_set_usb(u8 mode)
{
	int ret;

	ret = ec_write_byte(0x155, mode);
	if (ret)
		return ret;
	return 0;
}

static int ec_led_set_disk(u8 mode)
{
	int ret;

	ret = ec_write_byte(0x155, mode);
	if (ret)
		return ret;
	return 0;
}

static void ec_button_poll(struct input_dev *input)
{

	int state = ec_button_get_state();

	input_event(input, EV_KEY, BTN_1, !!(state & EC_BTN_RESET));
	input_event(input, EV_KEY, BTN_0, !!(state & EC_BTN_COPY));
	input_sync(input);
	pr_debug("Poll buttons %x (reset=%d, copy=%d)", state, state & EC_BTN_RESET, state & EC_BTN_COPY);
}

static struct qnap_model_config *tsx73a_locate_config(void)
{
	int i;
	char mb_model[33];
	char bp_model[33];

	ec_vpd_read_raw(0x00, 0x42, 0x20, mb_model);
	ec_vpd_read_raw(0x01, 0x6a, 0x20, bp_model);
	mb_model[32] = 0;
	bp_model[32] = 0;

	pr_debug("Looking for config for MB=%s BP=%s", mb_model, bp_model);

	for (i = 0; tsx73a_configs[i].model_name; i++) {
		pr_debug("Checking config model %s", tsx73a_configs[i].model_name);
		if ((memcmp(tsx73a_configs[i].mb_code.code, mb_model +  tsx73a_configs[i].mb_code.offset, tsx73a_configs[i].mb_code.length) == 0) &&
				(memcmp(tsx73a_configs[i].bp_code.code, bp_model +  tsx73a_configs[i].bp_code.offset, tsx73a_configs[i].bp_code.length) == 0))
		  return &tsx73a_configs[i];
	}

	return NULL;
}

static int ec_driver_probe(struct platform_device *pdev)
{
	int ret;
	struct device *hwmon_dev;
	struct qnap_model_config *config;
	int i;

	config = tsx73a_locate_config();
	if (!config) {
		pr_debug("Failed to find configuration for device model");
		ret = -EINVAL;
		goto ec_probe_ret;
	}
	pr_debug("Detected QNAP NAS model %s", config->model_name);

	ret = sysfs_create_group(&pdev->dev.kobj, &ec_attr_group);
	if (ret)
		goto ec_probe_ret;

	ret = sysfs_create_group(&pdev->dev.kobj, &ec_vpd_attr_group);
	if (ret)
		goto ec_probe_ret;


	for (i = 0; i < TSX73A_MAX_HWMON_CHANNELS; ++i)
		ec_hwmon_fan_config[i] = HWMON_F_INPUT | HWMON_F_LABEL;
	ec_hwmon_fan_config[i] = 0;

	for (i = 0; i < TSX73A_MAX_HWMON_CHANNELS; ++i)
		ec_hwmon_temp_config[i] = HWMON_T_INPUT | HWMON_T_LABEL;
	ec_hwmon_temp_config[i] = 0;

	for (i = 0; i < TSX73A_MAX_HWMON_CHANNELS; ++i)
		ec_hwmon_pwm_config[i] = HWMON_PWM_INPUT;
	ec_hwmon_pwm_config[i] = 0;



	hwmon_dev = devm_hwmon_device_register_with_info(&pdev->dev, "tsx73a_ec", config, &ec_hwmon_chip_info, NULL);
	if (IS_ERR(hwmon_dev)) {
		ret = PTR_ERR(hwmon_dev);
		goto ec_probe_sysfs_ret;
	}

	ec_bdev = devm_input_allocate_device(&pdev->dev);
	if (IS_ERR(ec_bdev)) {
		ret = PTR_ERR(ec_bdev);
		goto ec_probe_sysfs_ret;
	}
	ec_bdev->name = "EC Buttons";
	ec_bdev->dev.parent = &pdev->dev;
	ec_bdev->phys = DRVNAME "/input0";
	ec_bdev->id.bustype = BUS_HOST;
	input_set_capability(ec_bdev, EV_KEY,  BTN_1);
	input_set_capability(ec_bdev, EV_KEY,  BTN_0);

	ret = input_setup_polling(ec_bdev, ec_button_poll);
	if (ret)
		goto ec_probe_sysfs_ret;
	pr_debug("Setup button polling");

	input_set_poll_interval(ec_bdev, 100);

	ret = input_register_device(ec_bdev);
	if (ret)
		goto ec_probe_sysfs_ret;


	pr_debug("Probe");
	goto ec_probe_ret;


ec_probe_sysfs_ret:
	sysfs_remove_group(&pdev->dev.kobj, &ec_attr_group);
	sysfs_remove_group(&pdev->dev.kobj, &ec_vpd_attr_group);
ec_probe_ret:
	return ret;
}

static int ec_driver_remove(struct platform_device *pdev)
{
	sysfs_remove_group(&pdev->dev.kobj, &ec_attr_group);
	sysfs_remove_group(&pdev->dev.kobj, &ec_vpd_attr_group);
	input_unregister_device(ec_bdev);
	pr_debug("Remove");
	return 0;
}

static umode_t ec_hwmon_is_visible(const void *data, enum hwmon_sensor_types type, u32 attribute, int channel)
{
	u8 tmp;
	struct qnap_model_config *config = (struct qnap_model_config *)data;

	switch (type) {
	case hwmon_fan:
		/**
		 * The fans on the QNAP are a bit annoying, you would think that the checking the
		 * fan status with the EC using ec_get_fan_status will return an error if the fan is
		 * invalid, but this is not the case for fan numbers that are 20-25/30-35. Using a combination of
		 * the fan status and the fan speed is also impossible to detect that a fan is invalid,
		 * when reading the speed of an invalid fan, it returns either max u16 (65535) or -1, thats in most cases
		 * for fans 33 and 34, the speed returned is 4853 and 11935 which breaks the rule. The system fan observed max
		 * speed is just shy of 2000rpm and the cpu fan ~5000 rpm in max speed. Therfore, if the check is run when fans
		 * might be at max speed, present fans might be erroneously omitted.
		 *
		 * There are three possible ways to bypass this:
		 * 1. Check fan status and speed, ignore fan number above 29.
		 * 2. Check fan status only, ignore all fans 19, this is the approach currently implemented, tested on TS-473A.
		 * 3. Be less generic and use per model configs / request bitmask from user as parameter on load.
		 *
		 * A note about the fan number: on the TS-473A, the system fan is 0 and the CPU fan is 6. So multiple fans
		 * in a system does NOT mean sequential fan numbers.s
		 */

		/*if (!ec_get_fan_status(channel) && channel < 20)
			*/
		if (BIT(channel) & config->fan_mask)
			return S_IRUGO;
		break;
	case hwmon_temp:
		/**
		 * Valid temperature channel is a channel which it's temperature ranges from 0 to 254
		 * All invalid channels return either 255 (tested on TS-473A).
		 *
		 * NOTE: Sensors 0 and 7 might be the same physical sensor?
		 */
		if (BIT(channel) & config->temp_mask)
			return S_IRUGO;
		break;
	case hwmon_pwm:
		/**
		 * Same story as the fans, reading a value should be between 0 and 255. If the value is out of range, it's invalid
		 * however, pwm channels above 19 seem to return 255.
		 *
		 * From testing it seems to be that PWM value for the fan groups seem to be controlled together (0-5, 6&7, 10, 11, 20-25, 30-35)
		 * so only one attribute/channel is needed per group. The TS-473A only has a single system fan and a single CPU fan, so
		 * no tested physically. Maybe a per model config would be best?
		 */
		if (BIT(channel) & config->pwm_mask)
			return S_IRUGO | S_IWUSR;
	default:
		break;
	}

	return 0;
}

static int ec_hwmon_read(struct device *dev, enum hwmon_sensor_types type, u32 attr, int channel, long *val)
{
	switch (type) {
	case hwmon_fan:
		*val =  ec_get_fan_rpm(channel);
		break;

	case hwmon_temp:
		*val =  ec_get_temperature(channel) * 1000;
		break;

	case hwmon_pwm:
		*val =  (ec_get_fan_pwm(channel) * 0xff) / 100;
		break;
	default:
		return -EOPNOTSUPP;
	}
	return 0;
}

static int ec_hwmon_write(struct device *dev, enum hwmon_sensor_types type, u32 attr, int channel, long val)
{
	if (type == hwmon_pwm) {
		pr_debug("Setting fan %x to %x", channel, (u8)val);
		 return ec_set_fan_pwm(channel, (u8)val);
	}
	return -EOPNOTSUPP;
}

static int ec_hwmon_read_string(struct device *dev, enum hwmon_sensor_types type, u32 attr, int channel, const char **str)
{
	pr_debug("String: t:%d a:%d c:%d", type, attr, channel);

	switch (type) {
	case hwmon_fan:
		break;

	case hwmon_temp:
		break;

	case hwmon_pwm:
		break;

	default:
		return -EOPNOTSUPP;
	}
	return -EOPNOTSUPP;
}



static int __init tsx73a_init(void)
{
	int ret;
	struct ec_platform_data platdata;

	ret = ec_check_exists();
	if (ret) {
		pr_err("Could not find ITE8528");
		goto tsx73a_exit_init;
	}

	ec_pdev = platform_device_alloc(DRVNAME, -1);
	if (!ec_pdev) {
		ret = -ENOMEM;
		goto tsx73a_exit_init;
	}

	ret = platform_device_add_data(ec_pdev, &platdata, sizeof(struct ec_platform_data));
	if (ret)
		goto tsx73a_exit_init_put;

	ret = platform_device_add_resources(ec_pdev, ec_resources, 2);
	if (ret)
		goto tsx73a_exit_init_put;

	ret = platform_device_add(ec_pdev);
	if (ret)
		goto tsx73a_exit_init_put;

	ret = platform_driver_register(&ec_pdriver);
	if (ret)
		goto tsx73a_exit_init_unregister;

	goto tsx73a_exit_init;

tsx73a_exit_init_unregister:
	platform_device_unregister(ec_pdev);
tsx73a_exit_init_put:
	platform_device_put(ec_pdev);
tsx73a_exit_init:
	return ret;
}

static void __exit tsx73a_exit(void)
{
	if (ec_pdev)
		platform_device_unregister(ec_pdev);

	platform_driver_unregister(&ec_pdriver);
}

module_init(tsx73a_init);
module_exit(tsx73a_exit);

MODULE_DESCRIPTION("QNAP TS-x73A EC Driver");
MODULE_VERSION("0.0.0");
MODULE_AUTHOR("0xGiddi - <giddi AT giddi.net>");
MODULE_LICENSE("GPL");
