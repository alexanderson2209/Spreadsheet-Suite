/*******************************************************************************
  File: SpreadsheetServer.cpp
  Authors: Garrett Bigelow, Eric Stubbs, CJ Dimaano
  CS 3505 - Spring 2015
  Team SegFault
  Date created: April 5, 2015
  Last updated: April 24, 2015
*******************************************************************************/


//
// Class header file.
//
#include "SpreadsheetServer.h"

//
// Standard libraries.
//
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>

//
// Threading libraries.
//
#include <signal.h>


/*******************************************************************************
  Main.
*******************************************************************************/


int main(int argc, char **argv) {
  
  // Ignore the SIGPIPE signal.
  signal(SIGPIPE, SIG_IGN);

  // Set a pointer to the server to NULL.
	SpreadsheetServer *server = NULL;

  
	// Create a SpreadsheetServer with the default port if one was not specified.
	if (argc == 1)
		server = new SpreadsheetServer("2000");
  
  // Check if an argument was provided at the command prompt.
	else if (argc == 2) {
    
    // Try to convert the argument to a port number.
		int port = atoi(argv[1]);

    
		// Create a SpreadsheetServer with a specific port number if one was
    //   specified.
		if (port >= 2112 && port <= 2120)
			server = new SpreadsheetServer(argv[1]);
    
    // Print an error message if an invalid port number was provided and exit.
		else {
			std::cout << "Invalid port number. Port must be between 2112 and 2120."
          << std::endl;
      return -1;
		}
    
	}
  
  // Print the usage message and exit if too many arguments were provided.
	else {
    
		std::cout << "Usage: " << argv[0] << " <port>" << std::endl;
    std::cout << "\t<port>\tA valid port number used for accepting connections."
        << std::endl;
    std::cout << "\t      \t  Valid ports are 2000 and 2112 to 2120."
        << std::endl;
    return 0;
    
	}
	
  
  std::cout << "The server can be stopped with the STOP command." << std::endl;

  
	// Start the server
	server->Start();
  
  
  // Wait for the user to stop the server.
  std::string cmd = "";
  do {
    std::cin >> cmd;
  } while(cmd != "STOP");
  
  
  // Stop the server.
  server->Stop();
  delete server;
  
  
  return 0;
  
}


/*******************************************************************************
  SpreadsheetServer methods.
*******************************************************************************/


SpreadsheetServer::SpreadsheetServer(std::string portNumber)
    : port(portNumber), listener(NULL) {

  pthread_mutex_init(&this->associatedSpreadsheetsMutex, NULL);
  pthread_mutex_init(&this->openSpreadsheetsMutex, NULL);
  pthread_mutex_init(&this->registeredUsernamesMutex, NULL);
  pthread_mutex_init(&this->callbackStatesMutex, NULL);

}


SpreadsheetServer::SpreadsheetServer(const SpreadsheetServer & server)
    : port(server.port), listener(server.listener),
      associatedSpreadsheetsMutex(server.associatedSpreadsheetsMutex),
      openSpreadsheetsMutex(server.openSpreadsheetsMutex),
      registeredUsernamesMutex(server.registeredUsernamesMutex),
      callbackStatesMutex(server.callbackStatesMutex),
      associatedSpreadsheets(server.associatedSpreadsheets),
      openSpreadsheets(server.openSpreadsheets),
      registeredUsernames(server.registeredUsernames),
      callbackStates(server.callbackStates) {
  //
  // Do nothing.
  //
}


SpreadsheetServer::~SpreadsheetServer() {
  
  // Stop the server if it is still running.
  this->Stop();
  
  
  // Release all the lock objects.
  pthread_mutex_destroy(&this->associatedSpreadsheetsMutex);
  pthread_mutex_destroy(&this->openSpreadsheetsMutex);
  pthread_mutex_destroy(&this->registeredUsernamesMutex);
  pthread_mutex_destroy(&this->callbackStatesMutex);
  
}


