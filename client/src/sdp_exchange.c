#include <arpa/inet.h>
#include <errno.h>
#include <sdp_exchange.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>


int exchange_offer(const char* server_address, int port, char* offer, int offer_length,
    char* answer_buffer, int max_answer_length) {
  int sockfd = socket(AF_INET, SOCK_STREAM, 0); // Socket erstellen
  if(sockfd < 0) return 0;
  
  struct sockaddr_in serv_addr;

  memset(&serv_addr, 0, sizeof(serv_addr));

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port);
  serv_addr.sin_addr.s_addr = inet_addr(server_address);
  
  if(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    printf("Connection couldnt be established. %d\n", errno);
    return 0;
  }

  write(sockfd, offer, offer_length);


  memset(answer_buffer, 0, max_answer_length);

  int read_offer_len = read(sockfd, answer_buffer, max_answer_length);

  close(sockfd);

  return 1;
}
