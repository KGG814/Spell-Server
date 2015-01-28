
#include <iostream>
#include <fstream>
#include <string>
#include <set>
#include "SpellService.h"
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;
using boost::shared_ptr;

#define PORT 1
#define WORDS 2
#define MIN_ARGS 3

void printSeparator(void);
void printInfo (int port);
std::string wordFile;

class SpellServiceHandler : virtual public SpellServiceIf {
  public:

    // Use a set as a dictionary for quick lookup
    std::set<std::string> dict;

    // The constructor opens the file and imports it into our dictionary set

    SpellServiceHandler() {

      // Open the file containing the word we will use
      std::ifstream wordfile (wordFile);
      std::string word;

      // Check if the file is open, then iterate through and get our words
      if (wordfile.is_open()) {
        std::cout << "Wordlist Found" << std::endl;
        printSeparator();
        std::cout << "Waiting: " ;
        fflush(stdout);
        while(getline(wordfile,word)) {
          dict.insert(word);
        }
      } else {
        std::cout << "Unable to open file" << std::endl;
        exit(1);
      }
    }

    /*
    * spellcheck will go through the words and setup the response to the client.
    */
    void spellcheck(SpellResponse& _return, const SpellRequest& request) {

      std::cout << "Words Received!" << std::endl;

      // Iterate through the strings given
      for(std::vector<std::string>::const_iterator i = request.to_check.begin();
        i != request.to_check.end(); i++) {
        std::cout << *i << std::endl;
        // If the word is in the dictionary, set response bool to True, else set to False
        if (dict.count(*i) > 0) {
          _return.is_correct.push_back(true);
        } else {
          _return.is_correct.push_back(false);
        }
      }
      printSeparator();
      std::cout << "Waiting: ";
      fflush(stdout);
    }
};

int main(int argc, char **argv) {
  // Check if we have the minimum no. arguments
  int port;
  if (argc >= MIN_ARGS) {
    // Server Information
    port = atoi(argv[PORT]);
    wordFile = (argv[WORDS]);
  } else {
    std::cout << "Usage: ./server [port] [wordfile]" << std::endl;
    return 1;
  }

  printInfo(port);

  shared_ptr<SpellServiceHandler> handler(new SpellServiceHandler());
  shared_ptr<TProcessor> processor(new SpellServiceProcessor(handler));
  shared_ptr<TServerTransport> serverTransport(new TServerSocket(port));
  shared_ptr<TTransportFactory> transportFactory(new TBufferedTransportFactory());
  shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());
  TSimpleServer server(processor, serverTransport, transportFactory, protocolFactory);
  server.serve();
  return 0;
}

void printSeparator (void) {
  std::cout << "\033[1;31m--------------------------------------------\033[0m" << std::endl;
}

void printInfo (int port) {
  std::cout << "\033[1;31m--------------- Spell Server ---------------\033[0m" << std::endl;
  std::cout << "Written By Kalana Gamlath" << std::endl;
  std::cout << "Port : " << port << std::endl;
}
