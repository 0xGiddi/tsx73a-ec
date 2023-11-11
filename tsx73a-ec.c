#include <linux/module.h>
#include <linux/ioport.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>
#include <linux/hwmon.h>
#include "tsx73a-ec.h"

/**
 * TODO:
 *  Fan:
 *      - Get SPEED
 *      - Get STATUS
 *      - Get PWM
 *      - Set PWM
 *      - Set auto/manual?
 *  Temp:
 *      - Get TEMP
 *  CPLD:
 *      - Get version
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
 *  General:
 *      - Refactor ec_read_byte to return the value as signed int 
 *          w/ positive = raw value, negative = ERRNO
 *
*/

static struct class ec_class = {
    .name = "ec",
    .owner = THIS_MODULE,
};

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

static DEVICE_ATTR(fw_version, S_IRUGO, ec_fw_version_show, NULL);
static DEVICE_ATTR(cpld_version, S_IRUGO, ec_cpld_version_show, NULL);
static DEVICE_ATTR(ac_recovery, S_IRUGO | S_IWUSR, ec_ac_recovery_show,ec_ac_recovery_store);
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
 * */
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
static VPD_DEV_ATTR(enc_nickname, S_IRUGO | S_IWUSR, ec_vpd_entry_show, NULL, EC_VPD_ENC_NICKNAME); // Maybe add store here

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


