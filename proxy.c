#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PENDING 3

struct dns_cache
{
    char ip[1024];
    char name[1024];
    // struct dns_cache* next;
};

struct dns_cache cache[3];
int cache_count = 0;		// number of dns entries currently in cache

int check_cache(char *input_msg) {
    
    char type = input_msg[0];
    char ip_or_name[1023];
    strcpy(ip_or_name, input_msg+1);
    int i;
    printf("Checking for %c %s in cache\n",type,ip_or_name);
    for (i = 0; i < cache_count; ++i)
    {
    	if ((type == '2') && (strcmp(ip_or_name, cache[i].ip))==0) {
            return 1;
        }
        if ((type == '1') && (strcmp(ip_or_name, cache[i].name))==0) {
            return 1;
        }
    }
    return 0;
}

void resolve_cache(char *inp, char *outp) {
    char type = inp[0];
    char ip_or_name[1023];
    strcpy(ip_or_name, inp+1);

    // struct queue *ptr = cache_head;
    int i;
    for (i = 0; i < 3; ++i)
    {
    	if ((type == '2') && (strcmp(ip_or_name, cache[i].ip)==0)) {
            strcpy(outp, "3");
            strcat(outp+1, cache[i].name);
            return;
        }
        if ((type == '1') && (strcmp(ip_or_name, cache[i].name)==0)) {
            strcpy(outp, "3");
            strcat(outp+1, cache[i].ip);
            return;
        }
    }
    return;
}

// void empty(char * x)
// {
//     int i;
//     for(i=0;i<1024;i++)x[i]='\0';
// }


