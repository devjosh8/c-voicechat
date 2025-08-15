#include <netinet/in.h>
#include <errno.h>
#include <opus/opus_defines.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <threads.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <opus/opus.h>

#include <raylib.h>

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#define PORT 4000
#define MAXMSG 4096
#define MAX_FRAME_SIZE 960
#define MICROPHONE_DEVICE "Auna Mic CM900 Mono"
#define PLAYBACK_DEVICE "Navi 31 HDMI/DP Audio Digitales Stereo (HDMI)"


volatile sig_atomic_t running = 1;

int sockfd;
struct sockaddr_in server_addr;

ma_device playback_device;
ma_pcm_rb playback_ringbuffer;

OpusDecoder* opus_decoder;

void handle_sigint(int sig) {
    running = 0; 
}

void error(const char* err) {
  printf("%s\n", err);
  exit(EXIT_FAILURE);
}

void playback_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    // Ignoriere pInput für Playback
    (void)pInput;

    // Angenommen Format ist ma_format_f32 (Float32) und 2 Kanäle (Stereo)
    float* pOutputF32 = (float*)pOutput;
    ma_result mr;

    void* ring_buffer_output;
    mr = ma_pcm_rb_acquire_read(&playback_ringbuffer, &frameCount, &ring_buffer_output);
    float* rb_data = (float*) ring_buffer_output;
    memcpy(pOutput, rb_data, frameCount * sizeof(float) * 1);

    ma_pcm_rb_commit_read(&playback_ringbuffer, frameCount);
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

      float pcm[MAXMSG+1];
                                                                          
      int decode_amount = opus_decode_float(opus_decoder, buffer, n, pcm, MAXMSG, 0);
      
      if(decode_amount > 0) {

        printf("Daten erhalten: %d Bytes Framesize %d Decodiert %d\n", n, 0, decode_amount); // TODO: remove 
        ma_result mr;
        ma_uint32 requested_frames = decode_amount;
        
        void* ringpointer;
        mr = ma_pcm_rb_acquire_write(&playback_ringbuffer, &requested_frames, &ringpointer);

        if(mr != OPUS_OK) {
          printf("Fehler beim allokieren vom Ring Buffer!\n");
        } else {
          // In den Ringbuffer kopieren
          memcpy(ringpointer, pcm, decode_amount * sizeof(float));

          ma_pcm_rb_commit_write(&playback_ringbuffer, requested_frames);

        }
      }
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

ma_device_id get_recording_device(ma_context* miniaudio_context, const char* name) {

  ma_device_info* recording_devices;
  ma_uint32 recording_device_count;

  ma_context_get_devices(miniaudio_context, NULL, NULL, &recording_devices, &recording_device_count);
  for(ma_uint32 i = 0; i < recording_device_count; i++) {
    if(strcmp(recording_devices[i].name, name) == 0) return recording_devices[i].id;
  }
  
  error("Could not find recording device...\n");
}

ma_device_id get_playback_device(ma_context* miniaudio_context, const char* name) {

  ma_device_info* devices;
  ma_uint32 device_count;

  ma_context_get_devices(miniaudio_context,  &devices, &device_count, NULL, NULL);
  for(ma_uint32 i = 0; i < device_count; i++) {
    if(strcmp(devices[i].name, name) == 0) return devices[i].id;
  }
  
  error("Could not find playback device...\n");
}


OpusEncoder* opus_encoder;

void capture_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    const float* input = (const float*)pInput;

    unsigned char opus_data[MAXMSG];
    uint32_t framecount_32 = frameCount;

    int opus_len = opus_encode_float(opus_encoder, input, frameCount, opus_data, MAXMSG);
    if(opus_len > 0 & sockfd > 0) {
      sendto(sockfd, opus_data, opus_len, 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
    }
}


ma_device recording_device;

void initialize_audio(ma_context* miniaudio_context) {

  int opus_err;
  opus_encoder = opus_encoder_create(48000, 1, OPUS_APPLICATION_VOIP, &opus_err);
  if(opus_err != OPUS_OK) {
    printf("Opus Encoder couldnt be initialized\n");
  }

  opus_decoder = opus_decoder_create(48000, 1, &opus_err);
  if(opus_err != OPUS_OK) {
    printf("Opus Encoder couldnt be initialized\n");
  }

  ma_result mr;

  mr = ma_context_init(NULL, 0, NULL,  miniaudio_context);
  if(mr != MA_SUCCESS) error("Miniaudio couldnt be initialized\n"); 


  // Ringbuffer initialisieren
  mr = ma_pcm_rb_init(ma_format_f32, 1, 32768, NULL, NULL, &playback_ringbuffer);
  

  ma_device_id recording_device_id = get_recording_device( miniaudio_context, MICROPHONE_DEVICE);

  ma_device_id playback_device_id = get_playback_device( miniaudio_context, PLAYBACK_DEVICE);

  // Initialize Recording device
  ma_device_config device_config = ma_device_config_init(ma_device_type_capture);
  device_config.capture.format = ma_format_f32;
  device_config.capture.channels = 1;
  device_config.sampleRate = 48000;
  device_config.dataCallback = capture_callback;
  device_config.pUserData = NULL;
  device_config.capture.pDeviceID = &recording_device_id;

  device_config.periodSizeInFrames = MAX_FRAME_SIZE;

  if( ma_device_init(miniaudio_context, &device_config, &recording_device) != MA_SUCCESS) {
    error("Error while initializing capture device.\n");
  }

  if(ma_device_start(&recording_device) != MA_SUCCESS) {
    error("Error while starting device.\n");
  }

  // Initialize Playback device
  ma_device_config device_config1 = ma_device_config_init(ma_device_type_playback);
  device_config1.playback.format = ma_format_f32;
  device_config1.playback.channels = 1;
  device_config1.sampleRate = 48000;
  device_config1.pUserData = NULL;
  device_config1.capture.pDeviceID = &playback_device_id;
  device_config1.dataCallback = playback_callback;
  
  if( ma_device_init(miniaudio_context, &device_config1, &playback_device) != MA_SUCCESS) {
    error("Error while initializing playback device.\n");
  }

  if( ma_device_start(&playback_device) != MA_SUCCESS) {
    error("error while starting playback device.\n");
  }

printf("Capture: %u Hz, %u ch\n", recording_device.sampleRate, recording_device.capture.channels);
printf("Playback: %u Hz, %u ch\n", playback_device.sampleRate, playback_device.playback.channels);
  printf("Device successfully initialized!\n");
}

int main() {
  
  ma_context miniaudio_context;
  ma_result result;

  result = ma_context_init(NULL, 0, NULL, &miniaudio_context);
  if (result != MA_SUCCESS) {
    error("couldnt initialize miniaudio\n");
  }

  initialize_audio(&miniaudio_context);
   
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
  ma_device_uninit(&recording_device);
  ma_pcm_rb_uninit(&playback_ringbuffer);
  ma_device_uninit(&playback_device);
  ma_context_uninit(&miniaudio_context);
  return 0;
}
