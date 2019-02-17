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
#include <fcntl.h>
#include <sys/time.h>
#include <sys/select.h>

#define BOGUSRANDOMLEVEL 90

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

// Sends output file name.
void sendOutputFileName(PACKET* buffer, char* fileName){
    strcpy(buffer->data, fileName);
    buffer->header.length = strlen(fileName);
}

void sendBogusPacket(PACKET* buffer){
    if(rand() % 100 < BOGUSRANDOMLEVEL){
        buffer->header.checkSum = 0;
        printf("Sending bogus packet.\n");
    }
}

int main(int argc, char *argv[]){
    // Necessary variables.
    int sock;
    char data[10];
    struct sockaddr_in servAddr;
    socklen_t addrSize;

    PACKET buff;
    PACKET resp;
    PACKET *buffer = &buff;
    PACKET *response = &resp;

    if(argc != 5){
        perror("Error: ");
        exit(EXIT_FAILURE);
    }

    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(atoi(argv[4]));

    inet_pton(AF_INET, argv[3], &servAddr.sin_addr.s_addr);
    memset (servAddr.sin_zero, '\0', sizeof (servAddr.sin_zero));
    addrSize = sizeof(servAddr);

    if((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
        perror("Error: ");
        exit(EXIT_FAILURE);
    }

    struct timeval tv;
    int rv;
    fd_set readfds;
    fcntl(sock, F_SETFL, O_NONBLOCK);

    FILE* input;

    if((input = fopen(argv[1],"rb")) == NULL ){
        perror("Error: ");
        exit(EXIT_FAILURE);
    }

    int first = 1;
    int state = 0;
    int server_ack;
    int len;

    do{

        if(first)
            sendOutputFileName(buffer, argv[2]);
        else{
            len = fread(data, sizeof(char), 10, input);
            strcpy(buffer->data, data);
            buffer->header.length = len;
        }

        buffer->header.seqACK = state;
        printf("Sequnce of packet: %d\n", state);

        do{

            // Set the checksum for the packet.
            buffer->header.checkSum = 0;
            buffer->header.checkSum = checksum(buffer, sizeof(PACKET));

            // Send packet with checksum = 0.
            sendBogusPacket(buffer);

            sendto(sock, buffer, sizeof(PACKET), 0, (struct sockaddr*) &servAddr, addrSize);

            // Watch the socket file descriptor sock to see when it has inputs.
            FD_ZERO(&readfds);
            FD_SET(sock, &readfds);

            // Wait up to 1 sec.
            tv.tv_sec = 1;
            tv.tv_usec = 0;

            rv = select(sock+1 , &readfds, NULL, NULL, &tv);

            // See if there was a timeout for an ACK from the server.
            if(rv)
                recvfrom(sock, response, sizeof(PACKET), 0, NULL, NULL);
            else{
                printf("Packet drop detected. Resending the packet.\n");
                continue;
            }

        }while(response->header.seqACK != state);

        first = 0;
        state = (state + 1) % 2;

    }while(!feof(input));

    fclose(input);

    // Last packet to determine the end of communication.
    buffer->header.seqACK = state;
    buffer->header.length = 0;
    buffer->header.checkSum = 0;
    buffer->header.checkSum = checksum(buffer,sizeof(PACKET));
    sendto(sock, buffer, sizeof(PACKET), 0, (struct sockaddr *)&servAddr, addrSize);

    return 0;
}
