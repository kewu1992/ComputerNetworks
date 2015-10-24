/*
 * pkt_generator.h
 *
 *
 */

#ifndef _PKT_GENERATOR_H_
#define _PKT_GENERATOR_H_

#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>

/**
 * Constants
 */

#define MAX_PKT_LEN 1500
#define PKT_HEADER_LEN 16
#define WHOHAS_PAYLOAD_HL 4



char ** generate_whohas(int size, char ** hash, int h_len, int * packets_size, int * last_p_len);
char * generate_one_wi_pkt(int pk_len, char ** hashes, int n_hashes, int h_len, char pkt_type);
char ** parse_whohas(int len, char * data, int h_len, int * size) ;
int get_n_hashes_in_pkt(int pk_len, int h_len);
char * generate_Ihave(int size, char ** hash, int h_len, int * len);
void create_wi_pkt_header(char * ptr, int pk_len, char pkt_type);
void create_wi_pkt_payload_header(char * ptr, int n_hashes);
void create_wi_pkt_hashes(char * ptr, int n_hashes, char ** hashes, int h_len);
char ** parse_Ihave(int len, char * data, int h_len, int * size);
char ** parse_wi_pkt(int len, char * data, int h_len, int * size);
int demultiplexing(int len, char * data);


#endif
