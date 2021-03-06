/*
 * helper.c
 *
 * Authors: Ke Wu <kewu@andrew.cmu.edu>
 *
 * Date: 11/10/2015
 *
 * Description: some helper functions
 *
 */

#include "helper.h"

/* description: find a chunk in an array of chunks (compare hash value)
 * return: the index of the chunk in the array,
 *         -1 if not found
 */
int find_chunk(struct many_chunks *chunks, char* hash){
    for (int i = 0; i < chunks->size; i++)
        if (memcmp(hash, chunks->chunks[i].hash, CHUNK_HASH_SIZE) == 0)
            return i;
    return -1;
}

/* description: find a network address in a linkedlist of peers
 *               (compare IP address and port)
 *  return: the pointer of the peer that have the netowrk address
 *          NULL if not found
 */
bt_peer_t* find_peer(bt_peer_t *peers, struct sockaddr_in* addr) {
    char buf[255];

    bt_peer_t* peer = peers;
    while (peer){
        strcpy(buf, inet_ntoa(peer->addr.sin_addr));
        if ((strcmp(buf, inet_ntoa(addr->sin_addr)) == 0) &&
            (peer->addr.sin_port == addr->sin_port))
            return peer;
        peer = peer->next;
    }

    return NULL;
}

/* description: send a packet using sendto */
void send_packet(int socket, char* data, size_t packet_len, int flag,
                 struct sockaddr *dest_addr, socklen_t addr_len) {
    
    /* simulate packet loss */
    if (((double)rand() / (double)RAND_MAX) < PACKET_LOSS_RATIO)
        return;

    int has_send = 0, ret;
    while (has_send < packet_len){
        ret = spiffy_sendto(socket, data + has_send, packet_len - has_send, 0,
                            dest_addr, addr_len);
        if (ret < 0) {
            perror("send packet error");
            exit(-1);
        } else
            has_send += ret;
    }
}

/* read the data of a chunk from file (identified by hash) */
char* read_chunk_data_from_file(bt_config_t* config, char* hash){

    char buf[BT_FILENAME_LEN], buf2[BT_FILENAME_LEN];
    FILE* master_chunk_file = fopen(config->chunk_file, "r");
    /* read master data file name */
    if (fgets(buf, sizeof(buf), master_chunk_file) == NULL){
        perror("read master chunk file error");
        exit(-1);
    }
    buf[strlen(buf) - 1] = '\0';
    /* read chunk id */
    fgets(buf2, sizeof(buf2), master_chunk_file);
    int chunk_id;
    fscanf(master_chunk_file, "%d %s\n", &chunk_id, buf2);
    str2hash(buf2);
    while (memcmp(hash, buf2, CHUNK_HASH_SIZE) != 0){
        fscanf(master_chunk_file, "%d %s\n", &chunk_id, buf2);
        str2hash(buf2);
    }

    fclose(master_chunk_file);

    /* read data */
    FILE* master_data_file = fopen(buf+6, "r");
    fseek(master_data_file, chunk_id * 512 * 1024, SEEK_SET);

    char* data = malloc(512 * 1024);
    int ret = fread(data, 1, 512*1024, master_data_file);
    if (ret < 512*1024){
        perror("read master data file error");
        exit(-1);
    }
    fclose(master_data_file);

    return data;
}

/* write the data of a chunk to file (identified by hash) */
void write_chunk_data_to_file(bt_config_t* config, char* data, int len,
                                int offset){
    FILE* file = fopen(config->output_file, "r+");
    fseek(file, offset, SEEK_SET);
    int ret = fwrite(data, 1, len, file);
    if (ret != len){
        perror("write to output file error");
        exit(-1);
    }
    fclose(file);
}

/* convert character hex to real hex */
void str2hash(char* string){
    int i = 0;
    while (i>>1 < CHUNK_HASH_SIZE) {
        int b;
        sscanf(&string[i], "%2x", &b);
        string[i/2] = b;
        i += 2;
    }
}

/* compare two timeval structs */
int my_timercmp(struct timeval *a, struct timeval *b) {
    if (a->tv_sec > b->tv_sec)
        return 1;
    else if (a->tv_sec < b->tv_sec)
        return -1;
    else  // a.tv_sec == b.tv_sec
    {
        if (a->tv_usec > b->tv_usec)
            return 1;
        else if (a->tv_usec < b->tv_usec)
            return -1;  
    }
    return 0;
}