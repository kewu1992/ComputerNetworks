#include "pkt_helper.h"
#include "connection.h"
#include "data.h"
#include "ack.h"

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

	peer->down_con->successive_fail = 0;

	/* 3. store the data packet to window, get ack number*/
	int ack_num = window_recv_packet(peer->down_con, seq_num, data, len);
	/* do not free data, it should be free when connection is destroied */

	/* 4. send ack packet */
	send_ack_packet(ack_num, config, peer); 
}

void send_data_packet(int is_resend, bt_config_t* config, bt_peer_t* toPeer) {
	if (is_resend){
		send_packet(config->sock, 
					toPeer->up_con->packets[toPeer->up_con->last_pkt], 
					toPeer->up_con->packets_len[toPeer->up_con->last_pkt], 0,
					(struct sockaddr *)&toPeer->addr, sizeof(toPeer->addr));
	} else {
		while(window_is_able_send(toPeer->up_con)){
			send_packet(config->sock, 
					toPeer->up_con->packets[toPeer->up_con->cur_pkt], 
					toPeer->up_con->packets_len[toPeer->up_con->cur_pkt], 0,
					(struct sockaddr *)&toPeer->addr, sizeof(toPeer->addr));
			toPeer->up_con->cur_pkt++;	
		}
	}

	/* reset timer */
	set_connection_timeout(toPeer->up_con, 5, 0);
}