/*
 * pkt_helper.h
 *
 * Helper module for all the packet related work,
 * for example, packet generations and packet parsing
 *
 */

#ifndef _PKT_HELPER_H_
#define _PKT_HELPER_H_

#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include "bt_parse.h"

/**
 * Constants
 */

#define MAX_PKT_LEN 1500    // maximum packet length in bytes
#define PKT_HEADER_LEN 16   // packet header length in bytes
#define WI_PAYLOAD_HL 4     // payload header lengh in bytes for WHOHAS or IHave headers
#define MAGIC_NUMBER 15441  // magic number in header
#define VERSION 1           // version number in header

enum pkt_type { WHOHAS_PKT, IHAVE_PKT, GET_PKT, DATA_PKT, ACK_PKT, DENIED_PKT };

// generates an array of whohas packets
char ** generate_whohas(int size, char ** hash, int h_len, int * packets_size, int * last_p_len);

// generates one whohas / IHave packet
char * generate_one_wi_pkt(int pk_len, char ** hashes, int n_hashes, int h_len, char pkt_type);

// parses one whohas packet, returns the array of hashes in this packet
char ** parse_whohas(int len, char * data, int h_len, int * size) ;

// gets the number of chunk hashes in the given packet
int get_n_hashes_in_pkt(int pk_len, int h_len);

// generates one Ihave packet
char * generate_Ihave(int size, char ** hash, int h_len, int * len);

// creates whohas / Ihave packet header
void create_pkt_header(char * ptr, int pk_len, char pkt_type);

// creates whohas / Ihave packet's payload header
void create_wi_pkt_payload_header(char * ptr, int n_hashes);

// creates whohas / Ihave packet's chunk hashes
void create_pkt_hashes(char * ptr, int n_hashes, char ** hashes, int h_len);

// parses one Ihave packet
char ** parse_Ihave(int len, char * data, int h_len, int * size);

// parses one whohas / Ihave packet, returns the array of hashes in this packet
char ** parse_wi_pkt(int len, char * data, int h_len, int * size);

// returns the type of this packet, e.g. is it whohas or Ihave or others
int demultiplexing(int len, char * data);

char * generate_get(char * hash);
char * parse_get(char * pkt);
char * generate_data();

#endif
