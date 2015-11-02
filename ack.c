
void process_ack_packet(char* packet, bt_config_t* config,
                         struct sockaddr_in* from) {
	
}

void send_ack_packet(int ack_num, struct bt_config_t* config,
					 struct sockaddr_in* to) {
	int len;
	char* packet = generate_ack(ack_num, &len);
	send_packet(config->sock, packet, len, 0, (struct sockaddr *)to, 
				sizeof(*to));
	free(packet);
}

