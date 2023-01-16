#include<stdlib.h>
#include<stdio.h>
#include<sys/stat.h>
#include<errno.h>
#include<fcntl.h>
#include<string.h>
#include<stdio.h>
#include<unistd.h>
#include<sys/mman.h>
#include<sys/wait.h>
#include<time.h>
#include<ctype.h>

#define SIZE 1024
#define offset 200

struct word {
    char ** addrs;
    char * word;
};
typedef struct word word;

// programul primeste un singur parametru => un fisier
// - spre a fi criptat
// programul primeste fisierul criptat, respectiv permutarile folosite
// - returneaza fisierul decriptat (+ sterge fisierul cu permutari)

int getNumber(int *permutation, int length);
void generatePermutation(int * permutation, int length);

int main(int argc, char *argv[]){

    if (argc == 2){
        srand(time(0));
        // programul a primit un singur parametru, un fisier care trebuie criptat
        // criptarea fisierului

        char *src_path = argv[1]; // fisierul transmis ca parametru

        // folosesc o structura stat pentru a colecta informatii despre src_path
        struct stat stat_buf;
        if(stat(src_path, &stat_buf) < 0){
            // s-a produs o eroare => terminarea procesului
            perror("Stat failed to load");
            return errno;
        }

        int SRC_SIZE = stat_buf.st_size; // dimensiunea fisierului

        // deschid fisierul sursa
        int src_fd = open(src_path, O_RDWR);
        if(src_fd < 0){
            perror("Open src error");
            return errno;
        }
        // mapez sursa
        void *src_map = mmap(0, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, src_fd, 0);

        // creez un fisier de memorie partajata pentru a stoca permutarile folosite
        // de catre fiecare proces in parte
        char *shm_keys = "keys_file";

        int dst_key_fd = shm_open(shm_keys, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
        if(dst_key_fd < 0){
            perror("Open shm_keys mem error");
            return errno;
        }
        // def dimensiune
        if(ftruncate(dst_key_fd, SIZE) == -1){
            perror("Truncate error");
            return errno;
        }

        // mapez fisierul de permutari pentru a il putea formata
        void *key_ptr = mmap(0, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, dst_key_fd, 0);

        // citesc datele din fisierul sursa

        // buffer in care voi citi datele din fisierul sursa
        char *buff = (char*) malloc(SRC_SIZE * sizeof(char) + 1);
        if(buff == NULL){
            perror("Malloc failed");
            return errno;
        }
        
        // citirea datelor
        size_t bytes_read = read(src_fd, buff, SRC_SIZE);
        if(bytes_read < 0){
            perror("Read failed");
            return errno;
        }
        // ma asigur ca datele din fisier au fost corect citite
        for(size_t index = bytes_read; index < SRC_SIZE; index += bytes_read){
            bytes_read = read(src_fd, buff + index, SRC_SIZE - index);
            if(bytes_read < 0){
                perror("Read failed");
                return errno;
            }
        }
        // parcurg bufferul si extrag toate cuvintele in vectorul de cuvinte

        word words[1000];

        int word_index = 0;
        char * src_ptr = strtok(buff, "\n "); // adaugarea mai multor semne de punctuatie 

        while(src_ptr != NULL){
            word word_location;

            word_location.word = src_ptr;
            word_location.addrs = &src_ptr;

            words[word_index++] = word_location;
            src_ptr = strtok(0, "\n ");
        }
        int word_count = word_index;

        // printf("%d\n", word_count);

        // for(int i=0; i<word_count; i++)
        //     printf("%s\n", words[i]);


        // pentru fiecare cuvant
        // generez o permutare si realizez criptarea acestuia
        // salvez permutarea in shm_keys
        pid_t encriptor_pid;
        for(int index = 0; index < word_count; index ++){
            
            encriptor_pid = fork();

            if(encriptor_pid < 0){
                perror("Encription - Fork error");
                return errno;
            }
            else if(encriptor_pid == 0){
                int word_length = strlen(words[index].word); // lungimea cuvantului curent

                char * word = (char *) malloc(word_length * sizeof(char));
                word = words[index].word; //cuvantul curent
                // generez o permutare

                int *permutation = (int*)malloc(sizeof(int) * word_length);
                // initializez permutarea                 
                for(int index = 0; index < word_length; index++){
                    permutation[index] = index;
                }
                generatePermutation(permutation, word_length);
                // for(int i=0; i<word_length; i++)
                //     printf("%d ", permutation[i]);
                // printf("\n");
                // scriu permutarea in fisierul shm_keys
                int offset_number = 0;
                for(int i = 0; i < word_length; i++){
                    sprintf(key_ptr + (offset * index) + offset_number, "%d ", permutation[i]);
                    offset_number+=2;
                }
                sprintf(key_ptr + (offset * index) + offset_number, "\n");

                // modific cuvantul
                char * modified_word = (char *)malloc(sizeof(char) * word_length);
                for(int letter = 0; letter < word_length; letter++)
                    modified_word[letter] = word[permutation[letter]];
                    
                sprintf(src_map + (offset * index), "%s", modified_word);
                return 0;
            }
        }
        for(int index = 0; index < word_count; index ++)
            wait(NULL);

        return 0;
    
    }
    else if (argc == 3){
        // decriptarea fisierului
        return 0;
    }
    else{
        // apel invalid
        printf("Nr de argumente invalid\n");

        return -1;
    }
    return 0;
}
int getNumber(int * permutation, int length){
    // algoritmul fisher-yates
    // extrag un numar random de la 0-length
    int index = rand() % length;
    // si aleg numarul din permutare care se gaseste la indexul random extras
    int number = permutation[index];

    // salvez ultimul nr din vector, pe pozitia random aleasa
    permutation[index] = permutation[length - 1];
    // returnez numarul care se afla initial pe pozitia random
    // numar care este sters din array
    // pentru a nu fi ales din nou
    return number;
}
void generatePermutation(int * permutation, int length){
    // generator de permutari in sigma(lungimea cuvantului)

    int word_length = length;

    while(length > 0){
        // cu fiecare iteratie, salvez la adresa ultimului numar din array un numar random
        // si continui sa modific bucata de array ramasa
        
        permutation[length - 1] = getNumber(permutation, length);
        length = length - 1;
    }
    // <=> este realizat un swap intre numarul random ales si ultimul numar din array
}
// // fisierul destinatie, folosind denumirea unica a fisierului sursa, filename.out
            // char *dst_source = strcat(strtok(src_path, "."), ".out");

            // int dst_fd = open(dst_source, O_RDWR | O_CREAT, S_IRWXU);
            // if(dst_fd < 0){
            //     perror("Open destination file failed");
            //     return errno;
            // }

            // fisierul cu permutarile folosite in codarea cuvintelor, filename_key.out
            //char *dst_key = strcat(strtok(src_path, "."), "_key.out");

            //int dst_key_fd = open(dst_key, O_RDWR | O_CREAT, S_IRWXU);
            // if(dst_key_fd < 0){
            //     perror("Open destination key file failed");
            //     return errno;
            // }