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
#include <string>
#include <sstream>
#include <vector>
using namespace std;


#define OR_PORT "21307"
#define AND_PORT "22307"
#define TO_CLIENT_PORT "23307"
#define TO_BACKEND_PORT "24307"
#define BACKLOG 20
#define BUFFER_SIZE 8000



void as_talker(char* buffer, char* port);

void as_listener(vector<vector<string>> &back_to_client);

void divide(vector<string> v);

string trim(string s_to_trim);

string order_res(vector<vector<string>> res);

vector<string> get_string_vector(char* buffer, char dividor);

// The following code are from Beej's tutorial
void sigchld_handler(int s)
{
  while(waitpid(-1, NULL, WNOHANG) > 0);
}


void *get_in_addr(struct sockaddr *sa)
{
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }
  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}




int main(void)
{
  int sockfd, new_fd;
  struct addrinfo hints, *servinfo, *p;
  struct sockaddr_storage their_addr;
  socklen_t sin_size;
  struct sigaction sa;
  int yes=1;
  char s[INET6_ADDRSTRLEN];
  int rv;
  char buffer[BUFFER_SIZE];
  vector<vector<string>> back_to_client;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE; // use local host ip

  if ((rv = getaddrinfo(NULL, TO_CLIENT_PORT, &hints, &servinfo)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    return 1;
  }

  // find the first usable result
  for(p = servinfo; p != NULL; p = p->ai_next) {
    if ((sockfd = socket(p->ai_family, p->ai_socktype,
      p->ai_protocol)) == -1) {
      perror("server: socket");
      continue;
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
        sizeof(int)) == -1) {
      perror("setsockopt");
      exit(1);
    }

    if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
      close(sockfd);
      perror("server: bind");
      continue;
    }

    break;
  }

  if (p == NULL) {
    fprintf(stderr, "server: failed to bind\n");
    return 2;
  }

  freeaddrinfo(servinfo);

  if (listen(sockfd, BACKLOG) == -1) {
    perror("listen");
    exit(1);
  }

  sa.sa_handler = sigchld_handler; // clean processes
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;

  if (sigaction(SIGCHLD, &sa, NULL) == -1) {
    perror("sigaction");
    exit(1);
  }
// the end of the copied code


  cout << "The edge server is up and running." << endl;


  // accept the request from the client
  sin_size = sizeof their_addr;
  new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);

  if (new_fd == -1) {
    perror("accept");
    exit(1);
  }

  // get the data from the client
  int n = recv(new_fd, buffer, BUFFER_SIZE, 0);
  if(n < 0)
    perror("ERROR in recv");


  // put the data into a vector
  string str(buffer);
  vector<string> v;
  stringstream ss(str);
  string tok;
  while(getline(ss, tok, '\n')){
    stringstream temp(tok);
    string unit;
    while(getline(temp, unit, ','))
      v.push_back(unit);
  }
  

  cout << "The edge server has received " << v.size() / 3 << " lines from the client using TCP over port " << TO_CLIENT_PORT << "." << endl;

  // send the data to backend server
  divide(v);

  // get the result from the backends servers
  as_listener(back_to_client);
  
  // output the received result
  cout << "The computation results are:" << endl;
  for(unsigned i = 0; i < back_to_client.size(); i++){
    cout << back_to_client[i][1] << " " << back_to_client[i][2] << " " << back_to_client[i][3] << " = " << back_to_client[i][4] << endl;    
  }

  cout << "The edge server has successfully finished receiving all computation results from Backend-Server OR and Backend-Server And." << endl;

  // send result to the client
  string to_client_str = order_res(back_to_client);

  memset(buffer, 0, sizeof buffer);
  to_client_str.copy(buffer, to_client_str.length(), 0);

  n = send(new_fd, buffer, strlen(buffer), 0);
  if(n < 0)
    perror("send");

  cout << "The edge server has successfully finished sending all computation results to the client." << endl;

  // block the program
  n = recv(new_fd, buffer, BUFFER_SIZE, 0);
  if(n < 0)
    perror("ERROR in recv");

}











