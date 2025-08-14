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

#define PORT 4000
#define CLIENT_PORT 4001
#define MAXMSG 512


atomic_int running = 1;
atomic_int thread_id_counter = 1;

void handle_sigint(int sig) {
  (void) sig;
  running = 0;
}

struct NegotiationArgs {
  char* offer;
  int offer_length;
  int clientfd;
  int thread_id;
};

struct ClientContext {
  int incoming_datachannel_id;
  int outgoing_datachannel_id;
};

void on_message(int pc, const char *message, int size, void *user_ptr) {
  struct ClientContext *client_context = (struct ClientContext *) user_ptr;
  if(client_context->incoming_datachannel_id < 0)return;
  printf("Message auf Datachannel %d: %s\n", client_context->incoming_datachannel_id, message);
}

void on_datachannel_register(int pc, int dc, void *user_ptr) {
  struct ClientContext *client_context = (struct ClientContext *) user_ptr;
  client_context->incoming_datachannel_id = dc;
  rtcSetMessageCallback(dc, on_message);
  printf("Peer ID %d registriert einen neuen Datachannel ID %d\n\n\n\n\n\n", pc, dc);
}

int negotiate_and_start_peer_connection(void *args) {

  struct ClientContext client_context;
  client_context.incoming_datachannel_id = -1;

  printf("Verhandlung starten...\n"); 
  struct NegotiationArgs *negotiation_args = (struct NegotiationArgs *) args;

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

  int datachannel = rtcCreateDataChannel(pc, "server-to-client");
  client_context.outgoing_datachannel_id = datachannel;


  rtcSetUserPointer(pc, &client_context);
  rtcSetDataChannelCallback(pc, on_datachannel_register);

  rtcSetRemoteDescription(pc, negotiation_args->offer, "offer");

  char answer[2048];
  int answer_size = rtcCreateAnswer(pc, answer, 8192);
  
  sleep(1);
  write(negotiation_args->clientfd, answer, answer_size);
  
  printf("Verhandlung durchgeführt.\n");

  while(running) {
    sleep(1);
  }

  rtcClosePeerConnection(pc);
  rtcDeleteDataChannel(datachannel);

  rtcDeletePeerConnection(pc);

  rtcCleanup();

  close(negotiation_args->clientfd);
  return 1;
}

int start_handshake_socket(void *arg) {
  struct sockaddr_in serv_addr, cli_addr;
  int sockfd = socket(AF_INET, SOCK_STREAM, 0); // Socket erstellen
                                                //
  if(sockfd < 0) {
    printf("Handshake socket could not be initialized.\n");
    exit(EXIT_FAILURE);
  }

  memset(&serv_addr, 0, sizeof(serv_addr));

  struct timeval timeout;

  timeout.tv_sec = 1;
  timeout.tv_usec = 0;
  if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,
        (char*)&timeout, sizeof(timeout)) < 0) {
    perror("setsockopt SO_RCVTIMEO");
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
  while(running) {
    int newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &client_length);
    if (newsockfd < 0) {
      if (errno == EWOULDBLOCK || errno == EAGAIN) {
        continue;
      }
      printf("Error on accepting client...\n");
      continue;
    }

    // Socket schickt Anfrage an Server 
    char offer_buf[2048];
    memset(offer_buf, 0, sizeof(offer_buf));

    int read_offer_len = read(newsockfd, offer_buf, 2048-1);

    if(read_offer_len < 0) {
      printf("Error reading from socket.\n");
      close(newsockfd);
      continue;
    } else {
      struct NegotiationArgs negotiation_args;
      negotiation_args.offer = offer_buf;
      negotiation_args.offer_length = read_offer_len;
      negotiation_args.clientfd = newsockfd;
      negotiation_args.thread_id = atomic_fetch_add(&thread_id_counter, 1);

      thrd_t thread;
      thrd_create(&thread, negotiate_and_start_peer_connection, (void *)&negotiation_args);
      thrd_detach(thread);

    }

  }
  
  return 1;
}

void on_channel_open(int id, void *user_ptr) {
  printf("Channel with id %d was opened.\n", id);
}

void on_channel_close(int id, void *user_ptr) {
  printf("Channel with id %d was closed.\n", id);
}

int data_channel_id = 0;

void on_data_channel_callback(int pc, int dc, void *user_ptr) {
  printf("Ein Datachannel wurde registriert mit ID %d\n", dc);
  data_channel_id = dc;
}

int main() {
  signal(SIGINT, handle_sigint);
  printf("Starting Server...\n");
  rtcInitLogger(RTC_LOG_DEBUG, NULL);
  
  thrd_t handshake_thread;

  thrd_create(&handshake_thread, start_handshake_socket, NULL);
  
  int res;
  thrd_join(handshake_thread, &res);
  
  printf("Server beendet.\n");

  return 0;
}

