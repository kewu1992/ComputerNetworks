/*
 * ihave.c
 *
 * Authors: Ke Wu <kewu@andrew.cmu.edu>
 *
 * Date: 10/23/2015
 *
 * Description: the file is used for process the received ihave packet and 
 *              send a new ihave packet
 */

#include "ihave.h"

/* process a IHAVE packet that is received from network */
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
        
        struct timeval now, diff_result;
        gettimeofday(&now, NULL);
        timersub(&now, &peer->initRTT, &diff_result);
        peer->initRTT.tv_sec = diff_result.tv_sec;
        peer->initRTT.tv_usec = diff_result.tv_usec;
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

/* when need to send a ihave pakcet, invoke the function */
void send_Ihave_packet(int count, char** hash, bt_config_t *config, 
                        struct sockaddr_in* from){
    /* generate a IHAVE packce and send it */
    int len;
    char* Ihave_packet = generate_Ihave(count, hash, CHUNK_HASH_SIZE, &len);

    /* send packet */
    //printf("send a IHAVE packet\n");
    send_packet(config->sock, Ihave_packet, len, 0, (struct sockaddr *)from, 
                sizeof(*from));

    /* free memory that is allocted from generate_Ihave() */
    free(Ihave_packet);
}
