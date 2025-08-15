#include <netinet/in.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <threads.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>

// Client
#define PORT 4000
#define MAXMSG 4096

volatile sig_atomic_t running = 1;

int sockfd;
struct sockaddr_in server_addr;

void handle_sigint(int sig) {
    running = 0; 
}


int receiving_thread(void *arg) {
  struct sockaddr client_addr;
  int clientlen = sizeof(client_addr);

  memset(&client_addr, 0, clientlen);
  int n;
  void* buffer = malloc(MAXMSG);

  printf("Waiting for incoming traffic...\n");
  while(running) {
    memset(buffer, 0, MAXMSG);

    n = recvfrom(sockfd, buffer, MAXMSG, 0, (struct sockaddr *)&client_addr, &clientlen);
    if(n > 0) {
      printf("%s\n", buffer); // TODO: remove 
    }
  }

  return 1;
}

int sending_thread(void* arg) {

  const char* message = "Hello from client";
  int message_length = strlen(message);
  
  unsigned int server_struct_length = sizeof(server_addr);

  while(running) {
    if(sendto(sockfd, message, message_length, 0, (struct sockaddr*)&server_addr, server_struct_length) < 0) {
      printf("Message could not be sent.\n");
    }
    sleep(1);
  }


  return 1;
}

int main() {

  sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

  if(sockfd < 0) {
    printf("Server Socket could not be initialized. %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }

  printf("Created Socket successfully.\n");

  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(PORT);
  server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // TODO: IP als Argument
                                                        
  // Sichergehen, dass das Betriebssystem den Socket bindet 
  while( sendto(sockfd, "1", strlen("1"), 0, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0 ) { sleep(1); };
                                                      
  thrd_t thrd_sending;
  thrd_create(&thrd_sending, sending_thread, NULL);
  thrd_t thrd_receiving;
  thrd_create(&thrd_receiving, receiving_thread, NULL);

  int res_sending;
  int res_receiving;
  thrd_join(thrd_sending, &res_sending);
  thrd_join(thrd_receiving, &res_receiving);

  close(sockfd);

  printf("Exited. Codes: Sending: %d Receiving: %d", res_sending, res_receiving);
  return 0;
}
