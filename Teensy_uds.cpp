#include "Teensy_uds.h"
#include <FlexCAN_T4.h>
#include <inttypes.h>
#include <isotp.h>

#define DIAG_REPLY_LEN 0x200

/*
 * This file is flashed to the teensy to perform the Teensy UDS functions
 */

#include <Teensy_Cs.h>

// Amount of times a delay can be received (78 error code)
#define N_DELAYS	0x1000

FlexCAN_T4<(CAN_DEV_TABLE)1075642368u, (FLEXCAN_RXQUEUE_TABLE)256u, (FLEXCAN_TXQUEUE_TABLE)16u> can_bus;
isotp<RX_BANKS_16, 512> tp_comms;

uint8_t diag_reply[DIAG_REPLY_LEN];
uint32_t can_rx_id = 0;
int read_len = 0;


void Teensy_uds::print_msg(uint32_t can_id, uint16_t len, uint8_t data[]){
	char out_buf[0x8000], *put = out_buf;
	put += snprintf(put, 16, "%04x\t%04x\t", can_id, len);
	for(int i = 0; i < len; ++i){
		put += snprintf(put, sizeof out_buf - (put - out_buf), "%02x ", data[i]);
	}
	if(this->log_enabled == LOG_TEENSY_CS){
		// hacky to prevent buffer overflow on log
		if(len*3 > 0x1000)
			len = 0x100;
		log(out_buf, len*3 + 11);
	}
	else if(this->log_enabled == LOG_SERIAL){
		Serial.println(out_buf);
	}
}

void can_msg_read(const ISOTP_data &config, const uint8_t *buf){
	memcpy(diag_reply, buf, config.len);
	can_rx_id = config.id;
	read_len = config.len;
}


void Teensy_uds::clear_read_buf(){
	memset(diag_reply, 0, DIAG_REPLY_LEN);
	read_len = 0;
}

void Teensy_uds::diag_write(const uint8_t* buf, const uint16_t len){
	// we erase the read buffer when writing a new request - this is under the assumption that no other ECUs on the CAN bus send messages on the recv_canid
	if(this->log_enabled){
		print_msg(this->write_id, len, buf);
	}
	this->clear_read_buf();
	tp_comms.write(this->w_config, buf, len);
}

/*
 * Warning - blocks for timeout_mus until message received 
 * Reply: the length of the message read
*/
int Teensy_uds::diag_read(uint8_t *read_buf, long timeout_mus){	
	uint32_t begin_t;
	int n_attempts = 0;
	begin_t = micros();
	while(diag_reply[0] == 0 && micros() < begin_t + timeout_mus){
		delayMicroseconds(1000);
		if(diag_reply[0] == 0x7f && diag_reply[2] == 0x78){
			if(this->log_enabled){
				print_msg(can_rx_id, read_len, diag_reply);
			}
			memset(diag_reply, 0, 3);
			read_len = 0;
			begin_t = micros();
			timeout_mus = 5000000;
		}
	}
	/* If delay message received 
	while(diag_reply[0] == 0x7f && diag_reply[2] == 0x78){
		delayMicroseconds(1000);
	}
	*/
	memcpy(read_buf, diag_reply, read_len);
	if(this->log_enabled && read_len > 0){
		print_msg(can_rx_id, read_len, diag_reply);
	}
	return read_len;
}

/*
 * Set CANids for reading & writing
 */
void Teensy_uds::set_ids(uint16_t read_id, uint16_t write_id){
	this->write_id = write_id;
	this->read_id = read_id;
	this->w_config =  {.id = write_id, .flags = {.extended = 0, .usePadding = 1, .separation_uS = 1}, .len = 8, .blockSize = 0, .flow_control_type = 0, .separation_time = 1}; // TODO possibly put separation time as an argument to this function
	tp_comms.setWriteID(this->write_id);

}


