/*******************************************************************************
  File: TcpListener.cpp
  Author: CJ Dimaano
  Team: SegFault
  CS 3505 - Spring 2015
  Date created: April 4, 2015
  Last updated: April 17, 2015
  
  
  Compile with:
  g++ -pthread -lrt -c TcpListener.cpp


  Resources:
  - http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html
    Section 6.1: A Simple Stream Server
    Accessed on April 4, 2015
  
  
  Changelog:
  
  April 17, 2015
  - Added mutlithreaded functionality.
  
  April 14, 2015
  - Added ToString method.
  - Fixed accepting connections from anywhere.
  
  April 12, 2015
  - Updated documentation.
  
  April 11, 2015
  - Updated documentation.
  
  April 7, 2015
  - Removed the message terminator requirement for StringSocket.
  
  April 6, 2015
  - Added freeTcpListener global function implementation.
  
  April 4, 2015
  - Created TcpListener.cpp file.
  - Added implementation of class TcpListener.
*******************************************************************************/


//
// Class header file.
//
#include "TcpListener.h"

//
// Standard libraries.
//
#include <cstdio>
#include <cstdlib>

//
// Socket libraries.
//
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>


//
// Listener connection backlog.
//
#define TL_BACKLOG 20


/*******************************************************************************
  Static functions.
*******************************************************************************/


/// <summary>
///   Gets the remote socket address.
/// </summary>
static void * getRemoteAddr(struct sockaddr *sa) {
  
  // Return the IPv4 address if the socket address is in the IPv4 format.
  if(sa->sa_family == AF_INET)
    return &(((struct sockaddr_in *)sa)->sin_addr);
  
  
  // Return the IPv6 address.
  return &(((struct sockaddr_in6 *)sa)->sin6_addr);
  
}


/// <summary>
///   Gets the remote socket local port.
/// </summary>
static short getRemotePort(struct sockaddr *sa) {
  
  // Return the IPv4 address if the socket address is in the IPv4 format.
  if(sa->sa_family == AF_INET)
    return ntohs(((struct sockaddr_in *)sa)->sin_port);
  
  
  // Return the IPv6 address.
  return ntohs(((struct sockaddr_in6 *)sa)->sin6_port);
  
}


static std::string getSocketString(struct sockaddr *sa) {
  
  // Get the address of the socket.
  char addr[INET6_ADDRSTRLEN];
  inet_ntop(
      sa->sa_family,
      getRemoteAddr(sa),
      addr, sizeof(addr)
    );

  
  // Append the port to the socket address.
  const int len = sizeof(short) * 8 + 1;
  char portstr[len];
  sprintf(portstr, "%hu", getRemotePort(sa));
  std::string sockAddr(addr);
  sockAddr += ":";
  sockAddr += portstr;
  
  
  // Return the socket address.
  return sockAddr;
  
}


/*******************************************************************************
  Global functions.
*******************************************************************************/


/// <summary>
///   Safely frees a TcpListener object.
/// </summary>
/// <param name="tl">The TcpListener object to free.</param>
void freeTcpListener(TcpListener **tl) {
  
  if(*tl != NULL) {
    delete (*tl);
    *tl = NULL;
  }
  
}


/*******************************************************************************
  TcpListener methods.
*******************************************************************************/


/// <summary>
///   Default constructor.
/// </summary>
/// <remarks>
///   This class can only be created through the factory method.
/// </remarks>
TcpListener::TcpListener(int sockfd)
    : sockfd(sockfd), acceptSafeToJoin(false) {
  
  this->mreAccept.Set();
  
  pthread_mutex_init(&this->acceptQueueMutex, NULL);
  
}


/// <summary>
///   Copy constructor.
/// </summary>
TcpListener::TcpListener(const TcpListener &other)
    : sockfd(other.sockfd), sockstr(other.sockstr),
      acceptQueue(other.acceptQueue), acceptQueueMutex(other.acceptQueueMutex),
      mreAccept(other.mreAccept), mreClose(other.mreClose),
      acceptSafeToJoin(other.acceptSafeToJoin),
      acceptThread(other.acceptThread) {
  //
  // Do nothing.
  //
}


