/*
 * Interface for thread conversation
 *
 * This file is part of GTick
 *
 *
 * Copyright (c) 2003, 2004, 2005, 2006 Roland Stigge <stigge@antcom.de>
 *
 * GTick is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * GTick is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GTick; if not, see <http://www.gnu.org/licenses/>.
 *
 */

/* GNU headers */
#include <stdio.h>
#include <stdlib.h>

/* GTK+ headers */
#include <glib.h>

#ifdef USE_DMALLOC
#include <dmalloc.h>
#endif

/* own headers */
#include "threadtalk.h"

/*
 * returns initialized comm object
 */
comm_t* comm_new(void) {
  comm_t* result;
  
  result = (comm_t*) g_malloc(sizeof(comm_t));
  result->server = g_async_queue_new();
  result->client = g_async_queue_new();
  
  return result;
}

/*
 * destructor for comm objects
 */
void comm_delete(comm_t* comm) {
  g_async_queue_unref(comm->server);
  g_async_queue_unref(comm->client);
  free(comm);
}

/*
 * client sends query to server with separately allocated body
 * (to be only accessed by server and destroyed there)
 */
void comm_client_query(comm_t* comm, message_type_t type, void* body) {
  message_t* message;
  
  message = (message_t*) g_malloc (sizeof(message_t));
  message->type = type;
  message->body = body;
  g_async_queue_push(comm->server, message);
}

/*
 * client tries to read message from queue
 * stores message in body if body != 0
 * if no message is available, returns MESSAGE_TYPE_NO_MESSAGE 
 * NOTE: message from server must be freed by client thread
 */
message_type_t comm_client_try_get_reply(comm_t* comm, void** body) {
  message_t* message = (message_t*) g_async_queue_try_pop(comm->client);

  if (message) {
    message_type_t type = message->type;
    if (body) {
      *body = message->body;
    }
    free(message);
    return type;
  } else {
    return MESSAGE_TYPE_NO_MESSAGE;
  }
}

/*
 * register server at comm object (by increasing reference count)
 */
void comm_server_register(comm_t* comm) {
  g_async_queue_ref(comm->server);
  g_async_queue_ref(comm->client);
}

/*
 * un-register server at comm object (by decreasing reference count)
 */
void comm_server_unregister(comm_t* comm) {
  g_async_queue_unref(comm->server);
  g_async_queue_unref(comm->client);
}

/*
 * server tries to read message from queue
 * stores message in body if body != 0
 * if no message is available, returns MESSAGE_TYPE_NO_MESSAGE 
 * NOTE: message from client must be freed by server thread
 */
message_type_t comm_server_try_get_query(comm_t* comm, void** body) {
  message_t* message = (message_t*) g_async_queue_try_pop(comm->server);

  if (message) {
    message_type_t type = message->type;
    if (body) {
      *body = message->body;
    }
    free(message);
    return type;
  } else {
    return MESSAGE_TYPE_NO_MESSAGE;
  }
}

/*
 * send message (back) to client
 */
void comm_server_send_response(comm_t* comm, message_type_t type, void* body) {
  message_t* message;
  
  message = (message_t*) g_malloc (sizeof(message_t));
  message->type = type;
  message->body = body;
  g_async_queue_push(comm->client, message);
}

