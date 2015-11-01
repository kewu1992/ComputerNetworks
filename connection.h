#include "bt_parse.h"

#define FIXED_WINDOW_SIZE 8

struct Connection{
	 bt_peer_t* peer;

	int time_fd;
	int window[FIXED_WINDOW_SIZE];
	int last_seq, cur_seq;
	int successive_fail;
};