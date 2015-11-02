/**
 * pkt_helper.c
 *
 *
 */

#include "pkt_helper.h"

char ** generate_whohas(int size, char ** hash, int h_len, int * packets_size, int * last_p_len) {
    // size --> hash array size, i.e. number of chunk hashes
    // hash --> array of chunk hashes
    // h_len --> byte length of 1 chunk hash, i.e. 20
    // packets_size --> number of whohas packets generated
    // last_p_len --> last packet length after packet cutting
    int size_hashes = size * h_len;
    int total_hash_size_in_max_pkt = MAX_PKT_LEN - PKT_HEADER_LEN - WI_PAYLOAD_HL;

    int quotient = size_hashes / total_hash_size_in_max_pkt;
    int remainder = size_hashes % total_hash_size_in_max_pkt;
    int n_packets = 0;
    int last_packet_len = 0;
    if (remainder == 0) {
        // no leftover bytes
        n_packets = quotient;
        last_packet_len = MAX_PKT_LEN;
    } else {
        n_packets = quotient + 1;
        last_packet_len = remainder + WI_PAYLOAD_HL + PKT_HEADER_LEN;
    }

    char ** packets_arr = (char **) malloc(n_packets * sizeof(char *));
    for (int i = 0; i < n_packets; i++) {
        int pk_len = 0;
        if (i == n_packets - 1) {
            pk_len = last_packet_len;
        } else {
            pk_len = MAX_PKT_LEN;
        }
        // number of hashes in this packet
        int n_hashes = get_n_hashes_in_pkt(pk_len, h_len);
        char * one_whohas = generate_one_wi_pkt(pk_len, hash, n_hashes, h_len, WHOHAS_PKT);
        packets_arr[i] = one_whohas;
        hash += n_hashes;
    }
    *packets_size = n_packets;
    *last_p_len = last_packet_len;
    return packets_arr;
}

int get_n_hashes_in_pkt(int pk_len, int h_len) {
    return (pk_len - PKT_HEADER_LEN - WI_PAYLOAD_HL) / h_len;
}


void create_pkt_header(char * ptr, int pk_len, char pkt_type) {
    // create packet header
    short magic_num = htons(MAGIC_NUMBER);
    char version = VERSION;
    unsigned short header_len = htons(PKT_HEADER_LEN);
    unsigned short total_packet_len = htons((unsigned short) pk_len);
    memcpy(ptr, &magic_num, 2);
    ptr += 2;
    memcpy(ptr, &version, 1);
    ptr++;
    memcpy(ptr, &pkt_type, 1);
    ptr++;
    memcpy(ptr, &header_len, 2);
    ptr += 2;
    memcpy(ptr, &total_packet_len, 2);
    // end create packet header
}

void create_wi_pkt_payload_header(char * ptr, int n_hashes) {
    // create packet payload header
    unsigned char n_hashes_char = (unsigned char) n_hashes;
    memcpy(ptr, &n_hashes_char, 1);
    // end create packet payload header
}

void create_pkt_hashes(char * ptr, int n_hashes, char ** hashes, int h_len) {
    // put in hashes one by one
    for (int i = 0; i < n_hashes; i++) {
        char * hash = hashes[i];
        memcpy(ptr, hash, h_len);
        ptr += h_len;
    }
    // end put in hashes
}



char * generate_one_wi_pkt(int pk_len, char ** hashes, int n_hashes, int h_len, char pkt_type) {
    char * one_wi = (char *) malloc(pk_len);
    char * ptr = one_wi;

    create_pkt_header(ptr, pk_len, pkt_type);
    ptr += PKT_HEADER_LEN;

    create_wi_pkt_payload_header(ptr, n_hashes);
    ptr += WI_PAYLOAD_HL;

    create_pkt_hashes(ptr, n_hashes, hashes, h_len);
    return one_wi;
}


char ** parse_whohas(int len, char * data, int h_len, int * size) {
    return parse_wi_pkt(len, data, h_len, size);
}

