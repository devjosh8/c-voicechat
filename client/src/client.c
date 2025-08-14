#include <asm-generic/errno-base.h>
#include <asm-generic/errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <errno.h>
#include <sched.h>
#include <signal.h>
#include <stdatomic.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <tinycthread/tinycthread.h>
#include <rtc/rtc.h>

// Client
#define SERVER_PORT 4000
#define CLIENT_PORT 4001
#define SERVER_ADDRESS "127.0.0.1"
#define MAXMSG 512

#define MAX_ANSWER 4096

atomic_int running = 1;

void error(const char* msg) {
  puts(msg);
  exit(EXIT_FAILURE);
}

void handle_sigint(int sig) {
  (void) sig;
  running = 0;
}

void on_track(int peer_connection, int trc_track, void *arg) {
  printf("Daten vom Server empfangen!\n");
}

int make_offer_and_get_answer(const char* server_address, int port, char* offer, int offer_length,
    char* answer_buffer, int max_answer_length) {
  int sockfd = socket(AF_INET, SOCK_STREAM, 0); // Socket erstellen
  if(sockfd < 0) return 0;
  
  struct sockaddr_in serv_addr;

  memset(&serv_addr, 0, sizeof(serv_addr));

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port);
  serv_addr.sin_addr.s_addr = inet_addr(server_address);
  
  if(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    printf("Verbindung konnte nicht hergestellt werden. %d\n", errno);
    return 0;
  }

  write(sockfd, offer, offer_length);

  // Nachricht empfangen
  memset(answer_buffer, 0, max_answer_length);

  int read_offer_len = read(sockfd, answer_buffer, max_answer_length);

  close(sockfd);

  return 1;
}


int main() {
  signal(SIGINT, handle_sigint);
  
  rtcInitLogger(RTC_LOG_DEBUG, NULL);
  
  rtcConfiguration config;
  config.iceServers = NULL;
  config.iceServersCount = 0;
  config.proxyServer = NULL;
  config.bindAddress = NULL;
  config.certificateType = RTC_CERTIFICATE_DEFAULT;
  config.iceTransportPolicy = RTC_TRANSPORT_POLICY_ALL;
  config.enableIceTcp = 0;
  config.enableIceUdpMux = 0;
  config.disableAutoNegotiation = 0;
  config.forceMediaTransport = 0;
  config.portRangeBegin = 0;
  config.portRangeEnd = 0;
  config.mtu = 0;
  config.maxMessageSize = 0;

  int pc = rtcCreatePeerConnection(&config);
  printf("Peer connection created!\n");
  
  // DataChannel erstellen

  int datachannel = rtcCreateDataChannel(pc, "client-to-server");
  
  printf("Datachannel was successfully created!\n");
  // Das hier ist der Client Peer. Er erstellt ein Offer und sendet dieses an den
  // Server

  char offer[8192];
  int offer_size = rtcCreateOffer(pc, offer, 8192-1);
  printf("Offer successfully created. Offer-Size: %d, Offer:\n\n%s\n", offer_size, offer);
  
  

  // Offer in einer Datei für den Server abspeichern!
  char server_answer[MAX_ANSWER];

  int ret = make_offer_and_get_answer("127.0.0.1", SERVER_PORT, offer, offer_size, server_answer, MAX_ANSWER-1);
  if(ret == 0) {
    rtcCleanup();
    error("Error while making offer. Closing...\n");
  }

  rtcSetRemoteDescription(pc, server_answer, "answer");
  printf("Bekommene Antwort: %s\n", server_answer);

  const char* data = "Hallo Welt!";

  while(running) {
    sleep(5);
    rtcSendMessage(datachannel, data, strlen(data));
    printf("trying to send message...\n");
  }

  rtcClosePeerConnection(pc);


  rtcDeleteDataChannel(datachannel);
  rtcDeletePeerConnection(pc);

  rtcCleanup();
  return 0;
}

