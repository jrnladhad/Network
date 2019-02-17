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

// Structure for the header of the packet.
typedef struct{
    int seqACK;
    int length;
    int checkSum;
}HEADER;

// Structure of packet with header and data.
typedef struct{
    HEADER header;
    char data[10];
}PACKET;

// Calculate the checksum.
int checksum(PACKET* pkt, size_t size)
{
    pkt->header.checkSum = 0;

    char* head = (char*)pkt;
    char sum = head[0];

    int i;
    for(i=1; i<size; i++)
        sum ^= head[i];

    return (int)sum;
}

// Custom string copy function to copy data from received packet to local data buffer.
void stringCopy(char* string1, const char* string2, int len){
    int i;
    for(i = 0; i < len; i++)
        string1[i] = string2[i];

}


int main(int argc, char *argv[]){
    // Socket descriptor for connecting to server.
    int sock;

    ssize_t nBytes;

    // serverAddr contains data about server such as IP address, port number, and network family.
    // clientAddr contains data about the client such as the IP address, port number, and network family.
    struct sockaddr_in servAddr, clientAddr;
    struct sockaddr_storage servStorage;
    socklen_t addrSize, clientAddrSize;

    PACKET buf;
    PACKET resp;
    PACKET* buffer = &buf;
    PACKET* response = &resp;

    if(argc != 2){
        perror("Error: ");
        exit(EXIT_FAILURE);
    }

    // Setting up the socket requirements.
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons((short)atoi(argv[1]));
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    memset ((char *)servAddr.sin_zero, '\0', sizeof (servAddr.sin_zero));
    addrSize = sizeof(servStorage);


    // Creating socket for server.
    if((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
        perror("Error: ");
        exit(EXIT_FAILURE);
    }

    // Binding the socket to a port.
    if(bind(sock, (struct sockaddr*) &servAddr, sizeof(servAddr)) < 0){
        perror("Error: ");
        exit(EXIT_FAILURE);
    }

    // Local variables to store length of received packet, sequenceACK, and data.
    int len;
    int seq_ack;
    int pkt_checksum;
    char data[10];

    // Detects if the packet contains file name or data to be written to file.
    int gotFileName = 1;

    FILE* output;

    do{
        nBytes = recvfrom(sock, buffer, sizeof(PACKET), 0, (struct sockaddr*)&clientAddr, &addrSize);
        printf("Connected to: %s\n", inet_ntoa(clientAddr.sin_addr));
        seq_ack = buffer->header.seqACK;
        len = buffer->header.length;

        // Custom function to copy data from packet to local data buffer.
        stringCopy(data, buffer->data, len);

        pkt_checksum = buffer->header.checkSum;
        buffer->header.checkSum = 0;

        // If there is a bit error in packet data then request to resend the packet.
        if(checksum(buffer, sizeof(PACKET)) != pkt_checksum){
            response->header.seqACK = (seq_ack + 1) % 2;
            sendto(sock, response, sizeof(PACKET), 0, (struct sockaddr *)&clientAddr, addrSize);
            continue;
        }

        // Check if output file name is received.
        if(gotFileName){
            gotFileName = 0;
            if((output = fopen(data, "wb")) < 0){
                perror("Error: ");
                exit(EXIT_FAILURE);
            }

        }
        else
            fwrite(data, sizeof(char), (size_t)len, output);

        response->header.seqACK = seq_ack;
        sendto(sock, response, sizeof(PACKET), 0, (struct sockaddr *)&clientAddr, addrSize);

    }while(len > 0);

    fclose(output);

    return 0;
}