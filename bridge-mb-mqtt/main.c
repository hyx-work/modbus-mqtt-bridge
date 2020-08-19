#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include <mosquitto.h>
#include "common.h"
#include "v7.h"

struct mosquitto *g_mosq = NULL;
configuration config={0};
struct v7 *v7_env = NULL;
#define ALIGN(size, align) ((size + align - 1) & (~(align - 1)))

static int install_js(struct v7 *v7,const char* script)
{
	enum v7_err rcode = V7_OK;
	v7_val_t result;
	rcode = v7_exec(v7, script, &result);
	if (rcode != V7_OK)
	{
		v7_print_error(stderr, v7, "Evaluation error", result);
		return -1;
	}

	return 0;
}

#define MK_PAYLOAD_FUNC_NAME "makePayload"
#define PAYLOAD_PROC_FUNC_NAME "processPayLoad"

static v7_val_t call_makePayloadForCoil(struct v7 *v7,const char*js_func,uint8_t *pData,unsigned long dataLen,const char**payload) {
  v7_val_t func, result, args,data;
	unsigned long i;
  func = v7_get(v7, v7_get_global(v7), js_func, strlen(js_func));
	printf("JS func[%s - %d] called\n", js_func,(int)func);

  data = v7_mk_array(v7);

  for(i=0;i<dataLen;i++)
  {
	printf("%02X ", pData[i]);
	v7_array_push(v7, data, v7_mk_number(v7, pData[i]));
  }
	printf("\n");
  args = v7_mk_array(v7);
  v7_array_push(v7, args, data);
  v7_array_push(v7, args, v7_mk_number(v7, dataLen));


  if (v7_apply(v7, func, V7_UNDEFINED, args, &result) == V7_OK) {
	*payload = v7_get_cstring(v7, &result);
    printf("JS func[%s] Result[%d]: payload=%s\n", js_func,(int)result,*payload);
	return result;
  } else {
    v7_print_error(stderr, v7, "call_makePayloadForCoil: JS Error while calling", result);
	return 0;
  }
}

static v7_val_t call_makePayloadForReg(struct v7 *v7,const char*js_func,uint16_t*pData,unsigned long dataLen,const char**payload) {
  v7_val_t func, result, args,data;
	unsigned long i;
	
  func = v7_get(v7, v7_get_global(v7), js_func, strlen(js_func));

  data = v7_mk_array(v7);
  for(i=0;i<dataLen;i++)
  {
	printf("%04X ", pData[i]);
	v7_array_push(v7, data, v7_mk_number(v7, pData[i]));
  }
	printf("\n");
  args = v7_mk_array(v7);
  v7_array_push(v7, args, data);
  v7_array_push(v7, args, v7_mk_number(v7, dataLen));


  if (v7_apply(v7, func, V7_UNDEFINED, args, &result) == V7_OK) {
	*payload = v7_get_cstring(v7, &result);
    printf("JS func[%s] Result[%d]: payload=%s\n", js_func,(int)result,*payload);
	return result;
  } else {
    v7_print_error(stderr, v7, "call_makePayloadForReg: JS Error while calling", result);
	return 0;
  }
}


static v7_val_t call_writeSingleCoil(struct v7 *v7,const char*js_func, const char*payload, bool *state) {
	v7_val_t func, result, args;

	func = v7_get(v7, v7_get_global(v7), js_func, strlen(js_func));
	args = v7_mk_array(v7);
	v7_array_push(v7, args, v7_mk_string(v7,payload,strlen(payload),1));
	v7_array_push(v7, args, v7_mk_number(v7, 1));

	if (v7_apply(v7, func, V7_UNDEFINED, args, &result) == V7_OK)
	{
		printf("JS func[%s] Result[%d]\n", js_func,(int)result);
		if(v7_is_null(result))
			return 0;
		*state = (bool)v7_get_int( v7, result);
		return result;
	}
	else
	{
		v7_print_error(stderr, v7, "call_writeSingleCoil: JS Error while calling ", result);
		return 0;
	}
}

