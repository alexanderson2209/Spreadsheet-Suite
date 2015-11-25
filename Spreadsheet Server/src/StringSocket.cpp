/*******************************************************************************
  File: StringSocket.cpp
  Author: CJ Dimaano
  Team: SegFault
  CS 3505 - Spring 2015
  Date created: April 3, 2015
  Last updated: April 24, 2015
  
  
  Compile with:
  g++ -pthread -lrt -c StringSocket.cpp
  
  
  Resources:
  - CS 3500 - Fall 2014 - PS7: StringSocket
  - http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html
    Section 6.2: A Simple Stream Server
    April 4, 2015
  - http://pubs.opengroup.org/onlinepubs/007908799/xsh/pthread.h.html
    Accessed on April 11, 2015
  
  
  Changelog:
  
  April 24, 2015
  - Moved some locks around.
  - Fixed some broken messages.
  
  April 22, 2015
  - Fixed a bug with sending messages.
  
  April 18, 2015
  - Fixed a bug where closing does not finish in a timely manner.
  
  April 15, 2015
  - Fixed the issue where the received message had the \0 character at the end.
  
  April 13, 2015
  - Fixed a bug with removing messages from the buffer.
  
  April 12, 2015
  - Updated recvData helper function to handle socket closed exceptions.
  - Updated documentation.
  
  April 11, 2015
  - Added invokeSendCallback method implementation.
  - Added invokeRecvCallback method implementation.
  - Removed sendNext helper method.
  - Removed recvNext helper method.
  - Updated BeginSend, BeginReceive, recvData, and sendData methods to be
      multithreaded.
  - Updated constructors to initialize multithreading members.
  - Updated documentation.
  
  April 8, 2015
  - Fixed a bug with the recvMessages method.
  
  April 7, 2015
  - Removed message terminator from StringSocket.
  
  April 6, 2015
  - Added freeStringSocket global function implementation.
  
  April 4, 2015
  - Updated default constructor.
  - Added ToString implementation.
  - Added ConnectToRemoteHost implementation.
  - Added static helper functions.

  
  April 3, 2015
  - Created StringSocket.cpp file.
  - Added class methods.
  - Added constructor and destructor implementations.
  - Added partial implementations for public methods.
*******************************************************************************/


//
// Class header file.
//
#include "StringSocket.h"

//
// Standard libraries.
//
#include <cerrno>
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
// Threading libraries.
//
#include <signal.h>


//
// Maximum number of bytes that can be received.
//
#define MAX_RECV_BYTES 1024


/*******************************************************************************
  Static functions.
*******************************************************************************/


/// <summary>
///   Gets the remote socket address.
/// </summary>
static void * getAddr(struct sockaddr *sa) {
  
  // Return the IPv4 address if the socket address is in the IPv4 format.
  if(sa->sa_family == AF_INET)
    return &(((struct sockaddr_in *)sa)->sin_addr);
  
  
  // Return the IPv6 address.
  return &(((struct sockaddr_in6 *)sa)->sin6_addr);
  
}


/// <summary>
///   Gets the remote socket local port.
/// </summary>
static short getPort(struct sockaddr *sa) {
  
  // Return the IPv4 address if the socket address is in the IPv4 format.
  if(sa->sa_family == AF_INET)
    return ntohs(((struct sockaddr_in *)sa)->sin_port);
  
  
  // Return the IPv6 address.
  return ntohs(((struct sockaddr_in6 *)sa)->sin6_port);
  
}


/*******************************************************************************
  Global functions.
*******************************************************************************/


/// <summary>
///   Safely frees a StringSocket object from memory.
/// </summary>
/// <param name="ss">The StringSocket object to free.</param>
void freeStringSocket(StringSocket **ss) {
  
  if(*ss != NULL) {
    delete (*ss);
    *ss = NULL;
  }
  
}


/*******************************************************************************
  Public methods.
*******************************************************************************/


