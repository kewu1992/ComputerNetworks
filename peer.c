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

void peer_run(bt_config_t *config);

int main(int argc, char **argv) {
  bt_config_t config;

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

  /* main routine */
  peer_run(&config);
  return 0;
}


void process_inbound_udp(int sock, bt_config_t *config) {
  #define BUFLEN MAX_PKT_LEN
  struct sockaddr_in from;
  socklen_t fromlen;
  char buf[BUFLEN];

  /* receive UDP packet */
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
    case 2:
        process_getpkt(ret, buf, config, &from);
        break;
    case 3:
        process_data_packet(buf, config, &from);
        break;
    case 4:
        process_ack_packet(buf, config, &from);
        break;
    case 5:
        process_deniedpkt(ret, buf, config, &from);
        break;
  }

}

/* the function will flood the network with a WHOHAS packet */
void process_get(bt_config_t* config) {

    char** hashs = (char**) malloc(sizeof(char*) * config->get_chunks.size);
    for (int i = 0; i < config->get_chunks.size; i++){
        hashs[i] = (char*) malloc(CHUNK_HASH_SIZE);
        memcpy(hashs[i], config->get_chunks.chunks[i].hash, CHUNK_HASH_SIZE);
    }

    /* generate WHOHAS packets, maybe more than 1 packet due to max length
     * limit of a UDP packet */
    int max_packet_len, last_packet_len, packets_size;
    char** whohas_packet = generate_whohas(config->get_chunks.size,
                                            hashs, CHUNK_HASH_SIZE,
                                            &packets_size, &last_packet_len);
    /* free memory of malloc */
    for (int i = 0; i < config->get_chunks.size; i++)
        free(hashs[i]);
    free(hashs);

    max_packet_len = MAX_PKT_LEN;

    /* send WHOHAS packet to all peers */
    bt_peer_t *peer = config->peers;
    while (peer) {
        /* should not send WHOHAS to myself,
           should not send WHOHAS to the peer that known its has_chunk info */
        if (peer->id == config->identity || peer->has_chunks.size != -1){
          peer = peer->next;
          continue;
        }
        for (int i = 0; i < packets_size; i++){
            int packet_len =
                    (i == packets_size-1) ? last_packet_len : max_packet_len;

            send_packet(config->sock, whohas_packet[i], packet_len, 0,
                        (struct sockaddr *)&peer->addr, sizeof(peer->addr));
        }
        peer = peer->next;
    }

    /* free memory that is allocted from generate_whohas() */
    for (int i = 0; i < packets_size; i++)
        free(whohas_packet[i]);
    free(whohas_packet);
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
        /* write chunks that I already owned to output file */
        for (int i = 0; i < config->get_chunks.size; i++){
          int index = find_chunk(&conig->has_chunks,
                                  config->get_chunks.chunks[i].hash);
          if (index != -1){
            char* data = read_chunk_data_from_file(config,
                                          config->get_chunks.chunks[i].hash);
            write_chunk_data_to_file(config, data, 512*1024,
                                    config->get_chunks.chunks[i].id * 512*1024);
            config->written_chunks[i] = 1;
            free(data);
          }
        }
        /* flood WHOHAS packet */
        process_get(config);

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
  myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  myaddr.sin_port = htons(config->myport);

  if (bind(sock, (struct sockaddr *) &myaddr, sizeof(myaddr)) == -1) {
    perror("peer_run could not bind socket");
    exit(-1);
  }

  spiffy_init(config->identity, (struct sockaddr *)&myaddr, sizeof(myaddr));

  struct timeval timeoutSelect = {SELECT_TIMEOUT, 0};

  /* init readset */
  FD_ZERO(&config->readset)
  FD_SET(STDIN_FILENO, &config->readset);
  FD_SET(sock, &config->readset);
  config->max_fd = sock;
  while (1) {
    int nfds;
    readyset = config->readset;
    nfds = select(config->max_fd+1, &readyset, NULL, NULL, &timeoutSelect);

    if (nfds > 0) {
      /* a packet comes from Internet */
      if (FD_ISSET(sock, &readyset)) {
          process_inbound_udp(sock, config);
          nfds--;
      }

      /* get input from stdin */
      if (FD_ISSET(STDIN_FILENO, &readyset)) {
          process_user_input(STDIN_FILENO, userbuf, handle_user_input,
             (void*)config);
          nfds--;
      }

      /* some connection maybe timeout */
      bt_peer_t peer = config->peers;
      while (nfds > 0 && peer) {
        if (peer->up_con && FD_ISSET(peer->up_con->timer_fd, &readyset)){
          process_upload_timeout(peer, config)
          nfds--;
        } else if (peer->down_con && FD_ISSET(peer->down_con->timer_fd, &readyset)) {
          process_download_timeout(peer, config);
          nfds--;
        }
        peer = peer->next;
      }

      /* try to start a new chunk download (GET) */
      if (config->is_check == 1)
        process_download(config);
      else if (config->is_check == 2){
        /* finish donwonloading of all chunks */
        clear_state(config);
        printf("GOT %s\n", config->output_file);
      }
    } else {
      // SELECT TIMEOUT, check if know all peers' has_chunk info
      if (config->known_peer_num < config->peer_num)
          process_get(config);
    }
  }
}

void clear_state(bt_config_t *config){
  /* clear state of peer */
  bt_peer_t peer = config->peers;
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

  /* clear written_chunks */
  free(config->written_chunks);

  /* reset has_chunks ?? need? */
  //free(config->has_chunks.chunks);
  //read_has_chunk_file(config);

  config->is_check = 0;
  config->cur_download_num = 0;
  config->known_peer_num = 0;

}
