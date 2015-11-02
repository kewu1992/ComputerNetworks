/*
 * whohas_ihave.c
 *
 * Authors: Ke Wu <kewu@andrew.cmu.edu>
 *
 * Date: 10/23/2015
 *
 * Description: used for processing WHOHAS packet and IHAVE packet
 *
 */
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "spiffy.h"
#include "whohas_ihave.h"
#include "helper.h"
#include "pkt_helper.h"

void process_whohas_packet(int len, char* packet, bt_config_t* config,
                            struct sockaddr_in* from){
    int size;
    /* 1. first parse recieved WHOHAS packet */
    char** hash = parse_whohas(len, packet, CHUNK_HASH_SIZE, &size);

    int had_chunk_index[size];
    int count = 0;
    for (int i = 0; i < size; i++){
        int ret = find_chunk(&config->has_chunks, hash[i]);
        if (ret != -1)
            had_chunk_index[count++] = ret;
        /* free memory that is allocted from parse_whohas() */
        free(hash[i]);
    }
    free(hash);

    /* if I don't have any of the pack, then just return */
    if (count == 0)
        return;

    /* get the hash value of the chunks that I have */ hash = (char**) malloc(sizeof(char*) * count);
    for (int i = 0; i < count; i++){
        hash[i] = (char*) malloc(CHUNK_HASH_SIZE);
        memcpy(hash[i], config->has_chunks.chunks[had_chunk_index[i]].hash,
                CHUNK_HASH_SIZE);
    }

    /* 2. then generate a IHAVE packce and send it back */
    char* Ihave_packet = generate_Ihave(count, hash, CHUNK_HASH_SIZE, &len);

    /* free memory of malloc */
    for (int i = 0; i < count; i++)
        free(hash[i]);
    free(hash);

    /* send packet */
    send_packet(config->sock, Ihave_packet, len, 0, (struct sockaddr *)from, 
                sizeof(*from));

    /* free memory that is allocted from generate_Ihave() */
    free(Ihave_packet);
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
