#include "bt_parse.h"

void process_whohas_packet(int len, char* packet, bt_config_t* config,
                            struct sockaddr_in* from);

void process_Ihave_packet(int len, char* packet, bt_config_t* config,
                            struct sockaddr_in* from);