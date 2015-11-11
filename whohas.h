/*
 * whohas.h
 *
 * Authors: Ke Wu <kewu@andrew.cmu.edu>
 *
 * 15-441 Project 2
 *
 * Date: 11/10/2015
 *
 * Description: header file for whohas.c
 *
 */
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

/* when need to flood whohas pakcet, invoke the function */
void send_whohas_pkt(bt_config_t* config);

#endif