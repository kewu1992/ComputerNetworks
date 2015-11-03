#ifndef _GET_H_
#define _GET_H_

#include "bt_parse.h"
#include "Connection.h"
#include "pkt_helper.h"

void send_getpkt(bt_peer_t * peer, bt_config_t * config);
void process_getpkt(int len, char * packet, bt_config_t * config, struct sockaddr_in * from);


#endif
