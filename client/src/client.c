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
#include <rtc/rtc.h>

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


int main() {
  signal(SIGINT, handle_sigint);
  printf("Client Hello!\n");

  rtcInitLogger(RTC_LOG_INFO, NULL);


  rtcCleanup();
  return 0;
}