int main(int argc, char const *argv[])
{
	int proxy_port;
    switch(argc)
    {
        case 2:				// Proceed only if there's one arguement.
        	proxy_port = atoi(argv[1]);
        	if (proxy_port == 0)
        	{
        		fprintf(stderr, "%s\n", "Invalid or zero port");
        	}
            break;
        default:
            fprintf(stderr, "%s\n", "To setup a proxy server, type : ./proxy <Port>");
            exit(1);
            break;
    }


    struct sockaddr_in PROXY_SERV;
    struct sockaddr_in DNS_SERV;
    //int DNS_SERV;
    int DNS_SOCKET=0,PROXY_SOCKET=0;
    int address_size = sizeof(PROXY_SERV);

    char input_msg[1024],output_msg[1024];

    //int i;
    //for(i=0;i<1024;i++)input_msg[i]=output_msg[i]='\0';

    if ((DNS_SOCKET = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Socket creation error \n");
        return -1;
    }
    memset(&DNS_SERV, '0', sizeof(DNS_SERV));		// reset DNS_SERV to 0

    int DNS_port;
    char DNS_IP[15];
    printf("Enter DNS Server IP:");
    scanf("%s",DNS_IP);
    printf("Enter DNS Server Port:");
    scanf("%d",&DNS_port);

    DNS_SERV.sin_family = AF_INET;
    DNS_SERV.sin_port = htons(DNS_port);

    // Set IP addess (also check if it is vaild)
    if(inet_pton(AF_INET, DNS_IP, &DNS_SERV.sin_addr)<=0)
    {
        fprintf(stderr,"\nNot a valid IPv4 address\n");
        exit(1);
    }

    if (connect(DNS_SOCKET, (struct sockaddr *)&DNS_SERV, sizeof(DNS_SERV)) < 0)
    {
        fprintf(stderr,"Could not connect to DNS Server.\n");
        exit(1);
    }

    close(DNS_SOCKET);
	// printf("Socket closed...\n");

    // Create a socket to communicate with DNS, also check if it fails.
    if ((PROXY_SOCKET = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        fprintf(stderr,"\n Could not create socket\n");
        exit(1);
    }

	// We must re-use address and port for multiple queries
    if (setsockopt(PROXY_SOCKET, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &(int){1}, sizeof(int)))
    {
        fprintf(stderr,"Couldn't set socket option : ");
		exit(1);
    }

 	memset(&PROXY_SERV, '0', sizeof(PROXY_SERV));

    PROXY_SERV.sin_family = AF_INET;
    PROXY_SERV.sin_addr.s_addr = INADDR_ANY;
    PROXY_SERV.sin_port = htons(proxy_port);

    // Bind the DNS Server
    if (bind(PROXY_SOCKET, (struct sockaddr *)&PROXY_SERV,sizeof(PROXY_SERV))<0)
    {
        fprintf(stderr,"Couldn't bind address/port");
        exit(1);
    }
    if (listen(PROXY_SOCKET, PENDING) < 0)		//maximum 3 pending requests
    {
        fprintf(stderr,"Socket listen() failed : ");
        exit(1);
    }

    int ACCEPT_SOCK;
    int SOCK_READ;
    while(1)
    {
    	// clear buffers in every loop
		for (int i = 0; i < 1024; ++i)
		{
			input_msg[i] = '\0';
			output_msg[i] = '\0';
		}
    	// start processing a query
        if ((ACCEPT_SOCK = accept(PROXY_SOCKET, (struct sockaddr *)&PROXY_SERV, (socklen_t*)&address_size))<0)
        {
            fprintf(stderr,"Socket accept() failed : ");
            exit(1);
        }

        // Put the message in the string.
		int SOCK_READ = recv(ACCEPT_SOCK , input_msg, 1024, 0);			// 0 denotes no flags
        printf("Incoming query: Type: %c, Resolve: %s\n",input_msg[0], input_msg+1 );

        if(input_msg[0]=='1'||input_msg[0]=='2')
        {
            if(check_cache(input_msg))
            {
                resolve_cache(input_msg,output_msg);
                send(ACCEPT_SOCK , output_msg , strlen(output_msg) , 0 );
                close(ACCEPT_SOCK);
            	printf("ENTRY IS CACHED\n");
            }
            else
            {

                 if ((DNS_SOCKET = socket(AF_INET, SOCK_STREAM, 0)) < 0)
                {
                    fprintf(stderr,"\n Socket creation error \n");
                    exit(1);
                }

                if (connect(DNS_SOCKET, (struct sockaddr *)&DNS_SERV, sizeof(DNS_SERV)) < 0)
                {
                    fprintf(stderr,"\nConnection with DNS Failed \n");
                    exit(1);
                }

                send(DNS_SOCKET , input_msg , strlen(input_msg) , 0);
                char dns_resp[1024];
                for (int i = 0; i < 1024; ++i)
                {
                	dns_resp[i]='\0';
                }
                SOCK_READ = read( DNS_SOCKET , dns_resp, 1024);
                close(DNS_SOCKET);

                if(dns_resp[0]=='3')
                {
                	if(cache_count == 3)
                	{
                		for (int i = 0; i < 2; ++i)
                		{
                			strcpy(cache[i].name,cache[i+1].name);
                			strcpy(cache[i].ip,cache[i+1].ip);
                		}
                		cache_count--;
                		printf("CACHE WAS FULL, OLDEST ENTRY REMOVED\n");
                	}
                    if(input_msg[0]=='1')
                    {
                    	for (int i = 1; i < strlen(input_msg); ++i)
                    	{
                    		cache[cache_count].name[i-1] = input_msg[i];
                    	}
                    	for (int i = 1; i < strlen(dns_resp); ++i)
                    	{
                    		cache[cache_count].ip[i-1] = dns_resp[i];
                    	}
                    }
                    else
                    {
                    	for (int i = 1; i < strlen(input_msg); ++i)
                    	{
                    		cache[cache_count].ip[i-1] = input_msg[i];
                    	}
                    	for (int i = 1; i < strlen(dns_resp); ++i)
                    	{
                    		cache[cache_count].name[i-1] = dns_resp[i];
                    	}
                    }
            		printf("CACHE UPDATED\n");
                    cache_count++;
                }
                send(ACCEPT_SOCK ,dns_resp , strlen(dns_resp) , 0 );
                close(ACCEPT_SOCK);
                printf("DNS Response: Type: %c, Output: %s\n", dns_resp[0], dns_resp+1);
            }
        }
        else
        {
            send(ACCEPT_SOCK ,"4Invalid query type", strlen("4Invalid query type"), 0 );
            close(ACCEPT_SOCK);
        }
    }
    return 0;
}
