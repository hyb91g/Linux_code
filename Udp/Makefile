.PHONY:all
all:server client
cc=g++

client:UdpClient.cc 
	$(cc) -o $@ $^
server:UdpServer.cc 
	$(cc) -o $@ $^

.PHONY:clean
clean:
	rm -f client server
