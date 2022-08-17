// asgn3

#include <err.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

// my own includes and define
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include "queue.h"

# define BUFF_SIZE 4096
// working with erno
// https://stackoverflow.com/questions/11396589/why-does-open-fail-and-errno-is-not-set
extern int errno; 

// mutexes cond vars
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

/**
   Converts a string to an 16 bits unsigned integer.
   Returns 0 if the string is malformed or out of the range.
 */
uint16_t strtouint16(char number[]) {
  char *last;
  long num = strtol(number, &last, 10);
  if (num <= 0 || num > UINT16_MAX || *last != '\0') {
    return 0;
  }
  return num;
}

/**
   Creates a socket for listening for connections.
   Closes the program and prints an error message on error.
 */
int create_listen_socket(uint16_t port) {
  struct sockaddr_in addr;
  int listenfd = socket(AF_INET, SOCK_STREAM, 0);
  if (listenfd < 0) {
    err(EXIT_FAILURE, "socket error");
  }

  memset(&addr, 0, sizeof addr);
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htons(INADDR_ANY);
  addr.sin_port = htons(port);
  if (bind(listenfd, (struct sockaddr*)&addr, sizeof addr) < 0) {
    err(EXIT_FAILURE, "bind error");
  }

  if (listen(listenfd, 500) < 0) {
    err(EXIT_FAILURE, "listen error");
  }

  return listenfd;
}

// used example client.c that was given
/**
   Creates a socket for connecting to a server running on the same
   computer, listening on the specified port number.  Returns the
   socket file descriptor on succes.  On failure, returns -1 and sets
   errno appropriately.
 */
int create_client_socket(uint16_t port) {
  int clientfd = socket(AF_INET, SOCK_STREAM, 0);
  if (clientfd < 0) {
    return -1;
  }

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof addr);
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(port);
  if (connect(clientfd, (struct sockaddr*) &addr, sizeof addr)) {
    return -1;
  }
  return clientfd;
}
// end of client.c code

// check for validity of objName
// 0 = false, 1 = true
int isValidObj(char *objName) {
    // get rid of the '/'
    objName = objName + 1;
    if(strlen(objName) > 20) {
        return 0;
    }
    for(size_t i=0; i < strlen(objName); i++) {
        if(isalpha(objName[i]) == 0 && isdigit(objName[i]) == 0) {
            // || objName[i] == '-' <- dash condition to pass ?
            if(objName[i] == '.' || objName[i] == '_' || objName[i] == '-' ) {
                continue; // skip over any '.' or '_'
            }
            return 0;
        }
    }

    return 1;
}

