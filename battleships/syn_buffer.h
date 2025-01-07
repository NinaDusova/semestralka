#ifndef SYN_BUFFER
#define SYN_BUFFER

#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>
#include "shm.h"

typedef struct synchronized_buffer {
  buffer *buff_;
  int buff_fd_;
  sem_t *mut_pc_;
  sem_t *sem_produce_;
  sem_t *sem_consume_;
} synchronized_buffer;

void syn_shm_buffer_init(shared_names *names);
void syn_shm_buffer_destroy(shared_names *names);
void syn_shm_buffer_open(synchronized_buffer *this, shared_names *names);
void syn_shm_buffer_close(synchronized_buffer *this);
void syn_shm_buffer_push(synchronized_buffer *this, const game_action *input);
void syn_shm_buffer_pop(synchronized_buffer *this, game_action *output);

#endif
