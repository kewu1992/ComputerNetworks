#include "pkt_helper.h"

void process_data_packet(char* packet, bt_config_t* config,
                         struct sockaddr_in* from) {
	int seq_num, len;

	/* 1. parse data packet */
	char* data = parse_data(packet, &seq_num, &len);

	/* 2. find the peer that send the data packet */
	bt_peer_t* peer = find_peer(config->peers, from);

	if (peer->down_con == NULL)	{
		/* a "out-of-date" data packet, ignore it*/
		free(data);
		return;
	}

	/* store the data packet to window, get ack number*/
	int ack_num = window_recv_packet(peer->down_con, seq_num,
									 data, len);

	/* ack_num == -1 means duplicate data packet */
	if (ack_num != -1)
		send_ack_packet(ack_num, config, from); 
}

void send_data_packet()