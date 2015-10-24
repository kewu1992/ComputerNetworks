/*
 * helper.c
 *
 * Authors: Ke Wu <kewu@andrew.cmu.edu>
 *  
 * Date: 10/23/2015
 *
 * Description: some helper functions 
 *
 */
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
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