static v7_val_t call_writeMultiCoils(struct v7 *v7,const char*js_func, const char*payload, uint8_t*pData,unsigned long dataLen) {
	v7_val_t func, result, args;
	unsigned long i;
	unsigned long len;
	
	
	func = v7_get(v7, v7_get_global(v7), js_func, strlen(js_func));
	args = v7_mk_array(v7);
	v7_array_push(v7, args, v7_mk_string(v7,payload,strlen(payload),1));
  	v7_array_push(v7, args, v7_mk_number(v7, dataLen));
	printf("pData= %p\n",pData);
	//printf("*pData= %p\n",*pData);

	if (v7_apply(v7, func, V7_UNDEFINED, args, &result) == V7_OK)
	{
		if(!v7_is_array(v7,result))
			return 0;
		len= v7_array_length( v7, result);
		for(i=0;i<len;i++)
		{
			pData[i] = (uint8_t)v7_get_int(v7,v7_array_get( v7, result,i));
		}
		printf("JS func[%s] Result[%d]\n", js_func,(int)result);
		return result;
	}
	else
	{
		v7_print_error(stderr, v7, "call_writeMultiCoils: JS Error while calling", result);
		return 0;
	}
}

static v7_val_t call_writeSingleRegister(struct v7 *v7,const char*js_func, const char*payload, uint16_t *data) {
	v7_val_t func, result, args;

	func = v7_get(v7, v7_get_global(v7), js_func, strlen(js_func));
	args = v7_mk_array(v7);
	v7_array_push(v7, args, v7_mk_string(v7,payload,strlen(payload),1));
	v7_array_push(v7, args, v7_mk_number(v7, 1));

	if (v7_apply(v7, func, V7_UNDEFINED, args, &result) == V7_OK)
	{
		printf("JS func[%s] Result[%d]\n", js_func,(int)result);
		if(v7_is_null(result))
			return 0;
		*data = (uint16_t)v7_get_int( v7, result);
		return result;
	}
	else
	{
		v7_print_error(stderr, v7, "call_writeSingleRegister: JS Error while calling ", result);
		return 0;
	}
}

static v7_val_t call_writeMultiRegister(struct v7 *v7,const char*js_func, const char*payload, uint16_t*pData,unsigned long dataLen) {
	v7_val_t func, result, args;
	unsigned long i;
	unsigned long len;
	
	
	func = v7_get(v7, v7_get_global(v7), js_func, strlen(js_func));
	args = v7_mk_array(v7);
	v7_array_push(v7, args, v7_mk_string(v7,payload,strlen(payload),1));
  	v7_array_push(v7, args, v7_mk_number(v7, dataLen));
	//printf("*pData= %p\n",*pData);

	if (v7_apply(v7, func, V7_UNDEFINED, args, &result) == V7_OK)
	{
		if(!v7_is_array(v7,result))
			return 0;
		len= v7_array_length( v7, result);

		for(i=0;i<len;i++)
		{
			pData[i] = (uint16_t)v7_get_int(v7,v7_array_get( v7, result,i));
		}
		printf("JS func[%s] Result[%d]\n", js_func,(int)result);
		return result;
	}
	else
	{
		v7_print_error(stderr, v7, "call_writeMultiCoils: JS Error while calling", result);
		return 0;
	}
}

void mqtt_connect_callback(struct mosquitto *g_mosq, void *obj, int rc)
{
	(void) obj;
	if (rc == 0)
	{
		unsigned long i;
		for(i=0;i<config.mqtt_subTopicNum;i++)
		{
			if (mosquitto_subscribe(g_mosq, NULL, config.mqtt_subTopic[i], 1) != MOSQ_ERR_SUCCESS) {
				fprintf(stderr, "mosquitto_subscribe: Call failed %s\n", config.mqtt_subTopic[i]);
			}
			else {
				fprintf(stdout, "------- mosquitto_subscribe topic : %s\n", config.mqtt_subTopic[i]);
			}
		}
	} else {
		fprintf(stderr, "mqtt_connect_callback: Connection failed\n");
	}
}

