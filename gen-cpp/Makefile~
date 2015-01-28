all: server client

client: SpellClient.o SpellService.o spell_constants.o spell_types.o
	g++ SpellClient.o SpellService.o spell_constants.o spell_types.o -o client -l thrift -std=c++11 -lcrypto
server: SpellServer.o SpellService.o spell_constants.o spell_types.o
	g++ SpellServer.o SpellService.o spell_constants.o spell_types.o -o server -l thrift -std=c++11

SpellClient.o: SpellClient.cpp
	g++ -c SpellClient.cpp -std=c++11

SpellServer.o: SpellServer.cpp
	g++ -c SpellServer.cpp -std=c++11

SpellService.o: SpellService.cpp
	g++ -c SpellService.cpp -std=c++11

spell_constants.o: spell_constants.cpp
	g++ -c spell_constants.cpp -std=c++11

spell_types.o: spell_types.cpp
	g++ -c spell_types.cpp -std=c++11

clean:
	rm -rf *o server SpellService.log
