# Unified Diagnostic Services (UDS) for Teensy

This library builds on top of the [FlexCAN_T4](https://github.com/tonton81/FlexCAN_T4) library to provide some rudimentary UDS functionality for Teensy.

## Hardware
To connect to the CAN bus, mount a simple CAN transceiver (e.g., [this one](https://webshop.domoticx.nl/index.php?route=product/product&product_id=3935)). 

## Usage
**Careful, for some reason, when this library is included with <>, the code crashes (due to some interrupt address offset being wrongly calculated with the FlexCAN_T4 library). Link it to the current directory and include the library the following way:**

```
#include "teensy_uds.h"
```

### Arduino
The library works with a send and receive CANID (for isotp, UDS purposes). 

```
Teensy_uds uds_i = Teensy_uds(SEND_ID, RECV_ID);
```

It exposes several basic UDS functions. Careful, the exact implementation can differ from manufacturer to manufacturer (e.g., a reset_ecu being 37, or 37 01). This is to be expected and should be easy to adjust within the library.

```
Teensy_uds uds_i = Teensy_uds(0x7e0, 0x7e8);

void setup(){
	Serial.begin(115200);
	while(!Serial);
	uds_i.init_can(500000);
	uds_i.set_log_enabled(1);
}

void loop(){
	uint8_t ret_data[0x200]; 
	uint8_t routine_ctrl_data[] = {0x70, 0x10, 0x01, 0x00};
	uint16_t ret_len;
	int ret = 0;
	Serial.println("CAN test...");
	ret = uds_i.diag_session(2, ret_data, &ret_len);	
	ret = uds_i.sec_access(1, ret_data, &ret_len);
	ret = uds_i.req_download(0x70100100, secondary_bootloader_bin_len, 4, 4, ret_data, &ret_len);
	ret = uds_i.transfer_data(secondary_bootloader_bin, secondary_bootloader_bin_len, ret_data, &ret_len);
	ret = uds_i.transfer_exit(ret_data, &ret_len);
	ret = uds_i.routine_control(0x0301, routine_ctrl_data, 4, ret_data, &ret_len);
	while(1);
}


```



