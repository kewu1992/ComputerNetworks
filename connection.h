#ifndef _CONNECTION_H_
#define _CONNECTION_H_

#include <sys/timerfd.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdio.h>
#include "bt_parse.h"

#define FIXED_WINDOW_SIZE 8
#define CRASH_TIMES 5
#define MAX_DUPLICATE_ACK 3
#define INIT_SSTHRESH 64
#define USING_CONGESTION_WINDOW 1
#define ALPHA 0.875
#define BETA 0.25

struct Connection{
	/* 1 for download, I am receiver, the remote peer is sender
	 * 0 for upload, I am sender, the rmote peer is receiver */
	int is_download;

	bt_peer_t* peer;

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

    /* the following is for congestion control */
    struct timeval SRTT, rttvar, RTO;
    struct timeval* RTT;
    int is_slow_start;
    double cwnd;
    int ssthresh;


};

struct Connection * init_connection(bt_peer_t* peer, int is_download,
									char** packets, int size, 
									int max_pkt_len, int last_pkt_len, char* hash,
									int initRTT);

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

void inc_cwnd(struct Connection* con, int received_ack);

void reset_congestion(struct Connection* con);

void update_RTT(struct Connection* con, struct timeval* sample);

int set_timeout_by_RTO(struct Connection* con);

void timerdiff(struct timeval *a, struct timeval *b, struct timeval *res);

void printf_window(struct Connection *con);

long timeval2long(struct timeval* time);

void long2timeval(long num, struct timeval* time);

#endif
