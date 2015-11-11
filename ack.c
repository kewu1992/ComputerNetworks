/*
 * ack.c
 *
 * Authors: Ke Wu <kewu@andrew.cmu.edu>
 *
 * 15-441 Project 2
 *
 * Date: 11/10/2015
 *
 * Description: the file is used for process the received ack packet and send a 
 *              new ack packet
 *
 */

#include "ack.h"

/* when an ack packet is received, invoke the function to process the pakcet */
void process_ack_packet(char* packet, bt_config_t* config,
                         struct sockaddr_in* from) {
    /* 1. parse ack packet */
    int ack_num = parse_ack(packet);

    // fix 0-1 problem
    ack_num--;

    DPRINTF(DEBUG_SOCKETS, "recv ack: %d\n", ack_num);

    /* 2. find the peer that send the ack packet */
    bt_peer_t* peer = find_peer(config->peers, from);

    if (peer->up_con == NULL){
        /* a "out-of-date" ack packet, ignore it*/
        return;
    }

    /* successfully receive a packet, reset successive_fail */
    peer->up_con->successive_fail = 0;

    /* 3. ack to window */
    int is_resend = window_ack_packet(peer->up_con, ack_num);

    if (is_resend)
        DPRINTF(DEBUG_SOCKETS, "resend data packet due to duplicate ack\n");

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

/* when need to send an ack pakcet, invoke the function */
void send_ack_packet(int ack_num, bt_config_t* config, bt_peer_t* toPeer) {
    int len;

    /* ack_num == 0 means duplicate data packet */
    if (ack_num == 0)
        return;

    DPRINTF(DEBUG_SOCKETS, "send ack packet: %d\n", ack_num-1);

    /* generate ack packet */
    char* packet = generate_ack(ack_num, &len);
    /* send ack packet */
    send_packet(config->sock, packet, len, 0, (struct sockaddr *)&toPeer->addr,
                sizeof(toPeer->addr));
    free(packet);

    /* reset timer */
    set_connection_timeout(toPeer->down_con, CONNECTION_TIMEOUT, 0);
}

