#ifndef SERVER_H
#define SERVER_H

#include "syn_buffer.h"
#include "buffer.h"
#include "../sockets-lib/socket.h"

void *run_server(shared_names *names);

#endif // SERVER_H
