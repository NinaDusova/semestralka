#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "shm.h"

void shm_init(shared_names *names) {
  const int fd_shm = shm_open(names->shm_name_, O_RDWR | O_CREAT | O_EXCL,S_IRUSR | S_IWUSR);
  if (fd_shm == -1) {
    perror("Failed to create shared memory");
    exit(EXIT_FAILURE);
  }
  
  if (ftruncate(fd_shm, sizeof(buffer)) == -1) {
    perror("Failed to truncate shared memory");
    exit(EXIT_FAILURE);
  }
  buffer *buff = mmap(NULL, sizeof(buffer), PROT_READ | PROT_WRITE,MAP_SHARED, fd_shm, 0);
  buffer_init(buff);
  shm_buffer_close(fd_shm, buff);
}

void shm_destroy(shared_names *names) {
  if (shm_unlink(names->shm_name_) == -1) {
    perror("Failed to unlink shared memory");
    exit(EXIT_FAILURE);
  }
}

void shm_buffer_open(shared_names *names,buffer **out_buff, int *out_fd_shm) {
  const int fd_shm = shm_open(names->shm_name_, O_RDWR, 0);
  if (fd_shm == -1) {
    perror("Failed to open shared memory");
    exit(EXIT_FAILURE);
  }

  buffer *buff = mmap(NULL, sizeof(buffer), PROT_READ | PROT_WRITE,MAP_SHARED, fd_shm, 0);
  if (buff == MAP_FAILED) {
    perror("Failed to map shared memory");
    exit(EXIT_FAILURE);
  }
  *out_fd_shm = fd_shm;
  *out_buff = buff;
}

void shm_buffer_close(int fd_shm, buffer *buff) {
  if (munmap(buff, sizeof(buffer)) == -1) {
    perror("Failed to unmap shared memory");
    exit(EXIT_FAILURE);
  }

  if (close(fd_shm) == -1) {
    perror("Failed to close shared memory");
    exit(EXIT_FAILURE);
  }
}

