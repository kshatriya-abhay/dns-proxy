// DNS Server side C program
// Treats proxy as a client/user as well.

#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>

#define PENDING 3

int main(int argc, char const *argv[]) {
	switch (argc ){
        case 2:
            break;
        default:
            fprintf(stderr, "%s\n", "To create DNS server, type : ./dns <port>");
            exit(1);
            break;
    }

    int PORT = atoi(argv[1]);

	int DNS_SERV;
	struct sockaddr_in DNS_ADDR;
	int addrlen = sizeof(DNS_ADDR);
	char input_buffer[1024] = {0}, output_buffer[1024] = {0} ;
	FILE *DNS_database;
	int response_type;
	char ipv4_add[1023], domain[1023];

	// Socket to be used to accept DNS requests
	if ((DNS_SERV = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
		fprintf(stderr,"Couldn't create a socket : ");
		exit(1);
	}

	// We must re-use address and port for multiple queries
	if (setsockopt(DNS_SERV, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &(int){1}, sizeof(int))) {
		fprintf(stderr,"Couldn't set socket option : ");
		exit(1);
	}

	// To set DNS IP and Port
	DNS_ADDR.sin_family = AF_INET;
	DNS_ADDR.sin_addr.s_addr = INADDR_ANY;		// INADDR_ANY helps bind the DNS socket to all local interfaces.
	DNS_ADDR.sin_port = htons( PORT );

	// Bind the DNS socket
	if (bind(DNS_SERV, (struct sockaddr *)&DNS_ADDR, sizeof(DNS_ADDR))<0) {
		fprintf(stderr,"Couldn't bind address/port : ");
		exit(1);
	}

	// wait for a query on the socket
	// printf("Now listening...\n");
	if (listen(DNS_SERV, PENDING) < 0) {			//maximum 3 pending requests
		fprintf(stderr,"Socket listen() failed : ");
		exit(1);
	}

	struct sockaddr_in USER_ADDR;		//Variables needed to accept a request
	int ACCEPT_SOCK;

	while(1) {
		// clear buffers in every loop
		memset(&input_buffer, '\0', 1024);
		memset(&output_buffer, '\0', 1024);

		// start processing a query
		if ((ACCEPT_SOCK = accept(DNS_SERV, (struct sockaddr *)&USER_ADDR, (socklen_t*)&USER_ADDR))<0) {
			fprintf(stderr,"Socket accept() failed : ");
			exit(1);
		}
		// printf("Accepted...\n");


		// Put the message in the string.
		int SOCK_READ = recv(ACCEPT_SOCK , input_buffer, 1024, 0);			// 0 denotes no flags
		printf("Request :\tType-%c,\tMessage-%s\n",input_buffer[0], input_buffer+1 );

		// Switch case of msg type
		switch(input_buffer[0]) {

		case '1': 		//contains Domain Name
			response_type = 4;
			DNS_database = fopen("DNS.csv","r");
			while(fscanf(DNS_database, "%[^,],%[^\n]\n", domain, ipv4_add) != -1){
				if(strcmp(domain, input_buffer+1) == 0){
					response_type = 3;
					output_buffer[0] = '3';
					strcat(output_buffer, ipv4_add);
					send(ACCEPT_SOCK, output_buffer, strlen(output_buffer), 0);
					break;
				}
			}
			fclose(DNS_database);
			if (response_type == 4) {
				strcpy(output_buffer, "4Domain name not present in DNS");
				send(ACCEPT_SOCK, output_buffer, strlen(output_buffer), 0);
			}
			break;

		case '2': 		// contains IPv4 address
			response_type = 4;
			DNS_database = fopen("DNS.csv","r");
			while(fscanf(DNS_database, "%[^,],%[^\n]\n", domain, ipv4_add) != -1){
				if(strcmp(ipv4_add, input_buffer+1) == 0){
					response_type = 3;
					output_buffer[0] = '3';
					strcat(output_buffer, domain);
					send(ACCEPT_SOCK, output_buffer, strlen(output_buffer), 0);
					break;
				}
			}
			fclose(DNS_database);
			if (response_type == 4){
				strcpy(output_buffer, "4IP address not present in DNS");
				send(ACCEPT_SOCK, output_buffer, strlen(output_buffer), 0);
			}
			break;

		default:
			// If type is not 1 or 2, generate error
			strcpy(output_buffer, "4Invalid message syntax");
			send(ACCEPT_SOCK, output_buffer, strlen(output_buffer) , 0 );
			fprintf(stderr, "%s\n", "Invalid query syntax");
			break;
		}

		printf("Query output msg: ResponseType-%c, Message-%s\n\n", output_buffer[0], output_buffer+1);
		close(ACCEPT_SOCK);
	}
	return 0;
}
