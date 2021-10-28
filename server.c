
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <net/if.h>
#include <string.h>

 
#define CLIENT_QUEUE_LEN 10
#define SERVER_PORT 7002
 
int main(void)
{
	int listen_sock_fd = -1, client_sock_fd = -1;
	struct sockaddr_in6 server_addr, client_addr;
	socklen_t client_addr_len;
	char str_addr[INET6_ADDRSTRLEN];
	int ret, flag;
	char ch[1024];
 
	/* Create socket for listening (client requests) */
	listen_sock_fd = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
	if(listen_sock_fd == -1) {
		perror("socket()");
		return EXIT_FAILURE;
	}
	
	const char *opt;
	opt = "wave-data";
	const int len = strnlen(opt, IFNAMSIZ);
	if (len == IFNAMSIZ) {
	    fprintf(stderr, "Too long iface name");
	    return 1;
	}
	setsockopt(listen_sock_fd, SOL_SOCKET, SO_BINDTODEVICE, opt, len);
 
	/* Set socket to reuse address */
	flag = 1;
	ret = setsockopt(listen_sock_fd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
	if(ret == -1) {
		perror("setsockopt()");
		return EXIT_FAILURE;
	}

	server_addr.sin6_family = AF_INET6;
	server_addr.sin6_addr = in6addr_any;
	server_addr.sin6_port = htons(SERVER_PORT);
 
	/* Bind address and socket together */
	ret = bind(listen_sock_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
	if(ret == -1) {
		perror("bind()");
		close(listen_sock_fd);
		return EXIT_FAILURE;
	}
 
	/* Create listening queue (client requests) */
	ret = listen(listen_sock_fd, CLIENT_QUEUE_LEN);
	if (ret == -1) {
		perror("listen()");
		close(listen_sock_fd);
		return EXIT_FAILURE;
	}
 
	client_addr_len = sizeof(client_addr);
	printf("Waiting for connection from OBU.\n");
	while(ret != 1) {
		/* Do TCP handshake with client */
		printf("accepting incoming connections...\n");
		client_sock_fd = accept(listen_sock_fd,
				(struct sockaddr*)&client_addr,
				&client_addr_len);
		if (client_sock_fd == -1) {
			perror("accept()");
			close(listen_sock_fd);
			return EXIT_FAILURE;
		}
 
		inet_ntop(AF_INET6, &(client_addr.sin6_addr),
				str_addr, sizeof(str_addr));
		printf("New connection from: %s:%d ...\n",
				str_addr,
				ntohs(client_addr.sin6_port));
 
		/* Wait for data from client */
		printf("Waiting for data from %s", str_addr); 
		ret = read(client_sock_fd, &ch, 1024);
		if (ret == -1) {
			perror("read()");
			close(client_sock_fd);
			return EXIT_FAILURE;
		}
		ret = write(client_sock_fd, "Authenticated", 1024);
 
		/* Do very useful thing with received data :-) */
		printf("data received: %s \n", ch);
		if(strncmp(ch, "ID", 2) == 0){
			//forward the id onto server		
			
			int sock_fd = -1;
		        struct sockaddr_in acpserver_addr;


			const char *opt;
			opt = "eth0";
			const int len = strnlen(opt, IFNAMSIZ);
			if (len == IFNAMSIZ) {
			    fprintf(stderr, "Too long iface name");
			    return 1;
			}
		
			sock_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		        if (sock_fd == -1) {
                		perror("socket()");
                		return EXIT_FAILURE;
        		}
			/* Connect to server running on localhost */
		        
			setsockopt(sock_fd, SOL_SOCKET, SO_BINDTODEVICE, opt, len);
			acpserver_addr.sin_family = AF_INET;
		        inet_pton(AF_INET, "10.42.0.1", &acpserver_addr.sin_addr);
        		acpserver_addr.sin_port = htons(SERVER_PORT);

			 /* Try to do TCP handshake with server */
                	ret = connect(sock_fd, (struct sockaddr*)&acpserver_addr, sizeof(acpserver_addr));
                	if (ret == -1) {
                        	perror("connect()");
				write(client_sock_fd,"connection refused",17);	
                        	close(sock_fd);
				continue;	
			}
			char buffer[1024];
			strcpy(buffer, ch);
			ret = write(sock_fd, buffer, 1024);
			if (ret == -1) {
                		perror("write");
                		close(sock_fd);
                		
		        }
			
			//ret = read(sock_fd, ch, 2);
			//ch[2] = '\0';
			//int len = atoi(ch);
			ret = read(sock_fd, ch, 1024);
			printf("1. Data from server: %s\n", ch);
			//close(sock_fd);	
			if (ret == -1) {
				perror("read()");
				close(sock_fd);
				ret = 0;
				//return EXIT_FAILURE;
         		}
        		
			ret = 1;
		}
		 
		/* Send response to client */
		printf("2. Data from acpserver: %s", ch);
		ret = write(client_sock_fd, &ch, 1024);
		if (ret == -1) {
			perror("write()");
			close(client_sock_fd);
			continue;
		}
 
		/* Do TCP teardown */
		ret = close(client_sock_fd);
		if (ret == -1) {
			perror("close()");
			client_sock_fd = -1;
		}
 
		printf("Connection closed\n");
	}
	return EXIT_SUCCESS;
}
