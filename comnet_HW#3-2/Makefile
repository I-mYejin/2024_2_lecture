CC = gcc
# PORT = 9190
# SERVER_IP = 172.17.0.3

# 실행 파일
SERVER = occ_server
CLIENT = occ_client

#소스 파일
SERVER_SRC = occ_server.c
CLIENT_SRC = occ_client.c

# build 규칙
all: $(SERVER) $(CLIENT)

# build server
$(SERVER): $(SERVER_SRC)
	$(CC) $(SERVER_SRC) -o $(SERVER)

# Build client
$(CLIENT): $(CLIENT_SRC)
	$(CC) $(CLIENT_SRC) -o $(CLIENT)

# Clean up build
clean:
	rm -f $(SERVER) $(CLIENT)