void SpreadsheetServer::Start() {
  
  // Load usernames.
  std::cout << "Loading usernames...";
  this->loadUsernames();
  std::cout << " done." << std::endl;

  
  // Create a TcpListener to listen for connections.
	TcpListener::CreateTcpListener("", this->port, &this->listener);

  // Start listening for connections.
	this->listener->Start();
  std::cout << "Listening for connections on " << this->listener->ToString()
      << std::endl;
  
  
  // Accept the first connection.
  this->listener->BeginAcceptSocket(
      SpreadsheetServer::listenerAcceptCallback,
      this
    );

}


void SpreadsheetServer::Stop() {
  
  // Shut down the server if the listener is not NULL.
  if(this->listener != NULL) {
    
    // Save all the spreadsheets.
    for(std::map<std::string, SpreadsheetSession *>::iterator it =
        this->openSpreadsheets.begin(); it != this->openSpreadsheets.end();
        it++) {

      (*it).second->Save();
      delete (*it).second;

    }
    this->openSpreadsheets.clear();
    this->associatedSpreadsheets.clear();

    
    // Stop listening for connections and close TcpListener.
    listener->Stop();
    freeTcpListener(&listener);
    
    // Close all of the sockets.
    for(std::map<StringSocket *, callbackState *>::iterator it =
        this->callbackStates.begin(); it != this->callbackStates.end(); it++)
      (*it).first->Close();
    
    
    // Save the user names.
    this->saveUsernames();
    this->registeredUsernames.clear();
    
  }
  
}


void SpreadsheetServer::clientSendCallback(int ex, void *payload) {
  //
  // Do nothing.
  //
}









