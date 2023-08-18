#include "Teensy_uds.h"
#include <Cs_Target.h>
#include <Teensy_Cs.h>


int Teensy_uds::seedkey(uint8_t seed[], uint8_t key[]){
	return 0;
}

int Cs_Target::prepare_target(){}
int Cs_Target::pre_glitch(uint8_t* ret_data, uint16_t* ret_len){}
int Cs_Target::post_glitch(uint8_t* ret_data, uint16_t* ret_len){}
int Cs_Target::check(uint8_t* ret_data, uint16_t* ret_len){}


// TODO make this dynamic
Teensy_uds uds_i = Teensy_uds(0x750, 0x758);


int Cs_Target::setup(){
	uds_i.init_can(500000);
	uds_i.set_log_enabled(LOG_TEENSY_CS);
}


void test(uint8_t* ret_data, uint16_t* ret_len){
}

#define RAW_UDS		0x21
#define CMD_TEST	0x22
#define SEC_ACCESS	0x23
#define CAN_SNIFF	0x24
#define UDS_SEND	0x25
#define UDS_RECV	0x26
#define SET_IDS		0x27
#define	RAW_CAN		0x28	// Just send a raw CAN message on the bus, without UDS
#define SET_MODE	0x29	// Set the operating mode of the teensy_uds - CAN or UDS. To be called before init_can()

char log_buff[0x1000];

uint16_t read_additional_bytes(uint8_t* data_buf){
	char in[0x100] = {0x0};
	memset(in, 0, 0x100);
	Serial.readBytes(in, 2);
	uint16_t len = *((uint16_t*) in);
	Serial.readBytes(data_buf, len);
	return len;

}


int Cs_Target::process_cmd(int cmd, uint8_t* ret_data, uint16_t* ret_len){
	uint8_t data_buf[0x200];

	UDS_Ret_Data r;
	
	char in[0x100] = {0x0};
	switch(cmd){
		case RAW_UDS:
			{
			uint16_t len = read_additional_bytes(data_buf);
			int ret_code = uds_i.uds_req(data_buf, len, &r);
			*ret_len = r.len;
			break;
			}
		case SET_MODE:
			{
			uint8_t mode;
			Serial.readBytes(&mode, 1);
			uds_i.set_mode((Modus) mode);
			return 0;
			break;
			}
		case SET_IDS:
			{
			uint16_t len = read_additional_bytes(data_buf);
			uint16_t r_id = *((uint16_t*) data_buf);
			uint16_t w_id = *(((uint16_t*) data_buf) + 1);
			uds_i.set_ids(r_id, w_id);
			return 0;
			break;
			}
		case CMD_TEST:
			{
			test(ret_data, ret_len);
			break;
			}
		case SEC_ACCESS:
			{
			int ret = uds_i.sec_access(1, &r);
			break;
			}
		case UDS_RECV:
			{
			int len = uds_i.diag_read(&r, 1000);
			uds_i.clear_read_queue();

			if(len){
				memcpy(ret_data, &(r.rcv_id), 2);
				memcpy(ret_data + 2, r.data, len);
				*ret_len = 2 + len;
			}
			else{
				*ret_len = 0;
			}

			return *ret_len;
			break;
			}
		case UDS_SEND:
			{
			uint16_t len = read_additional_bytes(data_buf);
			uds_i.diag_write(data_buf, len);
			*ret_len = 0; // IMPORTANT - do not forget this
			return 0;
			break;
			}
		case CAN_SNIFF:
			{
			break;
			}
		case RAW_CAN:
			{
			uint16_t len = read_additional_bytes(data_buf);
			uint16_t can_id = *((uint16_t*) data_buf);
			uds_i.set_ids(0x0, can_id);
			return uds_i.write_can_buffer(data_buf + 2, len - 2);
			break;
			}
		default:
			break;
	}
	return 0;
}
