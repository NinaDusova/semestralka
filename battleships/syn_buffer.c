#include "syn_buffer.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

void syn_shm_buffer_init(shared_names *names) {
  if (sem_open(names->mut_pc_, O_RDWR | O_CREAT | O_EXCL,S_IRUSR | S_IWUSR, 1) == SEM_FAILED) {
    perror("Failed to create mutex PC");
    exit(EXIT_FAILURE);
  }

  if (sem_open(names->sem_produce_, O_RDWR | O_CREAT | O_EXCL,S_IRUSR | S_IWUSR, BUFFER_CAPACITY) == SEM_FAILED) {
    perror("Failed to create sem produce");
    exit(EXIT_FAILURE);
  }

  if (sem_open(names->sem_consume_, O_RDWR | O_CREAT | O_EXCL,S_IRUSR | S_IWUSR, 0) == SEM_FAILED) {
    perror("Failed to create sem consume");
    exit(EXIT_FAILURE);
  }
}

void syn_shm_buffer_destroy(shared_names *names) {
  if (sem_unlink(names->mut_pc_) == -1) {
    perror("Failed to unlink mutex PC");
    exit(EXIT_FAILURE);
  }

  if (sem_unlink(names->sem_produce_) == -1) {
    perror("Failed to unlink sem produce");
    exit(EXIT_FAILURE);
  }

  if (sem_unlink(names->sem_consume_) == -1) {
    perror("Failed to unlink sem consume");
    exit(EXIT_FAILURE);
  }
}
void syn_shm_buffer_open(synchronized_buffer *this, shared_names *names) {
  shm_buffer_open(names, &this->buff_, &this->buff_fd_);
  this->mut_pc_ = sem_open(names->mut_pc_, O_RDWR);
  if (this->mut_pc_ == SEM_FAILED) {
    perror("Failed to open mutex PC");
    exit(EXIT_FAILURE);
  }

  this->sem_produce_ = sem_open(names->sem_produce_, O_RDWR);
  if (this->sem_produce_ == SEM_FAILED) {
    perror("Failed to open sem produce");
    exit(EXIT_FAILURE);
  }

  this->sem_consume_ = sem_open(names->sem_consume_, O_RDWR);
  if (this->sem_produce_ == SEM_FAILED) {
    perror("Failed to open sem consume");
    exit(EXIT_FAILURE);
  }
  
  // Inicializácia mutexu
  pthread_mutex_init(&this->mutex, NULL);
}
void syn_shm_buffer_close(synchronized_buffer *this) {
  shm_buffer_close(this->buff_fd_, this->buff_);

  if (sem_close(this->mut_pc_) == -1) {
    perror("Failed to close mutex PC");
    exit(EXIT_FAILURE);
  }
 
  if (sem_close(this->sem_produce_) == -1) {
    perror("Failed to close sem produce");
    exit(EXIT_FAILURE);
  }

  if (sem_close(this->sem_consume_) == -1) {
    perror("Failed to close mutex sem consume");
    exit(EXIT_FAILURE);
  }

  // Zničenie mutexu
  pthread_mutex_destroy(&this->mutex);
}

void syn_shm_buffer_push(synchronized_buffer *this, const game_action *input) {
  sem_wait(this->sem_produce_);
  sem_wait(this->mut_pc_);

  // Zabezpečenie prístupu k bufferu pomocou mutexu
  pthread_mutex_lock(&this->mutex);  // Zabezpečíme prístup k bufferu

  buffer_push(this->buff_, input);

  pthread_mutex_unlock(&this->mutex);  // Uvoľníme mutex

  sem_post(this->mut_pc_);
  sem_post(this->sem_consume_);
}

void syn_shm_buffer_pop(synchronized_buffer *this, game_action *output) {
  sem_wait(this->sem_consume_);
  sem_wait(this->mut_pc_);

  // Zabezpečenie prístupu k bufferu pomocou mutexu
  pthread_mutex_lock(&this->mutex);  // Zabezpečíme prístup k bufferu

  buffer_pop(this->buff_, output);

  pthread_mutex_unlock(&this->mutex);  // Uvoľníme mutex

  sem_post(this->mut_pc_);
  sem_post(this->sem_produce_);
}