void SpreadsheetServer::clientRecvCallback(std::string message, int ex, void *payload)
{
  // Try to get the callbackState from the payload.
  callbackState *state = static_cast<callbackState*>(payload);

  // If there is no state, there is nothing to do here.
  if(state == NULL)
    return;

  // Pull the client socket and the pointer to this from the payload.
  StringSocket *client = state->clientPayload;
  SpreadsheetServer *p_this = state->p_this;

  // If there are no exceptions, parse the message.
  if (ex == SS_NO_EXCEPTION)
  {
    std::cout << client->ToString() << ": " << message << std::endl;
    
    // Get the command and info out of the message.
    std::string cmd = message;
    std::string info;
    size_t br = message.find(' ');
    if(br != std::string::npos) {
      cmd = message.substr(0, br);
      info = message.substr(br + 1);
    }
    
    

    // Parse the message and process it accordingly.
    if (cmd == "connect")
    {
      // Stop any other threads from accessing the map at the same time.
      int connected = 0;
      pthread_mutex_lock(&p_this->associatedSpreadsheetsMutex); {
        connected = p_this->associatedSpreadsheets.count(client);
      } pthread_mutex_unlock(&p_this->associatedSpreadsheetsMutex);

      // If this StringSocket is already connected to a spreadsheet, send an error message.
      if (connected)
      {
        client->BeginSend("error 2 You are already connected to a Spreadsheet: you must connect to a new Spreadsheet using a new connection.", SpreadsheetServer::clientSendCallback, state);

        // Continue receiving messages from the client and return;
        client->BeginReceive(SpreadsheetServer::clientRecvCallback, state);
        return;
      }

      // Get the username and spreadsheet name from the info.
      std::string username = info;
      std::string spreadsheetName;
      br = info.find(' ');
      if(br != std::string::npos) {
        username = info.substr(0, br);
        spreadsheetName = info.substr(br + 1);
      }
      

      // Stop any other threads from accessing the set at the same time.
      int registered = 0;
      pthread_mutex_lock(&p_this->registeredUsernamesMutex); {
        registered = p_this->registeredUsernames.count(username);
      } pthread_mutex_unlock(&p_this->registeredUsernamesMutex);

      // If the username is not registered, send an error message to the client.
      if (!registered)
      {
        client->BeginSend("error 4 " + username, SpreadsheetServer::clientSendCallback, state);

        // Continue receiving messages from the client and return.
        client->BeginReceive(SpreadsheetServer::clientRecvCallback, state);
        return;
      }

      SpreadsheetSession *session = NULL;

      // Stop any other threads from accessing the map at the same time.
      int opened = 0;
      pthread_mutex_lock(&p_this->openSpreadsheetsMutex); {
        opened = p_this->openSpreadsheets.count(spreadsheetName);
      } pthread_mutex_unlock(&p_this->openSpreadsheetsMutex);

      // If the spreadsheet is not opened by another client, add a new SpreadsheetSession to the server.
      if (!opened)
      {
        session = new SpreadsheetSession(spreadsheetName);
        bool successful = session->Load();
        if (!successful)
        {
          // Since the spreadsheet could not be opened, delete the session and send an error message.
          delete session;

          client->BeginSend("error 0 The spreadsheet could not be loaded correctly.", SpreadsheetServer::clientSendCallback, state);

          // Continue receiving messages from the client and return.
          client->BeginReceive(SpreadsheetServer::clientRecvCallback, state);
          return;
        }

        // Stop any other threads from accessing the map at the same time.
        pthread_mutex_lock(&p_this->openSpreadsheetsMutex); {
          p_this->openSpreadsheets.insert(std::pair<std::string, SpreadsheetSession*>(spreadsheetName, session));
        } pthread_mutex_unlock(&p_this->openSpreadsheetsMutex);

      }
      else
      {
        // Stop any other threads from accessing the map at the same time.
        pthread_mutex_lock(&p_this->openSpreadsheetsMutex); {
          session = p_this->openSpreadsheets.find(spreadsheetName)->second;
        } pthread_mutex_unlock(&p_this->openSpreadsheetsMutex);
      }

      // Stop any other threads from accessing the map at the same time.
      pthread_mutex_lock(&p_this->associatedSpreadsheetsMutex); {
      // Add the StringSocket to the map of Sockets to SpreadsheetSessions.
        p_this->associatedSpreadsheets.insert(std::pair<StringSocket*, SpreadsheetSession*>(client, session));
      } pthread_mutex_unlock(&p_this->associatedSpreadsheetsMutex);

      // Add the socket to the SpreadsheetSession.
      // In addition to adding the client to the session, AddClient sends the client all needed spreadsheet data.
      bool added = session->AddClient(client);
      if (!added)
      {
        client->BeginSend("error 3 You are already connected to this spreadsheet.", p_this->clientSendCallback, state);

        // Continue receiving messages from the client and return.
        client->BeginReceive(SpreadsheetServer::clientRecvCallback, state);
        return;
      }
    }


    else if (cmd == "register")
    {
      int connected = 0;
      // Stop any other threads from accessing the map at the same time.
      pthread_mutex_lock(&p_this->associatedSpreadsheetsMutex); {
        connected = p_this->associatedSpreadsheets.count(client);
      } pthread_mutex_unlock(&p_this->associatedSpreadsheetsMutex);

      // If the client trying to register the username isn't connected, send an error message.
      if (!connected)
      {
        client->BeginSend("error 3 You must be connected to a spreadsheet in order to register a user name.", p_this->clientSendCallback, state);

        // Continue receiving messages from the client and return.
        client->BeginReceive(SpreadsheetServer::clientRecvCallback, state);
        return;
      }

      // Get the user name out of the info.
      std::string username = info;
      br = info.find(' ');
      if(br != std::string::npos)
        username = info.substr(0, br);

      int alreadyAdded = 0;
      // Stop any other threads from accessing the set at the same time.
      pthread_mutex_lock(&p_this->registeredUsernamesMutex); {
        alreadyAdded = p_this->registeredUsernames.count(username);
      } pthread_mutex_unlock(&p_this->registeredUsernamesMutex);

      // Add the username to the list of registered usernames.
      if (!alreadyAdded)
      {
        // Stop any other threads from accessing the set at the same time
        bool added = false;
        pthread_mutex_lock(&p_this->registeredUsernamesMutex); {
          added = p_this->registeredUsernames.insert(username).second;
        } pthread_mutex_unlock(&p_this->registeredUsernamesMutex);

        if (!added)
        {
          // If the name could not be added, send an error message to the client.
          client->BeginSend("error 0 There was a problem registering the username.", p_this->clientSendCallback, state);

          // Continue receiving messages from the client and return.
          client->BeginReceive(SpreadsheetServer::clientRecvCallback, state);
          return;
        }
        
        // Append the username to the users file.
        std::ofstream users("users", std::ofstream::out | std::ofstream::app);
        users << username << "\n";
        users.close();
      }
      else
      {
        client->BeginSend("error 4 The username you are trying to register is already registered.", p_this->clientSendCallback, state);
      }

      // Continue receiving messages from the client and return.
      client->BeginReceive(SpreadsheetServer::clientRecvCallback, state);
      return;
    }
    
    
    else if (cmd == "cell")
    {
      int connected = 0;
      // Stop any other threads from accessing the map at the same time.
      pthread_mutex_lock(&p_this->associatedSpreadsheetsMutex); {
        connected = p_this->associatedSpreadsheets.count(client);
      } pthread_mutex_unlock(&p_this->associatedSpreadsheetsMutex);

      // If the client isn't connected to a spreadsheet, send an error message.
      if (!connected)
      {
        client->BeginSend("error 3 You must be connected to a spreadsheet in order to use an edit command.", SpreadsheetServer::clientSendCallback, state);

        // Continue receiving messages from the client and return;
        client->BeginReceive(SpreadsheetServer::clientRecvCallback, state);
        return;
      }

      SpreadsheetSession *session = NULL;

      // Stop any other threads from accessing the map at the same time.
      pthread_mutex_lock(&p_this->associatedSpreadsheetsMutex); {
        session = p_this->associatedSpreadsheets[client];
      } pthread_mutex_unlock(&p_this->associatedSpreadsheetsMutex);

      // Get the cell name and contents out of the info.
      std::string cellName = info;
      std::string cellContents;
      br = info.find(' ');
      if(br != std::string::npos) {
        cellName = info.substr(0, br);
        cellContents = info.substr(br + 1);
      }

      // If there is a circular dependency, send an error message to the client who attempted the edit.
      // Otherwise, EditCell pushes edits out to all clients connected to the session.
      if (!session->EditCell(cellName, cellContents))
      {
        client->BeginSend("error 1 When trying to edit cell " + cellName + ", a circular dependency occured: the edit was not made.", p_this->clientSendCallback, state);

        // Continue receiving messages from the client and return;
        client->BeginReceive(SpreadsheetServer::clientRecvCallback, state);
        return;
      }
    }
    
    
    else if (cmd == "undo")
    {
      // Stop any other threads from accessing the map at the same time.
      int connected = 0;
      pthread_mutex_lock(&p_this->associatedSpreadsheetsMutex); {
        connected = p_this->associatedSpreadsheets.count(client);
      } pthread_mutex_unlock(&p_this->associatedSpreadsheetsMutex);

      // If the client isn't connected to a spreadsheet, send an error message.
      if (!connected)
      {
        client->BeginSend("error 3 You must be connected to a spreadsheet in order to use an undo command.", SpreadsheetServer::clientSendCallback, state);

        // Continue receiving messages from the client and return;
        client->BeginReceive(SpreadsheetServer::clientRecvCallback, state);
        return;
      }

      SpreadsheetSession *session = NULL;

      // Stop any other threads from accessing the map at the same time.
      pthread_mutex_lock(&p_this->associatedSpreadsheetsMutex); {
        session = p_this->associatedSpreadsheets[client];
      } pthread_mutex_unlock(&p_this->associatedSpreadsheetsMutex);

      // Undo the last edit made to the spreadsheet.
      bool successful = session->UndoAll();
	  if (!successful)
	  {
		  client->BeginSend("error 3 Your undo command was unable to be processed.", SpreadsheetServer::clientSendCallback, state);
	  }

      // Continue receiving messages from the client and return;
      client->BeginReceive(SpreadsheetServer::clientRecvCallback, state);
      return;
    }


    // Since the keyword is not acceptable, send an error message back to the client.
    else
    {
      client->BeginSend("error 2 " + cmd, p_this->clientSendCallback, state);

      // Continue receiving messages from the client and return.
      client->BeginReceive(SpreadsheetServer::clientRecvCallback, state);
      return;
    }
  }


  else if (ex == SS_EXCEPTION)
  {
    client->BeginSend("error 0 An error occured while sending or receiving data.", p_this->clientSendCallback, state);

    // Continue receiving messages from the client and return.
    client->BeginReceive(SpreadsheetServer::clientRecvCallback, state);
    return;
  }


  else if (ex == SS_CLOSED_EXCEPTION)
  {
    std::cout << "Connection closed: " << client->ToString() << std::endl;

    int connected = 0;
    pthread_mutex_lock(&p_this->associatedSpreadsheetsMutex); {
      connected = p_this->associatedSpreadsheets.count(client);

      //if the client was connected to a session
      if (connected)
      {
        //remove the client from the session
        SpreadsheetSession *session = NULL;
        session = p_this->associatedSpreadsheets[client];
        session->RemoveClient(client);

        // Remove the client from the map of sockets to sessions.
        std::map<StringSocket*, SpreadsheetSession*>::iterator associated_it = p_this->associatedSpreadsheets.find(client);
        p_this->associatedSpreadsheets.erase(associated_it);

        //if the session has no more connected clients
        if (session->GetUserCount() == 0)
        {
          // Save the spreadsheet.
          session->Save();
          
          // Remove the spreadsheet from the map of open spreadsheets.
          pthread_mutex_lock(&p_this->openSpreadsheetsMutex); {
            std::map<std::string, SpreadsheetSession *>::iterator open_it = p_this->openSpreadsheets.find(session->GetName());
            p_this->openSpreadsheets.erase(open_it);
          } pthread_mutex_unlock(&p_this->openSpreadsheetsMutex);
          
          // Delete the spreadsheet session.
          delete session;
        }
      }
    } pthread_mutex_unlock(&p_this->associatedSpreadsheetsMutex);
    
    // Remove the client and callback state from the callback states map.
    pthread_mutex_lock(&p_this->callbackStatesMutex); {
      std::map<StringSocket *, callbackState *>::iterator it = p_this->callbackStates.find(client);
      delete (*it).second;
      p_this->callbackStates.erase(it);
    } pthread_mutex_unlock(&p_this->callbackStatesMutex);

    // Delete the client
    freeStringSocket(&client);

    // Return before calling BeginReceive again.
    return;
  }

  // Continue receiving messages from the client.
  client->BeginReceive(SpreadsheetServer::clientRecvCallback, state);
}


