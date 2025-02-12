#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define FILE_LEN 32
#define BUF_SIZE 1024
#define BUF_RST_SIZE 2048
void error_handling(char *message);

int main(int argc, char *argv[])
{
	int sd;
	FILE *fp;
	
	char file_name[FILE_LEN];
	char buf[BUF_SIZE];
	//char result_buf[BUF_RST_SIZE];
	int read_cnt;
	int read_size;
	struct sockaddr_in serv_adr;
	if (argc != 4) {
		printf("Usage: %s <IP> <port> <file name> \n", argv[0]);
		exit(1);
	}
	
	fp = fopen(argv[3], "rb");
	sd = socket(PF_INET, SOCK_STREAM, 0);   

	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.s_addr = inet_addr(argv[1]);
	serv_adr.sin_port = htons(atoi(argv[2]));

	connect(sd, (struct sockaddr*)&serv_adr, sizeof(serv_adr));
	
	// Send file name 
	strcpy(file_name, argv[3]);
	write(sd, file_name, FILE_LEN);

	// Send file data 
	read_size = 0;
	while(1)
	{
		read_cnt = fread((void*)buf, 1, BUF_SIZE, fp);
		read_size += read_cnt;
		if (read_size % 1024 == 0)
			printf("Send %d bytes \n", read_size);

		if (read_cnt < BUF_SIZE)
		{
			write(sd, buf, read_cnt);

			break;
		}
		write(sd, buf, BUF_SIZE);
	}
	
	//printf("Send %d bytes \n", read_size);
	fclose(fp);
	shutdown(sd, SHUT_WR);	

	//실행결과 읽고 출력
	int total_read = 0;
	while((read_cnt = read(sd, buf, BUF_SIZE-1))>0){
		buf[read_cnt] = '\0';
		if (total_read + read_cnt < BUF_RST_SIZE - 1) {
            total_read += read_cnt;
        } else {
            fprintf(stderr, "result_buf overflow!!\n");
            break;
        }
	}

	if(total_read >0){
		printf("Result from Server\n");
		printf("%s\n", buf);
	} else {
		printf("no result\n");
	}
	//read(sd, buf, BUF_SIZE);
	//printf("Result from Server\n");
	//printf("%s\n", buf);
	close(sd);
	return 0;
}

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}