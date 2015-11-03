#ifndef _PEER_H_
#define _PEER_H_

#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "debug.h"
#include "spiffy.h"
#include "bt_parse.h"
#include "input_buffer.h"
#include "whohas_ihave.h"
#include "pkt_helper.h"
#include "helper.h"
#include "get.h"
#include "data.h"
#include "ack.h"
#include "denied.h"
#include "timeout.h"



void peer_run(bt_config_t *config);
void clear_state(bt_config_t *config);
void handle_user_input(char *line, void *cbdata);
void process_get(bt_config_t* config);
void process_inbound_udp(int sock, bt_config_t *config);

#endif