void SpreadsheetServer::listenerAcceptCallback(int ex, StringSocket *socket,
    void *payload) {
  
  std::cout << "Connection established: " << socket->ToString() << std::endl;
  
  
  // Get a SpreadsheetServer out of the payload.
  SpreadsheetServer *pthis = static_cast<SpreadsheetServer *>(payload);
  
  
  // Return from this method if a SpreadsheetServer could not be taken from the
  //   payload.
  if(pthis == NULL)
    return;
  
  
  // Take care of the new socket if there were no exceptions.
  if(ex == TL_NO_EXCEPTION) {
      
    // Create a new callbackState for the StringSocket.
    callbackState *state =
        static_cast<callbackState*>(malloc(sizeof(callbackState)));
    state->clientPayload = socket;
    state->p_this = pthis;
    
    
    // Add the callbackState to the map of callbackStates.
    pthread_mutex_lock(&pthis->callbackStatesMutex); {
      pthis->callbackStates[socket] = state;
    } pthread_mutex_unlock(&pthis->callbackStatesMutex);

    
    // Start receiving messages on the StringSocket.
    socket->BeginReceive(SpreadsheetServer::clientRecvCallback, state);
    
  }
  
  
  // Start waiting for the next connection.
  pthis->listener->BeginAcceptSocket(SpreadsheetServer::listenerAcceptCallback,
      pthis);
  
}


void SpreadsheetServer::loadUsernames() {
  
  // Add sysadmin to the set of registered names.
  this->registeredUsernames.insert("sysadmin");
  
  
  // Open the users text file for reading.
  std::ifstream users("users");
  
  
  // Add each name to the set of usernames from the text file.
  std::string name;
  while(users.peek() != EOF) {
    name = "";
    std::getline(users, name);
    if(name != "")
      this->registeredUsernames.insert(name);
  }
  if(name != "")
    this->registeredUsernames.insert(name);
  
  
  // Close the users text file.
  users.close();
  
}


void SpreadsheetServer::saveUsernames() {
  
  // Open the users text file for writing.
  std::ofstream users("users");
  
  
  // Write all the user names to the text file.
  for(std::set<std::string>::iterator it = this->registeredUsernames.begin();
      it != this->registeredUsernames.end(); it++)
    users << *it << "\n";
  
  
  // Close the users text file.
  users.close();
  
}