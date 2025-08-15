#pragma once

#include <stdatomic.h>

struct NegotiationArgs {
  char* offer;
  int offer_length;
  int clientfd;
  int thread_id;
};

struct ClientContext {
  int incoming_datachannel_id;
  int outgoing_datachannel_id;
  int clientfd;
  int thread_id;
};

/**
 * 0 = all communication threads will stop immediately
 */
extern atomic_int worker_running;

/**
 * Negotiates the connection between client and server
 * @param *args Struct of type NegotiationArgs with all fields set.
 *    Will be freed by this function, do not use afterwards!
 */
int negotiate_and_start_peer_connection(void *args);
