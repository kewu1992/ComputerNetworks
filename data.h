#ifndef _DATA_H_
#define _DATA_H_

#include <stdlib.h>
#include "bt_parse.h"
#include "connection.h"
#include "ack.h"
#include "sha.h"
#include "pkt_helper.h"
#include "helper.h"
#include "timeout.h"



void process_data_packet(char* packet, bt_config_t* config,
                         struct sockaddr_in* from);
void send_data_packet(int is_resend, bt_config_t* config, bt_peer_t* toPeer);

int finish_chunk(bt_config_t* config, bt_peer_t* peer);
#endif
