#include "ack.h"

void process_ack_packet(char* packet, bt_config_t* config,
                         struct sockaddr_in* from) {
	/* 1. parse ack packet */
	int ack_num = parse_ack(packet);

	/* 2. find the peer that send the ack packet */
	bt_peer_t* peer = find_peer(config->peers, from);

	if (peer->up_con == NULL){
		/* a "out-of-date" ack packet, ignore it*/
		return;
	}

	peer->up_con->successive_fail = 0;

	/* 3. ack to window */
	int is_resend = window_ack_packet(peer->up_con, ack_num);

	/* 4. send (or resend) data packet */
	send_data_packet(is_resend, config, peer);

	/* 5. check if have received all acks (finish uploading) */
	if (window_finish_ack(peer->up_con)){
		/* CLR select set*/
		FD_CLR(peer->up_con->timer_fd, &config->readset);
		/* destroy connection with peer */
        destroy_connection(peer->up_con);
        peer->up_con = NULL;
        /* dec current upload number */
        config->cur_upload_num--;
	}
}

void send_ack_packet(int ack_num, bt_config_t* config, bt_peer_t* toPeer) {
	int len;

	/* ack_num == -1 means duplicate data packet */
	if (ack_num == -1)
		return;

	char* packet = generate_ack(ack_num, &len);
	send_packet(config->sock, packet, len, 0, (struct sockaddr *)&toPeer->addr, 
				sizeof(toPeer->addr));
	free(packet);

	/* reset timer */
	set_connection_timeout(toPeer->down_con, 5, 0);
}

