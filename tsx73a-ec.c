#include "tsx73a-ec.h"

static struct platform_device *ec_pdev;

static struct resource ec_resources[] = {
    DEFINE_RES_IO_NAMED(EC_CMD_PORT, 1, "tsx73a-ec"),
    DEFINE_RES_IO_NAMED(EC_DAT_PORT, 1, "tsx73a-ec")
};

static struct platform_driver ec_pdriver = {
    .driver = {
        .owner = THIS_MODULE,
        .name  = "tsx73a-ec",
    },
    .probe	= ec_driver_probe,
    .remove	= ec_driver_remove
};

static DEVICE_ATTR(fw_version, S_IRUGO, NULL, NULL);

static struct attribute *ec_attrs[] = {
    &dev_attr_fw_version.attr,
    NULL
};

static const struct attribute_group ec_attr_group = {
    .attrs = ec_attrs
};

/* These are generic VPD attributes -  exists to make future development easier */
static VPD_DEV_ATTR(dbg_table0, S_IRUGO, ec_vpd_table_show, NULL, 0);
static VPD_DEV_ATTR(dbg_table1, S_IRUGO, ec_vpd_table_show, NULL, 0);
static VPD_DEV_ATTR(dbg_table2, S_IRUGO, ec_vpd_table_show, NULL, 0);
static VPD_DEV_ATTR(dbg_table3, S_IRUGO, ec_vpd_table_show, NULL, 0);
static VPD_DEV_ATTR(dbg_read_single, S_IRUGO | S_IWUSR, NULL, ec_vpd_read_single_store, 0);
/* These are known VPD attributes as discovered from RE the hal library */
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
static VPD_DEV_ATTR(enc_nickname, S_IRUGO | S_IWUSR, ec_vpd_entry_show, ec_vpd_entry_store, EC_VPD_ENC_NICKNAME);
static VPD_DEV_ATTR(enc_serial, S_IRUGO, ec_vpd_entry_show, NULL, EC_VPD_ENC_SERIAL);

