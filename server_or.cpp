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
#include <sstream>
#include <string>
#include <vector>
using namespace std;


#define OR_PORT "21307"
#define EDGE_PORT "24307"
#define BUFFER_SIZE 8000


void as_listener(string & back_to_edge, bool is_first);

void as_talker(char* buffer);

string calculate_or(string s1, string s2);


// The following code are from Beej's tutorial
// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
  if (sa->sa_family == AF_INET) {
  return &(((struct sockaddr_in*)sa)->sin_addr);
}

  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
//end of the copied server





int main(void)
{
	char buffer[BUFFER_SIZE];
  string back_to_edge;

  // receive the data from the edge server
  as_listener(back_to_edge, true);

  memset(buffer, 0, sizeof buffer);
  back_to_edge.copy(buffer, back_to_edge.length(), 0);

  // send back the calculation result to edge server
  as_talker(buffer);
  cout << "The Server OR has successfully finished sending all computation results to the edge server." << endl;

  // block the program
  as_listener(back_to_edge, false);

}







void as_listener(string & back_to_edge, bool is_first){
// The following code are from Beej's tutorial
	int sockfd;
  	struct addrinfo hints, *servinfo, *p;
  	int rv;
  	int numbytes;
  	struct sockaddr_storage their_addr;
  	char buffer[BUFFER_SIZE];
  	socklen_t addr_len;
  	char s[INET6_ADDRSTRLEN];

  	memset(&hints, 0, sizeof hints);

  	hints.ai_family = AF_UNSPEC;
  	hints.ai_socktype = SOCK_DGRAM;
  	hints.ai_flags = AI_PASSIVE;

  	if ((rv = getaddrinfo(NULL, OR_PORT, &hints, &servinfo)) != 0) {
    	fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    	exit(1);
  	}

  	// loop to get the socket
  	for(p = servinfo; p != NULL; p = p->ai_next) {

	    if ((sockfd = socket(p->ai_family, p->ai_socktype,
	         p->ai_protocol)) == -1) {
	      	perror("listener: socket");
	      	continue;
	    }

	    if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
	      	close(sockfd);
	      	perror("listener: bind");
	      	continue;
	    }

    	break;
  	}

  	if (p == NULL) {
    	fprintf(stderr, "listener: failed to bind socket\n");
    	exit(2);
  	}

  	freeaddrinfo(servinfo);
  	addr_len = sizeof their_addr;
// end of the copied code

    if(is_first){
      cout << "The Server OR is up and running using UDP on port " << OR_PORT << "." << endl;
      cout << "The Server OR start receiving lines from the edge server for OR computation. The computation results are:" << endl;
    }


    int linecount = 0;
    while(1){

      memset(buffer, 0 , BUFFER_SIZE);
  	  if ((numbytes = recvfrom(sockfd, buffer, BUFFER_SIZE-1 , 0, 
        (struct sockaddr *)&their_addr, &addr_len)) == -1) {

        perror("recvfrom");
        exit(1);
      }



      string data(buffer);
      stringstream ss(data);
      string tok;
      vector<string> v;
      while(getline(ss, tok, ','))
        v.push_back(tok);
      if(v.size() == 1){
        cout << "The Server OR has successfully received " << linecount << " lines from the edge server and finished all OR computations." << endl;
        break;
      }
      
      string res = calculate_or(v[1], v[2]);
      cout << v[1] << " or " << v[2] << " = " << res << endl;

      // store the result
      string send_back = v[0] + "," + v[1] + ",or," + v[2] + "," + res + "\n";
      back_to_edge += send_back;
      
      linecount++;
    }
    close(sockfd);

}









void as_talker(char* buffer){
// The following code are from Beej's tutorial
	// create a udp scoket
  	int sockfd;
  	struct addrinfo hints, *servinfo, *p;
  	int rv;
  	int numbytes;

  	memset(&hints, 0, sizeof hints);
  	hints.ai_family = AF_UNSPEC;
  	hints.ai_socktype = SOCK_DGRAM;

	if ((rv = getaddrinfo("localhost", EDGE_PORT, &hints, &servinfo)) != 0) {
    	fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    	exit(1);
  	}

  	// loop to get a socket
  	for(p = servinfo; p != NULL; p = p->ai_next) {
	    if ((sockfd = socket(p->ai_family, p->ai_socktype,
	        p->ai_protocol)) == -1) {
	      	perror("talker: socket");
	      	continue;
	    }

	    break;
  	}

  	if (p == NULL) {
	    fprintf(stderr, "talker: failed to bind socket\n");
	    exit(2);
  	}

  	if ((numbytes = sendto(sockfd, buffer, strlen(buffer), 0,
	    p->ai_addr, p->ai_addrlen)) == -1) {

	    perror("talker: sendto");
	    exit(1);
	}

  	freeaddrinfo(servinfo);
  	close(sockfd); 

}
// end of the copied code




/**
  the "or" calculation function
*/
string calculate_or(string s1, string s2){
  string longer;
  string shorter;
  string result;
  if(s1.length() > s2.length()){
    longer = s1;
    shorter = s2;
  }
  else{
    longer = s2;
    shorter = s1;
  }
    unsigned len = longer.length() - shorter.length();
  for(unsigned i = 0; i < len; i++){
    shorter = "0" + shorter;
  }
  for(unsigned i = 0; i < longer.length(); i++){
    if(longer.at(i) == '0' && shorter.at(i) == '0')
      result += "0";
    else
      result += "1";
  }
  size_t first = result.find_first_not_of('0');
  if(first == string::npos)
    result = "0";
  else
    result = result.substr(first);
  return result;
}