#include <linux/module.h>
#include <linux/ioport.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>

#define EC_MAX_RETRY    1000
#define EC_CMD_PORT     0x6c
#define EC_DAT_PORT     0x68
#define EC_UDELAY       300
#define EC_CHIP_ID      0x8528

#define EC_VPD_TABLE_LEN	0x200

// Entry long format:
// 1111 11 11 11111111 1111111111111111
//      ta ty llllllll eeeeeeeeeeeeeeee
// 0000 11 00 00010000 0000000011001011 = VPD:c1000cb
// TODO: Set = EOVERFLOW


#define EC_VPD_MB_DATE		0x00203000		// Ta:00 Of:0b Ty:02 Le:03
#define EC_VPD_MB_MANUF 	0x0010000f      // Ta:00 Of:0f Ty:00 Le:10
#define EC_VPD_MB_NAME		0x00100020      // Ta:00 Of:20 Ty:00 Le:10
#define EC_VPD_MB_SERIAL	0x00100031      // Ta:00 Of:31 Ty:00 Le:10
#define EC_VPD_MB_MODEL		0x00200042      // Ta:00 Of:42 Ty:00 Le:20
//#define EC_VPD_MB_ 		0x01100064      // Ta:00 Of:64 Ty:01 Le:10
#define EC_VPD_MB_VENDOR	0x0010007c      // Ta:00 Of:7c Ty:00 Le:10
//#define EC_VPD_MB_		0x0020008d      // Ta:00 Of:8d Ty:00 Le:20
//#define EC_VPD_MB_		0x001000ae      // Ta:00 Of:ae Ty:00 Le:10
//#define EC_VPD_MB_		0x000300bf      // Ta:00 Of:bf Ty:00 Le:03
//#define EC_VPD_MB_		0x001000c3      // Ta:00 Of:c3 Ty:00 Le:10
#define EC_VPD_ENC_NICKNAME	0x001000d6      // Ta:00 Of:d6 Ty:00 Le:10
//#define EC_VPD_MB_		0x010400e7      // Ta:00 Of:e7 Ty:01 Le:04

//#define EC_VPD_BP_		0x0501000a      // Ta:01 Of:0a Ty:01 Le:01
//#define EC_VPD_BP_ 		0x0410000c      // Ta:01 Of:0c Ty:00 Le:10
#define EC_VPD_ENC_SERIAL 	0x0410001d      // Ta:01 Of:1d Ty:00 Le:10
#define EC_VPD_BP_DATE	 	0x06030033      // Ta:01 Of:33 Ty:02 Le:03
#define EC_VPD_BP_MANUF 	0x04100037      // Ta:01 Of:37 Ty:00 Le:10
#define EC_VPD_BP_NAME 		0x04100048      // Ta:01 Of:48 Ty:00 Le:10
#define EC_VPD_BP_SERIAL 	0x04100059      // Ta:01 Of:59 Ty:00 Le:10
#define EC_VPD_BP_MODEL 	0x0420006a      // Ta:01 Of:6a Ty:00 Le:20
#define EC_VPD_BP_VENDOR 	0x04100094      // Ta:01 Of:94 Ty:00 Le:10
//#define EC_VPD_BP_		0x042000a5      // Ta:01 Of:a5 Ty:00 Le:20
//#define EC_VPD_BP_		0x040100c6      // Ta:01 Of:c6 Ty:00 Le:01
//#define EC_VPD_BP_		0x040200c8      // Ta:01 Of:c8 Ty:00 Le:02
//#define EC_VPD_BP_		0x041000cb      // Ta:01 Of:cb Ty:00 Le:10

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
//
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

#undef pr_fmt
#define pr_fmt(fmt) "%s @ %s: " fmt "\n", "tsx73a-ec", __FUNCTION__


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
static ssize_t ec_vpd_read_single_store(struct device *dev, struct ec_vpd_attribute *attr, const char *buf, size_t count);
static ssize_t ec_vpd_entry_store(struct device *dev, struct ec_vpd_attribute *attr, const char *buf, size_t count);

static int __init tsx73a_init(void);
static void __exit tsx73a_exit(void);