static struct attribute *ec_vpd_attrs[] = {
    &dev_attr_dbg_table0.attr,
    &dev_attr_dbg_table1.attr,
    &dev_attr_dbg_table2.attr,
    &dev_attr_dbg_table3.attr,
    &dev_attr_dbg_read_single.attr,
    
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

static int ec_check_exists(void) {
    int ret = 0;
    u16 ec_id;

    if(!request_muxed_region(0x2e, 2, "tsx73a-ec")) {
        ret = -EBUSY;
        goto ec_check_exists_ret;
    }

    outb(0x20, 0x2e);
    ec_id = inb(0x2f) << 8;
    outb(0x21, 0x2e);
    ec_id |= inb(0x2f);

    if (EC_CHIP_ID != ec_id) 
        //ret = -ENODEV;
        ret = 0;
    release_region(0x2e, 2);

ec_check_exists_ret:
    return ret;
}

static int ec_wait_obf_set(void) {
    int retries = 0;
    do {
        if (inb(EC_CMD_PORT) & BIT(0))
            return 0;
        udelay(EC_UDELAY);
    } while (retries++ < EC_MAX_RETRY);
    return -EBUSY;
}

static int ec_wait_ibf_clear(void) {
    int retries = 0;
    do {
        if (!(inb(EC_CMD_PORT) & BIT(1)))
            return 0;
        udelay(EC_UDELAY);
    } while (retries++ < EC_MAX_RETRY);
    return -EBUSY;
}


static int ec_clear_obf(void) {
    int retries = 0;
    do {
        if (!(inb(EC_CMD_PORT) & BIT(0)))
            return 0;
        udelay(EC_UDELAY);
    } while (retries++ < EC_MAX_RETRY);
    return -EBUSY;
}

static int ec_send_command(u16 command) {
    int ret;

    ret = ec_wait_ibf_clear();
    if (ret)
        goto ec_send_cmd_out;
    outb(0x88, EC_CMD_PORT);

    ret = ec_wait_ibf_clear();
    if (ret)
        goto ec_send_cmd_out;
    outb(EC_CMD_PORT, (command >> 8) & 0xff);

    ret = ec_wait_ibf_clear();
    if (ret)
        goto ec_send_cmd_out;
    outb(EC_CMD_PORT, command & 0xff);

ec_send_cmd_out:
    return ret;
}


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

static ssize_t ec_vpd_parse_data(struct ec_vpd_entry *vpd, char *raw, char *buf) {
    struct tm *tm_real;
    time64_t timestamp;

    switch (vpd->type) {
        case 0: // str
            strncpy(buf, raw, vpd->length);
            buf[vpd->length + 1] = '\0';
            return vpd->length + 1;
        case 1: // num

            break;
        case 2: // date
            // TODO 
            //timestamp = mktime64(113, 0, 1,0, 0, 0) + (time64_t)raw;
            //tm_real = gmtime(&timestamp);
            //return snprintf(buf, PAGE_SIZE, "%04d/%02d/%02d %02d:%02d", tm_real->ym_year + 1900, tm_real->tm_mon + 1, tm_real->tm_mday, tm_real->tm_hour, tm_real->tm_sec);            
    }
    return 0;
}

static ssize_t ec_vpd_entry_show(struct device *dev, struct ec_vpd_attribute *attr, char *buf) {
    return scnprintf(buf, PAGE_SIZE, "VPD: t=%d, o=%d, l=%d, ty=%d\n", attr->vpd.table, attr->vpd.offset, attr->vpd.length, attr->vpd.type);
    return -ENOMEM;
}

static ssize_t ec_vpd_entry_store(struct device *dev, struct ec_vpd_attribute *attr, const char *buf, size_t count) {
    return scnprintf(buf, PAGE_SIZE, "VPD: t=%d, o=%d, l=%d, ty=%d\n", attr->vpd.table, attr->vpd.offset, attr->vpd.length, attr->vpd.type);
    return -ENOMEM;
}

static ssize_t ec_vpd_read_single_store(struct device *dev, struct ec_vpd_attribute *attr, const char *buf, size_t count) {
    return -ENOMEM;
}

static ssize_t ec_vpd_table_show(struct device *dev, struct ec_vpd_attribute *attr, char *buf) {
    int table_id = 0;
    ssize_t i, reg0, reg1, reg2;
    
    if (!(sscanf(attr->attr.name, "table%d", &table_id) == 1)) {
        return -EINVAL;
    }

    switch (table_id) {
        case 0:
            reg0 = 0x56;
            reg1 = 0x57;
            reg2 = 0x58;
            break;
        case 1:
            reg0 = 0x59;
            reg1 = 0x5a;
            reg2 = 0x5b;
            break;
        case 2:
            reg0 = 0x5c;
            reg1 = 0x5d;
            reg2 = 0x5e;
            break;
        case 3:
            reg0 = 0x60;
            reg1 = 0x61;
            reg2 = 0x62;
            break;
        default:
            return -EINVAL;
    }

    for (i=0; i < 256; i++) {
        if(ec_write_byte(reg0, (i >> 8) & 0xff) ||
            ec_write_byte(reg1, i & 0xff) ||
            ec_read_byte(reg2, &buf[i]))
            return -EBUSY;
    }
    return i;
}

static int ec_driver_probe(struct platform_device *pdev) {
    int ret;

    ret = sysfs_create_group(&pdev->dev.kobj, &ec_attr_group);
    if (ret)
        return ret;

    ret = sysfs_create_group(&pdev->dev.kobj, &ec_vpd_attr_group);
    if (ret)
        return ret;

    pr_debug("Probe");
    return ret;
}

static int ec_driver_remove(struct platform_device *pdev) {
    sysfs_remove_group(&pdev->dev.kobj, &ec_attr_group);
    sysfs_remove_group(&pdev->dev.kobj, &ec_vpd_attr_group);

    pr_debug("Remove");
    return 0;
}

static int __init tsx73a_init(void) {
    int ret;
    struct ec_platform_data platdata;
    
    // Check we have the correct EC
    ret = ec_check_exists();
    if (ret) {
        pr_err("Could not find ITE8528");
        goto tsx73a_exit_init;    
    }

    // Create and register the platform device
    ec_pdev = platform_device_alloc("tsx73a-ec", -1);
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
    pr_debug("Device registred");

    // Create and setup the platform driver
    ret = platform_driver_register(&ec_pdriver);
    if (ret)
        goto tsx73a_exit_init_unregister;
    pr_debug("Driver registred");

    pr_debug("Init");

    goto tsx73a_exit_init;

tsx73a_exit_init_unregister:
    platform_device_unregister(ec_pdev);
tsx73a_exit_init_put:
    platform_device_put(ec_pdev);
tsx73a_exit_init:
    return ret;
}

static void __exit tsx73a_exit(void) {
    if (ec_pdev)
        platform_device_unregister(ec_pdev);
    platform_driver_unregister(&ec_pdriver);
    pr_debug("Exit");
}

module_init(tsx73a_init);
module_exit(tsx73a_exit)

MODULE_DESCRIPTION("QNAP TS-x73A EC Driver");
MODULE_VERSION("0.0.0");
MODULE_AUTHOR("0xGiddi - <giddi AT giddi.net>");
MODULE_LICENSE("GPL");