void mqtt_message_callback(struct mosquitto *g_mosq, void *obj, const struct mosquitto_message *message)
{
	unsigned long i=0;
	const char* payload =NULL;
	v7_val_t result;
	bool state;
	uint16_t regState;

	(void) obj;
	(void) g_mosq;
	
	fprintf(stdout, "======= %s %s\n",message->topic,(char*)message->payload);

	for(i=0; i<config.modbusSubTopicNum;i++)
	{
		if(strcmp(message->topic, config.modbusSubTopic[i].topic)==0)
		{
			if(config.modbusSubTopic[i].script)
			{
				if(install_js(v7_env,(const char*)config.modbusSubTopic[i].script)==-1)
				{
					return;
				}
				switch(config.modbusSubTopic[i].funCode)
				{
				case 0x05: // WRITE_SINGLE_COIL
					result = call_writeSingleCoil(v7_env,PAYLOAD_PROC_FUNC_NAME,message->payload,&state);
					if(result==0)
						return;
					if(state)
					{
						mb_write_bit(config.modbusSubTopic[i].slaveid,config.modbusSubTopic[i].address,true);
					}
					else
					{
						mb_write_bit(config.modbusSubTopic[i].slaveid,config.modbusSubTopic[i].address,false);
					}
					return;
				case 0x0F: // WRITE_MULTIPLE_COILS
					result = call_writeMultiCoils(v7_env,PAYLOAD_PROC_FUNC_NAME,message->payload, config.modbusSubTopic[i].bitData,config.modbusSubTopic[i].dataLen);
					if(result==0)
						return;
					mb_write_bits(config.modbusSubTopic[i].slaveid,config.modbusSubTopic[i].address,
								config.modbusSubTopic[i].dataLen,config.modbusSubTopic[i].bitData);
					return;
				case 0x06: //WRITE_SINGLE_REGISTER
					result = call_writeSingleRegister(v7_env,PAYLOAD_PROC_FUNC_NAME,message->payload, &regState);
					if(result==0)
						return;
					mb_write_register(config.modbusSubTopic[i].slaveid,config.modbusSubTopic[i].address,regState);
					break;
				case 0x10: //WRITE_MULTIPLE_REGISTERS
					result = call_writeMultiRegister(v7_env,PAYLOAD_PROC_FUNC_NAME,message->payload, config.modbusSubTopic[i].regData,config.modbusSubTopic[i].dataLen);
					if(result==0)
						return;
					mb_write_registers(config.modbusSubTopic[i].slaveid,config.modbusSubTopic[i].address,
								config.modbusSubTopic[i].dataLen,config.modbusSubTopic[i].regData);
					break;
				default:
					break;
				}
			}
			
		}

		// 以modbus/开头的主题
		if(strstr(message->topic,"modbus/")==message->topic)
		{
			for(i=0; i<config.modbusPubTopicNum;i++)
			{
				if(strstr(message->topic, config.modbusPubTopic[i].topic)==NULL)
					continue;
				fprintf(stdout, "======= read funcode %d\n",config.modbusPubTopic[i].funCode);
				if(install_js(v7_env,(const char*)config.modbusPubTopic[i].script)==-1)
				{
					return;
				}
				switch(config.modbusPubTopic[i].funCode)
				{
				case 0x01: // READ_COILS
					mb_read_bits(config.modbusPubTopic[i].slaveid,config.modbusPubTopic[i].address,config.modbusPubTopic[i].dataLen,config.modbusPubTopic[i].bitData);
					if(errno!=0){
						return;
					}
					result = call_makePayloadForCoil(v7_env,MK_PAYLOAD_FUNC_NAME,config.modbusPubTopic[i].bitData,config.modbusPubTopic[i].dataLen,&payload);
					break;
				case 0x02: // READ_DISCRETE_INPUTS
					mb_read_input_bits(config.modbusPubTopic[i].slaveid,config.modbusPubTopic[i].address,config.modbusPubTopic[i].dataLen,config.modbusPubTopic[i].bitData);
					if(errno!=0)
						return;
					result = call_makePayloadForCoil(v7_env,MK_PAYLOAD_FUNC_NAME,config.modbusPubTopic[i].bitData,config.modbusPubTopic[i].dataLen,&payload);
					break;
				case 0x03: //READ_HOLDING_REGISTERS
					mb_read_register(config.modbusPubTopic[i].slaveid,config.modbusPubTopic[i].address,config.modbusPubTopic[i].dataLen,config.modbusPubTopic[i].regData);
					if(errno!=0)
						return;
					result = call_makePayloadForReg(v7_env,MK_PAYLOAD_FUNC_NAME,config.modbusPubTopic[i].regData,config.modbusPubTopic[i].dataLen,&payload);
					break;
				case 0x04: //READ_INPUT_REGISTERS
					mb_read_input_register(config.modbusPubTopic[i].slaveid,config.modbusPubTopic[i].address,config.modbusPubTopic[i].dataLen,config.modbusPubTopic[i].regData);
					if(errno!=0)
						return;
					result = call_makePayloadForReg(v7_env,MK_PAYLOAD_FUNC_NAME,config.modbusPubTopic[i].regData,config.modbusPubTopic[i].dataLen,&payload);
					break;
				default:
					break;
				}		

				if(payload)
				{
					if (mosquitto_publish(g_mosq, NULL, config.modbusPubTopic[i].topic, 
	                          strlen(payload), payload, 1, false) != MOSQ_ERR_SUCCESS)
					{
						fprintf(stderr, "mosquitto_publish: Call failed %s\n", config.modbusPubTopic[i].topic);
					}
				}

			}
		}
	}
	
}

