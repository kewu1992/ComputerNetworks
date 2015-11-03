#ifndef _TIMEOUT_H_
#define _TIMEOUT_H_

#include <sys/select.h>
#include "get.h"
#include "connection.h"
#include "bt_parse.h"

void process_download_timeout(bt_peer_t *peer, bt_config_t * config);
void process_upload_timeout(bt_peer_t *peer, bt_config_t * config);

#endif