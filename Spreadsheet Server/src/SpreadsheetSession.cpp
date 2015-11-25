/*******************************************************************************
File: SpreadsheetSession.cpp
Author: Garrett Bigelow, CJ Dimaano
CS 3505 - Spring 2015
Date created: April 5, 2015
Last updated: April 24, 2015
*******************************************************************************/


#include "SpreadsheetSession.h"
#include "StringSocket.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <sys/stat.h>

using namespace std;

/// <summary>
///		Constructs a spreadsheet session with the given name. Does not load the spreadsheet or 
///		add any clients.
///		
///		If the name does not include a ".txt" extension, adds the extension to the name.
/// </summary>
SpreadsheetSession::SpreadsheetSession(string name)
{
  sprdName = name;
  pthread_mutex_init(&clientsMutex, NULL);
  pthread_mutex_init(&cellsMutex, NULL);
}

/// <summary>
///		Copy constructor
/// </summary>
SpreadsheetSession::SpreadsheetSession(const SpreadsheetSession & other)
  : sprdName(other.sprdName), history(other.history), clientNames(other.clientNames), clientSockets(other.clientSockets), cellMap(other.cellMap), depGraph(other.depGraph), clientsMutex(other.clientsMutex), cellsMutex(other.cellsMutex)
{
	// Does nothing
}

/// <summary>
///		Destructor
/// </summary>
SpreadsheetSession::~SpreadsheetSession()
{
	pthread_mutex_destroy(&clientsMutex);
	pthread_mutex_destroy(&cellsMutex);
}

/// <summary>
///		Unless there is a Circular Exception, replaces the cell contents in this spreadsheet 
///		and adds the edit to the stack history. If there is a circular exception, sends an edit 
///		command to the client to undo their edit.
///
///		Returns whether or not an edit was made.
/// </summary>
bool SpreadsheetSession::EditCell(string cellName, string cellContents)
{
	pthread_mutex_lock(&cellsMutex);

  string oldContents = cellMap[cellName];
  if(oldContents == "")
    cellMap.erase(cellName);
  
  // Return false if editing the cell would result in a circular dependency.
  if(!updateCell(cellName, cellContents)) {
    pthread_mutex_unlock(&cellsMutex);
    return false;
  }
  
  // Update the edit history.
	history.push(make_pair(cellName, oldContents));

	// Send to clients
	for (set<StringSocket*>::iterator it = clientSockets.begin(); it != clientSockets.end(); it++)
	{
    sendCell(cellName, cellContents, *it);
	}

	pthread_mutex_unlock(&cellsMutex);
  
  // Save the spreadsheet.
  this->Save();

	return true;
}

/// <summary>
///		Sends a command to every client to revert the last cell edit by 
///		sending them the information of the last edit.
///
///		Returns false if there are no edits in the history to undo
/// </summary>
bool SpreadsheetSession::UndoAll()
{
	pthread_mutex_lock(&cellsMutex);

	// If there is nothing in history, do nothing
	if (history.size() == 0)
	{
		pthread_mutex_unlock(&cellsMutex);
		return false;
	}

	// Otherwise, get last edit
	pair<string, string> edit = history.top();
	history.pop();
  
  // Update the cell.
  updateCell(edit.first, edit.second);

	// Send the edit to every client
	for (set<StringSocket*>::iterator it = clientSockets.begin(); it != clientSockets.end(); it++)
	{
    sendCell(edit.first, edit.second, *it);
	}
  
	pthread_mutex_unlock(&cellsMutex);
  
  // Save the spreadsheet.
  this->Save();

	return true;
}

/// <summary>
///		Attempts to add a client's socket to the spreadsheet session, allowing communication between
///		the two. Will not add the same client socket to the session.
///
///		Returns true if the client and their associated socket are added to this spreadsheet session.
/// </summary>
bool SpreadsheetSession::AddClient(StringSocket* client)
{
	pthread_mutex_lock(&clientsMutex);

	pair<set<StringSocket*>::iterator, bool> ret;
	ret = clientSockets.insert(client);		// Returns true if the socket was added to the set, false otherwise
	
	if (ret.second)
	{
		// Send a message to the client to confirm the connection
		ostringstream cmd;
		cmd << "connected " << cellMap.size();
		client->BeginSend(cmd.str(), clientSendCallback, NULL);

		// Send the client the spreadsheet data
		for (map<string, string>::iterator it = cellMap.begin(); it != cellMap.end(); it++)
		{
      sendCell(it->first, it->second, client);
		}
	}

	pthread_mutex_unlock(&clientsMutex);

	return ret.second;
}

/// <summary>
///		Attempts to remove a client's socket to the spreadsheet session. If the client's socket does 
///		not exist in this spreadsheet session, does nothing.
///
///		Returns true if the client and their associated socket are added to this spreadsheet session.
///		False otherwise.
/// </summary>
bool SpreadsheetSession::RemoveClient(StringSocket* client)
{
	pthread_mutex_lock(&clientsMutex);

	set<StringSocket*>::iterator it;
	it = clientSockets.find(client);
	if (it != clientSockets.end())
	{
		clientSockets.erase(client);
    
		pthread_mutex_unlock(&clientsMutex);

		return true;
	}

	pthread_mutex_unlock(&clientsMutex);

	return false;
}