void config_cleanup()
{
	unsigned long i;
	printf(">>> config_cleanup\n");

	if(config.mqtt_subTopic)
	{
		for(i=0;i<config.mqtt_subTopicNum;i++)
			free(config.mqtt_subTopic[i]);
		
		free(config.mqtt_subTopic);
		config.mqtt_subTopic=NULL;
	}
	if(config.mqtt_host)
	{
		free(config.mqtt_host);
		config.mqtt_usr=NULL;
	}
	if(config.mqtt_usr)
	{
		free(config.mqtt_usr);
		config.mqtt_usr=NULL;
	}
	if(config.mqtt_pwd)
	{
		free(config.mqtt_pwd);
		config.mqtt_pwd=NULL;
	}


	if(config.modbus_uart)
	{
		free(config.modbus_uart);
		config.modbusPubTopic=NULL;
	}
	if(config.modbusPubTopic)
	{
		for(i=0;i<config.modbusPubTopicNum;i++)
		{
			if(config.modbusPubTopic[i].bitData)
				free(config.modbusPubTopic[i].bitData);
			if(config.modbusPubTopic[i].regData)
				free(config.modbusPubTopic[i].regData);
			
			if(config.modbusPubTopic[i].topic)
				free(config.modbusPubTopic[i].topic);
			if(config.modbusPubTopic[i].label)
				free(config.modbusPubTopic[i].label);
			if(config.modbusPubTopic[i].script)
				free(config.modbusPubTopic[i].script);
		}

		free(config.modbusPubTopic);
		config.modbusPubTopic=NULL;
	}		
	if(config.modbusSubTopic)
	{
		for(i=0;i<config.modbusSubTopicNum;i++)
		{
			if(config.modbusSubTopic[i].bitData)
				free(config.modbusSubTopic[i].bitData);
			if(config.modbusSubTopic[i].regData)
				free(config.modbusSubTopic[i].regData);
			
			if(config.modbusSubTopic[i].topic)
				free(config.modbusSubTopic[i].topic);
			if(config.modbusSubTopic[i].label)
				free(config.modbusSubTopic[i].label);
			if(config.modbusSubTopic[i].script)
				free(config.modbusSubTopic[i].script);
		}
		
		free(config.modbusSubTopic);
		config.modbusPubTopic=NULL;
	}	
}

void cleanup(void)
{
	printf(">>> cleanup\n");
	
	if (g_mosq != NULL) {
		mosquitto_destroy(g_mosq);
	}

	mosquitto_lib_cleanup();

	mb_cleanup();
	config_cleanup();
	v7_destroy(v7_env);
	printf(">>> v7_destroy\n");
}

void sigint_handler(int sig)
{
	if(SIGINT==sig)
	{
		printf("\r\n>>> ctrl+c exit %d\n",sig);
		exit(EXIT_SUCCESS);
	}
}

void config_parse_jsonstring(struct v7 *v7, v7_val_t *v7_val, char** out)
{
	size_t n;
	const char *str;
	str=v7_get_string(v7, v7_val,&n);
	if(n>0)
	{
		*out = malloc(n+1);
		memset(*out,0,n+1);
		memcpy(*out,str,n);
	}
	else
	{
		*out = NULL;
	}
}

