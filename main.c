#include<stdlib.h>
#include<sys/stat.h>
#include<errno.h>
#include<fcntl.h>

// programul primeste un singur parametru => un fisier
// - spre a fi encriptat, respectiv decriptat
int main(int argc, char *argv[]){

    // realizez citirea fisierului sursa

    // extrag denumirea acestuia din randul argumentelor
    char* src_path = argv[1];

    // folosesc o structura stat pentru a colecta informatii despre src_path
    struct stat stat_buf;
    if(stat(src_path, &stat_buf) < 0){
        // s-a produs o eroare => terminarea procesului
        perror("Stat failed to load");
        return errno;
    }
    
    // deschid fisierul cu permisiunea de read
    int file_descriptor = open(src_path, O_RDONLY);
    if(file_descriptor < 0){
        // eroare la deschidere => terminarea procesului
        perror("Open source file failed");
        return errno;
    }
    
      

        
    
    
    return 0;
}