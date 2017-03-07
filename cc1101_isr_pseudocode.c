gdo_isr(){
	if (in_rx_state){
		if (rising){
			packet_in_rx = true;
		}else if (packet_in_rx){
			if (check_for_overflow() == true){
				packet_in_rx = false;
				flush_fifos();
				goto_rx_state();
			}else{
				read_remaining_bytes();
				process_headers();
				process_error_coding();
				goto_rx_state();
				packet_in_rx = false;
			}
		}
	}
	if (in_tx_state){
		if (rising){
			packet_in_tx = true;
		}else if (packet_in_tx){
			if (check_for_underflow() == true){
				packet_in_tx = false;
				flush_fifos();
				goto_rx_state();
			}else{
				goto_rx_state();
				packet_in_tx = false;
			}
		}
	}
}

gdo2_isr(){
	if (rising){
		if (packet_in_rx){
			read_remaining_bytes();
		}
	}
	if (falling){
		if (packet_in_tx){
			write_remaining_bytes();
		}
	}	
}
