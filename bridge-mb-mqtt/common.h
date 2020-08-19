#ifndef LIBCOMMON_H__
#define LIBCOMMON_H__
#include <stdint.h>
typedef struct
{
	char* topic;
	char* label;
	char* script;
	int slaveid;
	int address;
	int funCode;
	unsigned long dataLen;
	uint8_t *bitData;
	unsigned long bitDataSize;
	uint16_t *regData;
	unsigned long regDataSize;
} modbus_topic_cfg;

typedef struct
{
    char* modbus_uart;
	int modbus_baudrate;
	int modbus_slave_id;

    char* mqtt_host;
    char* mqtt_usr;
    char* mqtt_pwd;
    int mqtt_port;
    int mqtt_keepalive;
	
	char** mqtt_subTopic;
	unsigned long mqtt_subTopicNum;

	modbus_topic_cfg *modbusPubTopic;
	unsigned long modbusPubTopicNum;
	
	modbus_topic_cfg *modbusSubTopic;
	unsigned long modbusSubTopicNum;
} configuration;

extern void mb_init(const char* uart,int baudrate, int slaveid);
extern void mb_cleanup(void);

extern int mb_write_bit(int slaveid,int address, bool state);
extern int mb_write_bits(int slaveid,int address, int nb, const uint8_t *data);
extern int mb_write_register(int slaveid,int address, const uint16_t value);
int mb_write_registers(int slaveid,int address, int nb, const uint16_t *data);

extern int mb_read_bits(int slaveid,int address,  int nb, uint8_t *data);
extern int mb_read_input_bits(int slaveid,int address, int nb, uint8_t *data);
extern int mb_read_register(int slaveid,int address, int nb, uint16_t *data);
extern int mb_read_input_register(int slaveid,int address, int nb, uint16_t *data);



#endif

