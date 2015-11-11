/*
 * connection.c
 *
 * Authors: Ke Wu <kewu@andrew.cmu.edu>
 *
 * 15-441 Project 2
 *
 * Date: 11/10/2015
 *
 * Description: The file includes a struct Connection and its method 
 *              A Connection is either a download connection (for receiver) 
 *              or a upload connection (for sender). The struct Connection 
 *              is used for simulate a stateful connection of TCP 
 *              (with flow control and congestion control)
 *
 */

#include "connection.h"

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
                                    char* hash, int initRTT) {
    struct Connection * con = 
        (struct Connection *) malloc(sizeof(struct Connection));
    
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
        /* download, array is auto scaling */
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
    con->RTT = 
        (struct timeval*) calloc(con->whole_size, sizeof(struct timeval));
    con->send_times = (int*) calloc(con->whole_size, sizeof(int));

    con->is_slow_start = 1;
    con->cwnd = 1;
    con->ssthresh = INIT_SSTHRESH;

    return con;
}

/* we use timerfd for timeout timer of every connection. Add the timerfd to 
 * select(), when tiemout, select will break. The function is used to set 
 * timeout senconds and nanoseconds */
int set_connection_timeout(struct Connection* con, int seconds,
                            int nanoseconds) {
    struct itimerspec timer;
    /* set interval to zero, only timer once */
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_nsec = 0;
    /* set timer value */
    timer.it_value.tv_sec = seconds;
    timer.it_value.tv_nsec = nanoseconds;

    return timerfd_settime(con->timer_fd, 0, &timer, NULL);
}

/* free all resources (timer fd, memory) the connection used */
void destroy_connection(struct Connection* con){
    /* close timer file descriptor */
    close(con->timer_fd);

    /* free memory */
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
    free(con->send_times);
    free(con);
}

/* detect if the connection is crash */
int is_crash(struct Connection* con){
    return (con->successive_fail >= CRASH_TIMES);
}

/* helper functon, douable the packets array */
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

/* description: for receiver, receive a data packet 
 * parameters: 1. pointer of Connection
 *             2. the sequence num of received data packet 
 *             3. the data of received data packet
 *             4. the length of received data packet
 * return: the ack number (the next packet receiver expects to receive)
 */
int window_recv_packet(struct Connection* con, int pkt_seq,
                        char* data, int pkt_len){
    while (pkt_seq >= con->whole_size)
        douleArray(con);
    /* a duplicate data packet */
    if (con->packets[pkt_seq] != NULL){
        free(data);
        return -1;  // return -1 means duplicate data packet
    } else {
        /* stores data in memory with its length */
        con->packets[pkt_seq] = data;
        con->packets_len[pkt_seq] = pkt_len;
        /* add received bytes size */
        con->recv_size += pkt_len;
    }
    
    /* if the received packet is the next expect */
    if (pkt_seq == con->cur_pkt){
        /* find the next packet that has not received */
        while(con->cur_pkt < con->whole_size && con->packets[con->cur_pkt])
            con->cur_pkt++;
    }

    return con->cur_pkt - 1;    // return the next expect packet - 1
}

/* for sender, detect if is able to send the next data packet 
 * (not exceed window size)
 */
int window_is_able_send(struct Connection* con){
    // without congestion control, window_size = FIXED_WINDOW_SIZE
    // with congestion control, window_size = (int)cwnd
    if (USING_CONGESTION_WINDOW)
        con->window_size = (int)con->cwnd;
    return ((con->cur_pkt - con->last_pkt < con->window_size) &&
            (con->cur_pkt < con->whole_size));
}

/* description: for sender, receive a ack packet 
 * parameters: 1. pointer of Connection
               2. the ack number of received ack packet
 * return: if need to resend data packet (if the ack is a duplicate ack)
 */
