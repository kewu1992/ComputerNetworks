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

    /* get the hash value of the chunks that I have */
    hash = (char**) malloc(sizeof(char*) * count);
    for (int i = 0; i < count; i++){
        hash[i] = (char*) malloc(CHUNK_HASH_SIZE);
        memcpy(hash[i], config->has_chunks.chunks[had_chunk_index[i]].hash, 
                CHUNK_HASH_SIZE);
    }

    /* generate IHAVE packce */
    char* Ihave_packet = generate_Ihave(count, hash, CHUNK_HASH_SIZE, &len);

    /* free memory of malloc */
    for (int i = 0; i < count; i++)
        free(hash[i]);
    free(hash);

    /* send packet */
    int has_send = 0, ret;
    while (has_send < len){
        ret = spiffy_sendto(config->sock, Ihave_packet + has_send, 
                    len - has_send, 0, (struct sockaddr *)from, sizeof(*from));
        if (ret < 0) {
            perror("send packet error");
            exit(-1);
        } else
            has_send += ret;
    }

    /* free memory that is allocted from generate_Ihave() */
    free(Ihave_packet);  
}

void process_Ihave_packet(int len, char* packet, bt_config_t* config,
                            struct sockaddr_in* from){
    int size;
    char** hash = parse_Ihave(len, packet, CHUNK_HASH_SIZE, &size);

    bt_peer_t* peer = find_peer(config->peers, from);

    peer->has_chunks.size = size;
    peer->has_chunks.chunks = (struct single_chunk*) 
                                malloc(sizeof(struct single_chunk) * size);
    for (int i  = 0; i < size; i++){
        memcpy(peer->has_chunks.chunks[i].hash, hash[i], CHUNK_HASH_SIZE);
        /* free memory that is allocted from parse_Ihave() */
        free(hash[i]);
    }    
    free(hash);
}