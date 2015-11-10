#include "data.h"


void process_data_packet(char* packet, bt_config_t* config,
                         struct sockaddr_in* from) {
	int seq_num, len;

	/* 1. parse data packet */
	char* data = parse_data(packet, &seq_num, &len);

	// fix 0-1
	seq_num--;

	printf("recv data: %d\n", seq_num);

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

	// fix 0-1
	ack_num++;

	/* 4. send ack packet */
	send_ack_packet(ack_num, config, peer);

	/* 5. check if have received all data (finish downloading of the chunk) */
	if (window_finish_data(peer->down_con)){
		int is_all_finish = finish_chunk(config, peer);
		if (is_all_finish)
			/* finish downloading of all chunks */
			config->is_check = 2;
		else
			/* need check new connection */
	        config->is_check = 1;
	    /* CLR select set*/
	    FD_CLR(peer->down_con->timer_fd, &config->readset);
		/* destroy connection with peer */
        destroy_connection(peer->down_con);
        peer->down_con = NULL;
        /* dec current download number */
        config->cur_download_num--;
	}
}

void send_data_packet(int is_resend, bt_config_t* config, bt_peer_t* toPeer) {
	if (is_resend){
		printf("resend data packet: %d\n", toPeer->up_con->last_pkt);
		send_packet(config->sock,
					toPeer->up_con->packets[toPeer->up_con->last_pkt],
					toPeer->up_con->packets_len[toPeer->up_con->last_pkt], 0,
					(struct sockaddr *)&toPeer->addr, sizeof(toPeer->addr));
		// the packet is resent, ignore it when calcualte RTT
		toPeer->up_con->RTT[toPeer->up_con->last_pkt].tv_sec = 0;
		toPeer->up_con->RTT[toPeer->up_con->last_pkt].tv_usec = 0;
		// a packet loss, reset sender connection
		reset_sender_connection(toPeer->up_con);
		/* reset timer */
		set_connection_timeout(toPeer->up_con, toPeer->up_con->RTO.tv_sec, toPeer->up_con->RTO.tv_usec * 1000);
	} else {
		while(window_is_able_send(toPeer->up_con)){
			printf("send data packet: %d\n", toPeer->up_con->cur_pkt);
			send_packet(config->sock,
					toPeer->up_con->packets[toPeer->up_con->cur_pkt],
					toPeer->up_con->packets_len[toPeer->up_con->cur_pkt], 0,
					(struct sockaddr *)&toPeer->addr, sizeof(toPeer->addr));
			// set packet send time
			struct timeval now;
			gettimeofday(&now, NULL);
			toPeer->up_con->RTT[toPeer->up_con->cur_pkt].tv_sec = now.tv_sec;
			toPeer->up_con->RTT[toPeer->up_con->cur_pkt].tv_usec = now.tv_usec;
			// inc send times;
			toPeer->up_con->send_times[toPeer->up_con->cur_pkt]++;
			
			toPeer->up_con->cur_pkt++;
		}
	}

}

int finish_chunk(bt_config_t* config, bt_peer_t* peer){
	char data[512*1024];
	int count = 0;
	for (int i = 0; i < peer->down_con->whole_size; i++)
		if (peer->down_con->packets[i]){
			memcpy(data+count, peer->down_con->packets[i],
					peer->down_con->packets_len[i]);
			count+= peer->down_con->packets_len[i];
		} else
			break;

	/* check byte size */
	if (count != 512*1024)
		return 0;	// fatal error!

	/* compute and check hash */
	char hash[SHA1_HASH_SIZE];
	SHA1Context sc;
	SHA1Init(&sc);
	SHA1Update(&sc, (void*)data, count);
	SHA1Final(&sc, (uint8_t*)hash);
	if (memcmp(hash, peer->down_con->prev_get_hash, SHA1_HASH_SIZE) != 0)
		return 0;

	/* find chunk id */
	int index = find_chunk(&config->get_chunks, hash);
	if (index == -1)
		return 0;	// fatal error!

	/* write data to output file */
	write_chunk_data_to_file(config, data, 512*1024,
                             config->get_chunks.chunks[index].id * 512*1024);
    config->written_chunks[index] = 1;

    /* add chunk to has_chunk */
    struct single_chunk* temp = (struct single_chunk*)
    							malloc(sizeof(struct single_chunk) *
    							(config->has_chunks.size + 1));
    memcpy(temp, config->has_chunks.chunks,
    	 	sizeof(struct single_chunk) * config->has_chunks.size);
    memcpy(temp[config->has_chunks.size].hash, hash, SHA1_HASH_SIZE);
    free(config->has_chunks.chunks);
    config->has_chunks.chunks = temp;
    config->has_chunks.size++;

    /* check if all chunks are downloaded */
    int i;
    for (i = 0; i < config->get_chunks.size; i++)
    	if (!config->written_chunks[i])
    		break;

    return (i == config->get_chunks.size);
}
