/*
 * denied.c
 *
 * Authors: Junqiang Li <junqianl@andrew.cmu.edu>
 *
 * 15-441 Project 2
 *
 * Date: 11/10/2015
 *
 * Description: the file is used for process the received denied packet and 
 *              send a new denied packet
 *
 */
#include <sys/select.h>
#include "denied.h"
#include "connection.h"

/* when a denied packet is received, invoke the function to process the pakcet */
void send_deniedpkt(bt_peer_t * peer, bt_config_t * config) {
    int len;
    char * denied_pkt = generate_denied(&len);
    send_packet(config->sock, denied_pkt, len, 0, 
                (struct sockaddr *)&peer->addr, sizeof(peer->addr));
    free(denied_pkt);
}

/* when need to send a denied pakcet, invoke the function */
void process_deniedpkt(int len, char * packet, bt_config_t * config, 
                        struct sockaddr_in * from) {
    bt_peer_t * peer = find_peer(config->peers, from);
    config->cur_download_num--;
    FD_CLR(peer->down_con->timer_fd, &config->readset);
    destroy_connection(peer->down_con);
    peer->down_con = NULL;
    peer->is_crash = 1;
    config->is_check = 1;
}
