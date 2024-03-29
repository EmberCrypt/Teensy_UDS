/*
 * teensy_uds.h - library for uds messaging through teensy
 * Created by Jan Van den Herrewegen, January 22nd 2022
 *
*/
#ifndef __TEENSY_UDS_H
#define __TEENSY_UDS_H


#include <Arduino.h>
#include <isotp.h>

#define		DIAG_MODE	0x10
#define		ECU_RESET	0x11
#define		READ_DATA_BY_ID	0x22
#define		READ_MEM	0x23
#define		B_SEC_ACCESS	0x27
#define		ROUTINE_CTRL	0x31
#define		REQ_DL		0x34
#define		REQ_UL		0x35
#define		TF_DATA		0x36
#define		TF_EXIT		0x37
#define		TESTER_PRESENT	0x3e

typedef struct uds_ret_data{
	uint16_t rcv_id;
	uint16_t len;
	uint8_t data[0x1000];
} UDS_Ret_Data;

enum UDS_Log{
	LOG_DISABLED = 0,
	LOG_TEENSY_CS = 1, // Use the Teensy_CS log facility
	LOG_SERIAL = 2 // Print to the Serial interface
};

enum Modus{
	MODE_UDS = 0,
	MODE_CAN = 1
};


class Teensy_uds
{
	public:
		Teensy_uds(uint16_t write_id, uint16_t read_id);
		void init_can(int baud);
		void diag_write(const uint8_t *bf, const uint16_t len);
		int diag_read(UDS_Ret_Data*, long timeout_mus);

		void set_mode(Modus mode){this->mode = mode;}

		/*
		 * Define seedkey function here, to be included in separate source file. Returns the length of the key
		 */
		int seedkey(uint8_t seed[], uint8_t key[]);

		/*
		 * Set CANids for reading & writing
		 */
		void set_ids(uint16_t read_id, uint16_t write_id); 

		/* UDS funcs */

		/*
		 * Changes the diagnostic session to mode
		 */
		int diag_session(uint8_t mode, UDS_Ret_Data* r);

		/*
		 * Clears the read queue 
		 */
		void clear_read_queue();

		/*
		 * Reset ECU (send 11 01)
		 */
		int reset_ecu(UDS_Ret_Data* r);

		/*
		 * Security access request on given level
		 */
		int sec_access_req(uint8_t level, UDS_Ret_Data* r);

		/*
		 * Security access procedure
		 */
		int sec_access(uint8_t level, UDS_Ret_Data* r);

		/*
		 * Security access response on same level - sends the key reply based on the seed
		 */
		int sec_access_resp(uint8_t level, uint8_t* key_reply, int key_len, UDS_Ret_Data* r);

		/*
		 * Read data by identifier - can read VIN (FF01), software version, ...
		 */
		int read_data_by_id(uint16_t id, UDS_Ret_Data* r);

		/*
		 * Routine control
		 */
		int routine_control(uint16_t routine_id, uint8_t data[], uint16_t data_len, UDS_Ret_Data* r);

		/*
		 * Control to write to the can buffer, for glitching
		 */
		int write_can_buffer(uint8_t buf[], uint8_t len);


		/*
		 * TODO add encrypted / compressed possibly
		 */
		int req_download(uint32_t addr, uint32_t size, uint8_t n_addr, uint8_t n_size, UDS_Ret_Data* r);

		/* 
		 * Returns 0 if successful, otherwise error code of the request
		 */
		int uds_req(const uint8_t* req, const uint16_t len, UDS_Ret_Data* r);


		/*
		 *
		 */
		int request_ul(uint8_t comp_enc_b, uint8_t size_addr, uint32_t addr_st, uint32_t addr_e, UDS_Ret_Data* r);

		int tester_present();

		/*
		 * Transfer data blocks 
		 */
		void get_data();
		/*
		 * For online transfer data (big firmware images)
		 */
		int transfer_data(UDS_Ret_Data* r);
		int transfer_data(const uint8_t* data, const uint16_t len, UDS_Ret_Data* r);
		int transfer_exit(UDS_Ret_Data* r);

		void set_log_enabled(UDS_Log log_enabled){
			this->log_enabled = log_enabled;	
		};


	private:
		long timeout_mus;
		uint16_t write_id;
		uint16_t read_id;
		ISOTP_data w_config;

		UDS_Log log_enabled = LOG_DISABLED;
		Modus mode = MODE_UDS;

		void print_msg(uint32_t can_id, uint16_t len, uint8_t data[]);

		uint8_t tf_data_seq = 1;
};


#endif