int config_parse(struct v7 *v7, configuration* cfg, const char* path)
{
	unsigned long i;
	v7_val_t json_cfg;
	v7_val_t modbus, uart,baudrate;
	v7_val_t mqtt, host,port,keepalive,user,passwd,subTopic;

	v7_val_t modbusPubTopic;
	v7_val_t modbusSubTopic;

	v7_val_t topicItem,slaveid,address,funCode,dataLen,topic,label,script;

	/* Load JSON configuration */
	if (v7_parse_json_file(v7, path, &json_cfg) != V7_OK)
	{
		printf("%s\n", "Cannot load JSON config");
		return -1;
	}

	memset(&config,0x00,sizeof(configuration));

	/* Lookup values in JSON configuration object */
	modbus = v7_get(v7, json_cfg, "modbus", 6);
	uart = v7_get(v7, modbus, "uart", 4);
	baudrate = v7_get(v7, modbus, "baudrate", 8);
	//str = v7_get_string(v7, &uart,&n);
	//printf("modbus.uart [%.*s]\n",(int)n,str);

	config_parse_jsonstring(v7, &uart,&cfg->modbus_uart);
	cfg->modbus_baudrate = v7_get_int(v7, baudrate);
	printf("modbus uart %s baudrate %d\n",cfg->modbus_uart,cfg->modbus_baudrate);

	mqtt = v7_get(v7, json_cfg, "mqtt", 4);
	host = v7_get(v7, mqtt, "host", 4);
	port = v7_get(v7, mqtt, "port", 4);
	keepalive = v7_get(v7, mqtt, "keepalive", 9);
	user = v7_get(v7, mqtt, "user", 4);
	passwd = v7_get(v7, mqtt, "passwd", 6);

//	cfg->mqtt_host= v7_get_cstring(v7, &host);
	config_parse_jsonstring(v7, &host,&cfg->mqtt_host);
	cfg->mqtt_port= v7_get_int(v7, port);
	cfg->mqtt_keepalive = v7_get_int(v7, keepalive);

	config_parse_jsonstring(v7, &user,&cfg->mqtt_usr);
	config_parse_jsonstring(v7, &passwd,&cfg->mqtt_pwd);
	printf("mqtt host %s:%d, keepalive %d, usr %s, pwd %s\n",
		cfg->mqtt_host,cfg->mqtt_port,cfg->mqtt_keepalive,
		cfg->mqtt_usr,cfg->mqtt_pwd);

	subTopic = v7_get(v7, mqtt, "subTopic", 8);
	cfg->mqtt_subTopicNum = v7_array_length(v7,subTopic);
	if(cfg->mqtt_subTopicNum>0)
	{
		cfg->mqtt_subTopic= malloc(cfg->mqtt_subTopicNum*sizeof(char*));
		memset(cfg->mqtt_subTopic,0,cfg->mqtt_subTopicNum*sizeof(char*));
		for(i=0;i<cfg->mqtt_subTopicNum;i++)
		{
			v7_val_t item = v7_array_get(v7, subTopic,i);
//			cfg->mqtt_subTopic[i] = v7_get_cstring(v7, &item);
			config_parse_jsonstring(v7, &item,&cfg->mqtt_subTopic[i]);
			printf("mqtt.mqtt_subTopic[%ld] =%s\n",i,cfg->mqtt_subTopic[i]);
		}
	}

	modbusPubTopic = v7_get(v7, json_cfg, "modbusPubTopic", strlen("modbusPubTopic"));
	cfg->modbusPubTopicNum = v7_array_length(v7,modbusPubTopic);
	if(cfg->modbusPubTopicNum>0)
	{
		cfg->modbusPubTopic= malloc(sizeof(modbus_topic_cfg)*cfg->modbusPubTopicNum);
		memset(cfg->modbusPubTopic,0,sizeof(modbus_topic_cfg)*cfg->modbusPubTopicNum);
		for(i=0;i<cfg->modbusPubTopicNum;i++)
		{
			topicItem= v7_array_get(v7, modbusPubTopic,i);
			slaveid= v7_get(v7, topicItem,"slaveid",7);
			address= v7_get(v7, topicItem,"address",7);
			funCode= v7_get(v7, topicItem,"funCode",7);
			dataLen= v7_get(v7, topicItem,"dataLen",7);
			topic= v7_get(v7, topicItem,"topic",5);
			label= v7_get(v7, topicItem,"label",5);
			script= v7_get(v7, topicItem,"script",6);

			//cfg->modbusPubTopic[i].topic = v7_get_cstring(v7, &topic);
			//cfg->modbusPubTopic[i].label = v7_get_cstring(v7, &label);
			//cfg->modbusPubTopic[i].script = v7_get_cstring(v7, &script);
			config_parse_jsonstring(v7, &topic,&cfg->modbusPubTopic[i].topic);
			config_parse_jsonstring(v7, &label,&cfg->modbusPubTopic[i].label);
			config_parse_jsonstring(v7, &script,&cfg->modbusPubTopic[i].script);

			cfg->modbusPubTopic[i].slaveid = v7_get_int(v7,slaveid);
			cfg->modbusPubTopic[i].address= v7_get_int(v7,address);
			cfg->modbusPubTopic[i].funCode= v7_get_int(v7,funCode);
			cfg->modbusPubTopic[i].dataLen= v7_get_int(v7,dataLen);

			if(cfg->modbusPubTopic[i].funCode==0x01||cfg->modbusPubTopic[i].funCode==0x02)
			{
				cfg->modbusPubTopic[i].bitDataSize = cfg->modbusPubTopic[i].dataLen*sizeof(uint8_t);//ALIGN(cfg->modbusPubTopic[i].dataLen,8)/8*sizeof(uint8_t);
				cfg->modbusPubTopic[i].bitData = malloc(cfg->modbusPubTopic[i].bitDataSize);
				memset(cfg->modbusPubTopic[i].bitData,0,cfg->modbusPubTopic[i].bitDataSize);
			}

			if(cfg->modbusPubTopic[i].funCode==0x03||cfg->modbusPubTopic[i].funCode==0x04)
			{
				cfg->modbusPubTopic[i].regDataSize= cfg->modbusPubTopic[i].dataLen*sizeof(uint16_t);
				cfg->modbusPubTopic[i].regData = malloc(cfg->modbusPubTopic[i].regDataSize);
				memset(cfg->modbusPubTopic[i].regData,0,cfg->modbusPubTopic[i].regDataSize);
			}


			printf("#modbusPubTopic[%ld]\n  slaveid %d, address %d, funcode %d, dataLen %ld\n",i,
			cfg->modbusPubTopic[i].slaveid,cfg->modbusPubTopic[i].address,
			cfg->modbusPubTopic[i].funCode,cfg->modbusPubTopic[i].dataLen);
			printf("  topic=%s\n  label=%s\n  script=%s\n",
			cfg->modbusPubTopic[i].topic,cfg->modbusPubTopic[i].label,
			cfg->modbusPubTopic[i].script);
		}
	}

	modbusSubTopic = v7_get(v7, json_cfg, "modbusSubTopic", strlen("modbusSubTopic"));
	cfg->modbusSubTopicNum = v7_array_length(v7,modbusSubTopic);
	if(cfg->modbusSubTopicNum>0)
	{
		cfg->modbusSubTopic= malloc(sizeof(modbus_topic_cfg)*cfg->modbusSubTopicNum);
		memset(cfg->modbusSubTopic,0,sizeof(modbus_topic_cfg)*cfg->modbusSubTopicNum);
		for(i=0;i<cfg->modbusSubTopicNum;i++)
		{
			topicItem= v7_array_get(v7, modbusSubTopic,i);
			slaveid= v7_get(v7, topicItem,"slaveid",7);
			address= v7_get(v7, topicItem,"address",7);
			funCode= v7_get(v7, topicItem,"funCode",7);
			dataLen= v7_get(v7, topicItem,"dataLen",7);
			topic= v7_get(v7, topicItem,"topic",5);
			label= v7_get(v7, topicItem,"label",5);
			script= v7_get(v7, topicItem,"script",6);

			//cfg->modbusSubTopic[i].topic = v7_get_cstring(v7, &topic);
			//cfg->modbusSubTopic[i].label = v7_get_cstring(v7, &label);
			//cfg->modbusSubTopic[i].script = v7_get_cstring(v7, &script);
			config_parse_jsonstring(v7, &topic,&cfg->modbusSubTopic[i].topic);
			config_parse_jsonstring(v7, &label,&cfg->modbusSubTopic[i].label);
			config_parse_jsonstring(v7, &script,&cfg->modbusSubTopic[i].script);

			cfg->modbusSubTopic[i].slaveid = v7_get_int(v7,slaveid);
			cfg->modbusSubTopic[i].address= v7_get_int(v7,address);
			cfg->modbusSubTopic[i].funCode= v7_get_int(v7,funCode);
			cfg->modbusSubTopic[i].dataLen= v7_get_int(v7,dataLen);

			if(cfg->modbusSubTopic[i].funCode==0x05||cfg->modbusSubTopic[i].funCode==0x0F)
			{
				cfg->modbusSubTopic[i].bitDataSize= cfg->modbusSubTopic[i].dataLen*sizeof(uint8_t);//ALIGN(cfg->modbusSubTopic[i].dataLen,8)/8*sizeof(uint8_t);
				cfg->modbusSubTopic[i].bitData = malloc(cfg->modbusSubTopic[i].bitDataSize);
				memset(cfg->modbusSubTopic[i].bitData,0,cfg->modbusSubTopic[i].bitDataSize);
			}

			if(cfg->modbusSubTopic[i].funCode==0x06||cfg->modbusSubTopic[i].funCode==0x10)
			{
				cfg->modbusSubTopic[i].regDataSize= cfg->modbusSubTopic[i].dataLen*sizeof(uint16_t);
				cfg->modbusSubTopic[i].regData = malloc(cfg->modbusSubTopic[i].regDataSize);
				memset(cfg->modbusSubTopic[i].regData,0,cfg->modbusSubTopic[i].regDataSize);
			}
			

			printf("@modbusSubTopic[%ld]\n  slaveid %d, address %d, funcode %d, dataLen %ld\n",i,
			cfg->modbusSubTopic[i].slaveid,cfg->modbusSubTopic[i].address,
			cfg->modbusSubTopic[i].funCode,cfg->modbusSubTopic[i].dataLen);
			printf("  topic=%s\n  label=%s\n  script=%s\n",
			cfg->modbusSubTopic[i].topic,cfg->modbusSubTopic[i].label,
			cfg->modbusSubTopic[i].script);
		}
	}

	return 0;
}