/// <summary>
///		Saves the spreadsheet session's cells and correspondnig content information to a plain text file
///		in the following format:
///
///		cellname1 contents1 
///		cellname2 contents2 
///		
///		Returns true upon successfully saving the file. False otherwise.
/// </summary>
bool SpreadsheetSession::Save()
{
	pthread_mutex_lock(&cellsMutex);
  
	// Make the file
	ofstream sprdFile((string("./spreadsheets/") + sprdName).c_str());	// The data in this file will be overwritten
	if (sprdFile.is_open())
	{
		// Some temp variables
		string cell;
		string contents;

		// Write to the file
		for (map<string, string>::iterator it = cellMap.begin(); it != cellMap.end(); it++)
		{
			cell = it->first + " ";
			contents = it->second;
			sprdFile << cell;
			sprdFile << contents;
      sprdFile << "\n";
		}

		sprdFile.close();

		pthread_mutex_unlock(&cellsMutex);

		return true;
	}

	pthread_mutex_unlock(&cellsMutex);
	
	return false;
}

/// <summary>
///		Loads the cell names and contents for this spreadsheet session via the name of the spreadsheet's
///		associated file, established upon construction of the spreadsheet (a .txt file).
///
///		Returns true upon successfully loading the file. False otherwise.
/// </summary>
bool SpreadsheetSession::Load()
{
	pthread_mutex_lock(&cellsMutex);
  
	// Make sure the edit history and the cell map are empty so we don't reaload
	//   the data file if it has already been loaded.
	if (history.empty() && cellMap.empty())
	{
		string filename = string("./spreadsheets/") + sprdName;

		// Make sure that the subdirectory for spreadsheets exists.
		struct stat sb;
		if (stat("./spreadsheets", &sb) == -1)
		{
			mkdir("./spreadsheets", S_IRUSR | S_IWUSR | S_IXUSR);
		}

		// Grab the file data
		ifstream sprdFile(filename.c_str());
		if (sprdFile.is_open())
		{
			// Read the contents of the file
			string cell;
			string content;
			string line;

			// Get each line
			while (getline(sprdFile, line))
			{
        // Get the cell name and contents.
        size_t br = line.find(' ');
        cell = line.substr(0, br);
        content = line.substr(br + 1);

        // Update the cell.
        updateCell(cell, content);

      }

			// Done - close 
			sprdFile.close();
		}
		else
		{
      // Create the file if it does not exist.
			std::ofstream file(filename.c_str());
			file.close();

			// Make sure that the file was created.
			struct stat sb;

			if (stat(filename.c_str(), &sb) == -1)
			{
				pthread_mutex_unlock(&cellsMutex);
				return false;
			}
		}
		pthread_mutex_unlock(&cellsMutex);

		return true;
	}
}

/// <summary>
///		Returns the number of connected users to this spreadsheet session
/// </summary>
int SpreadsheetSession::GetUserCount()
{
	return clientSockets.size();
}

///	<summary>
///		Returns the name of the spreadsheet session, ".txt" appended
///	</summary>
string SpreadsheetSession::GetName()
{
	return sprdName;
}

///	<summary>
///		Returns the map of cells to contents in this spreadsheet session
///	</summary>
map<string, string> SpreadsheetSession::GetCellMap()
{
	return cellMap;
}


/****************************

Private functions

****************************/

///	<summary>
///		A callback function for client->BeginSend();
///	</summary>
void SpreadsheetSession::clientSendCallback(int ex, void *payload)
{
	// Does nothing
}

///	<summary>
///		Parses a string (assumed to be normalized cell contents) into a set of
///		cell names if that string is a formula and returns the set.
///	</summary>
set<string> SpreadsheetSession::GetCellsFromCommand(string contents)
{
	set<string> refCells;

	// If the contents are a formula, parse it for cell values
	if (contents[0] == '=')
	{
		for (int i = 1; i < contents.length() - 1; i++)
		{
			// Check if the character is the beginning of a cellname
			if((contents[i] >= 'a' && contents[i] <= 'z') || (contents[i] >= 'A' && contents[i] <= 'Z')) 
			{
				string cell;
        
				// Letters first followed by numbers.
				while (((contents[i] >= 'a' && contents[i] <= 'z') || (contents[i] >= 'A' && contents[i] <= 'Z')) && i < contents.length())
				{
          if(contents[i] >= 'a' && contents[i] <= 'z')
            cell += (char)(contents[i] - 'a' + 'A');
          else
            cell += contents[i];
					i++;
				}
				while ((contents[i] >= '0' && contents[i] <= '9') && i < contents.length())
				{
					cell += contents[i];
					i++;
				}
        
				refCells.insert(cell);
			}
		}
	}

	return refCells;
}

/// <summary>
///   Attempts to updates the contents of a cell and returns true if successful.
/// </summary>
bool SpreadsheetSession::updateCell(string name, string contents)
{
	// Parse the contents to discover if there are any cell names in it
	set<string> refCells;
	refCells = GetCellsFromCommand(contents);
  
  // Return false if a circular dependency would occur.
  if(!depGraph.replace_dependees(name, refCells))
    return false;

  // Update the cell map.
	if(contents == "")
		cellMap.erase(name);
	else
		cellMap[name] = contents;
	
	return true;
}

/// <summary>
/// </summary>
void SpreadsheetSession::sendCell(string name, string content, StringSocket *ss) {
  ss->BeginSend("cell " + name + " " + content, SpreadsheetSession::clientSendCallback, NULL);
}
