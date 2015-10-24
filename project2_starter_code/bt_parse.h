/*
 * bt_parse.h
 *
 * Initial Author: Ed Bardsley <ebardsle+441@andrew.cmu.edu>
 * Class: 15-441 (Spring 2005)
 *
 * Skeleton for 15-441 Project 2 command line and config file parsing
 * stubs.
 *
 * Modified by: Ke Wu <kewu@andrew.cmu.edu>
 *  
 * Date: 10/23/2015
 *
 * Description: add two functions to read get_chunk_file and has_chunk_file
 *              add more fields in bt_config_s
 *
 */

#ifndef _BT_PARSE_H_
#define _BT_PARSE_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define BT_FILENAME_LEN 255
#define BT_MAX_PEERS 1024
#define CHUNK_HASH_SIZE 20

/* added by Ke Wu, a single chunk record */
struct single_chunk{
  int id;
  char hash[CHUNK_HASH_SIZE];
};

/* added by Ke Wu, many chunk records (can be has_chunk or get_chunk)*/
struct many_chunks{
  int size;
  struct single_chunk* chunks;
};

typedef struct bt_peer_s {
  short  id;
  struct sockaddr_in addr;
  struct bt_peer_s *next;

  /* added by Ke Wu, represents which chunks the peer owns */
  struct many_chunks has_chunks;
} bt_peer_t;

struct bt_config_s {
  char  chunk_file[BT_FILENAME_LEN];
  char  has_chunk_file[BT_FILENAME_LEN];
  char  output_file[BT_FILENAME_LEN];
  char  peer_list_file[BT_FILENAME_LEN];
  int   max_conn;
  short identity;
  unsigned short myport;

  int argc; 
  char **argv;

  bt_peer_t *peers;

  /* added by Ke Wu, represents which chunks I want to get (download) */
  struct many_chunks get_chunks;
  /* added by Ke Wu, represents which chunks I onw */
  struct many_chunks has_chunks;
  /* added by Ke Wu, represents my socket file descriptor */
  int sock;
};
typedef struct bt_config_s bt_config_t;


void bt_init(bt_config_t *c, int argc, char **argv);
void bt_parse_command_line(bt_config_t *c);
void bt_parse_peer_list(bt_config_t *c);
void bt_dump_config(bt_config_t *c);
bt_peer_t *bt_peer_info(const bt_config_t *c, int peer_id);

/* added by Ke Wu, read a has_chunk_file */
void read_has_chunk_file(bt_config_t *c);
/* added by Ke Wu, read a get chunk file */
void read_get_chunk_file(bt_config_t *c, char* file);

#endif /* _BT_PARSE_H_ */