Teensy_uds::Teensy_uds(uint16_t write_id, uint16_t read_id){
	/* Class initialisation */
	this->write_id = write_id;
	this->read_id = read_id;
	this->w_config =  {.id = write_id, .flags = {.extended = 0, .usePadding = 1, .separation_uS = 1}, .len = 8, .blockSize = 0, .flow_control_type = 0, .separation_time = 1}; // TODO possibly put separation time as an argument to this function

	this->timeout_mus = 2000000;

}

static CAN_message_t msg;

void Teensy_uds::init_can(int baud)
{
	/* CAN initialisation */
	can_bus.begin(); // init CAN bus
	can_bus.setBaudRate(baud);

	// Configure diag replies
	can_bus.enableFIFO();
	can_bus.enableFIFOInterrupt();
	can_bus.setFIFOFilter(REJECT_ALL);
	can_bus.setFIFOFilter(0, this->read_id, STD);
	//can_bus.setFIFOFilter(1, 0x7df, STD); // TODO hardcoded	
	//can_bus.setFIFOFilter(ACCEPT_ALL);
	
	tp_comms.begin();
	tp_comms.setWriteBus(&can_bus); 
	tp_comms.onReceive(can_msg_read); 
	tp_comms.setWriteID(this->write_id);

	/*
	 * Set up the CAN_msg for glitching
	 */
	msg.id = this->write_id;
	msg.len = 8;
	msg.flags.extended = 0;
	msg.flags.remote   = 0;
	msg.flags.overrun  = 0;
	msg.flags.reserved = 0;
}


/* Returns 0 if successfull, otherwise the error code */
int Teensy_uds::uds_req(const uint8_t* buf, const uint16_t len, uint8_t* read_buf, uint16_t* r_len){
	int n_attempts = 1;
	this->diag_write(buf, len);
	*r_len = this->diag_read(read_buf, this->timeout_mus);
	/* 
	 * If UDS request successful 
	 */
	if(read_buf[0] == buf[0] + 0x40){
		return 0;
	}
	return -1;
}



int Teensy_uds::routine_control(uint16_t routine_id, uint8_t data[], uint16_t data_len, uint8_t* ret_data, uint16_t* r_len){
	int ret;
	// TODO start routine, check routine, (2, 3)...
	uint8_t RTC_CTRL[0x100] = {ROUTINE_CTRL, 0x01, routine_id >> 8, routine_id & 0xff};
	memcpy(RTC_CTRL + 4, data, data_len);
	ret = this->uds_req(RTC_CTRL, 4 + data_len, ret_data, r_len);
	return ret;
}


int Teensy_uds::diag_session(uint8_t mode, uint8_t* ret_data, uint16_t* r_len){
	uint8_t session_req[0x10] = {DIAG_MODE, mode};
	return this->uds_req(session_req, 2, ret_data, r_len);
}



int Teensy_uds::sec_access_req(uint8_t level, uint8_t* ret_data, uint16_t* r_len){
	uint8_t SEC_REQ[] = {B_SEC_ACCESS, level};
	return this->uds_req(SEC_REQ, 2, ret_data, r_len);
}

int Teensy_uds::sec_access(uint8_t level, uint8_t* ret_data, uint16_t* r_len){
	uint8_t SEC_REQ[] = {B_SEC_ACCESS, level};
	uint8_t SEC_RESP[0x100] = {B_SEC_ACCESS, level + 1}; // TODO potential buffer overflow
	int ret, key_len = 0;
	ret = this->uds_req(SEC_REQ, 2, ret_data, r_len);	
	if(ret != 0){
		return ret;
	}
	key_len = this->seedkey(ret_data + 2, SEC_RESP + 2);	
	return this->uds_req(SEC_RESP, 2 + key_len, ret_data, r_len);

}