int main(int argc, char **argv)
{
	int ch;
	(void) argc;
	(void) argv;
	const char*cfg_file_name=NULL;
	bool cfg_file_flag=false;
	
	signal(SIGINT, sigint_handler);
	
	if (atexit(cleanup) != 0)
	{
		fprintf(stderr, "atexit: Call failed\n");
		exit(EXIT_FAILURE);
	}

	while ((ch = getopt(argc, argv, "hc:")) != -1)
	{
		printf("optind: %d\n", optind);
		switch (ch) 
		{
		case 'c':
			cfg_file_flag=true;
			cfg_file_name = optarg;
		   break;
		case 'h':
			printf("Usage example: %s -c config.json\n",argv[0]);
			exit(0);
		   break;
		case '?':
			printf("Usage example: %s -c config.json\n",argv[0]);
			exit(EXIT_FAILURE);
		   break;
		}
	}
		
	if(!cfg_file_flag&&cfg_file_name==NULL)
	{
		cfg_file_name="config.json";
	}

	v7_env = v7_create();
    if (config_parse(v7_env,&config,cfg_file_name) < 0) {
        printf("Can't load 'config.json'\n");
		exit(EXIT_FAILURE);
    }


	config.modbus_slave_id=17;
	mb_init(config.modbus_uart,config.modbus_baudrate,config.modbus_slave_id);



	mosquitto_lib_init();
	g_mosq = mosquitto_new(NULL, true, NULL);

	if (g_mosq == NULL)
	{
		fprintf(stderr, "mosquitto_new: Call failed\n");
		exit(EXIT_FAILURE);
	}

	if (mosquitto_username_pw_set(g_mosq,config.mqtt_usr,config.mqtt_pwd) != MOSQ_ERR_SUCCESS)
	{
		fprintf(stderr, "mosquitto_username_pw_set ERROR\n");
	}
	
	mosquitto_connect_callback_set(g_mosq, mqtt_connect_callback);
	mosquitto_message_callback_set(g_mosq, mqtt_message_callback);
    printf("------- mqtt host=%s, mqtt port=%d, mqtt keepalive=%d, user[%s], pwd[%s]\n", 
    	config.mqtt_host, config.mqtt_port, config.mqtt_keepalive,config.mqtt_usr,config.mqtt_pwd);

	if (mosquitto_connect(g_mosq, config.mqtt_host, config.mqtt_port, config.mqtt_keepalive) != MOSQ_ERR_SUCCESS) {
		fprintf(stderr, "mosquitto_connect: Unable to connect\n");
		exit(EXIT_FAILURE);
	}

	mosquitto_loop_forever(g_mosq, 2000, 1);


	printf(">>> exit\n");
	exit(EXIT_SUCCESS);
}

