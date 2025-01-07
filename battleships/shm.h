#ifndef SHM 
#define SHM
#include "names.h"
#include "buffer.h"

void shm_init(shared_names *names);
void shm_destroy(shared_names *names);
void shm_buffer_open(shared_names *names, buffer **out_buff, int *out_fd_shm);
void shm_buffer_close(int fd_shm, buffer *buff);


#endif // !SHM 
