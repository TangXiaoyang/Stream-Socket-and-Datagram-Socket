server_or: 
	./or

server_and: 
	./and

edge: 
	./eg

client: 
	./client jobs.txt

all: server_or.cpp server_and.cpp edge.cpp client.cpp
	g++ -std=c++11 server_or.cpp -o or
	g++ -std=c++11 server_and.cpp -o and
	g++ -std=c++11 edge.cpp -o eg
	g++ -std=c++11 client.cpp -o client

clean:
	rm or and eg client