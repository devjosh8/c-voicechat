#include <asm-generic/errno-base.h>
#include <asm-generic/errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <errno.h>
#include <signal.h>
#include <stdatomic.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <tinycthread/tinycthread.h>

// Client
#define SERVER_PORT 4000
#define CLIENT_PORT 4001
#define SERVER_ADDRESS "127.0.0.1"
#define MAXMSG 512

atomic_int running = 1;

void handle_sigint(int sig) {
  (void) sig;
  running = 0;
}

int recording_loop(void *arg) {
  // Initialize Socket for Audio streaming
  
  const char* server_ip = arg;

  int sockfd;
  struct sockaddr_in server_addr;

  int server_struct_length = sizeof(server_addr);

  sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

  if(sockfd < 0) {
    printf("Datagram socket for streaming data could not be initialized. %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }

  printf("Created Datagram Socket successfully.\n");

  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(SERVER_PORT);
  server_addr.sin_addr.s_addr = inet_addr(server_ip);

  const char* message = "Hello from client";
  int message_length = strlen(message);
  
  // define timespec to sleep 1 second
  struct timespec sleeping_time;
  sleeping_time.tv_sec = 1;
  sleeping_time.tv_nsec = 0;

  // Enter recording loop

  while(running) {
    //usleep(1000000); // sleep 1 second

    thrd_sleep(&sleeping_time, NULL);

    if(sendto(sockfd, message, message_length, 0, (struct sockaddr*)&server_addr, server_struct_length) < 0) {
      printf("Message could not be sent.\n");
    } else {
      printf("Message was sent successfully.\n");
    }
  }

  close(sockfd);
  printf("Successfully closed socket.\n");
  return 1;
}


int receiving_loop(void* arg) {

  struct sockaddr_in recv_addr;

  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) {
    printf("Client receiving socket could not be initialized. %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }

  // Serveradresse und Clientadresse vorerst leeren
  memset(&recv_addr, 0, sizeof(recv_addr));


  // set socket recv timeout to 500ms

  struct timeval tv;
  tv.tv_sec = 0;
  tv.tv_usec = 500000;
  if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
    printf("Socket timeout option could not be set.\n");
    exit(EXIT_FAILURE);
  }
  
  recv_addr.sin_family = AF_INET;
  recv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  recv_addr.sin_port = htons((unsigned short)CLIENT_PORT);

  // Socket binden

  if (  bind(sockfd, (const struct sockaddr *)&recv_addr, sizeof(recv_addr)) < 0  ) {
    printf("Socket could not be bound.\n");
    exit(EXIT_FAILURE);
  }
  
  struct sockaddr client_addr;
  int clientlen = sizeof(client_addr);

  memset(&client_addr, 0, clientlen);
  int n;
  void* buffer = malloc(MAXMSG);

  printf("Waiting for incoming traffic...\n");
  while(running) {
    memset(buffer, 0, MAXMSG);

    n = recvfrom(sockfd, buffer, MAXMSG, 0, (struct sockaddr *)&client_addr, (unsigned int *)&clientlen);
    if(n < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
      printf("Error while receiving message.");
    } else if(n > 0) {
      printf("Received Bytes: %d\n", n); 
    }
  }
  close(sockfd);

  return 1;
}

int main() {
  signal(SIGINT, handle_sigint);
  printf("Client Hello!\n");

  // TODO: Implement threading
  //
  // THREAD 1: Recording Microphone
  // THREAD 2: Play Audio that was received from server (bind this socket to Port 4001)

  thrd_t recording_thread;
  thrd_t receiving_thread;

  const char* server_ip = SERVER_ADDRESS;

  thrd_create(&recording_thread, recording_loop, (void *) server_ip);
   
  thrd_create(&receiving_thread, receiving_loop, NULL);

  int res;
  thrd_join(recording_thread, &res);
  printf("Recording Thread terminated with code %d.\n", res);
  thrd_join(receiving_thread, &res);
  printf("Receiving Thread terminated with code %d.\n", res);
  return 0;
}
