#include <sys/timerfd.h>
#include "bt_parse.h"

#define FIXED_WINDOW_SIZE 8
#define CRASH_TIMES 5

struct Connection{
	/* 1 for download, I am receiver, the remote peer is sender
	 * 0 for upload, I am sender, the rmote peer is receiver */
	int is_download;	

	struct bt_peer_t* peer;

	int timer_fd;
	
	int whole_size;
	char* packets[1];

	int last_pkt, cur_pkt;
	int window_size;
	
	int successive_fail;
};

void init_connection(struct Connection* con, struct bt_peer_t* peer);

int set_connection_timeout(struct Connection* con, int seconds, 
							int nanoseconds);

void destroy_connection(struct Connection* con);

int is_crash(struct Connection* con);