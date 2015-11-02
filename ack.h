#include "pkt_helper.h"
#include "connection.h"

void process_ack_packet(char* packet, bt_config_t* config,
                         struct sockaddr_in* from);
void send_ack_packet(int ack_num, bt_config_t* config, bt_peer_t* toPeer);