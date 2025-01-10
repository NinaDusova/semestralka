#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "client.h"
#include "server.h"

char * add_suffix(const char * name, const char * suffix) {
  size_t nameLen = strlen(name);
  size_t suffixLen = strlen(suffix);
  char * result = calloc(nameLen + suffixLen + 2, sizeof(char));
  strcpy(result, name);
  result[nameLen] = '-';
  strcpy(result + nameLen + 1, suffix);
  return result;
}

void clear_names(shared_names * names) {
  free(names->shm_name_);
  free(names->mut_pc_);
  free(names->sem_produce_);
  free(names->sem_consume_);
}

int main(int argc, char *argv[]) {
    const char *user = getenv("USER");

    if (argc < 2) {
    fprintf(stderr, "Je potrebne zadat typ operacie: [init | destroy | producer | consumer]\n");
    return 1;
  }

    shared_names names; 
    names.shm_name_ = add_suffix("DIJKSTRA-SHM", user);
    names.mut_pc_ = add_suffix("MUT-PC", user);
    names.sem_produce_ = add_suffix("SEM-P", user);
    names.sem_consume_ = add_suffix("SEM-C", user);

    if(strcmp(argv[1], "init") == 0) {
        shm_init(&names);
        syn_shm_buffer_init(&names);
    } else if(strcmp(argv[1], "destroy") == 0) {
        shm_destroy(&names);
        syn_shm_buffer_destroy(&names);
    } else if(strcmp(argv[1], "client") == 0) {
        //dispatch_producer(PATH_COUNT, &names);
        //TODO implement producer
        run_client(&names);

    } else if(strcmp(argv[1], "server") == 0) {
        //run_consumer(PATH_COUNT, &names, argv[3]);
        //TODO implement consumer
        run_server(&names);
    } else {
        fprintf(stderr, "Nezadal sa spravny prikaz!\n");
        clear_names(&names);
        return 2;
    }

    clear_names(&names);
    return 0;
}
