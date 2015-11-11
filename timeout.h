/*
 * timeout.h
 *
 * Authors: Ke Wu <kewu@andrew.cmu.edu>
 *
 * 15-441 Project 2
 *
 * Date: 11/10/2015
 *
 * Description: header file for timeout.c
 *
 */
#ifndef _TIMEOUT_H_
#define _TIMEOUT_H_

#include <sys/select.h>
#include "get.h"
#include "connection.h"
#include "bt_parse.h"

/* default timeout seconds */
#define CONNECTION_TIMEOUT 3

/* receiver find that sender timeout (GET or ACK timeout, no DATA)*/
void process_download_timeout(bt_peer_t *peer, bt_config_t * config);

/* sender find that receiver timeout (DATA timeout, no ACK) */
void process_upload_timeout(bt_peer_t *peer, bt_config_t * config);

#endif
