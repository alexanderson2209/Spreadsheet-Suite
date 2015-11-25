/*******************************************************************************
  File: SpreadsheetServer.h
  Authors: Garrett Bigelow, Eric Stubbs, CJ Dimaano
  CS 3505 - Spring 2015
  Team SegFault
  Date created: April 5, 2015
  Last updated: April 21, 2015
*******************************************************************************/


#ifndef SPREADSHEETSERVER_H
#define SPREADSHEETSERVER_H


//
// Project headers.
//
#include "SpreadsheetSession.h"
#include "StringSocket.h"
#include "TcpListener.h"


//
// Multithreading library.
//
#include <pthread.h>


//
// Standard libraries.
//
#include <map>
#include <set>
#include <string>
#include <vector>


/// <summary>
///   Represents a server for hosting spreadsheets that can be edited by
///   multiple concurrent users.
/// </summary>
class SpreadsheetServer {

private :

  /// <summary>
  ///   Used as the payload for the StringSocket callbacks.
  /// </summary>
	typedef struct callbackState {
		StringSocket *clientPayload;    // The StringSocket associated with the
                                    //   callback.
		SpreadsheetServer *p_this;      // A pointer to the SpreadsheetServer object
                                    //   that the StringSocket belongs to.
	} callbackState;
  

public :

  /// <summary>
  ///   Default constructor.
  /// </summary>
  /// <param name="portNumber">
  ///   The specific port number to listen for connections.
  /// </param>
	SpreadsheetServer(std::string portNumber);
  
  
  /// <summary>
  ///   Copy constructor.
  /// </summary>
	SpreadsheetServer(const SpreadsheetServer & other);
  
  
  /// <summary>
  ///   Destructor.
  /// </summary>
	~SpreadsheetServer();
  

  /// <summary>
  ///   Initializes the server and begins listening for connections.
  /// </summary>
	void Start();
  
  
  /// <summary>
  ///   Shuts down the server.
  /// </summary>
	void Stop();

  
private :	

  /// <summary>
  ///   Keeps track of the port to listen for connections.
  /// </summary>
	std::string port;
  
  /// <summary>
  ///   Keeps track of the TcpListener for accepting socket connections.
  /// </summary>
	TcpListener *listener;
  
  /// <summary>
  ///   Lock object for the associatedSpreadsheets map.
  /// </summary>
  pthread_mutex_t associatedSpreadsheetsMutex;
  
  /// <summary>
  ///   Lock object for the openSpreadsheets map.
  /// </summary>
  pthread_mutex_t openSpreadsheetsMutex;
  
  /// <summary>
  ///   Lock object for the registeredUsernames set.
  /// </summary>
  pthread_mutex_t registeredUsernamesMutex;
  
  /// <summary>
  ///   Lock object for the callbackStates map.
  /// </summary>
  pthread_mutex_t callbackStatesMutex;

  /// <summary>
  ///   Keeps track of associations between a SpreadsheetSession and
  ///   StringSocket.
  /// </summary>
	std::map<StringSocket *, SpreadsheetSession *> associatedSpreadsheets;
  
  /// <summary>
  ///   Keeps track of associations between a spreadsheet name and
  ///   SpreadsheetSession.
  /// </summary>
	std::map<std::string, SpreadsheetSession *> openSpreadsheets;
  
  /// <summary>
  ///   Keeps track of all registered user names.
  /// </summary>
	std::set<std::string> registeredUsernames;
  
  /// <summary>
  ///   Keeps track of associations between a StringSocket and callbackState.
  /// </summary>
	std::map<StringSocket *, callbackState *> callbackStates;

  
  /// <summary>
  ///   The callback for the StringSocket::BeginSend method.
  /// </summary>
  static void clientSendCallback(int ex, void *payload);
  
  
  /// <summary>
  ///   The callback for the StringSocket::BeginReceive method.
  /// </summary>
	static void clientRecvCallback(std::string message, int ex, void *payload);
  
  
  /// <summary>
  ///   The callback for the TcpListener::BeginAcceptSocket method.
  /// </summary>
  static void listenerAcceptCallback(int ex, StringSocket *socket,
      void *payload);
  
  
  /// <summary>
  ///   Loads the set of usernames from the users file.
  /// </summary>
  void loadUsernames();
  
  
  /// <summary>
  ///   Saves the set of usernames to the users file.
  /// </summary>
  void saveUsernames();

};

#endif