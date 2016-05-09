The following is psuedocode for generating a program that can run the calf bottle monitor.

It is designed to have everything asleep the majority of the time. When a calf bottle is placed
into the holder, it will cause the accelerometer to trigger an interrupt, which will wake the 
Arduino. The Arduino will collect data from the accelerometer and load cell (via amplifier) at a
rate of 10Hz, storing to an SD card.  This will continue until low readings (both force and acceleration)
 indicate that the bottle has been removed, at which point everything will sleep once again, 
 waiting on a specific time to occur such that data transfer can begin.
 When the scheduled time is reached (as seen by accessing the RTC), the Arduino will wake 
 the XBee radio and begin transmitting data from the SD card. When transmission has finished
 the system will sleep again, waiting for the accelerometer to wake the system when a bottle is
 inserted.

 
 setup(){
	start_up_all_devices();
}

loop(){
	if (wake_on_accelerometer){
		while(bottle_is_in_holder(){
			read_force_and_acc();
			store_vals_on_SD();
		}
		if (experienced_full_feeding_event){
			sleep_waiting_for_clock();
		}else{
			sleep_waiting_for_accelerometer();
		}
	}else{
		if (scheduled_transmit_time_reached()){
			send_data_from_SD_via_XBee();
			sleep_waiting_for_accelerometer();
		}else{
			sleep_waiting_for_clock();
		}
	}
}