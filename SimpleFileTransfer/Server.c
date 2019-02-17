// Including necessary libraries.
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <ctype.h>

#define MAXCONNCOUNT 10

void writeToFile(int, char*, FILE*);

void writeToFile(int socketfd, char* buffer, FILE* filedp){
    
    ssize_t bytesRead;
    
    do{
        bytesRead = read(socketfd, buffer, sizeof(buffer));
        
        if(bytesRead)
            fwrite(buffer, 1, bytesRead, filedp);
    
    }while(bytesRead > 0);
}

int main(int argc, char *argv[]){

    if(argc != 2){
        printf("No port number provided.\n");
        exit(EXIT_FAILURE);
    }

    // Socket descriptor for listening and connection.
    int listenfd, connfd;

    // serverAddr contains data about server such as IP address, port number, and network family.
    // clientAddr contains data about the client such as the IP address, port number, and network family.
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t serverSocketLen = sizeof(serverAddr), clientSocketLen = sizeof(clientAddr);

    char buffer[10];
    char OK ='\0';

    // File descriptor.
    FILE* fdst;

    memset(&serverAddr, '0', sizeof(serverAddr));
    memset(buffer, '0', sizeof(buffer));

    // Setting up the type of network(IPv4), get the address of the server, number of ports.
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons((short)atoi(argv[1]));

    if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        printf("Error creating a socket.\n");
        exit(EXIT_FAILURE);
    }

    if(bind(listenfd, (struct sockaddr*)&serverAddr, serverSocketLen) < 0){
        printf("Port binding error.\n");
        exit(EXIT_FAILURE);
    }

    if(listen(listenfd, MAXCONNCOUNT) < 0){
        printf("Listening error.\n");
        exit(EXIT_FAILURE);
    }

    while(1){

        if((connfd = accept(listenfd, (struct sockaddr*)&clientAddr, &clientSocketLen)) < 0){
            printf("Accept error.\n");
            exit(EXIT_FAILURE);
        }

        printf("Connected to: %s\n", inet_ntoa(clientAddr.sin_addr));
        read(connfd, buffer, sizeof(buffer));

        printf("Downloading file: %s\n", buffer);

        // Send an OK message
        write(connfd, &OK, sizeof(OK));
        fdst = fopen(buffer, "wb");

        if(fdst == NULL) {
            printf("Error opening file.\n");
        }

        // Function that writes data to output file.
        writeToFile(connfd, buffer, fdst);

        printf("Closing file.\n");

        // Closing the file.
        fclose(fdst);

        // Close the connection with the client, however, the server is still on and
        // may connect to other client(s).
        close(connfd);
    }
}

