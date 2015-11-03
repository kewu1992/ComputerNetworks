#ifndef _CONNECTION_H_
#define _CONNECTION_H_

#include <sys/timerfd.h>
#include <stdlib.h>
#include "bt_parse.h"

#define FIXED_WINDOW_SIZE 8
#define CRASH_TIMES 5
#define MAX_DUPLICATE_ACK 3

struct Connection{
	/* 1 for download, I am receiver, the remote peer is sender
	 * 0 for upload, I am sender, the rmote peer is receiver */
	int is_download;

	struct bt_peer_t* peer;

	int timer_fd;

	int whole_size; // size of packets array
	char** packets; // chunk data packets array
	int* packets_len;   // each packet's length in bytes

	int last_pkt;   // for sender to use, next expected ack

    // for receiver to use, next expected packet
    // for sender to use, it will be next pkt not yet sent
    int cur_pkt;

	int window_size;

    // for sender to use, # of times sender has received ack,
    // if >= MAX_DUPLICATE_ACK, sender will resend the data packet
	int duplicate_ack;

	// for receiver to use, how many bytes has received from sender
	int recv_size;

	int successive_fail;

    // for receiver to use, previous chunk hash in GET packet,
    // in case we need to resend GET
    char * prev_get_hash;
};

struct Connection * init_connection(struct bt_peer_t* peer, int is_download,
									char** packets, int size, 
									int max_pkt_len, int last_pkt_len);

int set_connection_timeout(struct Connection* con, int seconds,
							int nanoseconds);

void destroy_connection(struct Connection* con);

int is_crash(struct Connection* con);

int window_recv_packet(struct Connection* con, int pkt_seq,
						char* data, int pkt_len);

int window_is_able_send(struct Connection* con);

int window_ack_packet(struct Connection* con, int ack);

int window_finish_ack(struct Connection* con);

int window_finish_data(struct Connection* con);

#endif