char ** parse_wi_pkt(int len, char * data, int h_len, int * size) {
    // len --> total # of bytes in 1 whohas / IHave packet "data"
    // data --> one whohas / IHave packet
    // h_len --> one hash size, 20
    // size --> # of hashes in this packet

    // parse # of hashes in the packet
    char * ptr = data;
    ptr += PKT_HEADER_LEN;
    unsigned char n_hashes = 0;
    memcpy(&n_hashes, ptr, 1);
    ptr += WI_PAYLOAD_HL;
    // end parsing # of hashes

    // parse chunk hash one by one
    char ** hashes_arr = (char **) malloc(n_hashes * sizeof(char *));
    for (int i = 0; i < n_hashes; i++) {
        char * hash = (char *) malloc(h_len);
        memcpy(hash, ptr, h_len);
        hashes_arr[i] = hash;
        ptr += h_len;
    }
    // end parsing chunk hash

    *size = n_hashes;
    return hashes_arr;
}


char * generate_Ihave(int size, char ** hash, int h_len, int * len) {
    // size --> # of hashes
    // hash --> array of hashes
    // h_len --> one hash size, 20
    // len --> total # of bytes in 1 Ihave packet
    int pk_len = size * h_len + WI_PAYLOAD_HL + PKT_HEADER_LEN;
    *len = pk_len;
    return generate_one_wi_pkt(pk_len, hash, size, h_len, IHAVE_PKT);
}

char ** parse_Ihave(int len, char * data, int h_len, int * size) {
    return parse_wi_pkt(len, data, h_len, size);
}

int demultiplexing(int len, char * data) {
    char * ptr = data;
    ptr += 3;
    return *ptr;
}

char * generate_get(char * hash, int * len) {
    int pk_len = PKT_HEADER_LEN + CHUNK_HASH_SIZE;
    char * one_get = (char *) malloc(pk_len);
    char * ptr = one_get;

    create_pkt_header(ptr, pk_len, GET_PKT);
    ptr += PKT_HEADER_LEN;

    char * hashes[] = { hash };

    create_pkt_hashes(ptr, 1, hashes, CHUNK_HASH_SIZE);

    *len = pk_len;
    return one_get;
}

char * parse_get(char * pkt) {
    char * ptr = pkt;
    ptr += PKT_HEADER_LEN;

    // parse chunk hash
    char * hash = (char *) malloc(CHUNK_HASH_SIZE);
    memcpy(hash, ptr, CHUNK_HASH_SIZE);
    // end parsing chunk hash

    return hash;
}

char ** generate_data(char * data, int seq_num, int * packets_size, int * last_p_len) {
    // data --> actual chunk data from disk
    // seq_num --> sequence number
    // packets_size --> number of whohas packets generated
    int total_chunk_data_size_in_max_pkt = MAX_PKT_LEN - PKT_HEADER_LEN;

    int quotient = CHUNK_DATA_SIZE / total_chunk_data_size_in_max_pkt;
    int remainder = CHUNK_DATA_SIZE % total_chunk_data_size_in_max_pkt;
    int n_packets = 0;
    int last_packet_len = 0;
    if (remainder == 0) {
        // no leftover bytes
        n_packets = quotient;
        last_packet_len = MAX_PKT_LEN;
    } else {
        n_packets = quotient + 1;
        last_packet_len = remainder + PKT_HEADER_LEN;
    }

    char ** packets_arr = (char **) malloc(n_packets * sizeof(char *));
    for (int i = 0; i < n_packets; i++) {
        int pk_len = 0;
        if (i == n_packets - 1) {
            pk_len = last_packet_len;
        } else {
            pk_len = MAX_PKT_LEN;
        }

        // begin generate one data pkt
        char * one_data = (char *) malloc(pk_len);
        char * ptr = one_data;

        // begin create data pkt header
        short magic_num = htons(MAGIC_NUMBER);
        char version = VERSION;
        char pkt_type = DATA_PKT;
        unsigned short header_len = htons(PKT_HEADER_LEN);
        unsigned short total_packet_len = htons((unsigned short) pk_len);
        uint32_t seq_num_nw = htonl((uint32_t) seq_num++);
        memcpy(ptr, &magic_num, 2);
        ptr += 2;
        memcpy(ptr, &version, 1);
        ptr++;
        memcpy(ptr, &pkt_type, 1);
        ptr++;
        memcpy(ptr, &header_len, 2);
        ptr += 2;
        memcpy(ptr, &total_packet_len, 2);
        ptr += 2;
        memcpy(ptr, &seq_num_nw, 4);
        ptr += 8;
        // end create data pkt header

        // begin create data pkt chunk data
        int curr_pkt_chunk_data_size = pk_len - PKT_HEADER_LEN;
        memcpy(ptr, data, curr_pkt_chunk_data_size);
        data += curr_pkt_chunk_data_size;
        // end create data pkt chunk data

        packets_arr[i] = one_data;
        // end generate one data pkt
    }

    *packets_size = n_packets;
    *last_p_len = last_packet_len;
    return packets_arr;
}

