/*
 * get.c
 *
 * Authors: Junqiang Li <junqianl@andrew.cmu.edu>
 *
 * Description: used for processing GET packet and timeout
 *
 */

#include "get.h"

void send_getpkt(bt_peer_t * peer, bt_config_t * config) {
    struct Connection * down_con = peer->down_con;
    /* re-generate and resend get packet */
    int len;
    char * get_pkt = generate_get(down_con->prev_get_hash, &len);
    send_packet(config->sock, get_pkt, len, 0, (struct sockaddr *)&peer->addr, sizeof(peer->addr));
    set_connection_timeout(down_con, 5, 0);
    free(get_pkt);
}


void process_getpkt(int len, char * packet, bt_config_t * config, struct sockaddr_in * from) {
    /* 1. parse get packet */
    char * hash = parse_get(packet);
    /* 2. find peer that send the get packet */
    bt_peer_t * peer = find_peer(config->peers, from);
    /* 3. if connecting, destroy connection */
    if (peer->up_con != NULL) {
        destroy_connection(peer->up_con);
        peer->up_con = NULL;
    }
    /* 4. read chunk data from file */
    char * data = read_chunk_data_from_file(config, hash);
    free(hash);
    /* 5. generate data packets */
    int packets_size;
    int last_p_len;
    char ** data_packets = generate_data(data, 0, &packets_size, &last_p_len);
    free(data);
    /* 6 .init connection */
    peer->up_con = init_connection(peer, 0, data_packets, packets_size,
                                    MAX_PKT_LEN, last_p_len);
    /* do not free data_packets, it should be free when connection is destroied */

    /* 7. send data packets */
    send_data_packet(0, config, peer);
}


void process_download(bt_config_t * config) {
    if (!config->is_check) {
        // if is_check is 0, return, keep waiting
        return;
    }

    if (config->cur_download_num == config->max_conn) {
        // if has reached max download, return and keep waiting
        return;
    }


}
