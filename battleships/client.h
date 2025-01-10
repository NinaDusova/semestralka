#ifndef CLIENT_H
#define CLIENT_H

#include "syn_buffer.h"
#include "buffer.h"
#include "../sockets-lib/socket.h"

void *run_client(shared_names *names);

#endif // CLIENT_H