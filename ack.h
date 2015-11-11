/*
 * ack.h
 *
 * Authors: Ke Wu <kewu@andrew.cmu.edu>
 *
 * 15-441 Project 2
 *
 * Date: 11/10/2015
 *
 * Description: header file for ack.c
 *
 *
 */
#ifndef _ACK_H_
#define _ACK_H_
#include "pkt_helper.h"
#include "bt_parse.h"
#include "connection.h"
#include "data.h"
#include "helper.h"
#include "timeout.h"

/* when an ack packet is received, invoke the function to process the pakcet */
void process_ack_packet(char* packet, bt_config_t* config,
                         struct sockaddr_in* from);

/* when need to send an ack pakcet, invoke the function */
void send_ack_packet(int ack_num, bt_config_t* config, bt_peer_t* toPeer);

#endif