// The following code are from Beej's tutorial
void as_listener(vector<vector<string>> & back_to_client){
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

    if ((rv = getaddrinfo("localhost", TO_BACKEND_PORT, &hints, &servinfo)) != 0) {
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
//end of the copied code


    int counter = 0; //if counter == 2 then it means both server_or and server_and has sent back results
    while(1){
      memset(buffer, 0, sizeof buffer);
      if ((numbytes = recvfrom(sockfd, buffer, BUFFER_SIZE-1 , 0, 
        (struct sockaddr *)&their_addr, &addr_len)) == -1) {

        perror("recvfrom");
        exit(1);
      }

      if(counter == 0)
        cout << "The edge server start receiving the computation results from Backend-Server OR and Backend-Server AND using UDP over port " << TO_BACKEND_PORT << "." << endl;

      vector<string> lines = get_string_vector(buffer, '\n');
      for(unsigned i = 0; i < lines.size(); i++){
        memset(buffer, 0, sizeof buffer);
        lines[i].copy(buffer, lines[i].length(), 0);
        vector<string> line = get_string_vector(buffer, ',');
        back_to_client.push_back(line);
      }
      
      
      counter++;
      if(counter == 2) break;
    }
    

    close(sockfd);

}









// The following code are from Beej's tutorial
void as_talker(char* buffer, char* port){
  // create a udp scoket
  int sockfd;
  struct addrinfo hints, *servinfo, *p;
  int rv;
  int numbytes;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_DGRAM;

  if ((rv = getaddrinfo("localhost", port, &hints, &servinfo)) != 0) {
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
  divide the data, send "and" data to server_and and send "or" data to server_or
*/
void divide(vector<string> v){
  unsigned i = 0;
  int count_or = 0;
  int count_and = 0;
  int index = 0;
  while(i < v.size()){
    if(v[i].compare("or") == 0){
      count_or++;
      string s_or = to_string(index) + "," + trim(v[i + 1]) + "," + trim(v[i + 2]);
      char buffer[1024];
      memset(buffer, 0, sizeof buffer);
      s_or.copy(buffer, s_or.length(), 0);
      as_talker(buffer, OR_PORT);
    }else if(v[i].compare("and") == 0){
      count_and++;
      stringstream sstemp;
      sstemp << index;
      string s_and = sstemp.str() + "," + trim(v[i + 1]) + "," + trim(v[i + 2]);
      char buffer[1024];
      memset(buffer, 0, sizeof buffer);
      s_and.copy(buffer, s_and.length(), 0);
      as_talker(buffer, AND_PORT);
    }
    index++;
    i += 3;
  }

  string end = "end";
  char end_buffer[1024];
  memset(end_buffer, 0, sizeof end_buffer);
  end.copy(end_buffer, end.length(), 0);
  as_talker(end_buffer, OR_PORT);
  as_talker(end_buffer, AND_PORT);

  cout << "The edge has successfully sent " << count_or << " lines to Backend-Server OR." << endl;
  cout << "The edge has successfully sent " << count_and << " lines to Backend-Server AND." << endl;
}




/**
  chop the leading and trailing white-space
*/

string trim(string s_to_trim){
  size_t first = s_to_trim.find_first_not_of(' ');
  size_t last = s_to_trim.find_last_not_of(' ');
  return s_to_trim.substr(first, (last - first + 1));
}




/**
  solit the string by a char dividor
*/
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




/**
  order the result as received from the client
*/
string order_res(vector<vector<string>> res){
  vector<string> ordered_lines(res.size(), "");
  for(unsigned i = 0; i < res.size(); i++){
    int index = stoi(res[i][0]);
    string temp;
    temp += res[i][4] + "\n";
    ordered_lines[index] = temp;
  }
  string result;
  for(unsigned i = 0; i < ordered_lines.size(); i++)
    result += ordered_lines[i];

  return result;
}