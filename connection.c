#include <sys/timerfd.h>

void init_connection(struct Connection* con, struct bt_peer_t* peer, 
					 int is_download){
	con->is_download = is_download;

	con->peer = peer;

	con->timer_fd = timerfd_create(CLOCK_REALTIME, 0);

	/* whole data */
	con->whole_size = 1;
	con->packets = (char**) malloc(sizeof(char*));
	con->packets[0] = NULL;
	con->packets_len = (int*) malloc(sizeof(int));
	con->packets_len[0] = 0;

	/* window data */
	con->last_pkt = 0;
	con->cur_pkt = 0;
	con->window_size = FIXED_WINDOW_SIZE;
	con->duplicate_ack = 0;

	con->successive_fail = 0;
}

int set_connection_timeout(struct Connection* con, int seconds, 
							int nanoseconds){
	struct itimerspec timer;
	/* set interval to zero, only timer once */
	timer.it_interval.tv_sec = 0;
	timer.it_interval.tv_nsec = 0;
	/* set timer value */
	timer.it_value.tv_sec = seconds;
	timer.it_value.tv_nsec = nano_seconds;
	
	return timerfd_settime(con->timer_fd, 0, &timer, NULL);
}

void destroy_connection(struct Connection* con){
	close(con->timer_fd);

	for (int i = 0; i < whole_size; i++)
		if (con->packets[i] != NULL){
			free(con->packets[i]);
			con->packets[i] = NULL;
		}
	free(con->packets);
	free(con->packets_len);
}



int is_crash(struct Connection* con){
	return (con->successive_fail >= CRASH_TIMES);
}

void douleArray(struct Connection* con){
	int new_size = con->whole_size * 2;
	char** new_array = (char**) malloc(sizeof(char*) * new_size);
	int* new_len = (int*) malloc(sizeof(int) * new_size);
	for (int i = 0; i < con->whole_size; i++){
		new_array[i] = con->packets[i];
		new_len[i] = con->packets_len[i];
	}
	for (int i = con->whole_size; i < new_size; i++)
		new_array[i] = NULL;
	con->whole_size = new_size;
	free(con->packets);
	free(con->new_len);
	con->packets = new_array;
	con->packets_len = new_len;
}

int window_recv_packet(struct Connection* con, int pkt_seq, 
						char* data, int pkt_len){
	while (pkt_seq >= con->whole_size)
		douleArray(con);
	con->packets[pkt_seq] = data;
	con->packets_len[pkt_seq] = pkt_len;
	/* if the received packet is the next expect */
	if (pkt_seq == con->cur_pkt){
		/* find the next packet that is not received */
		while(con->cur_pkt < con->whole_size && con->packets[con->cur_pkt])
			con->cur_pkt++;
	}
	return con->cur_pkt - 1;	// return the next expect packet - 1
}

int window_is_able_send(struct Connection* con){
	return (con->cur_pkt - con->last_pkt < con->window_size);
}

int window_ack_packet(struct Connection* con, int ack){
	if (ack >= con->last_pkt){
		con->last_pkt = ack + 1;
		con->duplicate_ack = 0;
	} else {
		/* recv duplicate ack */
		con->duplicate_ack++;
		if (con->duplicate_ack == MAX_DUPLICATE_ACK)
			return 1;
	}
	return 0;
}


