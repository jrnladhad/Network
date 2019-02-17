// Including necessary library headers.
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>

int main(int argc, char* argv[]){

    if(argc != 5){
        printf("Need input file, output file, IP address, and port number. ");
        exit(EXIT_FAILURE);
    }

    // Necessary variables.
    int socketfd;
    char buffer[10] = { 0 }, fileName[10] = {0};
    struct sockaddr_in servaddr;
    size_t bytesRead;

    // Storing the output file name.
    strcpy(fileName, argv[2]);
    memset(&servaddr, 0, sizeof(servaddr));

    if((socketfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        printf("Error creating a socket.\n");
        exit(EXIT_FAILURE);
    }

    // Setting the family type(IPv4) of the network and port number of server.
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons((short)atoi(argv[4]));

    // Setting IP address of the server.
    if(inet_pton(AF_INET, argv[3], &servaddr.sin_addr) <= 0){
        printf("Error attaching server IP address.\n");
        exit(EXIT_FAILURE);
    }

    printf("Connecting to IP address: %s on port number: %s\n", argv[3], argv[4]);

    // Connecting to the server.
    if(connect(socketfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0){
        printf("Error connecting.\n");
        exit(EXIT_FAILURE);
    }

    printf("Connected to IP address: %s on port number: %s\n", argv[3], argv[4]);

    // Sending the output file name to the server.
    write(socketfd, fileName, sizeof(fileName));

    printf("Uploading file: %s.\n", fileName);

    read(socketfd, buffer, sizeof(buffer));

    // Confirmation for the receiving and opening of file on server
    if(buffer[0] == '\0'){

        printf("Opening input file.\n");
        FILE* fsrc = fopen(argv[1], "rb");

        printf("Opened input file.\n");
        printf("Sending data.\n");

        // Read from the input file and send data to server.
        do{
            memset(buffer, 0, sizeof(buffer));
            bytesRead = fread(buffer, 1, sizeof(buffer), fsrc);
            if(bytesRead)
                write(socketfd, buffer, bytesRead);
        }while(bytesRead > 0);

        // Close the input file.
        fclose(fsrc);
    }

    // Close the client socket.
    close(socketfd);

    return 0;
}