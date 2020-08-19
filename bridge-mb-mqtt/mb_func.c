/* mqtt4modbus
 * modbus tcp/rtu via mqtt
 * author: daixijiang@gmail.com
 */
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <modbus.h>
#include "common.h"

typedef struct
{
	const char *address;   /* ttySN*/
	int baudrate;
	modbus_t *ctx;
    int slaveid;

} mbslave_t;
mbslave_t g_mbx={.address=NULL,.ctx=NULL,.slaveid=0};

void mb_connect(int slaveid)
{
	mbslave_t *mbp = &g_mbx;

	if (mbp->ctx) { 
		modbus_close(mbp->ctx);
		modbus_free(mbp->ctx);
	}
	mbp->slaveid = slaveid;
	printf("------- modbus: %s %d, 'N', 8, 1 slaveid=%d\r\n",mbp->address,mbp->baudrate, mbp->slaveid);
	mbp->ctx = modbus_new_rtu(mbp->address, mbp->baudrate, 'N', 8, 1);

	if (mbp->ctx == NULL) {
		fprintf(stderr, "modbus_new_rtu: Call failed\n");
		//exit(EXIT_FAILURE);
	}

	if (modbus_set_slave(mbp->ctx, mbp->slaveid) == -1) {
		fprintf(stderr, "modbus_set_slave: %s on slave %d\n", modbus_strerror(errno), mbp->slaveid);
		//exit(EXIT_FAILURE);
	}

	if (modbus_connect(mbp->ctx) == -1) {
		fprintf(stderr, "modbus_connect: %s on slave %d \n", modbus_strerror(errno), mbp->slaveid);
		//exit(EXIT_FAILURE);
	}
	/* Define a new timeout of  200ms */
	modbus_set_response_timeout(mbp->ctx, 0, 200000);
    modbus_set_debug(mbp->ctx, TRUE);        //ÉèÖÃDubugÄ£Ê½
    modbus_set_error_recovery(mbp->ctx, MODBUS_ERROR_RECOVERY_LINK | MODBUS_ERROR_RECOVERY_PROTOCOL);

	//modbus_flush(mbp->ctx);
}


void mb_cleanup(void)
{
	if(g_mbx.ctx)
	{
		printf(">>> mb_cleanup\n");
		modbus_close(g_mbx.ctx);
		modbus_free(g_mbx.ctx);
		g_mbx.ctx=NULL;
	}
}

int mb_write_bit(int slaveid,int address, bool state)
{
	int ret =-1;
	mbslave_t *mbp = &g_mbx;

	if (mbp->ctx&&mbp->slaveid!=slaveid) {
		mbp->slaveid = slaveid;
		modbus_set_slave(mbp->ctx, mbp->slaveid);
	}
	
	fprintf(stdout, "modbus: wite bit address(%d) on slave %d \n", address, mbp->slaveid);
	if (mbp->ctx && (ret=modbus_write_bit(mbp->ctx, address, state) == -1)) {
		fprintf(stderr, "modbus_write_bit: %s(%d) on slave %d \n", modbus_strerror(errno), errno, mbp->slaveid);
		//exit(EXIT_FAILURE);

		if (MODBUS_ENOBASE > errno) {
			mb_connect(slaveid);
			if (mbp->ctx && (ret=modbus_write_bit(mbp->ctx, address, state) == -1)) {
				fprintf(stderr, "modbus_write_bit2: %s(%d) on slave %d \n", modbus_strerror(errno), errno, mbp->slaveid);
			}
		}
	}

	return ret;
}


int mb_write_bits(int slaveid,int address, int nb, const uint8_t *data)
{
	int ret =-1;
	mbslave_t *mbp = &g_mbx;

	if (mbp->ctx&&mbp->slaveid!=slaveid) {
		mbp->slaveid = slaveid;
		modbus_set_slave(mbp->ctx, mbp->slaveid);
	}
	
	fprintf(stdout, "modbus: wite bit address(%d) on slave %d \n", address, mbp->slaveid);
	if (mbp->ctx && (ret=modbus_write_bits(mbp->ctx, address,nb, data) == -1)) {
		fprintf(stderr, "modbus_write_bits: %s(%d) on slave %d \n", modbus_strerror(errno), errno, mbp->slaveid);
		//exit(EXIT_FAILURE);

		if (MODBUS_ENOBASE > errno) {
			mb_connect(slaveid);
			if (mbp->ctx && (ret=modbus_write_bits(mbp->ctx, address,nb, data) == -1)) {
				fprintf(stderr, "modbus_write_bits2: %s(%d) on slave %d \n", modbus_strerror(errno), errno, mbp->slaveid);
			}
		}
	}

	return ret;
}

int mb_write_register(int slaveid,int address, const uint16_t value)
{
	int ret =-1;
	mbslave_t *mbp = &g_mbx;

	if (mbp->ctx&&mbp->slaveid!=slaveid) {
		mbp->slaveid = slaveid;
		modbus_set_slave(mbp->ctx, mbp->slaveid);
	}
	
	fprintf(stdout, "modbus: wite register address(%d) on slave %d \n", address, mbp->slaveid);
	if (mbp->ctx && (ret=modbus_write_register(mbp->ctx, address, value) == -1)) {
		fprintf(stderr, "mb_write_register: %s(%d) on slave %d \n", modbus_strerror(errno), errno, mbp->slaveid);
		//exit(EXIT_FAILURE);

		if (MODBUS_ENOBASE > errno) {
			mb_connect(slaveid);
			if (mbp->ctx && (ret=modbus_write_register(mbp->ctx, address, value) == -1)) {
				fprintf(stderr, "mb_write_register2: %s(%d) on slave %d \n", modbus_strerror(errno), errno, mbp->slaveid);
			}
		}
	}
	return ret;
}


