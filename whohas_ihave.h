/*
 * whohas_ihave.h
 *
 * Authors: Ke Wu <kewu@andrew.cmu.edu>
 *  
 * Date: 10/23/2015
 *
 * Description: header file of whohas_ihave.c
 *
 */
#ifndef _WHOHAS_IHAVE_H
#define _WHOHAS_IHAVE_H

#include "bt_parse.h"

/* process a WHOHAS packet that is received from network */
void process_whohas_packet(int len, char* packet, bt_config_t* config,
                            struct sockaddr_in* from);

/* process a IHAVE packet that is received from network */
void process_Ihave_packet(int len, char* packet, bt_config_t* config,
                            struct sockaddr_in* from);
#endif