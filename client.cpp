#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
using namespace std;


#define EDGE_TCP_PORT "23307"
#define BUFFER_SIZE 8000

vector<string> get_string_vector(char* buffer, char dividor);


// The following code are from Beej's tutorial
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}
	
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
	int sockfd, numbytes;
	char buffer[BUFFER_SIZE];
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];
	char* filename;

	
	if (argc != 2) {
		fprintf(stderr,"Please enter the filename.\n");
		exit(1);
	}

	filename = argv[1];

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo("localhost", EDGE_TCP_PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
			p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("client: connect");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);

	freeaddrinfo(servinfo);
//end of the copied code

	cout << "The client is up and running." << endl;

	memset(buffer, 0, BUFFER_SIZE);

	// read the file
	ifstream my_reader(filename);
	string contents((istreambuf_iterator<char>(my_reader)), istreambuf_iterator<char>());
	contents.copy(buffer, contents.length(), 0);

	// send the data to the edge server
	int n = send(sockfd, buffer, strlen(buffer), 0);
	if(n < 0)
		perror("ERROR in send");


	vector<string> count_lines = get_string_vector(buffer, '\n');

	cout << "The client has successfully finished sending " << count_lines.size() << " lines to the edge server." << endl;

	// get the results from the edge server
	memset(buffer, 0, BUFFER_SIZE);
	if ((numbytes = recv(sockfd, buffer, BUFFER_SIZE-1, 0)) == -1) {
		perror("recv");
		exit(1);
	}

	cout << "The client has successfully finished receiving all computation results from the edge server." << endl;
	cout << "The final computation result are:" << endl;
	// display the calculation result
	cout << buffer << endl;

	// block the program
	memset(buffer, 0, BUFFER_SIZE);
	if ((numbytes = recv(sockfd, buffer, BUFFER_SIZE-1, 0)) == -1) {
		perror("recv");
		exit(1);
	}

	close(sockfd);
	return 0;
}






vector<string> get_string_vector(char* buffer, char dividor){
  vector<string> str_v;
  string s(buffer);
  stringstream ss(s);
  string tok;
  while(getline(ss, tok, dividor)){
    str_v.push_back(tok);
  }
  return str_v;
}