# Voicechat in C using WebRTC

This is supposed to be a simple voicechat in C using a WebRTC implementation, opus codec and hopefully unbreakable encryption. This is project for me to learn more about C development, encryption and different protocols.

## Building 

`mkdir build`
`cd build`
`cmake ..`
`make`, use `make -j` to use all available cores on your processor

Server executable in `build/server`
Client executable in `build/client`

Networking is currently only working when server and client are running on the same machine.

## Dependencies

Current dependencies:
- tinycthread to handle cross-plattform multithreading
- libdatachannel for WebRTC implementation
- portaudio
