#include <asm-generic/errno-base.h>
#include <asm-generic/errno.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>


#define PORT 4000
#define MAXMSG 4096

// Server
int datagram_socket;
char message[MAXMSG];

volatile sig_atomic_t running = 1;

void handle_sigint(int sig) {
    running = 0; 
}

void start_datagram_socket() {
  
  struct sockaddr_in server_addr;

  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) {
    printf("Server Socket could not be initialized. %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }

  // Serveradresse und Clientadresse vorerst leeren
  memset(&server_addr, 0, sizeof(server_addr));
  
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_port = htons((unsigned short)PORT);

  // Socket binden

  if (  bind(sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0  ) {
    printf("Socket could not be bound.\n");
    exit(EXIT_FAILURE);
  }

  fcntl(sockfd, F_SETFL, O_NONBLOCK);
  
  struct sockaddr client_addr;
  int clientlen = sizeof(client_addr);

  memset(&client_addr, 0, clientlen);
  int n;
  void* buffer = malloc(MAXMSG);

  printf("Waiting for incoming traffic...\n");
  while(running) {
    memset(buffer, 0, MAXMSG);

    n = recvfrom(sockfd, buffer, MAXMSG, 0, (struct sockaddr *)&client_addr, &clientlen);
    if(n < 0) {
      if(errno != EAGAIN && errno != EWOULDBLOCK) printf("Error while receiving message.");
    } else {
      sendto(sockfd,buffer, n, 0, (struct sockaddr *)&client_addr, clientlen);
    }
  }

  close(sockfd);
}

int main() {
  signal(SIGINT, handle_sigint);
  printf("Starting Server...\n");
  start_datagram_socket();
  

  printf("Exiting...\n");
  return 0;
}

