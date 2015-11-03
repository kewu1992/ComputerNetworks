#include "timeout.h"

void process_download_timeout(bt_peer_t *peer, bt_config_t * config){
    /* receiver find that sender timeout (GET or ACK timeout, no DATA)*/

    /* inc timeout times*/
    peer->down_con->successive_fail++;
    if (is_crash(peer->down_con)){
        /* sender is crash, destroy connection */
        /* CLR select set*/
        FD_CLR(peer->down_con->timer_fd, &config->readset);
        /* destroy connection with peer */
        destroy_connection(peer->down_con);
        peer->down_con = NULL;
        /* set peer crash */
        peer->is_crash = 1;
        /* need check new connection */
        config->is_check = 1;
        /* dec current download number */
        config->cur_download_num--;
        return;
    }

    /* if it is a GET timeout, resend GET packet*/
    if (peer->down_con->cur_pkt == 0)
        send_getpkt(peer, config);
    else
        /* it is a ACK timeout, just wait? */
        return;
}

void process_upload_timeout(bt_peer_t *peer, bt_config_t * config){
    /* sender find that receiver timeout (DATA timeout, no ACK) */
    
    /* inc timeout times */
    peer->up_con->successive_fail++;
    if (is_crash(peer->up_con)){
        /* receiver is crash, destroy connection */
        /* CLR select set*/
        FD_CLR(peer->up_con->timer_fd, &config->readset);
        /* destroy connection with peer */
        destroy_connection(peer->up_con);
        peer->up_con = NULL;
        /* dec current upload number */
        config->cur_upload_num--;
        return;
    }

    /* resend DATA packet */
    send_data_packet(1, config, peer);
}