/// <summary>
///   Copy constructor.
/// </summary>
StringSocket::StringSocket(const StringSocket &other)
    : sockfd(other.sockfd), sockaddr(other.sockaddr),
      recvQueue(other.recvQueue), sendQueue(other.sendQueue),
      recvBuf(other.recvBuf), recvBufLen(other.recvBufLen),
      recvBufDLen(other.recvBufDLen), searchIndex(other.searchIndex), 
      sendQueueMutex(other.sendQueueMutex), 
      recvQueueMutex(other.recvQueueMutex), sendThread(other.sendThread),
      recvThread(other.recvThread),
      mreSend(other.mreSend), mreRecv(other.mreRecv), mreClose(other.mreClose),
      sendSafeToJoin(other.sendSafeToJoin),
      recvSafeToJoin(other.recvSafeToJoin) {
  //
  // Do nothing.
  //
}


/// <summary>
///   Destructor.
/// </summary>
StringSocket::~StringSocket(void) {
  
  // Close the socket.
  this->Close();
  
  
  // Delete the receive buffer.
  delete [] this->recvBuf;
  
  
  // Destroy the mutex handles.
  pthread_mutex_destroy(&this->sendQueueMutex);
  pthread_mutex_destroy(&this->recvQueueMutex);
  
}


/// <summary>
///   Attempts to connect to a server.
/// </summary>
/// <param name="host">The remote host with which to connect.</param>
/// <param name="service">
///   The service port with which to make a connection.  A service name or a
///   port number may be used.  The default is http.
/// </param>
/// <param name="client">
///   An output parameter for the StringSocket that is created upon a
///   successful connection.
/// </param>
/// <returns>
///   0 if no exceptions occur while attempting to connect; otherwise,
///   SS_EXCEPTION is returned.
/// </returns>
int StringSocket::ConnectToRemoteHost(std::string host, std::string service,
    StringSocket **client) {
  
  // Set defaults.
  if(host == "")
    host = "localhost";
  if(service == "")
    service = "http";
  
  
  // Set up socket hints.
  struct addrinfo hints = { 0 };
  hints.ai_family    =    AF_UNSPEC;  // Either IPv4 or IPv6.
  hints.ai_socktype  =  SOCK_STREAM;  // TCP stream sockets.
  
  
  // Get a linked list of addresses for the socket connection.
  struct addrinfo *servinfo = NULL;
  if(getaddrinfo(host.c_str(), service.c_str(), &hints, &servinfo) != 0)
    return SS_EXCEPTION;
  
  
  // Try to connect to any of the addresses from the linked list.
  int sockfd = -1;
  int yes = 1;
  struct addrinfo *p = NULL;
  for(p = servinfo; p != NULL; p = p->ai_next) {
    
    // Try to get a socket from the current address.
    if((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
      continue;
    
    // Try to connect the socket.
    if(connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
      close(sockfd);
      continue;
    }
    
    break;
    
  }
  
  // Return with an error code if the socket failed to connect.
  if(p == NULL) {
    // Free the linked list of addresses before returning.
    freeaddrinfo(servinfo);
    return SS_EXCEPTION;
  }
  
  
  // Get the address of the socket.
  char addr[INET6_ADDRSTRLEN];
  inet_ntop(
      p->ai_family,
      getAddr((struct sockaddr *)p->ai_addr),
      addr, sizeof(addr)
    );

  
  // Append the port to the address.
  const int len = sizeof(short) * 8 + 1;
  char portstr[len];
  sprintf(portstr, "%hu", getPort((struct sockaddr *)p->ai_addr));
  std::string addrStr(addr);
  addrStr += ":";
  addrStr += portstr;
    
    
  // Create the StringSocket.
  (*client) = new StringSocket(sockfd, addrStr);


  // Free the linked list of addresses.
  freeaddrinfo(servinfo);
  
  
  // Return no exception code.
  return SS_NO_EXCEPTION;
  
}


/// <summary>
///   Gets the string representation of the StringSocket.
/// </summary>
std::string StringSocket::ToString(void) const {
  return this->sockaddr;
}


/// <summary>
///   Begins the asynchronous call for sending a message.
/// </summary>
/// <param name="msg">The message to be sent.</param>
/// <param name="callback">The send callback function.</param>
/// <param name="payload">
///   A pointer to an object that uniquely identifies this send request.
/// </param>
/// <remarks>
/// <para>
///   This method will append the message with the appropriate message
///   terminator if it is missing.
/// </para>
/// <para>
///   Once the send is complete, the send callback function is called.
/// </para>
/// </remarks>
void StringSocket::BeginSend(std::string msg, sendCallback callback,
    void *payload) {
      
  // Send the socket closed exception of the socket is closed.
  if(this->mreClose.IsSet()) {
    
    // Create a new state for the callback thread.
    sendCallbackState *state =
        static_cast<sendCallbackState *>(malloc(sizeof(sendCallbackState)));
    state->callback = callback;
    state->payload = payload;
    state->buf = NULL;
    state->bufLen = 0;
    state->ex = SS_CLOSED_EXCEPTION;
    
    // Call the callback on a separate thread.
    pthread_t dthread;
    pthread_create(
        &dthread, 
        NULL, 
        StringSocket::invokeSendCallback,
        (void *)state
      );
    pthread_detach(dthread);
    
  }
  
  // Add the message to the queue and send it off if the socket is not closed.
  else {
    
    // Make sure only one thread is accessing the send queue at a time.
    pthread_mutex_lock(&this->sendQueueMutex); {
        
      // Append the message terminator to the end of the message if it does not
      //   already end in one.
      int ti = msg.rfind("\n");
      if(ti == std::string::npos || ti != msg.size() - 1)
        msg += "\n";
      
      
      // Set up the callback state.
      sendCallbackState *state =
          static_cast<sendCallbackState *>(malloc(sizeof(sendCallbackState)));
      state->callback = callback;
      state->payload = payload;
      state->bufLen = msg.length();
      state->ex = SS_NO_EXCEPTION;

      
      // Copy the message to the send callback state buffer.
      const char *src = msg.c_str();
      char * const buf = new char[state->bufLen];
      char *dst = buf;
      for(int i = 0; i < msg.length(); i++)
        (*(dst++)) = (*(src++));
      state->buf = buf;

      
      // Push the callback state onto the queue.
      this->sendQueue.push(state);
    
    } pthread_mutex_unlock(&this->sendQueueMutex);
    
    

    // Start sending the message on a separate thread if one is not already
    //   running.
    if(this->mreSend.Wait(0)) {
      
      // Reset the event trigger.
      this->mreSend.Reset();
      
      
      // Join the previous send thread.
      if(this->sendSafeToJoin)
        pthread_join(this->sendThread, NULL);
    
    
      // Send the data on a separate thread.
      int rtcreate = pthread_create(
          &this->sendThread,
          NULL,
          StringSocket::sendData,
          (void*)this
        );
        
        
      // Set the send safe to join flag to true if a thread was successfully
      //   created.
      if(rtcreate == 0)
        this->sendSafeToJoin = true;
        
    }
    
  }
  
}


/// <summary>
///   Begins the asynchronous call for receiving a message.
/// </summary>
/// <param name="callback">The receive callback function.</param>
/// <param name="payload">
///   A pointer to an object that uniquely identifies this receive request.
/// </param>
/// <remarks>
///   Once a receive has completed, the receive callback function is called.
/// </remarks>
void StringSocket::BeginReceive(recvCallback callback, void *payload) {
      
  // Send the socket closed exception of the socket is closed.
  if(this->mreClose.IsSet()) {
    
    // Create a new state for the callback thread.
  recvCallbackState *state = 
      static_cast<recvCallbackState *>(malloc(sizeof(recvCallbackState)));
  state->callback = callback;
  state->payload = payload;
  state->buf = NULL;
  state->bufLen = 0;
  state->ex = SS_CLOSED_EXCEPTION;
    
    // Call the callback on a separate thread.
    pthread_t dthread;
    pthread_create(
        &dthread, 
        NULL, 
        StringSocket::invokeRecvCallback,
        (void *)state
      );
    pthread_detach(dthread);
    
  }
  
  // Add the receive request to the queue and wait for data if the socket is not
  //   closed.
  else {

    // Set up the callback state.
    recvCallbackState *state = 
        static_cast<recvCallbackState *>(malloc(sizeof(recvCallbackState)));
    state->callback = callback;
    state->payload = payload;
    
    
    // Make sure only one thread is accessing the receive queue at a time.
    pthread_mutex_lock(&this->recvQueueMutex); {
    
      // Push the state onto the queue.
      this->recvQueue.push(state);
    
    } pthread_mutex_unlock(&this->recvQueueMutex);
    
    
    // Send received messages to the queued callbacks that are in the buffer.
    this->recvMessages();
    
    
    // Start receiving the message on a separate thread if one is not already
    //   running.
    if(this->mreRecv.Wait(0)) {
      
      // Reset the event trigger.
      this->mreRecv.Reset();
      
      
      // Join the previous receive thread.
      if(this->recvSafeToJoin)
        pthread_join(this->recvThread, NULL);
    
    
      // Receive the data on a separate thread.
      int rtcreate = pthread_create(
          &this->recvThread,
          NULL,
          StringSocket::recvData,
          (void*)this
        );
        
        
      // Set the receive safe to join flag to true if a thread was successfully
      //   created.
      if(rtcreate == 0)
        this->recvSafeToJoin = true;
        
    }
  
  }
  
}


/// <summary>
///   Closes the socket.
/// </summary>
void StringSocket::Close(void) {
  
  // Make sure the socket is not already closed.
  if(!this->mreClose.IsSet()) {
    
    // Set this socket to closed.
    this->mreClose.Set();

    // Wait for the send thread to finish executing.
    if(this->sendSafeToJoin)
      pthread_join(this->sendThread, NULL);
    
    // Close the socket.
    close(this->sockfd);

    // Wait for the receive thread to finish executing.
    if(this->recvSafeToJoin)
      pthread_join(this->recvThread, NULL);
    
  }
  
}


/*******************************************************************************
  Protected methods.
*******************************************************************************/


/// <summary>
///   Default constructor.
/// </summary>
/// <param name="sockfd">
///   The socket descriptor associated with this connection.
/// </param>
/// <param name="addr">
///   The socket address.
/// </param>
/// <remarks>
///   Only TcpListeners and the factory method can construct this object.
/// </remarks>
StringSocket::StringSocket(int sockfd, std::string addr)
    : sockfd(sockfd), sockaddr(addr), recvBufLen(1024), recvBufDLen(0),
      searchIndex(0), sendSafeToJoin(false), recvSafeToJoin(false) {
  
  this->mreSend.Set();
  this->mreRecv.Set();
  
  pthread_mutex_init(&this->sendQueueMutex, NULL);
  pthread_mutex_init(&this->recvQueueMutex, NULL);
  
  this->recvBuf = new char[this->recvBufLen];
  this->recvBuf[0] = '\0';
  
}


/*******************************************************************************
  Private methods.
*******************************************************************************/


/// <summary>
///   Sends complete messages over the socket and calls the send callback for
///   every message that is completely sent.
/// </summary>
/// <param name="stringSocket">
///   Pointer to the StringSocket object for which this method is intended.
/// </param>
/// <remarks>
///   This method is executed on a separate thread.
/// </remarks>
void * StringSocket::sendData(void *stringSocket) {
    
  // Cast the argument as a pointer to the StringSocket.
  StringSocket *pthis = static_cast<StringSocket *>(stringSocket);
  
  
  // Execute the send loop if the argument was successfully cast.
  if(pthis != NULL) {
    
    // Keep sending messages until the send queue is empty.
    while(!pthis->sendQueue.empty()) {
      
      int res = 0;
      const char *buf = NULL;
      int len;
      sendCallbackState *state = NULL;

      
      // Make sure only one thread is accessing the send queue at a time.
      pthread_mutex_lock(&pthis->sendQueueMutex);

      
      // Get the current element in the queue and remove it.
      state = pthis->sendQueue.front();
      pthis->sendQueue.pop();
      
      
      // Exit the thread if no sendCallbackState was in the queue.
      if(state == NULL) {
        pthis->mreSend.Set();
        pthread_mutex_unlock(&pthis->sendQueueMutex);
        pthread_exit(NULL);
      }
      
      
      // Set the buffer pointer and length.
      buf = state->buf;
      len = state->bufLen;
      
      
      // Try to send the message.
      do {
        res = send(pthis->sockfd, buf, len, 0);
        if(res > 0) {
          buf += res;
          len -= res;
        }
      } while(res > 0 && len > 0);
      
      
      // Set the send state exception if one was encountered while attempting to
      //   send the message.
      if(res < 0)
        state->ex = SS_EXCEPTION;
      
      
      // Invoke the send callback on a separate thread.
      pthread_t dthread;
      pthread_create(
          &dthread, 
          NULL, 
          StringSocket::invokeSendCallback,
          (void *)state
        );
      pthread_detach(dthread);


      // Done with the send queue.
      pthread_mutex_unlock(&pthis->sendQueueMutex);
      
    }
    
    // Signal to other threads that this thread is finished.
    pthis->mreSend.Set();
    
  }
  
  
  // Exit this thread.
  pthread_exit(NULL);
  
}


/// <summary>
///   Receives data from the socket and calls the receive callback once a
///   complete message has been received.
/// </summary>
/// <param name="stringSocket">
///   Pointer to the StringSocket object for which this method is intended.
/// </param>
/// <remarks>
///   This method is executed on a separate thread.
/// </remarks>
void * StringSocket::recvData(void *stringSocket) {
    
  // Cast the argument as a pointer to the StringSocket.
  StringSocket *pthis = static_cast<StringSocket *>(stringSocket);
  
  
  // Execute the send loop if the argument was successfully cast.
  if(pthis != NULL) {
    
    // Set a timeout value.
    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    
    // Declare a master fd set and a read fd set.
    fd_set master;
    fd_set read_fds;
    
    // Put the socket into the master set.
    FD_SET(pthis->sockfd, &master);
      
    // Keep receiving messages while the reqeive queue is not empty and the
    //   close event is not set.
    while(!pthis->recvQueue.empty() && !pthis->mreClose.Wait(0)) {
      
      // Copy the master fd set into the read set.
      read_fds = master;

      // Assume that select works.
      select(pthis->sockfd + 1, &read_fds, NULL, NULL, &timeout);
      
      // Receive the data from the socket if there is any.
      if(FD_ISSET(pthis->sockfd, &read_fds)) {
            
        // Receive data on the socket.
        char buf[MAX_RECV_BYTES];
        int res = recv(pthis->sockfd, buf, MAX_RECV_BYTES - 1, 0);
        
        // Append data to the receive buffer and handle any complete messages
        //   in the buffer if bytes were received.
        if(res > 0) {

          // Append the received data to the receive buffer.
          pthis->appendData(buf, res);
          
          
          // Call receive callbacks for every message terminator in the
          //   receive buffer.
          pthis->recvMessages();
          
        }
        
        // Call the receive callback with an appropriate exception ID if no
        //   bytes were received.
        else {
          
          recvCallbackState *state = NULL;
          
          
          // Make sure only one thread is accessing the receive queue at a
          //   time.
          pthread_mutex_lock(&pthis->recvQueueMutex);
          
          
          // Get the current element in the queue and remove it.
          state = pthis->recvQueue.front();
          pthis->recvQueue.pop();
          
          
          // Exit this thread if no receive callback states were in the queue.
          if(state == NULL) {
            pthis->mreRecv.Set();
            pthread_mutex_unlock(&pthis->recvQueueMutex);
            pthread_exit(NULL);
          }
          
          
          // Set the exception code for the receive callback.
          if((errno == ENOTCONN || errno == ECONNRESET) || (res == 0))
            state->ex = SS_CLOSED_EXCEPTION;
          else
            state->ex = SS_EXCEPTION;
          
          
          // Set the message to an empty string.
          state->bufLen = 0;
          state->buf = NULL;
        
        
          // Invoke the receive callback on a separate thread.
          pthread_t dthread;
          pthread_create(
              &dthread, 
              NULL, 
              StringSocket::invokeRecvCallback,
              (void *)state
            );
          pthread_detach(dthread);
          
          
          // Done with the receive queue.
          pthread_mutex_unlock(&pthis->recvQueueMutex);
          
        }

      }
        
    }
    
    
    // Empty the receive queue with closed exceptions if the socket was closed.
    while(!pthis->recvQueue.empty() && pthis->mreClose.IsSet()) {
      
      recvCallbackState *state = NULL;

      
      // Make sure only one thread is accessing the receive queue at a time.
      pthread_mutex_lock(&pthis->recvQueueMutex);

      
      // Get the current element in the queue and remove it.
      state = pthis->recvQueue.front();
      pthis->recvQueue.pop();
      
      
      // Exit this thread if no receive callback states were in the queue.
      if(state == NULL) {
        pthis->mreRecv.Set();
        pthread_mutex_unlock(&pthis->recvQueueMutex);
        pthread_exit(NULL);
      }
      
      
      // Set the state properties for a closed exception.
      state->bufLen = 0;
      state->buf = NULL;
      state->ex = SS_CLOSED_EXCEPTION;
    
    
      // Invoke the receive callback on a separate thread.
      pthread_t dthread;
      pthread_create(
          &dthread, 
          NULL, 
          StringSocket::invokeRecvCallback,
          (void *)state
        );
      pthread_detach(dthread);

      
      // Done with the receive queue.
      pthread_mutex_unlock(&pthis->recvQueueMutex);
      
    }
    
    
    // Signal to other threads that this thread is finished.
    pthis->mreRecv.Set();
      
  }
  
  
  // Exit this thread.
  pthread_exit(NULL);
  
}


/// <summary>
///   Sends received messages in the buffer to the queued receive callbacks.
/// </summary>
void StringSocket::recvMessages(void) {
  
  // Send received messages to the receive callbacks.
  while(!this->recvQueue.empty() && this->searchIndex < this->recvBufDLen) {
    
    
    // Resume searching for the next message terminator.
    bool found = false;
    const char *p = &this->recvBuf[this->searchIndex];
    while(this->searchIndex < this->recvBufDLen && !found) {
      if((*p) == '\n')
        found = true;
      this->searchIndex++;
      p++;
    }
    
    
    // Call the receive callback with the message and remove the message from
    //   the buffer if the message terminator was found.
    if(found) {
      
      recvCallbackState *state = NULL;
      
      // Make sure only one thread is accessing the receive queue at a time.
      pthread_mutex_lock(&this->recvQueueMutex);
      
      
      // Get the current element in the queue and remove it.
      state = this->recvQueue.front();
      this->recvQueue.pop();
      
      
      // Call the receive callback if a receive callback state was in the
      //   queue.
      if(state != NULL) {

        // Set the exception for the receive callback.
        state->ex = SS_NO_EXCEPTION;
        
        
        // Copy the complete message to the message buffer in the state object.
        state->bufLen = this->searchIndex - 1;
        state->buf = new char[state->bufLen];
        const char *src = this->recvBuf;
        char *dst = state->buf;
        for(int i = 0; i < state->bufLen; i++)
          (*(dst++)) = (*(src++));
        *dst = '\0';
        
        
        // Remove the message from the buffer.
        src = &this->recvBuf[this->searchIndex];
        dst = this->recvBuf;
        while((*src) != '\0')
          (*(dst++)) = (*(src++));
        *dst = '\0';
        this->recvBufDLen -= this->searchIndex;
        this->searchIndex = 0;

        
        // Invoke the receive callback on a separate thread.
        pthread_t dthread;
        pthread_create(
            &dthread, 
            NULL, 
            StringSocket::invokeRecvCallback,
            (void *)state
          );
        pthread_detach(dthread);
        
      }

      
      // Done with the receive queue.
      pthread_mutex_unlock(&this->recvQueueMutex);
      
    }
    
    // Set the search index to the end of the buffer if the message terminator
    //   could not be found.
    else
      this->searchIndex = this->recvBufDLen;
    
  }
  
}


/// <summary>
///   Invokes the send callback on a separate thread.
/// </summary>
/// <param name="state">The send callback state.</param>
void * StringSocket::invokeSendCallback(void *arg) {

  // Get the callback state from the argument.
  sendCallbackState *state = static_cast<sendCallbackState *>(arg);
  
  
  // Call the send callback if the argument was successfully cast.
  if(state != NULL) {
    
    // Get the callback, exception, and payload out of the callback state.
    int ex = state->ex;
    void *payload = state->payload;
    sendCallback callback = state->callback;
    
    
    // Call the callback.
    callback(ex, payload);
      
      
    // Delete the message buffer.
    delete [] state->buf;
      
    // Free the callback state.
    free(state);
    
  }
  
  
  // Exit this thread.
  pthread_exit(NULL);
  
}


/// <summary>
///   Invokes the receive callback on a separate thread.
/// </summary>
/// <param name="arg">Pointer to the receive callback state.</param>
/// <remarks>
///   This method also frees the state from memory.
/// </remarks>
void * StringSocket::invokeRecvCallback(void *arg) {

  // Get the callback state from the argument.
  recvCallbackState *state = static_cast<recvCallbackState *>(arg);
  
  
  // Call the receive callback if the argument was successfully cast.
  if(state != NULL) {
    
    // Get the callback, exception, and payload out of the callback state.
    std::string msg(state->buf, state->bufLen);
    int ex = state->ex;
    void *payload = state->payload;
    recvCallback callback = state->callback;
    
    
    // Call the callback.
    callback(msg, ex, payload);
    
    
    // Delete the message buffer.
    //   NOTE: For whatever reason, the CADE lab 1 machines regard state->buf as
    //         an invalid pointer.  Commenting out the following line does not
    //         produce any seg faults during runtime.  --April 13, 2015
    //delete [] state->buf;
      
    // Free the callback state.
    free(state);
    
  }
  
  
  // Exit this thread.
  pthread_exit(NULL);
  
}


/// <summary>
///   Appends data to the receive buffer.
/// </summary>
/// <param name="buf">The data to append.</param>
/// <param name="len">The length of the data buffer.</param>
void StringSocket::appendData(const char * const data, const int len) {
      
    // Get the first address of the receive buffer.
    char *dst = &this->recvBuf[this->recvBufDLen];

    
    // Grow the receive buffer if needed.
    if(this->recvBufDLen + len >= this->recvBufLen - 1) {
      
      // Get the old buffer and create a new one.
      const char * const temp = this->recvBuf;
      this->recvBufLen *= 2;
      this->recvBuf = new char[this->recvBufLen];
      
      
      // Copy the old buffer into the new one.
      const char *src = temp;
      dst = this->recvBuf;
      while((*src) != '\0')
        (*(dst++)) = (*(src++));
      
      
      // Delete the old buffer.
      delete [] temp;
      
    }
    

    // Copy the data to the receive buffer.
    const char *src = data;
    int cpy = len;
    for(int i = 0; i < len; i++) {
      if(*src != '\r' && *src != '\0')
        (*(dst++)) = *src;
      else
        cpy--;
      src++;
    }
    (*dst) = '\0';
    this->recvBufDLen += cpy;
    
}
