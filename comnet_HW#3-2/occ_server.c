#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define FILE_LEN 32
#define BUF_SIZE 1024
#define BUF_RST_SIZE 2048
void error_handling(char *message);
void *handle_client(void *arg);

pthread_mutex_t mutx;

typedef struct {
    int clnt_sock;
    char client_IP[16];
} clientInfo;

int main(int argc, char *argv[])
{
	int serv_sd, clnt_sd;
	pthread_t thread; 
	
	struct sockaddr_in serv_adr, clnt_adr;
	socklen_t clnt_adr_sz;
	
	if (argc != 2) {
		printf("Usage: %s <port>\n", argv[0]);
		exit(1);
	}

	pthread_mutex_init(&mutx, NULL);
	
	serv_sd = socket(PF_INET, SOCK_STREAM, 0);   
	
	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_adr.sin_port = htons(atoi(argv[1]));
	
	if (bind(serv_sd, (struct sockaddr*)&serv_adr, sizeof(serv_adr)) == -1)
		error_handling("bind() error");
	if (listen(serv_sd, 5) == -1)
		error_handling("listen() error");

	while (1)
	{
		clnt_adr_sz = sizeof(clnt_adr);    
		clnt_sd = accept(serv_sd, (struct sockaddr*)&clnt_adr, &clnt_adr_sz);	

		clientInfo *clntInfo = malloc(sizeof(clientInfo));
		if (clntInfo == NULL) {
            perror("malloc() error");
            close(clnt_sd);
            continue;
        }

		clntInfo->clnt_sock = clnt_sd;
        strncpy(clntInfo->client_IP, inet_ntoa(clnt_adr.sin_addr), 15);
        clntInfo->client_IP[15] = '\0';

		// TODO: pthread_create & detach 
		pthread_create(&thread, NULL, handle_client, (void *)clntInfo);
        pthread_detach(thread);
		//printf("Connected client IP(sock=%d): %s \n", clnt_sd, inet_ntoa(clnt_adr.sin_addr));
	}
	
	close(serv_sd);
	pthread_mutex_destroy(&mutx);
	return 0;
}

void *handle_client(void *arg)
{
	// TODO: file receiving 
	clientInfo *clntInfo = (clientInfo *)arg;
    int clnt_sock = clntInfo->clnt_sock;
	char file_name[FILE_LEN];
	char msg[BUF_SIZE];
	char result[BUF_SIZE*2];
	int str_len, read_size=0;
	FILE *fp;

	read(clnt_sock, file_name, FILE_LEN);
	//printf("Received file data: %s\n", file_name);

	pthread_mutex_lock(&mutx);
	fp = fopen(file_name, "wb");
	if (fp == NULL) {
        perror("File open error");
        close(clnt_sock);
        pthread_mutex_unlock(&mutx);
		free(clntInfo);
        return NULL;
    }

    // 파일 내용 쓰기
    while ((str_len = read(clnt_sock, msg, BUF_SIZE)) > 0) {
        read_size += str_len;
        fwrite((void *)msg, 1, str_len, fp);
    }
    fclose(fp);
    pthread_mutex_unlock(&mutx);
	
	//파일 수신 성공
	printf("Received %s from %s\n", file_name, clntInfo->client_IP);

	//컴파일 및 실행
	char compile[BUF_SIZE];
    snprintf(compile, BUF_SIZE, "gcc -o temp_exec %s 2>&1", file_name);
    FILE *compile_fp = popen(compile, "r");
    if (compile_fp == NULL) {
        perror("popen() error");
        write(clnt_sock, "compile failed", 15);
        close(clnt_sock);
		free(clntInfo);
        return NULL;
    }

	// 컴파일 결과
	memset(result, 0, BUF_RST_SIZE);
    fread(result, 1, BUF_RST_SIZE-1, compile_fp);
    pclose(compile_fp);

    if (access("./temp_exec", F_OK) == -1) { // 실패
		char *error_message = "[Compilation Error]:\n";
        write(clnt_sock, error_message, strlen(error_message));
        write(clnt_sock, result, strlen(result));
        printf("Compilation error..\n\n");
		printf("----------------------------------------\n\n");
    } else { // 성공
        if (strstr(result, "warning")) { // warning
            char *warning_message = "[Compilation warnings]:\n";
            write(clnt_sock, warning_message, strlen(warning_message));
            write(clnt_sock, result, strlen(result));
			write(clnt_sock, "\n", strlen(result));
            printf("Compilation warning..\n");
        }
        FILE *exec_fp = popen("./temp_exec 2>&1", "r");

        if (exec_fp == NULL) {
            perror("Execution error");
            write(clnt_sock, "Execution failed", 16);
        } else {
            // 실행결과 출력
			memset(result, 0, BUF_RST_SIZE); // 저장버퍼 초기화
            fread(result, 1, BUF_RST_SIZE-1, exec_fp);
            pclose(exec_fp);
			printf("Compile %s and return results\n", file_name);
			printf("%s\n", result);
			printf("----------------------------------------\n\n");
            write(clnt_sock, result, strlen(result));
            //printf("Execution result sent to client %s\n", clnt_info->ip);
        }
        remove("./temp_exec"); // 임시파일 삭제
    }

	//write(clnt_sock, "----------------------------------------\n", 40);
	close(clnt_sock);
	free(clntInfo);
	
	return NULL;
}

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}