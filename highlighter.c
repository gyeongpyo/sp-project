#include <stdio.h>
#include <termios.h>
#include <string.h>
#include <regex.h>
#include <libgen.h>
#include <ctype.h>

#define BLACK "\x1b[30m"
#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define YEL   "\x1B[33m"
#define BLU   "\x1B[34m"
#define MAG   "\x1B[35m"
#define CYN   "\x1B[36m"
#define WHT   "\x1B[37m"
#define RESET "\x1B[0m"

char buf[BUFSIZ];
char temp[BUFSIZ];
int p = 0;
int tail = 0;

char* keyword[100] = {"SELECT", "FROM", "WHERE"};

char* check_keyword(char* str)
{
	for (int i = 0; i < 3; ++i) {
		if (!strcasecmp(keyword[i], str)) {
			return keyword[i];
		}
	}
	return NULL;
}

void print_highlight(char* str)
{
	int i = 0;

	char temp[1000];
	int temp_idx = 0;
	while (str[i]) {
		if ( (isalnum(str[i]) || str[i]=='_' || str[i]=='-' || str[i] == ';' || str[i] == '&' ||
					str[i] == '|' || str[i] == '*') &&
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
			//printf("%s      ", buf);
			print_highlight(buf);	
			printf("\33[1000000D");
			printf("\33[%dC", p+1);
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
	} else if (c == ' ' || c == '_' || c == '-' || isalnum(c) || c == ';' || c == '&' || c == '|' || c == '*') {
		strcpy(temp, buf);
		temp[p] = c;
		strcpy(temp+p+1, buf+p);
		strcpy(buf, temp);
		p++;

		if (p > tail) tail = p;

		printf("\33[K");
		printf("\33[1000000D");
		//printf("%s       ", buf);
		print_highlight(buf);	
		printf("\33[1000000D");
		printf("\33[%dC", p);
	} else if (c == '\n') {
		printf("\n");
		memset(buf, 0, sizeof(buf));
		p = 0;
	}
	ter.c_lflag |= ICANON;
	ter.c_lflag |= ECHO;
	tcsetattr(0, TCSANOW, &ter);
	return c;
}
int main() 
{
	while(1) {
		char c = process();
	}
	return 0;
}

