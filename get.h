#ifndef _GET_H_
#define _GET_H_

#include "bt_parse.h"
#include "Connection.h"
#include "pkt_helper.h"
#include "denied.h"

void send_getpkt(bt_peer_t * peer, bt_config_t * config);
void process_getpkt(int len, char * packet, bt_config_t * config, struct sockaddr_in * from);

int is_in_has_chunks(char * hash, struct many_chunks * has_chunks);
int is_curr_downloading(char * hash, bt_peer_t * peers);
void process_download(bt_config_t * config);
int has_reached_max_download(bt_config_t * config);
bt_peer_t * find_first_noncrash_peer_with_chunk(char * hash, bt_peer_t * peers);
bt_peer_t * find_first_crashed_peer_with_chunk(char * hash, bt_peer_t * peers);
#endif
