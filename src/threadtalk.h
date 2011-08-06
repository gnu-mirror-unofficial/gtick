/*
 * Interface for asynchronous thread conversation
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

#ifndef THREADTALK_H
#define THREADTALK_H

/* GLib thread abstraction implementation */
#include <glib.h>

typedef struct comm_t {
  GAsyncQueue* client; /* e.g. in GTick: (messages to) main thread */
  GAsyncQueue* server; /* e.g. in GTick: (messages to) audio thread */
} comm_t;

/*
 * actual data in message_t body field, if documented
 * reply only for labelled messages
 */
enum message_type_t {
  MESSAGE_TYPE_NO_MESSAGE,

  MESSAGE_TYPE_STOP_SERVER,
  MESSAGE_TYPE_SET_DEVICE,      /* param: char*: device */
  MESSAGE_TYPE_SET_SOUND,       /* param: char* sound name or filename */
  MESSAGE_TYPE_SET_SOUNDSYSTEM,
  MESSAGE_TYPE_SET_METER,       /* param: int*: meter */
  MESSAGE_TYPE_SET_ACCENTS,     /* param: char*: accent list: {'0','1'}* */
  MESSAGE_TYPE_SET_FREQUENCY,   /* param: double*: frequency */
  MESSAGE_TYPE_START_METRONOME, /* response needed: OK / ERROR */
  MESSAGE_TYPE_STOP_METRONOME,
  MESSAGE_TYPE_START_SYNC,      /* start / stop messages from server */
  MESSAGE_TYPE_STOP_SYNC,
  MESSAGE_TYPE_SET_VOLUME,      /* param: double*: volume 0.0 ... 1.0 */
  MESSAGE_TYPE_GET_VOLUME,      /* response needed: double*: volume 0.0...1.0 */

  MESSAGE_TYPE_RESPONSE_VOLUME, /* param: int*: volume 0 ... 100 */
  MESSAGE_TYPE_RESPONSE_SYNC,   /* param: unsigned int*: position in meter*/
  MESSAGE_TYPE_RESPONSE_START_ERROR
};
typedef enum message_type_t message_type_t;

typedef struct message_t {
  message_type_t type;
  void* body;
} message_t;

comm_t* comm_new(void);
void comm_delete(comm_t* comm);

void comm_client_query(comm_t* comm, message_type_t type, void* body);
message_type_t comm_client_try_get_reply(comm_t* comm, void** body);

void comm_server_register(comm_t* comm);
void comm_server_unregister(comm_t* comm);
message_type_t comm_server_try_get_query(comm_t* comm, void** body);
void comm_server_send_response(comm_t* comm, message_type_t type, void* body);

#endif /* THREADTALK_H */

