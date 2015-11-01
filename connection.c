#include <sys/timerfd.h>

void init_connection(struct Connection* con, struct bt_peer_t* peer, 
					 int is_download){
	con->is_download = is_download;

	con->peer = peer;

	con->timer_fd = timerfd_create(CLOCK_REALTIME, 0);

	/* whole data */
	con->whole_size = 1;
	con->packets[0] = NULL;

	/* window data */
	con->last_pkt = 0;
	con->cur_pkt = 0;
	con->window_size = FIXED_WINDOW_SIZE;

	con->successive_fail = 0;
}

int set_connection_timeout(struct Connection* con, int seconds, 
							int nanoseconds){
	struct itimerspec timer;
	/* set interval to zero, only timer once */
	timer.it_interval.tv_sec = 0;
	timer.it_interval.tv_nsec = 0;
	/* set timer value */
	timer.it_value.tv_sec = seconds;
	timer.it_value.tv_nsec = nano_seconds;
	
	return timerfd_settime(con->timer_fd, 0, &timer, NULL);
}

void destroy_connection(struct Connection* con){
	close(con->timer_fd);

	for (int i = 0; i < whole_size; i++)
		if (con->packets[i] != NULL){
			free(con->packets[i]);
			con->packets[i] = NULL;
		}
}

int is_crash(struct Connection* con){
	return (con->successive_fail >= CRASH_TIMES);
}


