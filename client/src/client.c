#include <asm-generic/errno-base.h>
#include <asm-generic/errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sched.h>
#include <signal.h>
#include <stdatomic.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <tinycthread/tinycthread.h>
#include <sdp_exchange.h>
#include <rtc/rtc.h>


#define SERVER_PORT 4008
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


int main() {
  signal(SIGINT, handle_sigint);
  
  rtcInitLogger(RTC_LOG_INFO, NULL);
  
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
  
  int datachannel = rtcCreateDataChannel(pc, "client-to-server");
  
  printf("Datachannel was successfully created!\n");


  char offer[8192];
  int offer_size = rtcCreateOffer(pc, offer, 8192-1);
  printf("Offer successfully created. Offer-Size: %d, Offer:\n\n%s\n", offer_size, offer);
  
  

  char server_answer[MAX_ANSWER];

  int ret = exchange_offer("127.0.0.1", SERVER_PORT, offer, offer_size, server_answer, MAX_ANSWER-1);
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

