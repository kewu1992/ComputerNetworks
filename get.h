/*
 * get.h
 *
 * Authors: Junqiang Li <junqianl@andrew.cmu.edu>
 *
 * 15-441 Project 2
 *
 * Date: 11/10/2015
 *
 * Description: header file for get.c
 *
 */
#ifndef _GET_H_
#define _GET_H_

#include "bt_parse.h"
#include "connection.h"
#include "pkt_helper.h"
#include "denied.h"
#include "data.h"
#include "timeout.h"

/* when need to send a get pakcet, invoke the function */
void send_getpkt(bt_peer_t * peer, bt_config_t * config);

/* when a get packet is received, invoke the function to process the pakcet */
void process_getpkt(int len, char * packet, bt_config_t * config, 
					struct sockaddr_in * from);

/* try to find a downloadable chunk, and send it get packet */
void process_download(bt_config_t * config);

/* helper function, detect if a chunk (hash value) is in an array of chunks */
int is_in_has_chunks(char * hash, struct many_chunks * has_chunks);

/* helper function, detect if a chunk (hash value) is in currently downloading
 * by other connections */
int is_curr_downloading(char * hash, bt_peer_t * peers);

/* helper function, detect if the peer has reached max download times */
int has_reached_max_download(bt_config_t * config);

/* helper function, find a peer that has the chunk (identified by hash) */
bt_peer_t * find_first_noncrash_peer_with_chunk(char * hash, bt_peer_t * peers);

/* helper function, find a crash peer that has the chunk (identified by hash) */
bt_peer_t * find_first_crashed_peer_with_chunk(char * hash, bt_peer_t * peers);

#endif