int window_ack_packet(struct Connection* con, int ack){
    /* if ack >= the next pkt ready (not yet) send, means a cumulative ack
     * (after timeout and resend), window should shift */
    if (ack >= con->cur_pkt)
        con->cur_pkt = ack + 1;
    if (ack >= con->last_pkt){
        inc_cwnd(con);
        /* a expect ack (cumulative or not), window should shift */
        con->last_pkt = ack + 1;
        con->duplicate_ack = 0;

        /* if the packet only send once (has not resent), it can be used for
         * measuring RTT */
        if (con->send_times[ack] == 1 && 
            (con->RTT[ack].tv_sec != 0 || con->RTT[ack].tv_usec != 0)){
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
        /* if reach MAX_DUPLICATE_ACK, need to resend data packet */
        if (con->duplicate_ack == MAX_DUPLICATE_ACK)
            return 1;
    }
    return 0;
}

/* for sender, detect if all acks are received so that uploading is finished */
int window_finish_ack(struct Connection* con) {
    return (con->cur_pkt == con->whole_size && 
            con->last_pkt == con->whole_size);
}

/* for receiver, detect if all data packets are received so that download is 
 * finished */
int window_finish_data(struct Connection* con) {
    return (con->recv_size ==  512 * 1024);
}

/* description: increase congestion window, based on the state (slow start or 
 * congestion avoidance), cwnd += 1 or cwnd += 1/cwnd 
 */
void inc_cwnd(struct Connection* con){
    if (con->is_slow_start){
        // slow start
        con->cwnd += 1;
        printf_window(con);
        if ((int)con->cwnd >= con->ssthresh)
            con->is_slow_start = 0;
    } else {
        // congestion avoidance
        con->cwnd += 1/con->cwnd;
        printf_window(con);
    }
}

/* when a pakcet loss, ignore all timer of the following sent packets, restart 
 * to send data packet at the last unacked packet
 */
void reset_sender_connection(struct Connection* con){
    /* all timer of the following pakcets are meaningless */
    for (int i = con->last_pkt; i < con->cur_pkt; i++){
        con->RTT[i].tv_sec = 0;
        con->RTT[i].tv_usec = 0;
    }
    /* restart to send data packet at the last unacked packet */
    con->cur_pkt = con->last_pkt + 1;
    /* reset congestion window */
    reset_congestion(con);
}

/* reset congestion window = 1 and ssthresh = cwnd/2, go back to slow start */
void reset_congestion(struct Connection* con){
    con->ssthresh = (con->cwnd/2 > 2) ? (int)(con->cwnd/2) : 2;
    con->cwnd = 1;
    con->is_slow_start = 1;
    printf_window(con);
}

/* update RTT, RTO, rttvar with a new sample */
void update_RTT(struct Connection* con, struct timeval* sample){
    print_time("New sample", sample);

    /* new_rttvar = (1-BETA)*old_rttvar + BETA * abs(RTT - sample) */
    struct timeval diff;
    timerdiff(sample, &con->SRTT, &diff);

    long temp1 = timeval2long(&con->rttvar);
    long temp2 = timeval2long(&diff);
    long temp3 = (1-BETA) * temp1 + BETA * temp2;
    long2timeval(temp3, &con->rttvar);
    print_time("rttvar", &con->rttvar);

    /* new_RTT = ALPHA * old_RTT + (1-ALPHA) * sample */
    temp1 = timeval2long(&con->SRTT);
    temp2 = timeval2long(sample);
    temp3 = ALPHA * temp1 + (1-ALPHA) * temp2;
    long2timeval(temp3, &con->SRTT);
    print_time("SRTT", &con->SRTT);

    /* RTO = RTT + 4 * rttvar */
    temp1 = timeval2long(&con->SRTT);
    temp2 = timeval2long(&con->rttvar);
    //temp3 = temp1 + 4 * temp2;
    temp3 = 2 * temp1;
    long2timeval(temp3, &con->RTO);
    print_time("RTO", &con->RTO);
}

/* set the timeout of the next packet */
int set_timeout_by_RTO(struct Connection* con){
    if (con->cur_pkt == con->last_pkt){
        // the next packet hasn't sent, set timer to RTO
        return set_connection_timeout(con, con->RTO.tv_sec, 
                                                    con->RTO.tv_usec * 1000);
    }
    // the next packet has sent, set timer to RTO - lapse_time
    struct timeval now, lapse, diff_result;
    gettimeofday(&now, NULL);

    // con->RTT[con->last_pkt] stores the time that last_pkt sent
    timersub(&now, &con->RTT[con->last_pkt], &lapse);

    if (my_timercmp(&lapse, &con->RTO) > 0){ 
        // lapse time for the next ack > RTO, already timeout
        return set_connection_timeout(con, 0, 1);
    }
    else{
        timersub(&con->RTO, &lapse, &diff_result);
        print_time("new left", &diff_result);
        return set_connection_timeout(con, diff_result.tv_sec, 
                                                    diff_result.tv_usec * 1000);
    }
    return 0;
}

/* print window when it changes */
void printf_window(struct Connection *con){
    struct timeval now, diff_result;
    gettimeofday(&now, NULL);
    timersub(&now, &global_timer, &diff_result);
    int time_in_millisec = 
                    diff_result.tv_sec * 1000 + diff_result.tv_usec / 1000;
    FILE* file = fopen("problem2-peer.txt", "a");
    fprintf(file, "f%d\t%d\t%d\n", con->peer->id, 
            time_in_millisec, (int)con->cwnd);
    fclose(file);
}

/* helper function, convert timeval to long (in microsecond) */
long timeval2long(struct timeval* time){
    return time->tv_sec * 1000000 + time->tv_usec;
}

/* helper function, convert long (in microsecond) to timeval */
void long2timeval(long num, struct timeval* time){
    time->tv_sec = num / 1000000;
    time->tv_usec = num - time->tv_sec * 1000000;
}

/* helper function, calcualte difference (abs) of two timeval */
void timerdiff(struct timeval *a, struct timeval *b, struct timeval *res){
    if (my_timercmp(a,b) > 0)
        timersub(a, b, res);
    else
        timersub(b, a, res);
}

/* helper function, print a timeval with message (for debuging) */
void print_time(char* message, struct timeval* time){
    DPRINTF(DEBUG_SOCKETS, "%s:seconds:%d, useconds:%d\n", message, time->tv_sec, 
                                                                time->tv_usec);
}

