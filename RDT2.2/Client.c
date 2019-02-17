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

#define BOGUSRANDOMLEVEL 60

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

    FILE* input;

    if((input = fopen(argv[1],"rb")) == NULL ){
        perror("Error: ");
        exit(EXIT_FAILURE);
    }

    int sentOutputFileName = 0;
    int state = 0;
    int len;

    do{
        // Send either output file name or file data.
        if(sentOutputFileName == 0)
            sendOutputFileName(buffer, argv[2]);
        else{
            len = (int)fread(data, sizeof(char), 10, input);
            strcpy(buffer->data, data);
            buffer->header.length = len;
        }

        // Detects the state of the packet.
        buffer->header.seqACK = state;

        // Calculate the checksum and send the packet.
        do{
            buffer->header.checkSum = 0;
            buffer->header.checkSum = checksum(buffer, sizeof(PACKET));

            sendBogusPacket(buffer);

            sendto(sock, buffer, sizeof(PACKET), 0, (struct sockaddr*) &servAddr, addrSize);
            printf("Packet sent.\n");
            recvfrom(sock, response, sizeof(PACKET), 0, NULL, NULL);

        }while(response->header.seqACK != state);

        sentOutputFileName = 1;
        printf("Valid ACK received.\n");

        // Change the state to either 0 or 1.
        state ^= state;

    }while(!feof(input));

    fclose(input);

    // Last packet to determine the end of communication.
    buffer->header.seqACK = state;
    buffer->header.length = 0;
    buffer->header.checkSum = 0;
    buffer->header.checkSum = checksum(buffer,sizeof(PACKET));
    sendto (sock, buffer, sizeof(PACKET), 0, (struct sockaddr *)&servAddr, addrSize);

    return 0;
}