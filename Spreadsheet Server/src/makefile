
server:	ManualResetEvent.o StringSocket.o TcpListener.o dependency_graph.o SpreadsheetSession.o SpreadsheetServer.h SpreadsheetServer.cpp
	g++ -pthread -lrt -o server ManualResetEvent.o StringSocket.o TcpListener.o dependency_graph.o SpreadsheetSession.o SpreadsheetServer.h SpreadsheetServer.cpp

.PHONY:	all test demo clean cleardata

ManualResetEvent.o:	ManualResetEvent.h ManualResetEvent.cpp
	g++ -pthread -lrt -c ManualResetEvent.cpp

StringSocket.o:	ManualResetEvent.h StringSocket.h StringSocket.cpp
	g++ -pthread -lrt -c StringSocket.cpp

TcpListener.o:	StringSocket.h TcpListener.h TcpListener.cpp
	g++ -pthread -lrt -c TcpListener.cpp

dependency_graph.o:	dependency_graph.h dependency_graph.cpp
	g++ -c dependency_graph.cpp

SpreadsheetSession.o:	StringSocket.h dependency_graph.h SpreadsheetSession.h SpreadsheetSession.cpp
	g++ -pthread -lrt -c SpreadsheetSession.cpp

clean:
	rm -f *.o

cleardata:
	rm -f -r spreadsheets users