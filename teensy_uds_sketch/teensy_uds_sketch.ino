#include "Teensy_uds.h"
#include <Teensy_Cs.h>

#define TRIG_OUT 14

Cs_Target uds_target;
Teensy_Cs t_cs(TRIG_OUT, &uds_target);

void setup(){
	Serial.begin(115200);
	while(!Serial);
	t_cs.run();
}


void loop(){
}

