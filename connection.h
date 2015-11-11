/*
 * connection.h
 *
 * Authors: Ke Wu <kewu@andrew.cmu.edu>
 *
 * 15-441 Project 2
 *
 * Date: 11/10/2015
 *
 * Description: header file for connection.c
 *
 *
 */
#ifndef _CONNECTION_H_
#define _CONNECTION_H_

#include <sys/timerfd.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdio.h>
#include "bt_parse.h"
#include "helper.h"

/* fixed window size, usded for task 1 (without congestion control) */
#define FIXED_WINDOW_SIZE 8
/* how many successive timeout to consider the peer is crash */
#define CRASH_TIMES 5
/* how many duplicate acks to consider a packet loss */
#define MAX_DUPLICATE_ACK 3
/* initial ssthresh for slow start of congestion control */
#define INIT_SSTHRESH 64
/* if using congestion control */
#define USING_CONGESTION_WINDOW 1
/* used for RTT measuring, new_RTT = ALPHA * old_RTT + (1-ALPHA) * sample */
#define ALPHA 0.875
/* used for rttvar measuring, new_rttvar = (1-BETA)*old_rttvar + BETA*sample */
#define BETA 0.25

/* a Connection is either a download connection (for receiver) or a upload 
 * connection (for sender). The struct Connection is used for simulate a 
 * stateful connection of TCP (with flow control and congestion control)
 */
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
    // for sender to use, it will be next pkt ready (not yet) send
    int cur_pkt;

    // without congestion control, window_size = FIXED_WINDOW_SIZE
    // with congestion control, window_size = (int)cwnd
    int window_size;

    // for sender to use, # of times sender has received duplicate ack,
    // if >= MAX_DUPLICATE_ACK, sender will resend the data packet
    int duplicate_ack;

    // for receiver to use, how many bytes has received from sender
    int recv_size;

    /* to detect peer crash, if successive_fail >= CRASH_TIMES then consider 
        the peer is crash */
    int successive_fail;

    // for receiver to use, chunk hash in GET packet,
    char * prev_get_hash;

    /* the following is for congestion control, only for sender*/
    struct timeval SRTT, rttvar, RTO;
    // the RTT array stores the send time of each packets
    struct timeval* RTT;
    // stores which state currenly at (slow start or congestion avoidance)
    int is_slow_start;
    // stoes congestion control window
    double cwnd;
    int ssthresh;
    /* how many times each packet has sent, > 1 means there is resend, should
        not use the data to calcualte RTT */
    int * send_times;
};

/* desceiption: initialize connection 
 * parameters: 1. which peer the connection connect to
 *             2. if it is a download connection
 *             3. (for sender), the generated data packets
 *             4. (for sender), the numbwe of generated data packets
 *             5. (for sender), the maximum packet length
 *             6. (for sender), the last packet length
 *             7. hash value of the chunk that the connection is processing
 *             8. the initial RTT
 */
struct Connection * init_connection(bt_peer_t* peer, int is_download,
                                    char** packets, int size, 
                                    int max_pkt_len, int last_pkt_len, 
                                    char* hash, int initRTT);

/* we use timerfd for timeout timer of every connection. Add the timerfd to 
 * select(), when tiemout, select will break. The function is used to set 
 * timeout senconds and nanoseconds */
int set_connection_timeout(struct Connection* con, int seconds,
                            int nanoseconds);

/* free all resources (timer fd, memory) the connection used */
void destroy_connection(struct Connection* con);

/* detect if the connection is crash */
int is_crash(struct Connection* con);

/* description: for receiver, receive a data packet 
 * parameters: 1. pointer of Connection
 *             2. the sequence num of received data packet 
 *             3. the data of received data packet
 *             4. the length of received data packet
 * return: the ack number (the next packet receiver expects to receive)
 */
int window_recv_packet(struct Connection* con, int pkt_seq,
                        char* data, int pkt_len);

/* for sender, detect if is able to send the next data packet 
 * (not exceed window size)
 */
int window_is_able_send(struct Connection* con);

/* description: for sender, receive a ack packet 
 * parameters: 1. pointer of Connection
               2. the ack number of received ack packet
 * return: if need to resend data packet (if the ack is a duplicate ack)
 */
int window_ack_packet(struct Connection* con, int ack);

/* for sender, detect if all acks are received so that uploading is finished */
int window_finish_ack(struct Connection* con);

/* for receiver, detect if all data packets are received so that download is 
 * finished */
int window_finish_data(struct Connection* con);

/* description: increase congestion window, based on the state (slow start or 
 * congestion avoidance), cwnd += 1 or cwnd += 1/cwnd 
 */
void inc_cwnd(struct Connection* con);

/* when a pakcet loss, ignore all timer of the following sent packets, restart 
 * to send data packet at the last unacked packet
 */
void reset_sender_connection(struct Connection* con);

/* reset congestion window = 1 and ssthresh = cwnd/2, go back to slow start */
void reset_congestion(struct Connection* con);

/* update RTT, RTO, rttvar with a new sample */
void update_RTT(struct Connection* con, struct timeval* sample);

/* set the timeout of the next packet */
int set_timeout_by_RTO(struct Connection* con);

/* print window when it changes */
void printf_window(struct Connection *con);

/* helper functon, douable the packets array */
void douleArray(struct Connection* con);

/* helper function, convert timeval to long (in microsecond) */
long timeval2long(struct timeval* time);

/* helper function, convert long (in microsecond) to timeval */
void long2timeval(long num, struct timeval* time);

/* helper function, calcualte difference (abs) of two timeval */
void timerdiff(struct timeval *a, struct timeval *b, struct timeval *res);

/* helper function, print a timeval with message (for debuging) */
void print_time(char* message, struct timeval* time);

#endif
