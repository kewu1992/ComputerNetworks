/*
 * peer.c
 *
 * Authors: Ed Bardsley <ebardsle+441@andrew.cmu.edu>,
 *          Dave Andersen
 * Class: 15-441 (Spring 2005)
 *
 * Skeleton for 15-441 Project 2.
 *
 * ==================================================
 *
 * Modified by: Ke Wu <kewu@andrew.cmu.edu>
 *
 * Date: 10/23/2015
 *
 * Description: Skeleton for 15-441 Project 2, receive input from user or
 *              receive packets from peers, then process them.
 *
 */
#include "peer.h"

#define SELECT_TIMEOUT 5

int main(int argc, char **argv) {
  bt_config_t config;

  /* save program begin time */
  gettimeofday(&global_timer, NULL);

  /* initialize configuration */
  bt_init(&config, argc, argv);

  DPRINTF(DEBUG_INIT, "peer.c main beginning\n");

#ifdef TESTING
  config.identity = 1; // your group number here
  strcpy(config.chunk_file, "chunkfile");
  strcpy(config.has_chunk_file, "haschunks");
#endif

  bt_parse_command_line(&config);

#ifdef DEBUG
  if (debug & DEBUG_INIT) {
    bt_dump_config(&config);
  }
#endif

  read_has_chunk_file(&config);

  /* ignore SIGPIPE in case server is terminated due to broken pipe */
  signal(SIGPIPE, SIG_IGN);

  if (PACKET_LOSS_RATIO > 0)
    /* initialize random seed */
    srand (time(NULL));

  /* main routine */
  peer_run(&config);
  return 0;
}


void process_inbound_udp(int sock, bt_config_t *config) {
  #define BUFLEN MAX_PKT_LEN
  struct sockaddr_in from;
  socklen_t fromlen;
  char buf[BUFLEN];

  /* receive a UDP packet */
  fromlen = sizeof(from);
  int ret = spiffy_recvfrom(sock, buf, BUFLEN, 0,
                            (struct sockaddr *) &from, &fromlen);

  DPRINTF(DEBUG_SOCKETS, "Incoming message from %s:%d\n%s\n\n",
     inet_ntoa(from.sin_addr),
     ntohs(from.sin_port),
     buf);


  /* Demultiplex */
  int type = demultiplexing(ret, buf);
  switch (type){
    /* received a WHOHAS packet */
    case 0:
        process_whohas_packet(ret, buf, config, &from);
        break;
    /* received a IHAVE packet */
    case 1:
        process_Ihave_packet(ret, buf, config, &from);
        break;
    /* received a GET packet */
    case 2:
        process_getpkt(ret, buf, config, &from);
        break;
    /* received a DATA packet */
    case 3:
        process_data_packet(buf, config, &from);
        break;
    /* received an ACK packet */
    case 4:
        process_ack_packet(buf, config, &from);
        break;
    /* received a DENIED packet */
    case 5:
        process_deniedpkt(ret, buf, config, &from);
        break;
  }

}

void handle_user_input(char *line, void *cbdata) {
  char chunkf[128], outf[128];
  bt_config_t *config = (bt_config_t*) cbdata;

  bzero(chunkf, sizeof(chunkf));
  bzero(outf, sizeof(outf));

  if (sscanf(line, "GET %120s %120s", chunkf, outf)) {
    if (strlen(outf) > 0) {
        /* read get_chunk_file */
        read_get_chunk_file(config, chunkf);
        /* save output filename */
        strcpy(config->output_file, outf);
        /* create file */
        FILE* file = fopen(config->output_file, "w");
        fclose(file);
        /* write chunks that I already owned to output file */
        int count = 0;
        for (int i = 0; i < config->get_chunks.size; i++){
          int index = find_chunk(&config->has_chunks,
                                  config->get_chunks.chunks[i].hash);
          if (index != -1){
            count++;
            char* data = read_chunk_data_from_file(config,
                                          config->get_chunks.chunks[i].hash);
            write_chunk_data_to_file(config, data, 512*1024,
                                    config->get_chunks.chunks[i].id * 512*1024);
            config->written_chunks[i] = 1;
            free(data);
          }
        }
        /* check if all chunks can be accessed locally */
        if (count == config->get_chunks.size)
          config->is_check = 2;
        else
          /* flood WHOHAS packet */
          send_whohas_pkt(config);

    }
  }
}


