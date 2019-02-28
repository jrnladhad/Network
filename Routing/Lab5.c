//  Including necessary libraries.
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
#include <sys/time.h>
#include <limits.h>
#include <pthread.h>

// Constants for timing and the number of machines to be in the network.
#define updateMinWait 10
#define updateMaxWait 20
#define maxMachines 4

// Structure for machine information.
typedef struct{
    char name[50];
    char ip[50];
    int port;
}MACHINE;

int N = maxMachines;
int costs[100][100];
// Information about my router.
int myID, myIP, myPort, mySock;

int inData[3];
int outData[3];

MACHINE hosts[100];

// Mutex for locking various threads, namely, receiver thread and link state thread.
pthread_mutex_t lock;

// Function prototypes.
void parseFiles(FILE* fCosts, FILE* fHosts);
void *receiveUpdates(void* nameless);
void *linkState(void* nameless);
void sendData();
int receiveData(int port);
void userInputCost();
int minDistance(const int dist[], const int visited[]);

// Parse files containing the cost table for links between each router.
void parseFiles(FILE* fCosts, FILE* fHosts){
    int i, j, n;
    printf("Parsing costs file: \n");
    for(i=0; i< N; i++){
        for(j=0; j< N; j++){
            if((n =fscanf(fCosts, "%d", &costs[i][j])) != 1)
                break;
            printf("%d ", costs[i][j]);
        }
        printf("\n");
    }

    printf("Parsing hosts files: \n");
    for(i=0; i< N; i++){
        if((n = fscanf(fHosts, "%s %s %d", hosts[i].name, hosts[i].ip, &(hosts[i].port))) < 1)
            break;
        printf("%s %s %d\n", hosts[i].name, hosts[i].ip, hosts[i].port);
    }
}

// Receive data about any changes in cost for the routers.
int receiveData(int port)
{

    int nBytes = recvfrom (mySock, &inData, sizeof(inData), 0, NULL,NULL);
    printf("Received update\n");

    return 0;
}

