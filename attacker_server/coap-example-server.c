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
 *      Erbium (Er) CoAP Engine example.
 * \author
 *      Matthias Kovatsch <kovatsch@inf.ethz.ch>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "contiki.h"
#include "coap-engine.h"

#include "coap-blocking-api.h"

#if PLATFORM_SUPPORTS_BUTTON_HAL
#include "dev/button-hal.h"
#else
#include "dev/button-sensor.h"
#endif

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_APP
/*
 * Resources to be activated need to be imported through the extern keyword.
 * The build system automatically compiles the resources in the corresponding sub-directory.
 */
extern coap_resource_t res_mitm;


PROCESS(er_example_server, "Erbium Example Server");



#define SERVER_EP "coap://[fd00::1]"      // Real server address
static coap_endpoint_t server_ep;
static coap_message_t* expected_response;

#define SEND_DATA() { \
  static coap_message_t request[1]; \
  coap_init_message(request, COAP_TYPE_CON, COAP_GET, 0); \
  coap_set_header_uri_path(request, "test"); \
  const char msg[] = "Toggle!"; \
  coap_set_payload(request, (uint8_t *)msg, sizeof(msg) - 1); \
  response_handler(expected_response); \
  /*COAP_BLOCKING_REQUEST(&server_ep, request, response_handler);*/ \
}

void
response_handler(coap_message_t *response)
{
  puts("Got response");
}


PROCESS(send_request_main, "TBA2");
PROCESS_THREAD(send_request_main, ev, data)
{
  PROCESS_BEGIN();
  puts("Processing request (main)");
  
  //coap_message_t *request = (coap_message_t*)data;
  
  SEND_DATA();
  
  puts("Finished processing request (main)");
  
  //process_poll((struct process*)data);
  
  PROCESS_END();
}



PROCESS(input_ctl, "Input control to launch attack steps");
PROCESS_THREAD(input_ctl, ev, data)
{
  PROCESS_BEGIN();
  
  
  
  PROCESS_END();
}

AUTOSTART_PROCESSES(&er_example_server);

PROCESS_THREAD(er_example_server, ev, data)
{
  PROCESS_BEGIN();

  PROCESS_PAUSE();

  LOG_INFO("Starting Erbium Example Server\n");

  /*
   * Bind the resources to their Uri-Path.
   * WARNING: Activating twice only means alternate path, not two instances!
   * All static variables are the same for each URI path.
   */
   coap_activate_resource(&res_mitm, "test");

  coap_endpoint_parse(SERVER_EP, strlen(SERVER_EP), &server_ep);
  
  /* Define application-specific events here. */
  expected_response = NULL;
  //SEND_DATA();
#if 0
  while(1) {
    PROCESS_WAIT_EVENT();

      /* Call the event_handler for this application-specific event. */
      puts("Got request at main");
      //process_start(&send_request_main, PROCESS_CURRENT());
      //PROCESS_YIELD();
      //while (process_is_running(&send_request_main)) {}
      puts("Finished preprocessing");
      //res_mitm.trigger();

      /* Also call the separate response example handler. */
      //res_separate.resume();
  }                             /* while (1) */
#endif
  PROCESS_END();
}
