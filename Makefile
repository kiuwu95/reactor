SERVER_SRC=Acceptor.cpp TcpServer.cpp Channel.cpp EventLoop.cpp Svr.cpp Connection.cpp ServerBuffer.cpp 
TARGET1=Server

CLIENT_SRC=Client.cpp Clt.cpp ClientBuffer.cpp
TARGET2=Client

HEAD=include/GetTime.h include/MemoryPool.h

all:$(TARGET1) $(TARGET2)

$(TARGET1): $(SERVER_SRC) $(HEAD) 
	g++ -Wall -pthread -std=c++17 -I/client $(SERVER_SRC) -O2 -o $(TARGET1) -ltcmalloc

$(TARGET2): $(CLIENT_SRC) $(HEAD)
	g++ -Wall -std=c++17 -I/TcpServer $(CLIENT_SRC) -O2 -o $(TARGET2) -ltcmalloc
	
clean:
	rm -f $(TARGET1) $(TARGET2)
	
.PHONY: all clean