/*******************************************************************************
File: SpreadsheetSession.h
Authors: Garrett Bigelow, CJ Dimaano
CS 3505 - Spring 2015
Date created: April 5, 2015
Last updated: April 24, 2015
*******************************************************************************/

#ifndef SPREADSHEETSESSION_H
#define SPREADSHEETSESSION_H

#include "StringSocket.h"
#include "dependency_graph.h"
#include <string>
#include <vector>
#include <stack>
#include <map>
#include <set>
#include <pthread.h>

class SpreadsheetSession {

public:
	SpreadsheetSession(std::string name);					// Normal Constructor
	SpreadsheetSession(const SpreadsheetSession & other);	// Copy Constructor
	~SpreadsheetSession();									// Destructor 

	// pair<cellName, newContents> , userSendingTheCommand  
	// Checks for dependencies, then edits the cell's contents
	bool EditCell(std::string cellName, std::string editCommand);	

	bool AddClient(StringSocket* client1);			// Attempts to add a client to the session. Returns true if added
	bool RemoveClient(StringSocket* client2);		// Attempts to remove a client from this session. Returns true if removed
	bool Save();									// Saves the state of the spreadsheet to a plain text file
	bool Load();									// Loads the spreadsheet via the name of the plain text file
	bool UndoAll();									// Sends an undo command to all connected clients

	int GetUserCount();								// Returns the number of connected users to the server
	std::string GetName();
	std::map<std::string, std::string> GetCellMap();

private:
	std::set<std::string> GetCellsFromCommand(std::string);		// Takes in a command string and parses it, creating and return a set of cell names if possible
	static void clientSendCallback(int ex, void *payload);		// Callback for sending clients messages
  bool updateCell(std::string name, std::string contents);  // Updates the contents of a cell.
  
  void sendCell(std::string name, std::string content, StringSocket *ss);

	std::string sprdName;
	std::stack < std::pair < std::string, std::string > > history;
	std::set < std::string > clientNames;
	std::set < StringSocket* > clientSockets;
	std::map < std::string, std::string > cellMap;
	dependency_graph depGraph;
  
  pthread_mutex_t clientsMutex;
  pthread_mutex_t cellsMutex;
  
};

#endif