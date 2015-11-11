/*
 * helper.h
 *
 * Authors: Ke Wu <kewu@andrew.cmu.edu>
 *  
 * Date: 11/10/2015
 *
 * Description: header file of helper.c
 *
 */
#ifndef _HELP_H_
#define _HELP_H_

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include "bt_parse.h"
#include "spiffy.h"

#define PACKET_LOSS_RATIO 0

/* description: find a chunk in an array of chunks (compare hash value) 
 * return: the index of the chunk in the array, 
 *         -1 if not found
 */
int find_chunk(struct many_chunks *chunks, char* hash);

/* description: find a network address in a linkedlist of peers 
 *               (compare IP address and port)
 *  return: the pointer of the peer that have the netowrk address
 *          NULL if not found
 */
bt_peer_t* find_peer(bt_peer_t *peers, struct sockaddr_in* addr);

/* description: send a packet using sendto */
void send_packet(int socket, char* data, size_t packet_len, int flag, 
                 struct sockaddr *dest_addr, socklen_t addr_len);

/* read the data of a chunk from file (identified by hash) */
char* read_chunk_data_from_file(bt_config_t* config, char* hash);

/* write the data of a chunk to file (identified by hash) */
void write_chunk_data_to_file(bt_config_t* config, char* data, int len, 
                                int offset);

/* convert character hex to real hex */
void str2hash(char* string);

/* compare two timeval structs */
int my_timercmp(struct timeval *a, struct timeval *b);
#endif