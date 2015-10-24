#include "bt_parse.h"

int find_chunk(struct many_chunks *chunks, char* hash);

bt_peer_t* find_peer(bt_peer_t *peers, struct sockaddr_in* addr);