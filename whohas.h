#ifndef _WHOHAS_H_
#define _WHOHAS_H_

#include <stdio.h>
#include <stdlib.h>
#include "pkt_helper.h"
#include "bt_parse.h"
#include "ihave.h"
#include "helper.h"

/* process a WHOHAS packet that is received from network */
void process_whohas_packet(int len, char* packet, bt_config_t* config,
                            struct sockaddr_in* from);

void send_whohas_pkt(bt_config_t* config);

#endif