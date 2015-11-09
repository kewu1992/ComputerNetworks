#include "connection.h"


struct Connection * init_connection(bt_peer_t* peer, int is_download,
									char** packets, int size, 
									int max_pkt_len, int last_pkt_len, char* hash,
									int initRTT){
    struct Connection * con = (struct Connection *) malloc(sizeof(struct Connection));
	
	/* connection data */
	con->is_download = is_download;

	con->peer = peer;

	con->timer_fd = timerfd_create(CLOCK_REALTIME, 0);

	con->successive_fail = 0;

	con->prev_get_hash = hash;

	/* whole data */
	if (!is_download){
		/* upload, array is passed from outside */
		con->whole_size = size;
		con->packets = packets;
		con->packets_len = (int*) malloc(sizeof(int) * size);
		for (int i = 0; i < size-1; i++)
			con->packets_len[i] = max_pkt_len;
		con->packets_len[size-1] = last_pkt_len;
	} else {
		/* download, array is auto scalling */
		con->whole_size = 1;
		con->packets = (char**) malloc(sizeof(char*));
		con->packets[0] = NULL;
		con->packets_len = (int*) malloc(sizeof(int));
		con->packets_len[0] = 0;	
	}

	/* window data */
	con->last_pkt = 0;
	con->cur_pkt = 0;
	con->window_size = FIXED_WINDOW_SIZE;

	/* window data for upload (sender) */
	con->duplicate_ack = 0;

	/* window data for download (receiver) */
	con->recv_size = 0;

	/* congestion control data */
	con->SRTT.tv_sec = initRTT;
	con->SRTT.tv_usec = 0;
	con->rttvar.tv_sec = 0;
	con->rttvar.tv_usec = 0;
	con->RTT = (struct timeval*) calloc(con->whole_size, sizeof(struct timeval));

	con->is_slow_start = 1;
	con->cwnd = 1;
	con->ssthresh = INIT_SSTHRESH;

    return con;
}

int set_connection_timeout(struct Connection* con, int seconds,
							int nanoseconds){
	struct itimerspec timer;
	/* set interval to zero, only timer once */
	timer.it_interval.tv_sec = 0;
	timer.it_interval.tv_nsec = 0;
	/* set timer value */
	timer.it_value.tv_sec = seconds;
	timer.it_value.tv_nsec = nanoseconds;

	return timerfd_settime(con->timer_fd, 0, &timer, NULL);
}

void destroy_connection(struct Connection* con){
	close(con->timer_fd);

	for (int i = 0; i < con->whole_size; i++)
		if (con->packets[i] != NULL){
			free(con->packets[i]);
			con->packets[i] = NULL;
		}
	free(con->packets);
	free(con->packets_len);
	if (con->prev_get_hash)
	    free(con->prev_get_hash);
	free(con->RTT);
    free(con);
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
	free(con->packets_len);
	con->packets = new_array;
	con->packets_len = new_len;
}

int window_recv_packet(struct Connection* con, int pkt_seq,
						char* data, int pkt_len){
	while (pkt_seq >= con->whole_size)
		douleArray(con);
	/* a duplicate data packet */
	if (con->packets[pkt_seq] != NULL){
		free(data);
		return -1;	// return -1 means duplicate data packet
	}
	con->packets[pkt_seq] = data;
	con->packets_len[pkt_seq] = pkt_len;
	/* if the received packet is the next expect */
	if (pkt_seq == con->cur_pkt){
		/* find the next packet that is not received */
		while(con->cur_pkt < con->whole_size && con->packets[con->cur_pkt])
			con->cur_pkt++;
	}

	/* add received bytes size */
	con->recv_size += pkt_len;
	return con->cur_pkt - 1;	// return the next expect packet - 1
}

int window_is_able_send(struct Connection* con){
	if (USING_CONGESTION_WINDOW)
		con->window_size = (int)con->cwnd;
	return ((con->cur_pkt - con->last_pkt < con->window_size) &&
			(con->cur_pkt < con->whole_size));
}

int window_ack_packet(struct Connection* con, int ack){
	if (ack >= con->last_pkt){
		inc_cwnd(con, ack + 1 - con->last_pkt);

		con->last_pkt = ack + 1;
		con->duplicate_ack = 0;

		if (con->RTT[ack].tv_sec != 0 || con->RTT[ack].tv_usec != 0){
			// calculate new sample of RTT
			struct timeval now, sample;
			gettimeofday(&now, NULL);
			timersub(&now, &con->RTT[ack], &sample);
			update_RTT(con, &sample);
		}
		// set timeout of the next ack
		set_timeout_by_RTO(con);
	} else {
		/* recv duplicate ack */
		con->duplicate_ack++;
		if (con->duplicate_ack == MAX_DUPLICATE_ACK)
			return 1;
	}
	return 0;
}