// handle GET request
void handle_get(int connfd, char *objName) {
    //printf("inside handle_get()\n");
    // based on testing format of objName and ver:
    // "/asdfadsf"
    // remove the "/" in front of the objName
    objName = objName + 1;

    // @@ REMINDER TO FREE
    // create a buffer for the response
    char *resp = (char *)malloc(BUFF_SIZE);
    uint8_t *fileBuff = (uint8_t *)malloc(BUFF_SIZE);
    
    // Attempt to open the file
    int openFile = open(objName, O_RDONLY);
    if (openFile < 0) {
        fprintf( stderr, "errno is %d\n", errno );
        if(errno == 13) {
            sprintf(resp, "HTTP/1.1 403 Forbidden\r\nContent-Length: 10\r\n\r\nForbidden\n");
            send(connfd, resp, strlen(resp), 0);
        }
        if(errno == 2) {
            sprintf(resp, "HTTP/1.1 404 File Not Found\r\nContent-Length: 15\r\n\r\nFile Not Found\n");
            send(connfd, resp, strlen(resp), 0);
        }
    }
    else {
        // use stat to get file properties
        struct stat st;
        int result;
        result = stat(objName, &st);

        // no access
        if(result == -1) {
            sprintf(resp, "HTTP/1.1 403 Forbidden\r\nContent-Length: 10\r\n\r\nForbidden\n");
            send(connfd, resp, strlen(resp), 0);
        } 
        else {
            // could continually send via buffer size but i think it is 
            // easier to just resize the buffer even though it will take
            // more memory

            // get the size of the file
            int fileSize = st.st_size;
            // buffer may not be big enough to hold everything
            if(fileSize > BUFF_SIZE) {
                fileBuff = (uint8_t *)realloc(fileBuff, fileSize);
            }

            // theoretically should get the file into buffer
            int amtRead = read(openFile, fileBuff, fileSize);

            // print out the fileBufer
            //printBuff(fileBuff, "GET_fileBuff", fileSize);

            // after printing buffer realized i need null terminator
            //fileBuff[amtRead] = '\0';

            // successfully read file
            if(amtRead > 0) {
                sprintf(resp, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\n\r\n", fileSize);
                send(connfd, resp, strlen(resp), 0);
                send(connfd, fileBuff, fileSize, 0);
            }
            else {
                sprintf(resp, "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 22\r\n\r\nInternal Server Error\n");
                send(connfd, resp, strlen(resp), 0);
            }
        }
    }
    
    // free malloc'd memory
    free(resp);
    free(fileBuff);
    // always close file descriptors
    close(openFile);
}

void print_buffer(char* buffer, char* name) {
    printf("printing %s\n", name);
    for(int i=0; buffer[i] != '\0'; i++) {
        printf("%c", buffer[i]);
    }
    printf("\n");
}

void* handle_connection(void *p_connfd) {
    int connfd = *((int*)p_connfd);
    free(p_connfd); // don't need this

    // allocate buffer
    char *readBuff = (char *)malloc(BUFF_SIZE);
    char *resp = (char *)malloc(BUFF_SIZE);
    // amount of bytes read from buffer
    // using 4096-1 ensures that we find the end of the buffer
    int amtRead = 0;

    while( (amtRead = recv(connfd, readBuff, BUFF_SIZE-1, 0)) > 0) {
        char method[5]; // chose 5 as we can only have GET, PUT, HEAD
        char objName[100]; // chose 20 as max can be 19 chars
        char httpver[9]; // http/1.1
       
        // parse the method, objName, http version
        sscanf(readBuff, "%s %s %s", method, objName, httpver);
        //printf("method: |%s| objName: |%s| httpver: |%s|\n", method, objName, httpver);

        // check for valid HTTP version
        if(strcmp(httpver, "HTTP/1.1") != 0) {
            sprintf(resp, "HTTP/1.1 400 Bad Request\r\nContent-Length: 12\r\n\r\nBad Request\n");
            send(connfd, resp, strlen(resp), 0);
            break;
        }

        // check if objName has all valid chars
        if(isValidObj(objName) == 0) {
            sprintf(resp, "HTTP/1.1 400 Bad Request\r\nContent-Length: 12\r\n\r\nBad Request\n");
            send(connfd, resp, strlen(resp), 0);
            break;
        }

        // move past the request method, object name, http version
        readBuff = readBuff + strlen(method) + 1 + strlen(objName) + 1 + strlen(httpver) + 2;
        // readBuff is now starting at "Host:..."

        // ignoring any potential errors right now
        // compares request method to string
        if(strcmp(method, "GET") == 0) {
            //printf("IN GET REQUEST\n");
            handle_get(connfd, objName);
        }
        else {
            sprintf(resp, "HTTP/1.1 501 Not Implemented\r\nContent-Length: 16\r\n\r\nNot Implemented\n");
            send(connfd, resp, strlen(resp), 0);
            break;
        }
    }

    //printf("bottom of handle_conn\n");
    // was getting seg fault from freeing, not sure why?
    //free(readBuff);
    // when done, close socket
    close(connfd);
    return NULL;
}

void *thread_function() {
    while(1) {
        int *qconn;

        // lock mutex and wait on condition variable
        pthread_mutex_lock(&mutex);
        
        // only wait until work cant be done from queue
        if( (qconn = dequeue()) == NULL) {
            pthread_cond_wait(&cond, &mutex);
            // request work again
            qconn = dequeue();
        }

        //unlock thread
        pthread_mutex_unlock(&mutex);

        if(qconn != NULL) {
            handle_connection(qconn);
        }
    }
}


int main(int argc, char *argv[]) {
    int listenfd;
    if (argc < 3) {
        errx(EXIT_FAILURE, "wrong arguments: %s port_num server_ports [optional: -N -R -s -m]\n", argv[0]);
    }
    
    int ports[6];
    // based on example from: 
    // https://www.gnu.org/software/libc/manual/html_node/Example-of-Getopt.html
    int c;
    uint16_t nThreads = 5; 
    int opt_seen = 0; // keeps track of how many parameters we have seen
    char *temp;
    int temp_num; 
    int req_num = 5; // number of requests before health check
    int cache_cap = 3; // size of the cache
    int max_fs = 1024; // maximum file size 
    int *cachefl = malloc(sizeof(int)); // flag to check if caching is on
    *cachefl = 1; // assume caching is enabled

    while((c = getopt(argc, argv, "N:R:s:m")) != -1) {
        switch(c) {
            case 'N':
                temp = optarg;
                for(size_t i=0; i < strlen(temp); i++) {
                    if(!isdigit(temp[i])) { // check if it's actually all digits
                        errx(EXIT_FAILURE, "numThreads is not an integer"); 
                    }
                }
                if( (strtouint16(temp)) <= 0) { // check if port is 0
                    errx(EXIT_FAILURE, "Invalid thread number, choose positive number");
                }
                nThreads = strtouint16(temp); // assign new value to nThreads
                break;
            case 'R':
                temp = optarg;
                if( (strtouint16(temp)) <= 0) { // check if conversion is 0
                    errx(EXIT_FAILURE, "Invalid thread number, choose positive number");
                }
                temp_num = strtouint16(temp);
                if(temp_num < 0) {
                    errx(EXIT_FAILURE, "Number of requests is a negative number");
                }
                else {
                    req_num = temp_num; // should be a positive number
                }
                break;
            case 's':
                temp = optarg;
                if( (strtouint16(temp)) <= 0) { // check if conversion is 0
                    errx(EXIT_FAILURE, "Invalid thread number, choose positive number");
                }
                temp_num = strtouint16(temp);
                if(temp_num < 0) {
                    errx(EXIT_FAILURE, "Capacity is a negative number");
                }
                else {
                    cache_cap = temp_num; // should be a positive number
                }
                break;
            case 'm':
                temp = optarg;
                if( (strtouint16(temp)) <= 0) { // check if conversion is 0
                    errx(EXIT_FAILURE, "Invalid thread number, choose positive number");
                }
                temp_num = strtouint16(temp);
                if(temp_num < 0) {
                    errx(EXIT_FAILURE, "Max file size is a negative number");
                }
                else {
                    max_fs = temp_num; // should be a positive number
                }
                break;
            case '?':
                errx(EXIT_FAILURE, "Invalid flag");
                break;
            default:
                break;
        }
    }

    if(cache_cap == 0 && max_fs == 0) {
        cachefl = 0;
    }

    temp_num = 0; // reset temp_num to 0 to use as counter
    int proxy_port = 0; // holds proxy port number
    for(opt_seen = optind; opt_seen < argc; opt_seen++) {
        //printf ("Non-option argument %s\n", argv[opt_seen]);
        ports[temp_num] = strtouint16(argv[opt_seen]);
        if (ports[temp_num] == 0) {
            errx(EXIT_FAILURE, "invalid port number\n");
        }
        temp_num++;
    }

    // get proxy port into 
    proxy_port = ports[0];
    int serv1_port = ports[1];
    int serv2_port = ports[2];
    int serv3_port = ports[3];
    int serv4_port = ports[4];
    if(serv1_port > 0 || serv2_port > 0|| serv3_port > 0 || serv4_port > 0 || req_num > 0) {
        // get rid of warning
    }


    //  printf("opt_seen: %d\n", opt_seen);
    //  printf("proxy port: %d\n", proxy_port);
    //  printf("nThreads: %d\n", nThreads);
    //  printf("req_num: %d\n", req_num);
    //  printf("caching: %d\n", *cachefl);
    //  printf("serv1_port: %d\n", ports[1]);
    //  printf("serv2_port: %d\n", ports[2]);
    //  printf("serv3_port: %d\n", ports[3]);
    //  printf("serv4_port: %d\n", ports[4]);


    // initialize thread pool with number of threads
    pthread_t thread_pool[nThreads];

    // create threads
    for(int i=0; i < nThreads; i++) {
        pthread_create(&thread_pool[i], NULL, thread_function, NULL);
    }

    listenfd = create_listen_socket(proxy_port);

    while(1) {
        int connfd = accept(listenfd, NULL, NULL);
        if (connfd < 0) {
            warn("accept error");
            continue;
        }

        // put threads in queue

        int *t_connfd = malloc(sizeof(int));
        *t_connfd = connfd;
        pthread_mutex_lock(&mutex);
        enqueue(t_connfd, cachefl);
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&mutex);

    }
    return EXIT_SUCCESS;
}
