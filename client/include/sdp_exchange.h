#pragma once


int exchange_offer(const char* server_address, int port, char* offer, int offer_length,
    char* answer_buffer, int max_answer_length);