int Teensy_uds::sec_access_resp(uint8_t level, uint8_t* key_reply, int key_len, uint8_t* ret_data, uint16_t* r_len){
	uint8_t SEC_RESP[0x20] = {B_SEC_ACCESS, level};
	memcpy(SEC_RESP + 2, key_reply, key_len);
	return this->uds_req(SEC_RESP, 2 + key_len, ret_data, r_len);
}

int Teensy_uds::read_data_by_id(uint16_t id, uint8_t* ret_data, uint16_t* r_len){
	uint8_t READ_BY_ID[] = {READ_DATA_BY_ID, ((uint8_t) (id >> 8) & 0xff), (uint8_t) id & 0xff};
	return this->uds_req(READ_BY_ID, 3, ret_data, r_len);
}



int Teensy_uds::request_ul(uint8_t comp_enc_b, uint8_t addr_size, uint32_t addr_st, uint32_t addr_e, uint8_t* ret_data, uint16_t* ret_len){
	uint8_t REQUEST_UL[0xb] = {REQ_UL, comp_enc_b, addr_size};
	int idx = 3;
	// First add the start addr
	for(int i = (addr_size >> 4) - 1; i >= 0; --i){
		REQUEST_UL[idx++] = (addr_st >> (i * 8)) & 0xff;
	}	
	// Then the end addr
	for(int i = (addr_size & 0x0f) - 1; i >= 0; --i){
		REQUEST_UL[idx++] = (addr_e >> (i * 8)) & 0xff;
	}
	return this->uds_req(REQUEST_UL, 0xb, ret_data, ret_len);

}

int Teensy_uds::reset_ecu(uint8_t* ret_data, uint16_t* r_len){
	uint8_t reset_req[] = {ECU_RESET, 0x01};
	return this->uds_req(reset_req, 2, ret_data, r_len);
}

int Teensy_uds::req_download(uint32_t addr, uint32_t size, uint8_t n_addr, uint8_t n_size, uint8_t* ret_data, uint16_t* r_len){
	uint8_t download_req[0x20] = {REQ_DL, 0x00};
	download_req[2] = ((n_addr & 0xf) << 4) | (n_size & 0xf);
	for(int i = 0; i < n_addr; ++i){
		download_req[3 + i] = addr >> ((n_addr - 1 -i) * 8);
	}
	for(int i = 0; i < n_size; ++i){
		download_req[3 + n_addr + i] = size >> ((n_size - 1 -i) * 8);
	}
	tf_data_seq = 1;
	return this->uds_req(download_req, 3 + n_addr + n_size, ret_data, r_len);
}




/*
 * 
 * TODO in / out 
 */
int Teensy_uds::transfer_data(const uint8_t* data, const uint16_t len, uint8_t* ret_data, uint16_t* r_len){
	uint8_t transfer_req[0x1000] = {TF_DATA, tf_data_seq};
	uint16_t MAX_BLOCK_LEN = 0xfff; // TODO set this at request download 
	uint16_t len_orig = len, len_tot = len;

	uint16_t len_transferred = 0;
	while(len_transferred < len_orig){
		uint16_t block_len = (len_tot > MAX_BLOCK_LEN - 2) ? MAX_BLOCK_LEN - 2: len_tot;
		memcpy(transfer_req + 2, data + len_transferred, block_len);	
		int ret = this->uds_req(transfer_req, block_len + 2, ret_data, r_len);
		if(ret != 0)
			return -1;
		transfer_req[1] = ++tf_data_seq;
		len_transferred += block_len;
		len_tot -= block_len;
	}
	return 0;

}

int Teensy_uds::transfer_exit(uint8_t* ret_data, uint16_t* r_len){
	// TODO can be 37 or 37 01 depending on ECU
	uint8_t exit_req[0x2] = {TF_EXIT, 0x01}; 
	return this->uds_req(exit_req, 1, ret_data, r_len);
}

void Teensy_uds::write_can_buffer(uint8_t buf[], uint8_t len){
	memset(diag_reply, 0, 0x20);

	memcpy(msg.buf, buf, len);
	can_bus.write(msg);
}



