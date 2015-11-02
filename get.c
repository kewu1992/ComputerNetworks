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
        /* set peer crash */
        peer->is_crash = 1;
        /* need check new connection */
        config->is_check = 1;
        /* destroy connection with peer */
        destroy_connection(down_con);
        peer->down_con = NULL;
    } else {
        down_con->successive_fail++;
        /* re-generate and resend get packet */
        int len;
        char * get_pkt = generate_get(down_con->prev_get_hash, &len);
        send_packet(config->sock, get_pkt, len, 0, (struct sockaddr *)&peer->addr, sizeof(peer->addr));
        set_connection_timeout(down_con, 5, 0);
        free(get_pkt);
    }
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
