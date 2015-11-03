#ifndef _DENIED_H_
#define _DENIED_H_

#include "bt_parse.h"
#include "helper.h"
#include "pkt_helper.h"
#include "connection.h"



void send_deniedpkt(bt_peer_t * peer, bt_config_t * config);
void process_deniedpkt(int len, char * packet, bt_config_t * config, struct sockaddr_in * from);

#endif
