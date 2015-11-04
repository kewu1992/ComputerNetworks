#include "whohas.h"

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

    /* if I don't have any of the packet, then just return */
    if (count == 0)
        return;

    /* get the hash value of the chunks that I have */ 
    hash = (char**) malloc(sizeof(char*) * count);
    for (int i = 0; i < count; i++){
        hash[i] = (char*) malloc(CHUNK_HASH_SIZE);
        memcpy(hash[i], config->has_chunks.chunks[had_chunk_index[i]].hash,
                CHUNK_HASH_SIZE);
    }

    /* 2. send Ihave packet */
    send_Ihave_packet(count, hash, config, from);
    
    /* free memory of hash */
    for (int i = 0; i < count; i++)
        free(hash[i]);
    free(hash);
}

/* the function will flood the network with a WHOHAS packet */
void send_whohas_pkt(bt_config_t* config) {
    printf("send whohas pkt\n");
    char** hashs = (char**) malloc(sizeof(char*) * config->get_chunks.size);
    for (int i = 0; i < config->get_chunks.size; i++){
        hashs[i] = (char*) malloc(CHUNK_HASH_SIZE);
        memcpy(hashs[i], config->get_chunks.chunks[i].hash, CHUNK_HASH_SIZE);
    }

    /* generate WHOHAS packets, maybe more than 1 packet due to max length
     * limit of a UDP packet */
    int max_packet_len, last_packet_len, packets_size;
    char** whohas_packet = generate_whohas(config->get_chunks.size,
                                            hashs, CHUNK_HASH_SIZE,
                                            &packets_size, &last_packet_len);
    /* free memory of malloc */
    for (int i = 0; i < config->get_chunks.size; i++)
        free(hashs[i]);
    free(hashs);

    max_packet_len = MAX_PKT_LEN;

    /* send WHOHAS packet to all peers */
    bt_peer_t *peer = config->peers;
    while (peer) {
        /* should not send WHOHAS to myself,
           should not send WHOHAS to the peer that known its has_chunk info */
        if (peer->id == config->identity || peer->has_chunks.size != -1){
          peer = peer->next;
          continue;
        }
        for (int i = 0; i < packets_size; i++){
            int packet_len =
                    (i == packets_size-1) ? last_packet_len : max_packet_len;

            send_packet(config->sock, whohas_packet[i], packet_len, 0,
                        (struct sockaddr *)&peer->addr, sizeof(peer->addr));
        }
        peer = peer->next;
    }

    /* free memory that is allocted from generate_whohas() */
    for (int i = 0; i < packets_size; i++)
        free(whohas_packet[i]);
    free(whohas_packet);
}