void peer_run(bt_config_t *config) {
  int sock;
  struct sockaddr_in myaddr;
  fd_set readyset;
  struct user_iobuf *userbuf;

  if ((userbuf = create_userbuf()) == NULL) {
    perror("peer_run could not allocate userbuf");
    exit(-1);
  }

  if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP)) == -1) {
    perror("peer_run could not create socket");
    exit(-1);
  }

  config->sock = sock;

  bzero(&myaddr, sizeof(myaddr));
  myaddr.sin_family = AF_INET;
  myaddr.sin_addr.s_addr = config->myaddr.sin_addr.s_addr;
  //inet_aton("127.0.0.1", (struct in_addr *)&myaddr.sin_addr.s_addr);
  myaddr.sin_port = htons(config->myport);

  if (bind(sock, (struct sockaddr *) &myaddr, sizeof(myaddr)) == -1) {
    perror("peer_run could not bind socket");
    exit(-1);
  }

  spiffy_init(config->identity, (struct sockaddr *)&myaddr, sizeof(myaddr));

  struct timeval timeoutSelect = {SELECT_TIMEOUT, 0};

  /* init readset */
  FD_ZERO(&config->readset);
  FD_SET(STDIN_FILENO, &config->readset);
  FD_SET(sock, &config->readset);
  config->max_fd = sock;
  while (1) {
    int nfds;
    readyset = config->readset;
    timeoutSelect.tv_sec = SELECT_TIMEOUT;
    nfds = select(config->max_fd+1, &readyset, NULL, NULL, &timeoutSelect);

    if (nfds > 0) {
      /* a packet comes from Internet */
      if (FD_ISSET(sock, &readyset)) {
          //printf("A pkt comes from internet.\n");
          process_inbound_udp(sock, config);
          nfds--;
      }

      /* get input from stdin */
      if (FD_ISSET(STDIN_FILENO, &readyset)) {
          // CLR STDIN from select, remember to remove this line when finish debug!
          FD_CLR(STDIN_FILENO, &config->readset);
          process_user_input(STDIN_FILENO, userbuf, handle_user_input,
             (void*)config);
          nfds--;
      }

      /* some connections maybe timeout */
      bt_peer_t * peer = config->peers;
      while (nfds > 0 && peer) {
        if (peer->up_con && FD_ISSET(peer->up_con->timer_fd, &readyset)){
            printf("Upload timeout\n");
            process_upload_timeout(peer, config);
          nfds--;
        } else if (peer->down_con && FD_ISSET(peer->down_con->timer_fd, &readyset)) {
            printf("Download timeout\n");
            process_download_timeout(peer, config);
          nfds--;
        }
        peer = peer->next;
      }

      /* try to start a new chunk download (send a GET packet) */
      if (config->is_check == 1) {
        process_download(config);
        config->is_check = 0;
      }
      else if (config->is_check == 2){
        /* finish donwonloading of all chunks */
        clear_state(config);
        printf("GOT %s\n", config->output_file);
        // SET STDIN from select, remember to remove this line when finish debug!
        FD_SET(STDIN_FILENO, &config->readset);
        config->is_check = 0;
      }
    }
    // check if know all peers' has_chunk info
    if (config->get_chunks.chunks != NULL &&
        config->known_peer_num+1 < config->peer_num)
        send_whohas_pkt(config);
  }
}

/* finsh a GET command, clear state */
void clear_state(bt_config_t *config){
  /* clear state of peer */
  bt_peer_t * peer = config->peers;
  while (peer){
    if (peer->down_con){
      /* CLR select set*/
      FD_CLR(peer->down_con->timer_fd, &config->readset);
      destroy_connection(peer->down_con);
      peer->down_con = NULL;
    }
    if (peer->has_chunks.size != -1){
      free(peer->has_chunks.chunks);
      peer->has_chunks.size = -1;
      peer->has_chunks.chunks = NULL;
    }
    peer->is_crash = 0;

    peer = peer->next;
  }

  /* clear get_chunks */
  free(config->get_chunks.chunks);
  config->get_chunks.chunks = NULL;

  /* clear written_chunks */
  free(config->written_chunks);

  config->is_check = 0;
  config->cur_download_num = 0;
  config->known_peer_num = 0;

}
