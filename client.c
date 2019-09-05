#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main(int argc, char *argv[])
{
    struct sockaddr_in PROXY_ADDR;			//To store proxy server IP address and port.
    memset(&PROXY_ADDR, '0', sizeof(PROXY_ADDR));

    switch (argc){
        case 3:
        	if(inet_pton(AF_INET, argv[1], &PROXY_ADDR.sin_addr)<=0)			//Check if given proxy server address is valid IPv4 address (AF_INET).
    			{
        		fprintf(stderr,"\nNot a valid IPv4 address\n");
    			exit(1);
	   			}
            break;
        default:
            fprintf(stderr, "To send DNS requests, type : ./fetch <Proxy IP Address> <Proxy Port number>\nFor DNS requests type 1[domain name] or 2[IPv4 address]\ntype 'quit' to exit\n");
            exit(1);
            break;
    }

    int CLIENT_SOCKET = 0;
    int port = atoi(argv[2]);		//Proxy port (converted to integer using atoi function)
    char output[1024] = {0};		//Initialize buffer of 1024 bytes with 0 values


    PROXY_ADDR.sin_family = AF_INET;
    PROXY_ADDR.sin_port = htons(port);		//Port number must to converted to network byte order using htons
	
	char* query;
    do
    {

        memset(output, '\0', 1024);
        if ((CLIENT_SOCKET = socket(AF_INET, SOCK_STREAM, 0)) < 0)								// Create a client socket to communicate with proxy server.
        {
            fprintf(stderr,"\n Couldn't create socket\n");
            exit(1);
        }
        scanf("%s",query);
        if (connect(CLIENT_SOCKET, (struct sockaddr *)&PROXY_ADDR, sizeof(PROXY_ADDR)) < 0)		// Connect to Proxy with TCP, returns positive value if successful.
        {
            fprintf(stderr,"Couldn't connect to DNS Proxy Server.\n");
            exit(1);
        }

        send(CLIENT_SOCKET , query , strlen(query) , 0 );										// Send DNS query
        int PROXY_RESPONSE = read(CLIENT_SOCKET, output, 1024);									// Receive response from server
        printf("%s\n",output);
        close(CLIENT_SOCKET);
    }while(strcmp(query,"quit"));
    return 0;
}