int window_finish_ack(struct Connection* con) {
	return (con->cur_pkt == con->whole_size && 
			con->last_pkt == con->whole_size);
}

int window_finish_data(struct Connection* con) {
	return (con->recv_size ==  512 * 1024);
}

void inc_cwnd(struct Connection* con, int received_ack){
	while(received_ack > 0){
		if (con->is_slow_start){
			// slow start
			con->cwnd += 1;
			printf_window(con);
			if (con->cwnd >= con->ssthresh)
				con->is_slow_start = 0;
		} else {
			// congestion avoidance
			con->cwnd += 1/con->cwnd;
			printf_window(con);
		}
		received_ack--;
	}
}

void reset_congestion(struct Connection* con){
	con->ssthresh = (con->cwnd/2 > 2) ? (int)(con->cwnd/2) : 2;
	con->cwnd = 1;
	con->is_slow_start = 1;
	printf_window(con);
}

void update_RTT(struct Connection* con, struct timeval* sample){
	//struct timeval zero, temp;
	//zero.tv_sec = 0;
	//zero.tv_usec = 0;

	struct timeval diff;
	timerdiff(sample, &con->SRTT, &diff);

	long temp1 = timeval2long(&con->rttvar);
	long temp2 = timeval2long(&diff);
	long temp3 = (1-BETA) * temp1 + BETA * temp2;
	long2timeval(temp3, &con->rttvar);
	//temp.tv_sec = (1-BETA) * con->rttvar.tv_sec + BETA * diff.tv_sec;
	//temp.tv_usec = (1-BETA) * con->rttvar.tv_usec + BETA * diff.tv_usec;
	//timeradd(&zero, &temp, &con->rttvar);

	temp1 = timeval2long(&con->SRTT);
	temp2 = timeval2long(sample);
	temp3 = ALPHA * temp1 + (1-ALPHA) * temp2;
	long2timeval(temp3, &con->SRTT);
	//temp.tv_sec = ALPHA * con->SRTT.tv_sec + (1-ALPHA) * sample->tv_sec;
	//temp.tv_usec = ALPHA * con->SRTT.tv_usec + (1-ALPHA) * sample->tv_usec;
	//timeradd(&zero, &temp, &con->SRTT);

	temp1 = timeval2long(&con->SRTT);
	temp2 = timeval2long(&con->rttvar);
	temp3 = temp1 + 4 * temp2;
	long2timeval(temp3, &con->RTO);
	//temp.tv_sec = con->SRTT.tv_sec + 4 * con->rttvar.tv_sec;
	//temp.tv_usec = con->SRTT.tv_usec + 4 * con->rttvar.tv_usec; 
	//timeradd(&zero, &temp, &con->RTO);
}

int set_timeout_by_RTO(struct Connection* con){
	struct timeval now, lapse, diff_result;
	gettimeofday(&now, NULL);

	timersub(&now, &con->RTT[con->last_pkt], &lapse);

	if (my_timercmp(&lapse, &con->RTO) > 0)	// lapse time for the next ack > RTO, already timeout
		return set_connection_timeout(con, 0, 1);
	else{
		timersub(&con->RTO, &lapse, &diff_result);
		return set_connection_timeout(con, diff_result.tv_sec, diff_result.tv_usec * 1000);
	}
	return 0;
}

void timerdiff(struct timeval *a, struct timeval *b, struct timeval *res){
	if (my_timercmp(a,b) > 0)
		timersub(a, b, res);
	else
		timersub(b, a, res);
}

void printf_window(struct Connection *con){
	struct timeval now, diff_result;
	gettimeofday(&now, NULL);
	timersub(&now, &global_timer, &diff_result);
	int time_in_millisec = diff_result.tv_sec * 1000 + diff_result.tv_usec / 1000;
	FILE* file = fopen("problem2-peer.txt", "a");
	fprintf(file, "f%d\t%d\t%d\n", con->peer->id, time_in_millisec, (int)con->cwnd);
	fclose(file);
}

long timeval2long(struct timeval* time){
	return time->tv_sec * 1000000 + time->tv_usec;
}

void long2timeval(long num, struct timeval* time){
	time->tv_sec = num / 1000000;
	time->tv_usec = num - time->tv_sec * 1000000;
}