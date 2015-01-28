#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <openssl/md5.h>


#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TTransportUtils.h>
#include "SpellService.h"

using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;


#define MIN_ARGS  		4			// Must have serverfiles and timeout to run
#define SERVERFILE1   1			// Argument for the server list file
#define SERVERFILE2   2
#define TIMEOUT				3			// Argument for the timeout
#define ARG_WORDS 		4			// The argument words start being read from

#define NUM_SHARDS    2     // Number of shards for dictionary
#define BUF_BYTES     256   // Bytes in buffer

void printSeparator(void);
void printInfo (void);
SpellResponse runClient (std::vector<std::string> serverList, SpellRequest request, int timeout);
void getResult (std::map<std::string, bool> &wordMap, SpellResponse response, SpellRequest request);

int main(int argc, char** argv) {

  int timeout;
  std::string serverFile [NUM_SHARDS];


  // Redirect errors to error log
  freopen ("SpellService.log","w",stderr);

  // Check if we have the minimum number of args and parse args
  if (argc >= MIN_ARGS) {
    serverFile[0] = argv[SERVERFILE1];
    serverFile[1] = argv[SERVERFILE2];
    timeout = atoi(argv[TIMEOUT]);
  } else {
    std::cout << "Usage: ./client [FILE1] [FILE2] [TIMEOUT] [words]" << std::endl;
    return 1;
  }

  /*####################    Get words   ####################*/
  int word = ARG_WORDS;
  std::vector<std::string> words;
  std::map<std::string, bool> wordMap;
  char buf[BUF_BYTES];
  char *p;
  std::string currWord;
  while(word < argc) {
    // Clear the buffer
    std::fill_n(buf, BUF_BYTES, 0);
    strcpy(buf, argv[word]);
    // Assure it is not an empty word
    assert(strlen(buf) > 0);
    // Remove trailing new line
    p = buf + strlen(buf) - 1;
    if (*p == '\n') *p = 0;
    // Remove trailing punctuation, could add more punctuation here, but just chose
    // full stop and question mark.
    p = buf + strlen(buf) - 1;
    if (*p == '?' || *p == '.') *p = 0;
    if (strlen(buf) > 0) {
      currWord = buf;
	    words.push_back(currWord);
      wordMap[argv[word]] = false;   
    }
    word++;
  }
  
  /*####################    Split words   ####################*/
  SpellRequest request [NUM_SHARDS];

  for (std::vector<std::string>::iterator it = words.begin();
       it != words.end(); ++it) {
    unsigned char shard, hash[16];
    MD5((const unsigned char *)(*it).c_str(), (*it).length(), hash);
    shard = hash[15] % NUM_SHARDS;
    request[shard].to_check.push_back(*it);
  }
  

  /*####################    Get servers   ####################*/

  std::string server;
  std::vector<std::string> serverList [NUM_SHARDS];
  
  for (int i = 0; i < NUM_SHARDS; i++) {
    // Read the servers in from file, line by line
    std::ifstream servers (serverFile[i].c_str());
    
    if (servers.is_open()) {
    	while(std::getline(servers, server)) {
    		serverList[i].push_back(server);
        std::cout << i << server << std::endl;
    	}
    } else {
    	std::cout << "Could not open file / File not found" << std::endl;
    }
    // Randomize the servers
    srand(time(0));
    std::random_shuffle(serverList[i].begin(), serverList[i].end());
  }
  /*######################## Send requests #######################*/
  SpellResponse response [NUM_SHARDS];
  printInfo();
  for (int i = 0; i < NUM_SHARDS; i++) {
    if (!request[i].to_check.empty()) {
      response[i] = runClient(serverList[i], request[i], timeout);
      getResult (wordMap, response[i], request[i]);
    }
  }
  printSeparator();
  for(auto it = words.begin(); it != words.end(); ++it) {
    if (wordMap[*it] == false) {
      std::cout << *it << std::endl;
    }
  }

}

SpellResponse runClient (std::vector<std::string> serverList, SpellRequest request, int timeout) {

	SpellResponse response;
  // Success flag for this shard
  int success = false;
	// Try each of the servers
	for(std::vector<std::string>::iterator currServer = serverList.begin();
				currServer != serverList.end(); ++currServer) {

		std::istringstream s_stream(*currServer);
		std::string server;
		std::string port;

		// Get the server IP and port no.
		std::getline(s_stream, server, ' ');
		std::getline(s_stream, port, ' ');

		printSeparator();
		std::cout << "Trying server: " << server << ":" << port << std::endl;

		// Create the socket, transport and protocol structures for thrift.
		// Must make for each new server contacted.
		TSocket* _socket = new TSocket(*currServer, atoi(port.c_str()));
		_socket->setRecvTimeout(1000*timeout);
		_socket->setConnTimeout(1000*timeout);
		boost::shared_ptr<TTransport> socket(_socket);
		boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
		boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
		SpellServiceClient client(protocol);


		// Attempt to use connection
		try {
			// Wait the user specified time for response
			transport->open();
      std::cout << "Checking" << std::endl;
			client.spellcheck(response, request);
			std::cout << "Connection Successful" << std::endl;
			transport->close();
      success = true;
			break;

		} catch (TException &tx) {
			std::cout << "Connection Failed" << std::endl;     
		}
	}
  // If none of them connected, exit on error
  if (success == false) {
    std::cout << "Service Failed" << std::endl;
    exit(1);
  }
	return response;
}

void getResult (std::map<std::string, bool> &wordMap, SpellResponse response, SpellRequest request) {

  // Iterate through the strings given
	for(int i = 0; i < response.is_correct.size(); i++) {
    wordMap[request.to_check[i]] = response.is_correct[i];
	}

}

void printSeparator (void) {
  std::cout << "\033[1;32m--------------------------------------------\033[0m" << std::endl;
}

void printInfo (void) {
  std::cout << "\033[1;32m------------------ Client ------------------\033[0m" << std::endl;
  std::cout << "Written By Kalana Gamlath" << std::endl;
}