char * parse_data(char * pkt, int * seq_num, int * len) {
    // pkt --> one data pkt
    // seq_num --> sequence number, 4 bytes
    // len --> chunk data size in pkt
    char * ptr = pkt;
    ptr += 6;

    // begin get packet length
    uint16_t pkt_len = 0;
    memcpy(&pkt_len, ptr, 2);
    pkt_len = ntohs(pkt_len);
    // end get packet length

    ptr += 2;

    // begin get sequence number
    uint32_t seq_num_host = 0;
    memcpy(&seq_num_host, ptr, 4);
    seq_num_host = ntohl(seq_num_host);
    // end get sequence number

    ptr += 8;

    // begin get chunk data
    int chunk_data_size = pkt_len - PKT_HEADER_LEN;
    char * chunk_data = (char *) malloc(chunk_data_size);
    memcpy(chunk_data, ptr, chunk_data_size);
    // end get chunk data

    *seq_num = seq_num_host;
    *len = chunk_data_size;
    return chunk_data;
}

char * generate_ack(int ack_num, int * len) {
    // ack_num --> ack number for this packet
    // len --> packet length in bytes
    int pk_len = PKT_HEADER_LEN;
    char * one_ack = (char *) malloc(pk_len);
    char * ptr = one_ack;

    // begin create ack pkt header
    short magic_num = htons(MAGIC_NUMBER);
    char version = VERSION;
    char pkt_type = ACK_PKT;
    unsigned short header_len = htons(PKT_HEADER_LEN);
    unsigned short total_packet_len = htons((unsigned short) pk_len);
    uint32_t ack_num_nw = htonl((uint32_t) ack_num);
    memcpy(ptr, &magic_num, 2);
    ptr += 2;
    memcpy(ptr, &version, 1);
    ptr++;
    memcpy(ptr, &pkt_type, 1);
    ptr++;
    memcpy(ptr, &header_len, 2);
    ptr += 2;
    memcpy(ptr, &total_packet_len, 2);
    ptr += 6;
    memcpy(ptr, &ack_num_nw, 4);
    // end create ack pkt header

    *len = pk_len;
    return one_ack;
}

int parse_ack(char * pkt) {
    char * ptr = pkt;
    ptr += 12;
    uint32_t ack_num = 0;
    memcpy(&ack_num, ptr, 4);
    ack_num = ntohl(ack_num);

    return ack_num;
}

char * generate_denied(int * len) {
    // len --> packet length in bytes
    int pk_len = PKT_HEADER_LEN;
    char * one_deny = (char *) malloc(pk_len);
    char * ptr = one_deny;

    // begin create ack pkt header
    short magic_num = htons(MAGIC_NUMBER);
    char version = VERSION;
    char pkt_type = DENIED_PKT;
    unsigned short header_len = htons(PKT_HEADER_LEN);
    unsigned short total_packet_len = htons((unsigned short) pk_len);
    memcpy(ptr, &magic_num, 2);
    ptr += 2;
    memcpy(ptr, &version, 1);
    ptr++;
    memcpy(ptr, &pkt_type, 1);
    ptr++;
    memcpy(ptr, &header_len, 2);
    ptr += 2;
    memcpy(ptr, &total_packet_len, 2);
    // end create ack pkt header

    *len = pk_len;
    return one_deny;
}
