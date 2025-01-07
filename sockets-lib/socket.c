#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "socket.h"

int passive_socket_init(const int port) {
  int passSock;
  struct sockaddr_in servAddr;
  // Vytvorenie schrÃ¡nky pre komunikÃ¡ciu cez internet
  passSock = socket(AF_INET, SOCK_STREAM, 0);
  if (passSock < 0) {
    perror("Chyba pri vytvarani schranky");
    return -1;
  }
  // Nastavenie adresy pre potreby komunikÃ¡cie
  memset((char*)&servAddr, 0, sizeof(servAddr));
  servAddr.sin_family = AF_INET;
  servAddr.sin_addr.s_addr = INADDR_ANY;
  servAddr.sin_port = htons(port);
  if (bind(passSock, (struct sockaddr*)&servAddr, sizeof(servAddr)) < 0) {
    perror("Chyba pri nastavovani adresy schranky");
    return -1;
  }
  // Vytvorenie pasÃ­vnej schrÃ¡nky pre prijÃ­manie pripojenÃ­
  listen(passSock, 5);
  return passSock;
}

int passive_socket_wait_for_client(int passiveSocket) {
  struct sockaddr_in clAddr;
  socklen_t clSize = sizeof(clAddr);
  int actSock = accept(passiveSocket, (struct sockaddr*)&clAddr, &clSize);
  if (actSock < 0) {
    perror("Chyba pri akceptacii schranky!");
    return -1;
  }
  return actSock;
}

void passive_socket_destroy(int socket) {
  close(socket);
}
 
int connect_to_server(const char * name, const int port) {
  struct addrinfo * server;
  struct addrinfo hints;
  hints.ai_family = AF_INET; // IP4 aj IP6
  hints.ai_socktype = SOCK_STREAM; // SpoÄ¾ahlivÃ¡ komunikÃ¡cia
  // hints.ai_protocol = IPPROTO_TCP; // TCP/IP
  // ZisÅ¥ovanie adresy servera podÄ¾a mena
  // Pozor! mÃ´Å¾e existovaÅ¥ v urÄitÃ½ch prÃ­padoch aj viac dostupnÃ½ch adries
  char portText[10]= {0};
  sprintf(portText, "%d", port);
  int s = getaddrinfo(name, portText, &hints, &server);
  if (s != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
    return -1;
  }
  for (struct addrinfo * rp = server; rp != NULL; rp = rp->ai_next) {
    // Vytvorenie schrÃ¡nky pre komunikÃ¡ciu cez internet
    int actSock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (actSock < 0) {
      continue;
    }
    // Pripojenie na server
    if(connect(actSock, server->ai_addr, server->ai_addrlen) < 0) {
      // NeÃºspeÅ¡nÃ©
      close(actSock);
    } else {
      // ÃšspeÅ¡nÃ©
      freeaddrinfo(server); 
      return actSock;
    }
  }
  perror("Chyba pripojenia na server!");
  freeaddrinfo(server); 
  return -1;
}

void active_socket_destroy(int socket) {
  close(socket);
}
