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

#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "debug.h"
#include "spiffy.h"
#include "bt_parse.h"
#include "input_buffer.h"

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
  
  /* */
  peer_run(&config);
  return 0;
}


void process_inbound_udp(int sock) {
  #define BUFLEN MAX_PACKET_LEN
  struct sockaddr_in from;
  socklen_t fromlen;
  char buf[BUFLEN];

  fromlen = sizeof(from);
  int ret = spiffy_recvfrom(sock, buf, BUFLEN, 0, 
                            (struct sockaddr *) &from, &fromlen);
  
  DPRINTF(DEBUG_SOCKETS, "Incoming message from %s:%d\n%s\n\n", 
     inet_ntoa(from.sin_addr),
     ntohs(from.sin_port),
     buf);
  

  /* TODO: Demultiplex */
  int type = demultiplexing(ret, buf);
  switch (type){
    case 0:
        process_whohas_packet(ret, buf, config, &from);
        break;
    case 1:
        process_Ihave_packet(ret, buf, config, &from);
        break;
    case 2:
        break;
    case 3:
        break;
    case 4:
        break;
    case 5:
        break; 
  }

}

/* the function will flood the network with a WHOHAS packet */
void process_get(bt_config_t* config) {

    /* TODO: generate WHOHAS packet */
    int max_packet_len, last_packet_len, packets_size;
    char** whohas_packet = generate_whohas(config->get_chunks->size,
                            config->get_chunks->hash, CHUNK_HASH_SIZE,
                            &packets_size, &last_packet_len);
    max_packet_len = MAX_PACKET_LEN;

    bt_peer_t *peer = config->peers;
    while (peer) {
        for (int i = 0; i < packets_size; i++){
            int has_send = 0, ret;
            int packet_len = 
                    (i == packets_size-1) ? last_packet_len : max_packet_len;
            while (has_send < packet_len){
                ret = spiffy_sendto(config->sock, whohas_packet[i] + has_send, 
                            packet_len - has_send, 0, 
                            (struct sockaddr *)&peer->addr, sizeof(peer->addr));
                if (ret < 0) {
                    perror("send packet error");
                    exit(-1);
                } else
                    has_send += ret;
            }  
        }
        peer = peer->next;
    }
}

void handle_user_input(char *line, void *cbdata) {
  char chunkf[128], outf[128];
  bt_config_t *config = (bt_config_t*) cbdata;

  bzero(chunkf, sizeof(chunkf));
  bzero(outf, sizeof(outf));

  if (sscanf(line, "GET %120s %120s", chunkf, outf)) {
    if (strlen(outf) > 0) {
        read_get_chunk_file(config, chunkf);
        strcpy(config->output_file, outf);
        process_get(config);
    }
  }
}


void peer_run(bt_config_t *config) {
  int sock;
  struct sockaddr_in myaddr;
  fd_set readfds;
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
  
  while (1) {
    int nfds;
    FD_SET(STDIN_FILENO, &readfds);
    FD_SET(sock, &readfds);
    
    nfds = select(sock+1, &readfds, NULL, NULL, NULL);
    
    if (nfds > 0) {
        if (FD_ISSET(sock, &readfds)) {
            process_inbound_udp(sock);
        }
    
        /* remember to free config->get_chunks->chunks when finish downloading */
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            process_user_input(STDIN_FILENO, userbuf, handle_user_input,
               (void*)config);
      }
    }
  }
}
