#include <asm-generic/socket.h>
#include <bits/types/struct_timeval.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>

#include <tinycthread/tinycthread.h>


#define PORT 4000
#define CLIENT_PORT 4001
#define MAXMSG 512

int datagram_socket;
char message[MAXMSG];

atomic_int running = 1;

void handle_sigint(int sig) {
  (void) sig;
  running = 0;
}

int start_datagram_socket(void* arg) {
  
  struct sockaddr_in server_addr;

  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) {
    printf("Server Socket could not be initialized. %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }

  memset(&server_addr, 0, sizeof(server_addr));


  // set socket recv timeout to 500ms

  struct timeval tv;
  tv.tv_sec = 0;
  tv.tv_usec = 500000;
  if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
    printf("Socket timeout option could not be set.\n");
    exit(EXIT_FAILURE);
  }
  
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_port = htons((unsigned short)PORT);

  // bind socket 

  if (  bind(sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0  ) {
    printf("Socket could not be bound.\n");
    exit(EXIT_FAILURE);
  }
  
  struct sockaddr_in client_addr;
  int clientlen = sizeof(client_addr);

  memset(&client_addr, 0, clientlen);
  int n;
  void* buffer = malloc(MAXMSG);

  printf("Waiting for incoming traffic...\n");

  // Receive and immediately mirror message back to sender
  while(running) {
    memset(buffer, 0, MAXMSG);

    n = recvfrom(sockfd, buffer, MAXMSG, 0, (const struct sockaddr *)&client_addr, &clientlen);
    if(n < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
      printf("Error while receiving message.\n");
    }
    
    if(n > 0) {
      struct sockaddr_in replay_address = client_addr;
      replay_address.sin_port = htons(CLIENT_PORT);

      printf("Received Bytes: %d. Mirroring back...\n", n, buffer);

      sendto(sockfd, buffer, n, 0, (struct sockaddr*) &replay_address, sizeof(replay_address));
    }

  }
  close(sockfd);
  return 1;
}

int main() {
  signal(SIGINT, handle_sigint);
  printf("Starting Server...\n");
  
  // Create server thread
  thrd_t server_thread;
  thrd_create(&server_thread, start_datagram_socket, NULL);

  int res;
  thrd_join(server_thread, &res);

  printf("Server Thread terminated with code %d.\n", res);
  return 0;
}

