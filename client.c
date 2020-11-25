#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h> 
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <termios.h>

#define oops(s)	{   perror(s); exit(1);	}

#define BUF_SIZE	1024
#define INVISIBLE_MODE	0
#define VISIBLE_MODE	1

#define TESTSIZ		100

void* request(void* arg);
void* response(void* arg);
void echomode(int option);

int log_fd, log_idx = 0, n_chars, new_nchars;
char logbuf[BUFSIZ] = { 0 };

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
    char namebuf[255] = { 0 };

    echomode(INVISIBLE_MODE);
    printf("Enter Password: ");
    scanf("%s", pw);
    echomode(VISIBLE_MODE);

    sprintf(login_info, "%s,%s,%s", argv[3], pw, argv[4]);
    write(sock, login_info, strlen(login_info));
    sprintf(namebuf, "%s.log", argv[4]);
    
    if((log_fd = open(namebuf, O_WRONLY | O_CREAT | O_APPEND, 0644)) == -1)
	oops("open");
 
    pthread_create(&res_thread, NULL, response, (void*)&sock);
    pthread_create(&req_thread, NULL, request, (void*)&sock);

    pthread_join(req_thread, &thread_return);
    pthread_join(res_thread, &thread_return);
    close(sock);  
    return 0;
}

// option == 0, INVISIBLE 
// option == 1, VISIBLE
void echomode(int option)
{
    static struct termios info, original;
    if(option == INVISIBLE_MODE)    
    {
	if(tcgetattr(STDIN_FILENO, &info) == -1)	
	    oops("tcgetattr");
	/* back up original setting */
	original = info;
	info.c_lflag &= ~ECHO;
	if(tcsetattr(STDIN_FILENO, TCSANOW, &info) == -1)
	    oops("tcsetattr");
    }
    else
    {	/* restore */
	if(tcsetattr(STDIN_FILENO, TCSANOW, &original) == -1)
	    oops("tcsetattr");
    }
}



void* request(void* arg) {

    int sock = *((int*)arg);
    char req[BUF_SIZE];
    while (1) {   
	fgets(req, BUF_SIZE, stdin);
	n_chars = strlen(req);
	if(log_idx + n_chars <= TESTSIZ - 1)
	{
	    strcat(logbuf, req);
	    log_idx += n_chars;
	}
	else
	{
	    logbuf[log_idx + 1] = '\0';
	    new_nchars = 0;
	    while((new_nchars += write(log_fd, logbuf, log_idx)) < log_idx)
		;
	    strcpy(logbuf, req);
	    log_idx = n_chars;
	}	
	req[n_chars - 1] = '\0';
	if (!strcmp(req,"q")||!strcmp(req,"Q")) {
	    if(log_idx > 0)
	    {
		logbuf[log_idx + 1] = '\0';
		new_nchars = 0;
		while((new_nchars += write(log_fd, logbuf, log_idx)) < log_idx)
		    ;
	    }
	    close(log_fd);
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