/// <summary>
///   Destructor.
/// </summary>
TcpListener::~TcpListener(void) {
  
  // Stop the TcpListener.
  this->Stop();
  
  // Destroy the mutex handle.
  pthread_mutex_destroy(&this->acceptQueueMutex);
  
}


/// <summary>
///   Factory method used to create instances of this class.
/// </summary>
/// <param name="host">
///   Where to listen for connections.  The default is any.
/// </param>
/// <param name="service">
///   The service port where connections are expected to arrive.  A service
///   name or a port number may be used.  The default is http.
/// </param>
/// <param name="listener">
///   An output parameter for the created TcpListener.  The object must be
///   freed by the caller.  Use the global function freeTcpListener to free
///   a TcpListener object.
/// </param>
/// <returns>
///   0 if no exceptions occur while setting up the Tcplistener; otherwise, an
///   exception code indicating what exception was encountered.
/// </returns>
/// <remarks>
///   Possible exception codes:
///   <UL>
///     <LI>TL_NO_EXCEPTION</LI>
///     <LI>TL_EX_GETADDRINFO</LI>
///     <LI>TL_EX_SETSOCKOPT</LI>
///     <LI>TL_EX_BINDFAIL</LI>
///   </UL>
/// </remarks>
int TcpListener::CreateTcpListener(std::string host, std::string service,
    TcpListener **listener) {
      
  // Set defaults.
  if(service == "")
    service = "http";
  
  
  // Set up socket hints.
  struct addrinfo hints = { 0 };
  hints.ai_family    =    AF_UNSPEC;  // Either IPv4 or IPv6.
  hints.ai_socktype  =  SOCK_STREAM;  // TCP stream sockets.
  hints.ai_flags     =   AI_PASSIVE;  // Fill in IP automagically.
  
  
  // Get a linked list of addresses for the TcpListener.
  struct addrinfo *servinfo = NULL;
  if((host == "" && getaddrinfo(NULL, service.c_str(), &hints, &servinfo) != 0)
      || (host != ""
      && getaddrinfo(host.c_str(), service.c_str(), &hints, &servinfo) != 0))
    return TL_EX_GETADDRINFO;
  
  
  // Try to bind to any of the sockets from the linked list.
  int sockfd = -1;
  int yes = 1;
  struct addrinfo *p = NULL;
  std::string sockstr;
  for(p = servinfo; p != NULL; p = p->ai_next) {
    
    // Try to get a socket from the current address.
    if((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
      continue;
    
    // Try to set the socket options.
    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
      return TL_EX_SETSOCKOPT;

    // Accept connections from anywhere.
    ((sockaddr_in *)p->ai_addr)->sin_addr.s_addr = INADDR_ANY;
    
    // Try to bind the socket.
    if(bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
      close(sockfd);
      continue;
    }
    
    // Get the socket as a string.
    sockstr = getSocketString(p->ai_addr);
    
    break;
    
  }
  
  // Return with an error code if the socket failed to bind.
  if(p == NULL) {
    // Free the linked list of addresses before returning.
    freeaddrinfo(servinfo);
    return TL_EX_BINDFAIL;
  }
  
  
  // Free the linked list of addresses.
  freeaddrinfo(servinfo);
  
  
  // Create the TcpListener.
  (*listener) = new TcpListener(sockfd);
  (*listener)->sockstr = sockstr;
  
  
  // Return no exception code.
  return TL_NO_EXCEPTION;

}


/// <summary>
///   Gets the string representation of this TcpListener.
/// </summary>
std::string TcpListener::ToString(void) const {
  return this->sockstr;
}


/// <summary>
///   Starts the service for accepting socket connections.
/// </summary>
/// <returns>
///   0 if no exceptions occured while starting the service; otherwise, an
///   error code indicating what exception was encountered.
/// </returns>
/// <remarks>
///   The only possible exception is TL_EXCEPTION.
/// </remarks>
int TcpListener::Start(void) {
  
  // Try to start listening for connections.
  if(listen(this->sockfd, TL_BACKLOG) == -1)
    return TL_EXCEPTION;
  
  return TL_NO_EXCEPTION;
  
}


/// <summary>
///   Stops the service for accepting socket connections and closes the
///   listener.
/// </summary>
void TcpListener::Stop(void) {

  // Make sure the socket is not already closed.
  if(!this->mreClose.IsSet()) {
      
    // Signal the close event.
    this->mreClose.Set();
    
    // Wait for the accept thread to finish executing.
    if(this->acceptSafeToJoin)
      pthread_join(this->acceptThread, NULL);
    
    // Close the socket.
    close(this->sockfd);
    
  }
  
}


/// <summary>
///   Accepts a connection and creates a StringSocket for the connection.
/// </summary>
/// <param name="remoteHost">
///   An output parameter for the created StringSocket.  The object must be
///   freed by the caller.  Use the global function freeStringSocket to free
///   a StringSocket object.
/// </param>
/// <returns>
///   0 if no exceptions occurred while attempting to accept a sockect
///   connection; otherwise, an exception code indicating what exception was
///   encountered.
/// </returns>
/// <remarks>
/// <para>
///   The only possible exception is TL_EXCEPTION.
/// </para>
/// <para>
///   This method blocks execution.
/// </para>
/// </remarks>
int TcpListener::AcceptSocket(StringSocket **remoteHost) {
  
  // Try to accept the incoming connection.
  struct sockaddr_storage remote_addr;
  socklen_t sin_size = sizeof(remote_addr);
  int remotefd = accept(
      this->sockfd,
      (struct sockaddr *)&remote_addr,
      &sin_size
    );
  if(remotefd == -1)
    return TL_EXCEPTION;
    
    
  // Create the StringSocket.
  (*remoteHost) = new StringSocket(remotefd,
      getSocketString((struct sockaddr *)&remote_addr));
  
  
  // Return no exceptions.
  return TL_NO_EXCEPTION;
  
}


/// <summary>
///   Begins the asynchronous call to accepting a single socket connection.
/// </summary>
/// <param name="callback">
///   The callback function to be called once a connection has been
///   established.
/// </param>
/// <param name="payload">
///   An object that uniquely identifies the call to this method.
/// </param>
void TcpListener::BeginAcceptSocket(acceptSocketCallback callback,
    void *payload) {

  // Set up the callback state.
  acceptSocketCallbackState *state = 
      static_cast<acceptSocketCallbackState *>(
          malloc(sizeof(acceptSocketCallbackState))
        );
  state->callback = callback;
  state->payload = payload;
  state->socket = NULL;
  state->ex = 0;
  
  
  // Make sure only one thread is accessing the accept queue at a time.
  pthread_mutex_lock(&this->acceptQueueMutex); {
  
    // Push the state onto the queue.
    this->acceptQueue.push(state);
  
  } pthread_mutex_unlock(&this->acceptQueueMutex);
  
  
  // Start receiving the message on a separate thread if one is not already
  //   running.
  if(this->mreAccept.Wait(0)) {
    
    // Reset the event trigger.
    this->mreAccept.Reset();
    
    
    // Join the previous receive thread.
    if(this->acceptSafeToJoin)
      pthread_join(this->acceptThread, NULL);
  
  
    // Receive the data on a separate thread.
    int rtcreate = pthread_create(
        &this->acceptThread,
        NULL,
        TcpListener::acceptSocket,
        (void*)this
      );
      
      
    // Set the receive safe to join flag to true if a thread was successfully
    //   created.
    if(rtcreate == 0)
      this->acceptSafeToJoin = true;
      
  }
  
}


/// <summary>
///   Begins the asynchronous call for accepting sockets on a TcpListener.
/// </summary>
/// <param name="tcpListener">
///   The TcpListener object for which this method is intended.
/// </param>
void * TcpListener::acceptSocket(void *tcpListener) {
  
  // Try to get the TcpListener for this call.
  TcpListener *pthis = static_cast<TcpListener *>(tcpListener);
  
  
  // Exit this thread of a TcpListener could not be found.
  if(pthis == NULL)
    pthread_exit(NULL);
  
  
  // Set the timeout value.
  struct timeval timeout;
  timeout.tv_sec = 1;
  timeout.tv_usec = 0;
  
  
  // Declare a master fd set and a read fd set.
  fd_set master;
  fd_set read_fds;
  
  
  // Put the TcpListener socket into the master set.
  FD_SET(pthis->sockfd, &master);
  
  
  // Keep listening for an incoming connection until the TcpListener is ready to
  //   close, an exception is caught, or a connection is made.
  while(!pthis->mreClose.Wait(0)) {
    
    // Copy the master fd set to the read set.
    read_fds = master;
    
    // Call the accept callback with an exception if something went wrong while
    //   attempting to select sockets that are ready to read.
    if(select(pthis->sockfd + 1, &read_fds, NULL, NULL, &timeout) == -1) {
      
      // Get the accept request out of the queue.
      acceptSocketCallbackState *state = NULL;
      pthread_mutex_lock(&pthis->acceptQueueMutex); {
        state = pthis->acceptQueue.front();
        pthis->acceptQueue.pop();
      } pthread_mutex_unlock(&pthis->acceptQueueMutex);
      
      // Exit this thread of there were no items in the queue.
      if(state == NULL) {
        pthis->mreAccept.Set();
        pthread_exit(NULL);
      }
      
      // Set the exception.
      state->ex = TL_EXCEPTION;
      
      // Invoke the accept callback on a separate thread.
      pthread_t dthread;
      pthread_create(
          &dthread, 
          NULL, 
          TcpListener::invokeAcceptSocketCallback,
          (void *)state
        );
      pthread_detach(dthread);
      
      // Break from the select loop.
      pthis->mreAccept.Set();
      break;
      
    }
    
    // Accept the socket connection and call the callback if there is a
    //   connection waiting on the backlog.
    if(FD_ISSET(pthis->sockfd, &read_fds)) {
      
      // Get the accept request from the queue.
      acceptSocketCallbackState *state = NULL;
      pthread_mutex_lock(&pthis->acceptQueueMutex); {
        state = pthis->acceptQueue.front();
        pthis->acceptQueue.pop();
      } pthread_mutex_unlock(&pthis->acceptQueueMutex);
      
      // Exit this thread if no items were in the queue.
      if(state == NULL) {
        pthis->mreAccept.Set();
        pthread_exit(NULL);
      }
      
      // Accept the socket.
      struct sockaddr_storage remote_addr;
      socklen_t sin_size = sizeof(remote_addr);
      int remotefd = accept(
          pthis->sockfd,
          (struct sockaddr *)&remote_addr,
          &sin_size
        );
      
      // Set the state exception if the connection could not be accepted.
      if(remotefd == -1)
        state->ex = TL_EXCEPTION;
      
      // Set the socket if the connection was accepted.
      else
        state->socket = new StringSocket(remotefd,
            getSocketString((struct sockaddr *)&remote_addr));

            
      // Invoke the accept callback on a separate thread.
      pthread_t dthread;
      pthread_create(
          &dthread, 
          NULL, 
          TcpListener::invokeAcceptSocketCallback,
          (void *)state
        );
      pthread_detach(dthread);
      
      // Break from the select loop.
      pthis->mreAccept.Set();
      break;
      
    }
  }

  
  // Exit this thread.
  pthread_exit(NULL);
  
}


/// <summary>
///   Begins the asynchronous call for the BeginAcceptSocket callback.
/// </summary>
/// <param name="arg">
///   The callback state for which this method is intended.
/// </param>
void * TcpListener::invokeAcceptSocketCallback(void *arg) {

  // Get the callback state from the argument.
  acceptSocketCallbackState *state =
      static_cast<acceptSocketCallbackState *>(arg);
  
  
  // Call the send callback if the argument was successfully cast.
  if(state != NULL) {
    
    // Get the callback, exception, and payload out of the callback state.
    int ex = state->ex;
    StringSocket *socket = state->socket;
    void *payload = state->payload;
    acceptSocketCallback callback = state->callback;
    
    
    // Call the callback.
    callback(ex, socket, payload);
      
      
    // Free the callback state.
    free(state);
    
  }
  
  
  // Exit this thread.
  pthread_exit(NULL);
  
}
