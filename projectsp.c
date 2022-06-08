#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <stdarg.h>
#include <time.h>

#define BACKLOG 10
#define MAX_SIZE 200
#define BACKLOG 10
#define BUFFSIZE 2048
#define MAXPENDING 5
#define MAX 2048




typedef struct server_arg {
    int portNum;
} server_arg;
typedef struct server_arg1 {
    int portNum;
} server_arg1;


void setup(char inputBuffer[], char *args[], int *background)
{
    const char s[4] = " \t\n";
    char *token;

    token = strtok(inputBuffer, s);
    int i = 0;

    while (token != NULL) {
        args[i] = token;
        i++;
        // printf("%s\n", token);
        token = strtok(NULL, s);
    }
    args[i] = NULL;
}

int open_remote(const char *ip,unsigned short port)
{
    int sock;
    struct sockaddr_in echoserver;


    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        perror("Failed to create socket");
        exit(1);
    }
    int enable = 1;

    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable,
        sizeof(int)) < 0) {
        perror("error");
    }
    memset(&echoserver, 0, sizeof(echoserver));
    echoserver.sin_family = AF_INET;
    echoserver.sin_addr.s_addr = inet_addr(ip);
    echoserver.sin_port = htons(port);


    if (connect(sock, (struct sockaddr *) &echoserver,
        sizeof(echoserver)) < 0) {
        perror("Failed to connect with server");
        exit(1);
    }

    return sock;
}

void *terminal_thread(void *arg)
{


    char *input_command = malloc(MAX_SIZE);
    char buffer[BUFFSIZE];
    int sock_ls = -1;

    while (1) {
       

        printf(">> ");

        fgets(input_command, MAX_SIZE, stdin);


        if ((strlen(input_command) > 0) &&
            (input_command[strlen(input_command) - 1] == '\n'))
            input_command[strlen(input_command) - 1] = '\0';

        input_command[strcspn(input_command,"\n")] = 0;


        char list[] = "ls";
        char cp[] = "cp";


        // exit program (and exit server)
        if (strcmp(input_command,"exit") == 0) {
            if (sock_ls >= 0) {
                if (send(sock_ls,"exit",4,0) < 0) {
                    perror("send/exit");
                    exit(1);
                }
                break;
            }
        }

        if (strcmp(input_command, list) == 0) {
            // ls code will run here

        }

        if ((input_command[0] == '.') && (input_command[1] == 'l')) {
            printf("remote ls\n");
            char ip[20];
            const char c[2] = " ";

            // strcpy(str,input_command);
            char *token;

            // get the first token
            token = strtok(input_command, c);

            // walk through other tokens
            int i = 0;

            while (token != NULL && i != -1) {
                token = strtok(NULL, c);
                i--;
            }


            if (token == NULL) {
                token = "127.0.0.1";
                printf("no IP address found -- using %s\n",token);
            }


            if (sock_ls < 0)
                sock_ls = open_remote(token,9191);

            char s[100];

            strcpy(s, "ls");

            unsigned int echolen;
            echolen = strlen(s);

            int received = 0;

            /* send() from client; */
            if (send(sock_ls, s, echolen, 0) != echolen) {
                perror("Mismatch in number of sent bytes");
            }

            fprintf(stdout, "Message from server: ");

            int bytes = 0;

            /* recv() from server; */
            if ((bytes = recv(sock_ls, buffer, echolen, 0)) < 1) {
                perror("Failed to receive bytes from server");
            }
            received += bytes;
            buffer[bytes] = '\0';
            /* Assure null terminated string */
            fprintf(stdout, buffer);

            bytes = 0;
// this d {...} while block will receive the buffer sent by server
            do {
                buffer[bytes] = '\0';
                printf("%s\n", buffer);
            } while ((bytes = recv(sock_ls, buffer, BUFFSIZE - 1, 0)) >= BUFFSIZE - 1);
            buffer[bytes] = '\0';
            printf("%s\n", buffer);
            printf("\n");

            continue;
        }
    }


    return (void *) 0;
}