int mb_write_registers(int slaveid,int address, int nb, const uint16_t *data)
{
	int ret =-1;
	mbslave_t *mbp = &g_mbx;

	if (mbp->ctx&&mbp->slaveid!=slaveid) {
		mbp->slaveid = slaveid;
		modbus_set_slave(mbp->ctx, mbp->slaveid);
	}
	
	if (mbp->ctx && (ret=modbus_write_registers(mbp->ctx, address,nb, data) == -1)) {
		fprintf(stderr, "mb_write_registers: %s(%d) on slave %d \n", modbus_strerror(errno), errno, mbp->slaveid);
		//exit(EXIT_FAILURE);

		if (MODBUS_ENOBASE > errno) {
			mb_connect(slaveid);
			if (mbp->ctx && (ret=modbus_write_registers(mbp->ctx, address,nb, data) == -1)) {
				fprintf(stderr, "mb_write_registers: %s(%d) on slave %d \n", modbus_strerror(errno), errno, mbp->slaveid);
			}
		}
	}
	return ret;
}


int mb_read_bits(int slaveid,int address,  int nb, uint8_t *data)
{
	int ret =-1;
	mbslave_t *mbp = &g_mbx;

	if (mbp->ctx&&mbp->slaveid!=slaveid) {
		mbp->slaveid = slaveid;
		modbus_set_slave(mbp->ctx, mbp->slaveid);
	}
	
	if (mbp->ctx && (ret=modbus_read_bits(mbp->ctx, address, nb, data) == -1)) {
		fprintf(stderr, "modbus_read_bits: %s(%d) on slave %d \n", modbus_strerror(errno), errno, mbp->slaveid);
		//exit(EXIT_FAILURE);

		if (MODBUS_ENOBASE > errno) {
			mb_connect(slaveid);
			if (mbp->ctx && (ret=modbus_read_bits(mbp->ctx, address, nb, data) == -1)) {
				fprintf(stderr, "modbus_read_bits 2: %s(%d) on slave %d \n", modbus_strerror(errno), errno, mbp->slaveid);
				//exit(EXIT_FAILURE);
			}
		}
	}
	return ret;
}

int mb_read_input_bits(int slaveid,int address, int nb, uint8_t *data)
{
	int ret =-1;
	mbslave_t *mbp = &g_mbx;

	if (mbp->ctx&&mbp->slaveid!=slaveid) {
		mbp->slaveid = slaveid;
		modbus_set_slave(mbp->ctx, mbp->slaveid);
	}
	
	if (mbp->ctx && (ret=modbus_read_input_bits(mbp->ctx, address, nb, data) == -1)) {
		fprintf(stderr, "modbus_read_input_bits: %s(%d) on slave %d \n", modbus_strerror(errno), errno, mbp->slaveid);
		//exit(EXIT_FAILURE);

		if (MODBUS_ENOBASE > errno) {
			mb_connect(slaveid);
			if (mbp->ctx && (ret=modbus_read_bits(mbp->ctx, address, nb, data) == -1)) {
				fprintf(stderr, "modbus_read_input_bits 2: %s(%d) on slave %d \n", modbus_strerror(errno), errno, mbp->slaveid);
				//exit(EXIT_FAILURE);
			}
		}
	}

	return ret;
}

int mb_read_register(int slaveid,int address, int nb, uint16_t *data)
{
	int ret =-1;
	mbslave_t *mbp = &g_mbx;

	if (mbp->ctx&&mbp->slaveid!=slaveid) {
		mbp->slaveid = slaveid;
		modbus_set_slave(mbp->ctx, mbp->slaveid);
	}
	
	if (mbp->ctx && (ret=modbus_read_registers(mbp->ctx, address, nb, data) == -1)) {
		fprintf(stderr, "modbus_read_registers: %s(%d) on slave %d \n", modbus_strerror(errno), errno, mbp->slaveid);
		//exit(EXIT_FAILURE);

		if (MODBUS_ENOBASE > errno) {
			mb_connect(slaveid);
			if (mbp->ctx && (ret=modbus_read_registers(mbp->ctx, address, nb, data) == -1)) {
				fprintf(stderr, "modbus_read_registers 2: %s(%d) on slave %d \n", modbus_strerror(errno), errno, mbp->slaveid);
				//exit(EXIT_FAILURE);
			}
		}
	}

	return ret;
}

int mb_read_input_register(int slaveid,int address, int nb, uint16_t *data)
{
	int ret =-1;
	mbslave_t *mbp = &g_mbx;

	if (mbp->ctx&&mbp->slaveid!=slaveid) {
		mbp->slaveid = slaveid;
		modbus_set_slave(mbp->ctx, mbp->slaveid);
	}
	
	if (mbp->ctx && (ret=modbus_read_input_registers(mbp->ctx, address, nb, data) == -1)) {
		fprintf(stderr, "modbus_read_input_registers: %s(%d) on slave %d \n", modbus_strerror(errno), errno, mbp->slaveid);
		//exit(EXIT_FAILURE);

		if (MODBUS_ENOBASE > errno) {
			mb_connect(slaveid);
			if (mbp->ctx && (ret=modbus_read_input_registers(mbp->ctx, address, nb, data) == -1)) {
				fprintf(stderr, "modbus_read_input_registers 2: %s(%d) on slave %d \n", modbus_strerror(errno), errno, mbp->slaveid);
				//exit(EXIT_FAILURE);
			}
		}
	}

	return ret;
}





void mb_init(const char* uart,int baudrate,int slaveid)
{
	g_mbx.address = uart;
	g_mbx.baudrate = baudrate;

	mb_connect(slaveid);
}

