/*
 * get.c
 *
 * Authors: Junqiang Li <junqianl@andrew.cmu.edu>
 *
 * Description: used for processing GET packet and timeout
 *
 */

#include "get.h"

void process_getpkt_timeout(bt_peer_t * peer, bt_config_t * config) {
    struct Connection * down_con = peer->down_con;
    down_con->successive_fail++;

    if (is_crash(down_con)) {
        peer->is_crash = 1;
        config->is_check = 1;
    } else {
        int len = 0;
        char * get_pkt = generate_get(down_con->prev_get_hash, &len);
        send_packet(config->sock, get_pkt, len, 0, (struct sockaddr *)&peer->addr, sizeof(peer->addr));
        set_connection_timeout(down_con, 5, 0);
    }
}


void process_getpkt(int len, char * packet, bt_config_t * config, struct sockaddr_in * from) {
    char * hash = parse_get(packet);
    bt_peer_t * peer = find_peer(config->peers, from);
    if (peer->up_con != NULL) {
        destroy_connection(peer->up_con);
        peer->up_con = NULL;
    }
    peer->up_con = init_connection(peer, 0);
    char * data = read_chunk_data_from_file(hash);
    int packets_size;
    int last_p_len;
    char ** data_packets = generate_data(data, 0, &packets_size, &last_p_len);
    send_data_packet(0, config, peer);
}

void process_Ihave_packet(int len, char* packet, bt_config_t* config,
                            struct sockaddr_in* from){
    int size;
    /* 1. parse IHAVE packet */
    char** hash = parse_Ihave(len, packet, CHUNK_HASH_SIZE, &size);

    /* 2. find the peer that send the IHAVE packet */
    bt_peer_t* peer = find_peer(config->peers, from);

    if (peer->has_chunks.size == -1){
        config->known_peer_num++;
        peer->has_chunks.size = size;
        peer->has_chunks.chunks = (struct single_chunk*)
                                    malloc(sizeof(struct single_chunk) * size);
        /* 3. save IHAVE info of the peer */
        for (int i  = 0; i < size; i++){
            memcpy(peer->has_chunks.chunks[i].hash, hash[i], CHUNK_HASH_SIZE);
            /* free memory that is allocted from parse_Ihave() */
            free(hash[i]);
        }
        free(hash);
    } else {
        /* already know the info, just free memory*/
        for (int i  = 0; i < size; i++){
            /* free memory that is allocted from parse_Ihave() */
            free(hash[i]);
        }
        free(hash);
    }

    /* 4. set is_check true */
        config->is_check = 1;
}