int ls_loop(int new_socket)
{

//code for ls

    char buffer[BUFFSIZE];
    int received = -1;
    char data[MAX];

    int stop = 0;

    while (1) {
        memset(data, 0, MAX);
        // this will make server wait for another command to run until it
        // receives exit
        data[0] = '\0';

        if ((received = recv(new_socket, buffer, BUFFSIZE, 0)) < 0) {
            perror("Failed");
        }
        buffer[received] = '\0';

        strcpy(data, buffer);
       
        if (strncmp(data, "exit", 4) == 0) {
            stop = 1;
            break;
        }


        char *args[100];

        setup(data, args, 0);
        int pipefd[2], length;

        if (pipe(pipefd))
            perror("Failed to create pipe");

        pid_t pid = fork();
        char path[MAX];

        if (pid == 0) {

            dup2(pipefd[1], 1);         // duplicate pipfd[1] to stdout
            close(pipefd[0]);           // close the readonly side of the pipe
            close(pipefd[1]);           // close the original write side of the pipe
            execvp(args[0], args);      // finally execute the command
            exit(1);
        }

        if (pid < 0) {
            perror("fork");
            exit(1);
        }


        close(pipefd[1]);

        while (length = read(pipefd[0], path, MAX - 1)) {


            if (send(new_socket, path, length, 0) != length) {
                perror("Failed");
            }

            memset(path, 0, MAX);
        }

        close(pipefd[0]);
    }

    
}

void *server_socket_ls(void *arg)
{

    do {
        server_arg *s = (server_arg *) arg;
        int server_fd, new_socket;
        struct sockaddr_in address;
        int addrlen = sizeof(address);

      

        // Creating socket file descriptor
        if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
            perror("socket failed");
            exit(EXIT_FAILURE);
        }
        int enable = 1;

        if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &enable,
            sizeof(int)) < 0) {
            perror("error");
        }

        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(s->portNum);

        if (bind(server_fd, (struct sockaddr *) &address, sizeof(address))
            < 0) {
            perror("bind failed");
        }

    
        if (listen(server_fd, 3) < 0) {
            perror("listen");
        }

        while (1) {
            if ((new_socket = accept(server_fd, (struct sockaddr *) &address,
                (socklen_t *) & addrlen)) < 0) {
                perror("accept");
            }


            int stop = ls_loop(new_socket);
            close(new_socket);

            if (stop) {
                break;
            }
        }
    } while (0);


    return (void *) 0;
}

void *server_socket_file(void *arg)
{


    server_arg1 *s1 = (server_arg1 *) arg;
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    int enable = 1;

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int))
        < 0) {
        perror("error");
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(s1->portNum);


    if (bind(server_fd, (struct sockaddr *) &address, sizeof(address)) < 0) {
        perror("bind failed");

    }
    if (listen(server_fd, 3) < 0) {
        perror("listen");
    }

    if ((new_socket = accept(server_fd, (struct sockaddr *) &address,
        (socklen_t *) & addrlen)) < 0) {
        perror("accept");
    }

    printf("Server Connected\n");
}

int main(int argc, char const *argv[])
{




    server_arg *s = (server_arg *) malloc(sizeof(server_arg));
    server_arg1 *s1 = (server_arg1 *) malloc(sizeof(server_arg1));
    pthread_t id_1;
    pthread_t id_2;
    pthread_t id_3;

   

    if (pthread_create(&id_3, NULL, terminal_thread, NULL) != 0) {
        perror("pthread_create");
    }

    s->portNum = 9191;
    pthread_create(&id_1, NULL, server_socket_ls, s);

    s1->portNum = 6123;
    if (0)
        pthread_create(&id_2, NULL, server_socket_file, s1);

    pthread_join(id_1, NULL);
    if (0)
        pthread_join(id_2, NULL);
    pthread_join(id_3, NULL);



    return 0;

}
