/*
 * ihave.h
 *
 * Authors: Ke Wu <kewu@andrew.cmu.edu>
 *  
 * Date: 10/23/2015
 *
 * Description: header file of ihave.c
 *
 */
#ifndef _IHAVE_H_
#define _IHAVE_H_

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include "spiffy.h"
#include "helper.h"
#include "pkt_helper.h"
#include "bt_parse.h"


/* process a IHAVE packet that is received from network */
void process_Ihave_packet(int len, char* packet, bt_config_t* config,
                            struct sockaddr_in* from);

/* when need to send a ihave pakcet, invoke the function */
void send_Ihave_packet(int count, char** hash, bt_config_t *config, 
						struct sockaddr_in* from);
#endif