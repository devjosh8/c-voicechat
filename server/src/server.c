#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>


#define PORT 4000
#define MAXMSG 512

// Server
int datagram_socket;
char message[MAXMSG];

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
  
  struct sockaddr client_addr;
  int clientlen = sizeof(client_addr);

  memset(&client_addr, 0, clientlen);
  int n;
  void* buffer = malloc(MAXMSG);

  printf("Waiting for incoming traffic...\n");
  while(1) {
    memset(buffer, 0, MAXMSG);

    n = recvfrom(sockfd, buffer, MAXMSG, 0, (const struct sockaddr *)&client_addr, &clientlen);
    if(n < 0) {
      printf("Error while receiving message.");
    }
    
    char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];

    if( getnameinfo(&client_addr, clientlen, hbuf, sizeof(hbuf), sbuf , sizeof(sbuf ), NI_NUMERICHOST | NI_NUMERICSERV) == 0) {
      printf("Received Bytes: %d from %s, %s\n", n, hbuf, sbuf); 
    }

  }
}

int main() {
  printf("Starting Server...\n");
  start_datagram_socket();
  // Start Server and wait for incoming connection

  return 0;
}