// Send the updated data to all the routers in the network for any change sensed in the cots.
void sendData(){
    int sock;
    struct sockaddr_in destAddr[N];
    socklen_t addrSize[N];

    int i;
    for(i=0; i< N; i++){
        destAddr[i].sin_family = AF_INET;
        destAddr[i].sin_port = htons (hosts[i].port);
        inet_pton (AF_INET, hosts[i].ip, &destAddr[i].sin_addr.s_addr);
        memset (destAddr[i].sin_zero, '\0', sizeof (destAddr[i].sin_zero));
        addrSize[i] = sizeof destAddr[i];
    }

    if((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
        perror("Error: ");
        exit(EXIT_FAILURE);
    }

    for(i =0; i < N; i++){
        if(i != myID)
            sendto(sock, &outData, sizeof(outData), 0, (struct sockaddr*)&destAddr[i], addrSize[i]);
    }

}

// Update the cost table from the received information.
void *receiveUpdates(void* nameless){
    while(1){

        receiveData(myPort);

        int host1 = ntohl(inData[0]);
        int host2 = ntohl(inData[1]);
        int weight = ntohl(inData[2]);

        pthread_mutex_lock(&lock);
        costs[host1][host2] = weight;
        costs[host2][host1] = weight;

        int i, j;
        for(i=0; i< N; i++){
            for(j=0; j<N; j++){
                printf("%d ", costs[i][j]);
            }

            printf("\n");
        }

        pthread_mutex_unlock(&lock);

        sleep(3);
    }
}

// Detect an user-input change for the cost of any router.
void userInputCost(){
    int neighbor;
    int newCost;

    printf("Update neighbor cost from node %d, format <neighbor> <new cost>:\n", myID);

    scanf("%d %d", &neighbor, &newCost);

    pthread_mutex_lock(&lock);
    costs[myID][neighbor] = newCost;
    costs[neighbor][myID] = newCost;
    outData[0] = htonl(myID);
    outData[1] = htonl(neighbor);
    outData[2] = htonl(newCost);
    sendData();

    printf("New matrix after user input: \n");
    int i, j;
    for(i=0; i< N; i++){
        for(j=0; j<N; j++)
            printf("%d ", costs[i][j]);
        printf("\n");
    }

    pthread_mutex_unlock(&lock);
}

// Know the minimum distance for the neighboring neighbors.
int minDistance(const int dist[], const int visited[]){
    int min = INT_MAX, minIndex;

    int v;
    for(v = 0; v < N; v++){
        if(visited[v] == 0 && dist[v] < min){
            min = dist[v];
            minIndex = v;
        }

    }
    return minIndex;
}

// Applies the Dijkstra's algorithm to find the best route between routers.
void *linkState(void* nameless){
    time_t  lastUpdate;
    lastUpdate = time(NULL);

    while(1){
        int threshold = rand()%(updateMaxWait - updateMinWait) + updateMinWait;
        if(time(NULL) - lastUpdate > threshold){

            int dist[N], visited[N], tmpCosts[N][N];
            int i, source;

            pthread_mutex_lock(&lock);

            for(source = 0; source < N; source++){

                for (i = 0; i < N; i++)
                    dist[i] = INT_MAX, visited[i] = 0;

                dist[source] = 0;

                int count, u, v;

                for(count = 0; count < N-1; count++){
                    u = minDistance(dist, visited);
                    visited[u] = 1;

                    for(v = 0; v < N; v++){
                        if(visited[v] == 0 && costs[u][v] && dist[u] != INT_MAX && dist[u]+costs[u][v] < dist[v])
                            dist[v] = dist[u] + costs[u][v];
                    }
                }

                printf("Distance computed from Dijkstra from node %d: ", source);
                for(i = 0; i<N; i++){
                    printf("%d ", dist[i]);
                    tmpCosts[source][i] = dist[i];
                    tmpCosts[i][source] = dist[i];
                }
                printf("\n");
            }
            printf("\n");

            pthread_mutex_unlock(&lock);
            lastUpdate = time(NULL);

            sleep(3);
        }
    }
}

int main(int argc, char* argv[]){
    if(argc != 5){
        printf("Arguments to be given in the given format: <Host ID> <Number of machines> <Costs file name> <Hosts file name>\n");
        exit(EXIT_FAILURE);
    }

    sscanf(argv[1], "%d", &myID);
    sscanf(argv[2], "%d", &N);

    FILE* fCosts;
    FILE* fHosts;

    if((fCosts = fopen(argv[3], "r")) == NULL){
        perror("Error: ");
        exit(EXIT_FAILURE);
    }

    if((fHosts = fopen(argv[4], "r")) == NULL){
        perror("Error: ");
        exit(EXIT_FAILURE);
    }

    pthread_mutex_init(&lock, NULL);

    parseFiles(fCosts, fHosts);

    myPort = hosts[myID].port;

    struct sockaddr_in myAddr, otherAddr;
    struct sockaddr_storage myStorage;
    socklen_t addrSize, otherAddrSize;

    myAddr.sin_family = AF_INET;
    myAddr.sin_port = htons((short)myPort);
    myAddr.sin_addr.s_addr = htonl (INADDR_ANY);
    memset ((char *)myAddr.sin_zero, '\0', sizeof (myAddr.sin_zero));
    addrSize = sizeof (myStorage);

    if ((mySock = socket (AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("Error: ");
        exit(EXIT_FAILURE);
    }

    if (bind (mySock, (struct sockaddr *)&myAddr, sizeof (myAddr)) != 0)
    {
       perror("Error: ");
        exit(EXIT_FAILURE);
    }

    // Receiver thread to update informaton regarding costs between routers.
    pthread_t receiveThr;
    pthread_create(&receiveThr, NULL, receiveUpdates, NULL);

    // Link-state thread to update the cost on detecting a change in the costs.
    pthread_t linkThr;
    pthread_create(&linkThr, NULL, linkState, NULL);

    int i;
    for(i=0; i< 2; i++){
        userInputCost();
        sleep(10);
    }

}