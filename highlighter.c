#include <stdio.h>
#include <termios.h>
#include <string.h>
#include <regex.h>
#include <libgen.h>

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
char _getch()
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
			printf("%s      ", buf);
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
	} else {
		strcpy(temp, buf);
		temp[p] = c;
		strcpy(temp+p+1, buf+p);
		strcpy(buf, temp);
		p++;

		if (p > tail) tail = p;

		printf("\33[K");
		printf("\33[1000000D");
		printf("%s       ", buf);
		printf("\33[1000000D");
		printf("\33[%dC", p);
	}
	ter.c_lflag |= ICANON;
	ter.c_lflag |= ECHO;
	tcsetattr(0, TCSANOW, &ter);
	return c;
}
int main() {
	while(1) {
		char c = _getch();
	}
  return 0;
}

