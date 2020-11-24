#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> 
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
	
#define BUF_SIZE 1024
	
void* request(void* arg);
void* response(void* arg);
	
int main(int argc, char *argv[]) {

	int sock;
	struct sockaddr_in serv_addr;
	pthread_t req_thread, res_thread;
	void * thread_return;
	if(argc != 5) {
		fprintf(stderr, "Usage : %s <IP> <port> <username> <DBname>\n", argv[0]);
		exit(1);
	}

	sock = socket(PF_INET, SOCK_STREAM, 0);
	
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
	serv_addr.sin_port = htons(atoi(argv[2]));
	  
	if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr))==-1) {
		fprintf(stderr, "connect error\n");
        exit(1);
    }
	
    /* login */
    char login_info[BUF_SIZE];
    char pw[100];
    
    printf("Enter Password: ");
    scanf("%s", pw);
    sprintf(login_info, "%s,%s,%s", argv[3], pw, argv[4]);
    write(sock, login_info, strlen(login_info));

    pthread_create(&res_thread, NULL, response, (void*)&sock);
	pthread_create(&req_thread, NULL, request, (void*)&sock);

	pthread_join(req_thread, &thread_return);
	pthread_join(res_thread, &thread_return);
	close(sock);  
	return 0;
}
	
void* request(void* arg) {

	int sock = *((int*)arg);
	char req[BUF_SIZE];
	while (1) {   
	    fgets(req, BUF_SIZE, stdin);
        req[strlen(req)-1] = '\0';
		if (!strcmp(req,"q\n")||!strcmp(req,"Q\n")) {
			close(sock);
			exit(0);
		}
		write(sock, req, strlen(req));
	}
	return NULL;
}
	
void* response(void* arg) {

	int sock = *((int*)arg);
	char res[BUF_SIZE];
	int str_len;
	while (1) {
		str_len=read(sock, res, BUF_SIZE-1);
		if (str_len==-1) 
			return (void*)-1;
		res[str_len]=0;
		fputs(res, stdout);
	}
	return NULL;
}
	