i#include <stdio.h>
#include <string.h>

struct qnap_code_match {
	char *code_str;
	unsigned int offset;
	unsigned int length;
};


struct qnap_model_config {
	char *model_name;					// Model name
	struct qnap_code_match bp_code;	// Model matching BP code
	struct qnap_code_match mb_code;	// Model matching MB code
	unsigned long long temp_mask;						// Bitmask of supported temperature channels
	unsigned long long fan_mask;						// Bitmask of supported fan channels
	unsigned long long pwm_mask; 						// Bitmask of supported PWM channels (multiple fan can be on a single PWM driver)
};

// Define the FAN_x macro
#define FAN_x(x) (1 << (x))

// Define the FAN macro to allow calling FAN_x without a parameter
#define FAN FAN_x

#define QNAP_CODE_MATCH(_code, _offset, _length) {.code_str = _code, .offset = _offset, .length = _length}
#define QNAP_CONFIG(model, _mb_code, _bp_code, temp, fan, pwm)  {.model_name = model, .mb_code = _mb_code,  .bp_code = _bp_code,  .temp_mask = temp, .fan_mask = fan, .pwm_mask=pwm}

static struct qnap_model_config configs[] = {
    QNAP_CONFIG("tsxAAA", QNAP_CODE_MATCH("Q07D0", 4, 5), QNAP_CODE_MATCH("Q07N0", 4, 5), 0, 0, 0),
	{
		.model_name = "TS-473A", 
		.mb_code = {
			.code_str = "Q07D1",
			.offset = 4,
			.length = 5,
			}, 
		.bp_code = {
			.code_str = "Q07N0",
			.offset = 4,
			.length = 5
			},
		.temp_mask = 0x000000000000001e,
		.fan_mask = 0x0000000000000041,
		.pwm_mask = 0x0000000000000041
	},
	{
		.model_name = "TS-673A", 
		.mb_code = {
			.code_str = "Q07D0",
			.offset = 3,
			.length = 5,
			}, 
		.bp_code = {
			.code_str = "Q07M0",
			.offset = 2,
			.length = 5
			},
		.temp_mask = 0x000000000000001e,
		.fan_mask = 0x0000000000000041,
		.pwm_mask = 0x0000000000000041
	},
    
	{ NULL }
};


void main(void) {
	int i;

    char *str1 = "70-0Q07D0250";
    char *str2 = "70-1Q07N0200";
    struct qnap_model_config config;
    for (i=0; configs[i].model_name; i++) {
        printf("Checking %s\n", str1  + configs[i].mb_code.offset);
        printf("Checking %s\n", str2  + configs[i].mb_code.offset);
        if ((memcmp(configs[i].mb_code.code_str, str1  + configs[i].mb_code.offset, configs[i].mb_code.length) == 0) && (memcmp(configs[i].bp_code.code_str, str2  + configs[i].bp_code.offset, configs[i].bp_code.length) == 0))
            config = configs[i];
    }

    printf("Config found %s\n", config.model_name);
}
