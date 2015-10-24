/*
 * helper.h
 *
 * Authors: Ke Wu <kewu@andrew.cmu.edu>
 *  
 * Date: 10/23/2015
 *
 * Description: header file of helper.c
 *
 */
#include "bt_parse.h"

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