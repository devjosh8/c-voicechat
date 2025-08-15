#include <stdatomic.h>
#include <stdlib.h>
#include <unistd.h>
#include <worker.h>
#include <stdio.h>
#include <rtc/rtc.h>

atomic_int worker_running = 1;

void on_message(int pc, const char *message, int size, void *user_ptr) {
  struct ClientContext *client_context = (struct ClientContext *) user_ptr;
  if(client_context->incoming_datachannel_id < 0)return;
  printf("Message on data channel %d: %s\n", client_context->incoming_datachannel_id, message);
}


// TODO: Rework later and implement correct Connection-Closing-Handling
void on_datachannel_close(int id, void *user_ptr) {
  struct ClientContext *client_context = (struct ClientContext *) user_ptr;
  if(id == client_context->incoming_datachannel_id) {
    client_context->running = 0;
  }
    printf("Datachannel with id %d was closed.\n", id);
}

void on_datachannel_register(int pc, int dc, void *user_ptr) {
  struct ClientContext *client_context = (struct ClientContext *) user_ptr;
  client_context->incoming_datachannel_id = dc;
  close(client_context->clientfd); 
  rtcSetMessageCallback(dc, on_message);
  rtcSetClosedCallback(dc, on_datachannel_close);
  printf("Peer ID %d registered successfully with new data channel %d.\n", pc, dc);
}

int negotiate_and_start_peer_connection(void *args) {

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
  struct ClientContext* client_context = malloc(sizeof(struct ClientContext));
  client_context->incoming_datachannel_id = -1;
  client_context->outgoing_datachannel_id = datachannel;
  client_context->clientfd = negotiation_args->clientfd;
  client_context->thread_id = negotiation_args->thread_id;
  client_context->running = 1;
  client_context->pc = pc;

  rtcSetUserPointer(pc, client_context);

  rtcSetDataChannelCallback(pc, on_datachannel_register);

  rtcSetRemoteDescription(pc, negotiation_args->offer, "offer");

  char answer[2048];
  int answer_size = rtcCreateAnswer(pc, answer, 8192);
  
  sleep(1);
  write(negotiation_args->clientfd, answer, answer_size);
  free(negotiation_args); 


  while (atomic_load(&worker_running) && client_context->running) {
    sleep(1);
  }
  free(client_context);
  rtcClosePeerConnection(pc);
  rtcDeleteDataChannel(datachannel);

  rtcDeletePeerConnection(pc);

  rtcCleanup();

  printf("Thread %d exited.\n", client_context->thread_id);
  return 1;
}
