/*
 * denied.h
 *
 * Authors: Junqiang Li <junqianl@andrew.cmu.edu>
 *
 * 15-441 Project 2
 *
 * Date: 11/10/2015
 *
 * Description: header file for denied.c
 *
 */
#ifndef _DENIED_H_
#define _DENIED_H_

#include "bt_parse.h"
#include "helper.h"
#include "pkt_helper.h"
#include "connection.h"

/* when a denied packet is received, invoke the function to process the pakcet */
void send_deniedpkt(bt_peer_t * peer, bt_config_t * config);

/* when need to send a denied pakcet, invoke the function */
void process_deniedpkt(int len, char * packet, bt_config_t * config, 
						struct sockaddr_in * from);

#endif
