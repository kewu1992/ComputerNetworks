/*
 * data.h
 *
 * Authors: Ke Wu <kewu@andrew.cmu.edu>
 *
 * 15-441 Project 2
 *
 * Date: 11/10/2015
 *
 * Description: header file for data.c
 *
 */

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

/* when a data packet is received, invoke the function to process the pakcet */
void process_data_packet(char* packet, bt_config_t* config,
                         struct sockaddr_in* from);

/* when need to send a data pakcet, invoke the function */
void send_data_packet(int is_resend, bt_config_t* config, bt_peer_t* toPeer);

/* when finish download a chunk, check if all chunks has been downloaded 
 * (so that a file is finish downloading)
 */
int finish_chunk(bt_config_t* config, bt_peer_t* peer);

#endif
