// testclient.c
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <ctype.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h> 
#include <string.h>
#include <time.h>
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
int log_fd, n_chars;
char logbuf[BUFSIZ] = { 0 };
FILE* log_fp;
int sock;
char buf[BUFSIZ];
char bp_buf[BUFSIZ];
char temp[BUFSIZ];
int p = 0;
struct termios origin;
#define BLACK "\x1b[30m"
#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define YEL   "\x1B[33m"
#define BLU   "\x1B[34m"
#define MAG   "\x1B[35m"
#define CYN   "\x1B[36m"
#define WHT   "\x1B[37m"
#define RESET "\x1B[0m"
char* keyword[100] = {"SELECT", "FROM", "WHERE", "INSERT", "INTO", "VALUES", "",NULL};
char* check_keyword(char* str)
{
    for (int i = 0; keyword[i]; ++i) {
	if (!strcasecmp(keyword[i], str)) {
	    return keyword[i];
	}
    }
    return NULL;
}
void loggingfunction(int signum)
{
    if(!log_fp){
	fflush(log_fp);
	fclose(log_fp);
    }
    close(log_fd);
    close(sock);
    tcsetattr(0, TCSANOW, &origin);
    exit(0);
}
void print_highlight(char* str)
{
    int i = 0;
    char temp[1000];
    int temp_idx = 0;
    while (str[i]) {
	if ( (isalnum(str[i]) || str[i]=='_' || str[i]=='-' || str[i] == ';' || str[i] == '&' ||
		    str[i] == '|' || str[i] == '*' || str[i] == '.' || str[i] == '(' ||
		    str[i] == '=' || str[i] ==')' || str[i] == '\'' || str[i] == ',') &&
		(str[i+1]==' '||str[i+1]=='\0' ) ) {
	    temp[temp_idx++] = str[i];
	    temp[temp_idx] = '\0';
	    temp_idx = 0;
	    char* res = check_keyword(temp);
	    if (res != NULL) {
		printf(CYN "%s", res);
	    } else {
		printf(RESET "%s", temp);
	    }
	} else if (str[i] == ' ') {
	    printf(" ");
	} else {
	    temp[temp_idx++] = str[i];	
	}
	i++;
    }
}
#define PROMPLEN 7
char process()
{
    char c;
    struct termios ter;
    tcgetattr(0, &ter);
    ter.c_lflag &= ~ICANON;
    ter.c_lflag &= ~ECHO;
    tcsetattr(0, TCSANOW, &ter);
    c = getchar();
    if (c == 127 && p >= 0) {
	if (p > 0) {
	    strcpy(temp, buf);
	    strcpy(temp+p-1, buf+p);
	    strcpy(buf, temp);	
	    --p;	
	    printf("\33[K");
	    printf("\33[1000000D");
	    printf("\33[%dC", PROMPLEN);
	    print_highlight(buf);	
	    printf(" ");
	    printf("\33[1000000D");
	    printf("\33[%dC", p+1+PROMPLEN);
	    printf("\33[D");
	}
    } else if (c == '\33') {
	c = getchar();
	if ( (c = getchar()) == 'D' ) {
	    printf("\33[D");
	    if (p > 0) {
		p--;
	    }
	} else if ( c == 'C') {
	    if (strlen(buf) > p) {
		printf("\33[C");
		p++;
	    }		
	}
    } else if (c == ' ' || c == '_' || c == '-' || isalnum(c) ||
	    c == ';' || c == '&' || c == '|' || c == '*' || c == '.' ||
   		c == '(' || c == ')' || c == '=' || c == '\'' || c == ',') {
	strcpy(temp, buf);
	temp[p] = c;
	strcpy(temp+p+1, buf+p);
	strcpy(buf, temp);
	p++;
	printf("\33[K");
	printf("\33[1000000D");
	printf("\33[%dC", PROMPLEN);
	print_highlight(buf);	
	printf("\33[1000000D");
	printf("\33[%dC", p+PROMPLEN);
    } else if (c == '\n') {
	printf("\n");
	strcpy(bp_buf, buf);
	memset(buf, 0, sizeof(buf));
	p = 0;
    }
    ter.c_lflag |= ICANON;
    ter.c_lflag |= ECHO;
    tcsetattr(0, TCSANOW, &ter);
    return c;
}
int main(int argc, char *argv[]) {
    struct sockaddr_in serv_addr;
    pthread_t req_thread, res_thread;
    void * thread_return;
    if(argc != 5) {
	fprintf(stderr, "Usage : %s <IP> <port> <username> <DBname>\n", argv[0]);
	exit(1);
    }
    signal(SIGINT, loggingfunction);
    signal(SIGQUIT, loggingfunction);
    signal(SIGKILL, loggingfunction);
    tcgetattr(0, &origin);
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
    sprintf(namebuf, "querylog/%s.log", argv[4]);
    if((log_fd = open(namebuf, O_WRONLY | O_CREAT | O_APPEND, 0644)) == -1)
	oops("open");
    time_t t = time(NULL);
    if((log_fp = fdopen(log_fd, "w")) == NULL)
	oops("fdopen");
    fprintf(log_fp, "[USER : %s DB : %s ] - %s \n", argv[3], argv[4], ctime(&t));
    fflush(log_fp);
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
    int req_idx = 0;
    while (1) {   
	while (1) {
	    // assume that it reads only one line
	    if (process() == '\n') {
		break;
	    }
	}
	strcpy(req, bp_buf);
	n_chars = strlen(req);
	req[n_chars] = '\0';
	fprintf(log_fp, "%s\n", req);
	fflush(log_fp);
	if (!strcmp(req,"q")||!strcmp(req,"Q")) {
	    fclose(log_fp);
	    close(log_fd);
	    close(sock);
	    exit(0);
	}
	write(sock, req, n_chars);
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
