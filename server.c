#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include "/usr/include/mysql/mysql.h"

#define BUF_SIZE 1024
#define oops(s)	{   perror(s);	exit(1);    }

void* handle_req(void* arg);
void error_handling(char* query);
void send_status(int destination, char* query);
void handle_mysql_error(MYSQL *con);
MYSQL* connect_mysql(int, char*, char*, char*, char*);

int main(int argc, char *argv[]) {

    int serv_sock, clnt_sock;
    struct sockaddr_in serv_adr, clnt_adr;
    int clnt_adr_sz;
    pthread_t t_id;
    if (argc != 2) {
	printf("Usage : %s <port>\n", argv[0]);
	exit(1);
    }

    if(mkdir("querylog", 0775) == -1 && errno != EEXIST)
	oops("log directory make failed");

    serv_sock = socket(PF_INET, SOCK_STREAM, 0);

    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET; 
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(atoi(argv[1]));

    if (bind(serv_sock, (struct sockaddr*) &serv_adr, sizeof(serv_adr))==-1) {
	fprintf(stderr, "bind error\n");
	exit(1);
    }

    if (listen(serv_sock, 5) == -1) {
	fprintf(stderr, "listen error\n");
	exit(1);
    }

    while (1) {
	clnt_adr_sz = sizeof(clnt_adr);
	clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_adr,&clnt_adr_sz);

	pthread_create(&t_id, NULL, handle_req, (void*)&clnt_sock);
	pthread_detach(t_id);
	printf("Connected client%d IP: %s \n", clnt_sock, inet_ntoa(clnt_adr.sin_addr));
    }
    close(serv_sock);
    return 0;
}

/* connect to the dbms */
MYSQL* connect_mysql(int clnt_sock, char* user, char* password, char* db_name, char* db_ip) {

    MYSQL *conn;

    if (!(conn = mysql_init((MYSQL*)NULL))) { 
	send_status(clnt_sock, "Error MySQL");
	handle_mysql_error(conn);
	return NULL;
    }
    send_status(clnt_sock, "mysql_init success.\n");

    if (!mysql_real_connect(conn, db_ip, user, password, NULL, 3306, NULL, 0)) {
	send_status(clnt_sock, "Login failed...");
	handle_mysql_error(conn);
	return NULL;
    }
    send_status(clnt_sock, "Login successed and connected....\n");

    if (mysql_select_db(conn, db_name)) {
	send_status(clnt_sock, "Fail to select the db_name...\n");
	handle_mysql_error(conn);
	return NULL;
    }
    send_status(clnt_sock, "Complete!\n");

    return conn;
}

void* handle_req(void* arg) {
    int clnt_sock=*((int*)arg);
    int str_len=0, i;
    char query[BUF_SIZE] = { '\0' };
    char resultbuf[BUFSIZ] = {'\0'};	
    /* information for connecting to DBMS */
    char *db_ip = "localhost";
    char user[BUF_SIZE] = {0,};
    char password[BUF_SIZE] = {0,};
    char db_name[BUF_SIZE] = {0,};

    if (!read(clnt_sock, query, sizeof(query))) {
	printf("Disconnect client%d\n\n", clnt_sock);
	return NULL;
    }

    /* parsing the message of a client */
    sprintf(user, "%s", strtok(query, ","));
    sprintf(password, "%s", strtok(NULL, ","));
    sprintf(db_name, "%s", strtok(NULL, ","));
    printf("User: %s\nconnected DB:%s\n", user, db_name);

    /* connect to the DBMS */
    MYSQL *conn = connect_mysql(clnt_sock, user, password, db_name, db_ip);


    /* processing query */
    MYSQL_RES *rs;    
    MYSQL_ROW row;
    while(1) {
	send_status(clnt_sock, "mysql> ");
	if (!(str_len=read(clnt_sock, query, BUF_SIZE-1))) {
	    break;
	}
	query[str_len] = '\0';
	if(query[str_len - 1] == ';')
	    query[str_len] = '\0';
	puts(query);

	if (mysql_query(conn, query)) {
	    fprintf(stderr, "query: fail\n");
	    //send_status(clnt_sock, "query: fail\n");
	} else {
	    printf("query: success\n");
	    /* execute if only select inst. */
	    rs = mysql_store_result(conn);
	    unsigned int i, num_fields;
	    MYSQL_FIELD * fields;
	    num_fields = mysql_num_fields(rs);
	    fields = mysql_fetch_fields(rs);

	    if(write(clnt_sock, "\n", 1) == -1)
		oops("write");
	    for(i = 0; i < num_fields; i++)
	    {
		sprintf(resultbuf, "[%s] ", fields[i].name);
		if(write(clnt_sock, resultbuf, strlen(resultbuf)) == -1)
		    oops("write");
	    }
	    printf("\n");
	    if(write(clnt_sock, "\n", 1) == -1)
		oops("write");

	    while ((row=mysql_fetch_row(rs)) != NULL) {
		unsigned long *lengths;
		lengths = mysql_fetch_lengths(rs);
		for(i = 0; i < num_fields; i++){
		    sprintf(resultbuf, "[%.*s] ", (int)lengths[i], row[i]? row[i]: "NULL");
		    if(write(clnt_sock, resultbuf, strlen(resultbuf)) == -1)
			oops("write");
		}

		if(write(clnt_sock, "\n", 1) == -1)
		    oops("write");
	    }
	}
    }
    printf("Disconnect client%d\n\n", clnt_sock);
    close(clnt_sock);
    mysql_close(conn);
    return NULL;
}

void send_status(int destination, char *msg) {

    write(destination, msg, strlen(msg));
}

void handle_mysql_error(MYSQL *con) {

    fprintf(stderr, "%s\n", mysql_error(con));
    mysql_close(con);
}
