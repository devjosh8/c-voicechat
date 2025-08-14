#include <asm-generic/errno-base.h>
#include <asm-generic/errno.h>
#include <asm-generic/socket.h>
#include <bits/types/struct_timeval.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>

#include <tinycthread/tinycthread.h>
#include <rtc/rtc.h>
#include <worker.h>

#define PORT 4008
#define CLIENT_PORT 4001
#define MAXMSG 512


atomic_int thread_id_counter = 1;

void handle_sigint(int sig) {
  (void) sig;
  atomic_store(&worker_running, 0); 
}



int start_handshake_socket(void *arg) {
  struct sockaddr_in serv_addr, cli_addr;
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
                                               
  if(sockfd < 0) {
    printf("Handshake socket could not be initialized.\n");
    exit(EXIT_FAILURE);
  }

  memset(&serv_addr, 0, sizeof(serv_addr));

  struct timeval timeout;

  timeout.tv_sec = 1;
  timeout.tv_usec = 0;
  if(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,
        (char*)&timeout, sizeof(timeout)) < 0) {
    printf("Socket timeout option could not be set.\n");
  }

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(PORT);
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  
  if(bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
    printf("Could not bind socket. \n");
    exit(EXIT_FAILURE);
  }

  listen(sockfd, 5);
  printf("Waiting for connection...\n");
  unsigned int client_length = sizeof(cli_addr);
  while(atomic_load(&worker_running)) {
    int newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &client_length);
    if (newsockfd < 0) {
      if (errno == EWOULDBLOCK || errno == EAGAIN) {
        continue;
      }
      printf("Error on accepting client...\n");
      continue;
    }


    char offer_buf[2048];
    memset(offer_buf, 0, sizeof(offer_buf));

    int read_offer_len = read(newsockfd, offer_buf, 2048-1);

    if(read_offer_len < 0) {
      printf("Error reading from socket.\n");
      close(newsockfd);
      continue;
    } else {
      struct NegotiationArgs* negotiation_args = malloc(sizeof(struct NegotiationArgs));
      negotiation_args->offer = offer_buf;
      negotiation_args->offer_length = read_offer_len;
      negotiation_args->clientfd = newsockfd;
      negotiation_args->thread_id = atomic_fetch_add(&thread_id_counter, 1);

      thrd_t thread;
      thrd_create(&thread, negotiate_and_start_peer_connection, (void *)negotiation_args);
      thrd_detach(thread);
    }

  }
  
  return 1;
}

int main() {
  signal(SIGINT, handle_sigint);
  printf("Starting Server...\n");
  rtcInitLogger(RTC_LOG_DEBUG, NULL);
  
  thrd_t handshake_thread;

  thrd_create(&handshake_thread, start_handshake_socket, NULL);
  
  int res;
  thrd_join(handshake_thread, &res);
  
  printf("Exiting.... Code %d\n", res);

  return 0;
}