static struct hwmon_channel_info const *hwmon_chan_info[] = {
    HWMON_CHANNEL_INFO(temp, HWMON_T_INPUT | HWMON_T_LABEL, HWMON_T_INPUT, HWMON_T_INPUT),
    HWMON_CHANNEL_INFO(fan, HWMON_F_INPUT, HWMON_F_INPUT, HWMON_F_INPUT),
    HWMON_CHANNEL_INFO(pwm,  HWMON_PWM_INPUT,  HWMON_PWM_INPUT,  HWMON_PWM_INPUT),
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

/**
 * ec_check_exists - Check that EC is ITE8528
 *
 * Check is done by reading idregs from the SIO port,
 * ID bytes should be 0x85 and 0x28 for the ITE8528.
 * Only SIO port combination on  0x2e,0x2f are checked
 * as this seems to be sufficiant for the QNAP system. 
 */
static int ec_check_exists(void) {
    int ret = 0;
    u16 ec_id;

    if(!request_muxed_region(0x2e, 2, DRVNAME)) {
        ret = -EBUSY;
        goto ec_check_exists_ret;
    }

    outb(0x20, 0x2e);
    ec_id = inb(0x2f) << 8;
    outb(0x21, 0x2e);
    ec_id |= inb(0x2f);

    // HACK HACK HACK - Bypass for testing
    if (EC_CHIP_ID != ec_id) 
        //ret = -ENODEV;
        ret = 0;
    release_region(0x2e, 2);

ec_check_exists_ret:
    return ret;
}

/**
 * ec_wait_obf_set - Wait for EC output buffer have data
 */
static int ec_wait_obf_set(void) {
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
static int ec_wait_ibf_clear(void) {
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
static int ec_clear_obf(void) {
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
static int ec_send_command(u16 command) {
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
static int ec_read_byte(u16 command, u8 *data) {
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
static int ec_write_byte(u16 command, u8 data) {
    int ret; 
    
    mutex_lock(&ec_lock);
    ret = ec_send_command(command & 0x8000);
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

static int ec_get_fan_status(unsigned int fan) {
    u16 reg; 
    u8 value;

    switch (fan)
    {
        case (fan <= 5):
            reg = 0x242;
            break;
        case (fan == 7 || fan == 6):
            reg = 0x244;
            break;
        case (fan >= 0x14 && fan <= 0x19):
            reg = 0x259;
            break;
        case (fan >= 0x1e && fan <= 0x23):
            reg = 0x25a;
            break;
        default:
            return -EINVAL;
    }

    if (ec_read_byte(reg, &value))
        return -EBUSY;

    switch (fan)
    {
        case (fan <= 5):
            return ((value >> (fan & 0x1f)) & 1) == 0;
        case (fan == 7 || fan == 6):
            return ((value >> ((fan - 0x06) & 0x1f)) & 1) == 0;
        case (fan >= 0x14 && fan <= 0x19):
            return ((value >> ((fan - 0x14) & 0x1f)) & 1) == 0;
        case (fan >= 0x1e && fan <= 0x23):
            return ((value >> ((fan - 0x1e) & 0x1f)) & 1) == 0;
        default:
            return -EINVAL;
    }
}

/**
 * ec_vpd_parse_data - Parses VPD entry according to its type
 */
static ssize_t ec_vpd_parse_data(struct ec_vpd_entry *vpd, char *raw, char *buf) {
    ssize_t ret = 0;
    int i, offs;
    time64_t ts, time_bytes=0;
    
    switch (vpd->type) {
        case 0: 
            // Field is a string
            strncpy(buf, raw, vpd->length);
            buf[vpd->length + 1] = '\0';
            ret = vpd->length + 1;
            break;
        case 1:
            // Field is a number, convert endianness
            ret += scnprintf(buf, PAGE_SIZE, "0x");
            for (i=0; i < vpd->length; i++) {
                offs = (i + 1) * 2;
                ret += scnprintf(buf + offs, PAGE_SIZE - offs, "%02x", raw[(vpd->length - i) - 1]);
            }
            break;
        case 2:
            // Field is date
            /**
            *  BUG BUG BUG: cannot seem to get the exact time right, cant't use `gmtime` as not exists in kernel API
            *      Using `sudo hal_app --vpd_get_field enc_sys_id=root,obj_index=0:0` returns '2023/05/22 14:14'
            *      raw tested VPD bytes are 63 5b 53, but this function returns `2023-05-22 16:03:00`. 
            *      Maybe DST/TZ/Leap correction in usernald API? It will not explain the minutes difference
            *      EC VPD value seems to be minutes since 2013/01/01. This isn't critical.
            */
            memcpy(&time_bytes, raw, vpd->length);
            ts = mktime64(2013, 1, 1,0, 0, 0) + (time_bytes * 0x3c); 
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
static ssize_t ec_vpd_entry_show(struct device *dev, struct ec_vpd_attribute *attr, char *buf) {
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

    // BUG BUG BUG: Only reading first byte
    for (i=0; i < attr->vpd.length; i++) {
        if(ec_write_byte(reg_a, ((attr->vpd.offset + i) >> 8) & 0xff) ||
            ec_write_byte(reg_b, (attr->vpd.offset + i) & 0xff) ||
            ec_read_byte(reg_c, &raw_data[i]))
            return -EBUSY;
    }
    pr_debug("VPD Data: %s", raw_data);
    return ec_vpd_parse_data(&attr->vpd, raw_data, buf);
}

/**
 * ec_vpd_entry_store - Store attribute data to associated VPD entry.
 * 
 * Under normal circumstances VPD should not be written to.
 */
/*
static ssize_t ec_vpd_entry_store(struct device *dev, struct ec_vpd_attribute *attr, const char *buf, size_t count) {
    ssize_t i, reg_a, reg_b, reg_c;

    if ((count > attr->vpd.length) || ((attr->vpd.offset + attr->vpd.length) > EC_VPD_TABLE_SIZE))
        return -ENOBUFS;

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

    for (i=0; i < attr->vpd.length; i++) {
        if(ec_write_byte(reg_a, (i >> 8) & 0xff) ||
            ec_write_byte(reg_b, i & 0xff) ||
            ec_write_byte(reg_c, buf[i]))
            return -EBUSY;
    }
    return 0;
}
*/

/**
 * ec_vpd_table_show - Dump an entire VPD table as binary.
 * 
 * Read the entire 512 long VPD data from the EC, used for debug and development.
 */
static ssize_t ec_vpd_table_show(struct device *dev, struct ec_vpd_attribute *attr, char *buf) {
    int table_id = 0, ret;
    u16 i, reg_a, reg_b, reg_c;
    ssize_t count = 0;
    u8 tmp;
    
    if (!(sscanf(attr->attr.name, "dbg_table%d", &table_id) == 1)) {
        return -EINVAL;
    }

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

    // BUG BUG BUG: Only reading first byte
    for (i=0; i < 64; i++) {
        pr_debug("VPD Table read: rega=%04x, regb=%04x, regc=%04x, table=%x, i=%x, last='%c', count=%d, bufp=%d", reg_a & 0xffff, reg_b & 0xffff, reg_c & 0xffff, table_id, i, tmp,(int)count, &buf[i]);
        if(ec_write_byte(reg_a, (i >> 8) & 0xff) || ec_write_byte(reg_b, i & 0xff)) 
            return -EBUSY;
        ret = ec_read_byte(reg_c, &tmp);
        if (ret)
            return -EBUSY;
        //count += scnprintf(&buf[i], PAGE_SIZE-i, "%c", tmp);
    }
    return count;
}

/**
 * ec_ac_recovery_show - Show AC recovery mode
 * 
 * See ec_ac_recovery_store for valid values returned
 */
static ssize_t ec_ac_recovery_show(struct device *dev, struct device_attribute *attr, char *buf) {
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
static ssize_t ec_ac_recovery_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {
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
static ssize_t ec_fw_version_show(struct device *dev, struct device_attribute *attr, char *buf) {
    u8 value;
    int i; 
    int ret;
    int read = 0;
    
    for (i=0; i <  EC_FW_VER_LEN; i++) {
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
static ssize_t ec_cpld_version_show(struct device *dev, struct device_attribute *attr, char *buf) {
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
static ssize_t ec_eup_mode_show(struct device *dev, struct device_attribute *attr, char *buf) {
    u8 ret, value;
    
    ret = ec_read_byte(EC_EUP_SUPPRT_REG, &value);
    if (ret)
        return ret;

    if (!(value & 0x08))
        return -ENOSYS;

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
static ssize_t ec_eup_mode_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {
    u8 value, tmp;
    int ret;
    
    ret = ec_read_byte(EC_EUP_SUPPRT_REG, &value);
    if (ret)
        return ret;

    if (!(value & 0x08))
        return -ENOSYS;

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

static int ec_driver_probe(struct platform_device *pdev) {
    int ret;
    struct device* hwmon_dev;

    ret = sysfs_create_group(&pdev->dev.kobj, &ec_attr_group);
    if (ret)
        goto ec_probe_ret;

    ret = sysfs_create_group(&pdev->dev.kobj, &ec_vpd_attr_group);
    if (ret)
        goto ec_probe_ret;

    hwmon_dev = devm_hwmon_device_register_with_info(&pdev->dev, "tsx73a_ec", dev_get_drvdata(&pdev->dev), &ec_hwmon_chip_info, NULL);
    if (IS_ERR(hwmon_dev)) {
        ret = PTR_ERR(hwmon_dev);
        goto ec_probe_sysfs_ret;
    }


    pr_debug("Probe");
    goto ec_probe_ret;
    

ec_probe_sysfs_ret:
    sysfs_remove_group(&pdev->dev.kobj, &ec_attr_group);
    sysfs_remove_group(&pdev->dev.kobj, &ec_vpd_attr_group);
ec_probe_ret:
    return ret;
}

static int ec_driver_remove(struct platform_device *pdev) {
    sysfs_remove_group(&pdev->dev.kobj, &ec_attr_group);
    sysfs_remove_group(&pdev->dev.kobj, &ec_vpd_attr_group);

    pr_debug("Remove");
    return 0;
}

static umode_t ec_hwmon_is_visible(const void* const_data, enum hwmon_sensor_types type, u32 attribute, int channel) {
    return S_IRUGO;
    switch (type) {
        case hwmon_fan: 
            return S_IRUGO;
        default:
            break;
    }

    return 0;
}

static int ec_hwmon_read(struct device *dev, enum hwmon_sensor_types type, u32 attr, int channel, long *val) {
    return -EOPNOTSUPP;
}

static int ec_hwmon_write(struct device *dev, enum hwmon_sensor_types type, u32 attr, int channel, long val) {
    return -EOPNOTSUPP;
}

static int ec_hwmon_read_string(struct device *dev, enum hwmon_sensor_types type, u32 attr, int channel, const char **str) {
    *str = "sensor str";
    return 0;
}

static int __init tsx73a_init(void) {
    int ret;
    struct ec_platform_data platdata;
    
    
    ret = ec_check_exists();
    if (ret) {
        pr_err("Could not find ITE8528");
        goto tsx73a_exit_init;    
    }

    ret = class_register(&ec_class);
    if (ret)
        goto tsx73a_exit_init;
    

    ec_pdev = platform_device_alloc(DRVNAME, -1);
    if (!ec_pdev) {
        ret = -ENOMEM;
        goto tsx73a_exit_init_class;
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
tsx73a_exit_init_class:
    class_unregister(&ec_class);
tsx73a_exit_init:
    return ret;
}

static void __exit tsx73a_exit(void) {
    if (ec_pdev)
        platform_device_unregister(ec_pdev);

    platform_driver_unregister(&ec_pdriver);
    class_unregister(&ec_class);
}

module_init(tsx73a_init);
module_exit(tsx73a_exit);

MODULE_DESCRIPTION("QNAP TS-x73A EC Driver");
MODULE_VERSION("0.0.0");
MODULE_AUTHOR("0xGiddi - <giddi AT giddi.net>");
MODULE_LICENSE("GPL");