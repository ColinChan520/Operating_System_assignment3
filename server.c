#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h> // 包含inet_ntoa函数的头文件
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>

#define MYPORT 3456
#define BACKLOG 10
#define MAX_CLIENTS 100

struct Client {
    int socket;
    char buffer[256];
};

struct Client clients[MAX_CLIENTS];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int client_count = 0;

void* handle_client(void* arg) {
    int client_index = *(int*)arg;
    int client_socket = clients[client_index].socket;
    char* buffer = clients[client_index].buffer;

    fcntl(client_socket, F_SETFL, O_NONBLOCK);

    while (1) {
        int n = recv(client_socket, buffer, sizeof(buffer), 0);
        if (n < 0) {
            // Handle error
            perror("recv");
        } else if (n == 0) {
            // Client disconnected
            printf("Client %d disconnected\n", client_index);
            break;
        } else {
            buffer[n] = '\0';
            printf("Received from client %d: %s\n", client_index, buffer);
            // Handle received data

            // Example: send(client_socket, response, strlen(response), 0);
        }
    }

    close(client_socket);
    pthread_mutex_lock(&mutex);
    client_count--;
    pthread_mutex_unlock(&mutex);
    pthread_exit(NULL);
}

int main() {
    int sockfd, new_fd; // 声明sockfd
    struct sockaddr_in my_addr;
    struct sockaddr_in client_addr;
    socklen_t sin_size;

    // 创建套接字，绑定端口，监听连接
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("socket");
        exit(1);
    }

    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(MYPORT);
    my_addr.sin_addr.s_addr = INADDR_ANY;
    bzero(&(my_addr.sin_zero), 8);

    if (bind(sockfd, (struct sockaddr*)&my_addr, sizeof(struct sockaddr)) == -1) {
        perror("bind");
        exit(1);
    }

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    printf("Server started. Listening on port %d...\n", MYPORT);

    while (1) {
        sin_size = sizeof(struct sockaddr_in);
        new_fd = accept(sockfd, (struct sockaddr*)&client_addr, &sin_size);
        if (new_fd < 0) {
            perror("accept");
            continue;
        }

        printf("Server: got connection from %s\n", inet_ntoa(client_addr.sin_addr));

        pthread_mutex_lock(&mutex);
        if (client_count >= MAX_CLIENTS) {
            printf("Too many clients. Connection rejected.\n");
            close(new_fd);
        } else {
            clients[client_count].socket = new_fd;
            pthread_t thread;
            pthread_create(&thread, NULL, handle_client, (void*)&client_count);
            client_count++;
        }
        pthread_mutex_unlock(&mutex);
    }

    close(sockfd);
    return 0;
}