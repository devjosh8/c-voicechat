#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>

// Client
#define PORT 4000
#define MAXMSG 512

void sendHelloMessage() {
  int sockfd;
  struct sockaddr_in server_addr;

  int server_struct_length = sizeof(server_addr);

  sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

  if(sockfd < 0) {
    printf("Server Socket could not be initialized. %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }

  printf("Created Socket successfully.\n");

  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(PORT);
  server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // TODO: IP als Argument

  const char* message = "Hello from client";
  int message_length = strlen(message);

  // Nachricht 10 mal an den Server senden
  for(int i = 0; i < 10; i++) {
    if(sendto(sockfd, message, message_length, 0, (struct sockaddr*)&server_addr, server_struct_length) < 0) {
      printf("Message could not be sent.\n");
    } else {
      printf("Message was sent successfully.\n");
    }
  }

  close(sockfd);
  printf("Bye bye...\n");
}

int main() {
  printf("Client Hello!\n");
  sendHelloMessage();
  return 0;
}
