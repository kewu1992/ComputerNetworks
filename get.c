/*
 * get.c
 *
 * Authors: Junqiang Li <junqianl@andrew.cmu.edu>
 *
 * Description: used for processing GET packet and timeout
 *
 */

#include <sys/select.h>
#include "get.h"

void send_getpkt(bt_peer_t * peer, bt_config_t * config) {
    struct Connection * down_con = peer->down_con;
    /* generate and send get packet */
    int len;
    char * get_pkt = generate_get(down_con->prev_get_hash, &len);
    send_packet(config->sock, get_pkt, len, 0, (struct sockaddr *)&peer->addr, sizeof(peer->addr));
    set_connection_timeout(down_con, 3600, 0);
    free(get_pkt);
}


void process_getpkt(int len, char * packet, bt_config_t * config, struct sockaddr_in * from) {
    /* 1. find peer that send the get packet */
    bt_peer_t * peer = find_peer(config->peers, from);

    if (config->cur_upload_num == config->max_conn) {
        // reached max upload limit, will deny this peer's GET request
        send_deniedpkt(peer, config);
        return;
    }

    config->cur_upload_num++;

    /* 2. parse get packet */
    char * hash = parse_get(packet);
    /* 3. if connecting, destroy connection */
    if (peer->up_con != NULL) {
        destroy_connection(peer->up_con);
        peer->up_con = NULL;
    }
    /* 4. read chunk data from file */
    char * data = read_chunk_data_from_file(config, hash);
    free(hash);
    /* 5. generate data packets */
    int packets_size;
    int last_p_len;
    char ** data_packets = generate_data(data, 0, &packets_size, &last_p_len);
    free(data);
    /* 6 .init connection */
    peer->up_con = init_connection(peer, 0, data_packets, packets_size,
                                    MAX_PKT_LEN, last_p_len);
    FD_SET(peer->up_con->timer_fd, &config->readset);
    config->max_fd = (peer->up_con->timer_fd > config->max_fd) ? peer->up_con->timer_fd : config->max_fd;
    /* do not free data_packets, it should be free when connection is destroied */

    /* 7. send data packets */
    send_data_packet(0, config, peer);
}


void process_download(bt_config_t * config) {
    if (!config->is_check) {
        // if is_check is 0, return, keep waiting
        return;
    }

    if (has_reached_max_download(config)) {
        // if has reached max download, return and keep waiting
        return;
    }

    struct many_chunks get_chunks = config->get_chunks;
    struct many_chunks has_chunks = config->has_chunks;

    int dlable_flags[get_chunks.size];

    // init flags to be all 1s, i.e. all downloable
    int i;
    for (i = 0; i < get_chunks.size; i++) {
        dlable_flags[i] = 1;
    }

    // clear out the ones already in has_chunks or currently downloading
    // count the number of chunks downloadable
    int flag_count = get_chunks.size;
    for (i = 0; i < get_chunks.size; i++) {
        if (is_in_has_chunks(get_chunks.chunks[i].hash, &has_chunks)
                || is_curr_downloading(get_chunks.chunks[i].hash, config->peers)) {
            dlable_flags[i] = 0;
            flag_count--;
        }
    }

    // try to download from non-crashed peers first
    for (i = 0; i < get_chunks.size; i++) {
        if (has_reached_max_download(config)) {
            break;
        }

        if (!dlable_flags[i]) {
            continue;
        }

        // below is downloable get chunk
        bt_peer_t * peer = find_first_noncrash_peer_with_chunk(get_chunks.chunks[i].hash, config->peers);
        if (peer != NULL) {
            peer->down_con = init_connection(peer, 1, NULL, 0, MAX_PKT_LEN, 0);
            FD_SET(peer->down_con->timer_fd, &config->readset);
            config->max_fd = (peer->down_con->timer_fd > config->max_fd) ? peer->down_con->timer_fd : config->max_fd;
            char * hash = (char *) malloc(CHUNK_HASH_SIZE);
            hash = memcpy(hash, get_chunks.chunks[i].hash, CHUNK_HASH_SIZE);
            peer->down_con->prev_get_hash = hash;
            send_getpkt(peer, config);
            config->cur_download_num++;
            dlable_flags[i] = 0;
            flag_count--;
        }
    }


    // still can download and still have downloable chunks
    // need to check crashed peer
    if (!has_reached_max_download(config) && flag_count > 0) {
        for (i = 0; i < get_chunks.size && dlable_flags[i]; i++) {
            if (has_reached_max_download(config)) {
                break;
            }
            bt_peer_t * peer = find_first_crashed_peer_with_chunk(
                    get_chunks.chunks[i].hash, config->peers);
            if (peer != NULL) {
                peer->down_con = init_connection(peer, 1, NULL, 0, MAX_PKT_LEN, 0);
                FD_SET(peer->down_con->timer_fd, &config->readset);
                config->max_fd = (peer->down_con->timer_fd > config->max_fd) ? peer->down_con->timer_fd : config->max_fd;
                char * hash = (char *) malloc(CHUNK_HASH_SIZE);
                hash = memcpy(hash, get_chunks.chunks[i].hash, CHUNK_HASH_SIZE);
                peer->down_con->prev_get_hash = hash;
                send_getpkt(peer, config);
                peer->is_crash = 0;
                config->cur_download_num++;
                dlable_flags[i] = 0;
                flag_count--;
            }
        }
    }
}

int has_reached_max_download(bt_config_t * config) {
    return config->cur_download_num == config->max_conn;
}

int is_in_has_chunks(char * hash, struct many_chunks * has_chunks) {
    struct single_chunk * chunks = has_chunks->chunks;
    for (int i = 0; i < has_chunks->size; i++) {
        struct single_chunk sc = chunks[i];
        if (memcmp(sc.hash, hash, CHUNK_HASH_SIZE) == 0) {
            return 1;
        }
    }
    return 0;
}

int is_curr_downloading(char * hash, bt_peer_t * peers) {
    for (bt_peer_t * p = peers;
            p != NULL && p->down_con != NULL; p = peers->next) {
        struct Connection * down_con = p->down_con;
        if (memcmp(down_con->prev_get_hash, hash, CHUNK_HASH_SIZE) == 0) {
            return 1;
        }
    }
    return 0;
}

bt_peer_t * find_first_noncrash_peer_with_chunk(char * hash, bt_peer_t * peers) {
    for (bt_peer_t * p = peers;
            p != NULL && !(p->is_crash); p = peers->next) {
        struct many_chunks has_chunks = p->has_chunks;
        int result = is_in_has_chunks(hash, &has_chunks);
        if (result == 1) {
            return p;
        }
    }
    return NULL;
}

bt_peer_t * find_first_crashed_peer_with_chunk(char * hash, bt_peer_t * peers) {
    for (bt_peer_t * p = peers;
            p != NULL && p->is_crash; p = peers->next) {
        struct many_chunks has_chunks = p->has_chunks;
        int result = is_in_has_chunks(hash, &has_chunks);
        if (result == 1) {
            return p;
        }
    }
    return NULL;
}
