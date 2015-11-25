/*******************************************************************************
  File: StringSocket.h
  Author: CJ Dimaano
  Team: SegFault
  CS 3505 - Spring 2015
  Date created: April 3, 2015
  Last updated: April 18, 2015
  
  
  Resources:
  - CS 3500 - Fall 2014 - PS7: StringSocket
  
  
  Changelog:
  
  April 18, 2015
  - Added a manual reset event object for when the socket is closed.
  
  April 11, 2015
  - Added mutex objects for thread locking data.
  - Added thread objects for keeping track of the send and receive thread.
  - Added ex member to sendCallbackState struct.
  - Added buf, bufLen, and ex members to recvCallbackState struct.
  - Added invokeSendCallback helper method.
  - Added invokeRecvCallback helper method.
  - Removed sendNext helper method.
  - Removed recvNext helper method.
  - Updated documentation.
  
  April 7, 2015
  - Removed message terminator from the class.
  
  April 6, 2015
  - Added freeStringSocket global function declaration.
  
  April 4, 2015
  - Made default constructor protected.
  - Added friend class access to the constructor for TcpListener and TcpClient.
  - Updated default constructor.
  - Added ConnectToRemoteHost to StringSocket.
  - Added ToString method to StringSocket.
  - Added appendData helper method to StringSocket.
  
  April 3, 2015
  - Created StringSocket.h file.
  - Added class declarations for StringSocket.
  - Added documentation.
*******************************************************************************/


#ifndef __STRINGSOCKET_H__
#define __STRINGSOCKET_H__


//
// ManualResetEvent.
//
#include "ManualResetEvent.h"

//
// Standard libraries.
//
#include <string>
#include <queue>

//
// Open Group multithreading library.
//
#include <pthread.h>


//
// StringSocket exception identifiers.
//
#define SS_NO_EXCEPTION        0
#define SS_EXCEPTION          -1
#define SS_CLOSED_EXCEPTION   -2


/// <summary>
///   The send callback delegate.  This callback function is called once a
///   send is completed.
/// </summary>
/// <param name="ex">
///   An error code indicating whether or not an exception occured while
///   sending the message.
/// </param>
/// <param name="payload">
///   An object associated with the message being sent.
/// </param>
/// <remarks>
///   Possible exception codes are SS_NO_EXCEPTION for no exceptions,
///   SS_EXCEPTION for any generic exception, and SS_CLOSED_EXCEPTION for the
///   case when the socket is closed.
/// </remarks>
typedef void (*sendCallback)(int ex, void *payload);


/// <summary>
///   The receive callback delegate.  This callback function is called once a
///   complete message is received.
/// </summary>
/// <param name="msg">
///   The complete message that was received with the message terminator
///   truncated.
/// </param>
/// <param name="ex">
///   An error code indicating whether or not an exception occured while
///   receiving the message.
/// </param>
/// <param name="payload">
///   An object associated with the message being sent.
/// </param>
/// <remarks>
///   Possible exception codes are SS_NO_EXCEPTION for no exceptions,
///   SS_EXCEPTION for any generic exception, and SS_CLOSED_EXCEPTION for the
///   case when the socket is closed.
/// </remarks>
typedef void (*recvCallback)(std::string msg, int ex, void *payload);


/// <summary>
///   A wrapper class around a socket that takes care of network
///   communications asynchronously.
/// </summary>
/// <remarks>
///   The implementation of this class is based off of the StringSocket class
///   from CS 3500, Fall 2014.
/// </remarks>
class StringSocket {
  
private:
  
  /// <summary>
  ///   Keeps track of the callback state for a single send message request.
  /// </summary>
  typedef struct sendCallbackState {
    sendCallback callback;    // The send callback function.
    void *payload;            // The payload object associated with a send.
    const char *buf;          // The buffer to the message to be sent.
    int bufLen;               // The length of the buffer.
    int ex;                   // The exception code encountered during a send.
  } sendCallbackState;
  
  
  /// <summary>
  ///   Keeps track of the callback state for a single receive message
  ///   request.
  /// </summary>
  typedef struct recvCallbackState {
    recvCallback callback;    // The receive callback function.
    void *payload;            // The payload object associated with a receive.
    char *buf;                // The complete received message.
    int bufLen;               // The length of the received message.
    int ex;                   // The exception code encountered during a
                              //   receive.
  } recvCallbackState;
  
  
  /// <summary>
  ///   The socket descriptor for sending and receiving data.
  /// </summary>
  const int sockfd;
  
  
  /// <summary>
  ///   The socket address in the form "IP:port".
  /// </summary>
  const std::string sockaddr;
  
  
  /// <summary>
  ///   Keeps track of each StringSocket::BeginReceive call in a queue.
  /// </summary>
  std::queue<recvCallbackState *> recvQueue;


  /// <summary>
  ///   Keeps track of each StringSocket::BeginSend call in a queue.
  /// </summary>
  std::queue<sendCallbackState *> sendQueue;


  /// <summary>
  ///   Keeps track of the receive buffer.
  /// </summary>
  char *recvBuf;
  
  
  /// <summary>
  ///   Keeps track of the total buffer size.
  /// </summary>
  int recvBufLen;
  
  
  /// <summary>
  ///   Keeps track of the total length of valid data in the buffer.
  /// </summary>
  int recvBufDLen;

  
  /// <summary>
  ///   Keeps track of the previous buffer index that was searched for the
  ///   message terminator.
  /// </summary>
  int searchIndex;
  
  
  /// <summary>
  ///   Mutex handle for locking the send queue across multiple threads.
  /// </summary>
  pthread_mutex_t sendQueueMutex;
  
  
  /// <summary>
  ///   Mutex handle for locking the receive queue across multiple threads.
  /// </summary>
  pthread_mutex_t recvQueueMutex;
  
  
  /// <summary>
  ///   Keeps track of the send thread.
  /// </summary>
  pthread_t sendThread;
  
  
  /// <summary>
  ///   Keeps track of the receive thread.
  /// </summary>
  pthread_t recvThread;
  
  
  /// <summary>
  ///   Manual reset event for the send thread.
  /// </summary>
  ManualResetEvent mreSend;
  
  
  /// <summary>
  ///   Manual reset event for the receive thread.
  /// </summary>
  ManualResetEvent mreRecv;
  
  
  /// <summary>
  ///   Manual reset event for when the socket is closed.
  /// </summary>
  ManualResetEvent mreClose;
  
  
  /// <summary>
  ///   Flag to determine whether or not it is safe to join the send thread.
  /// </summary>
  bool sendSafeToJoin;
  
  
  /// <summary>
  ///   Flag to determine whether or not it is safe to join the receive thread.
  /// </summary>
  bool recvSafeToJoin;
  
  
protected:

  friend class TcpListener;
  
  
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
  StringSocket(int sockfd, std::string addr);
  
  
public:

  /// <summary>
  ///   Copy constructor.
  /// </summary>
  StringSocket(const StringSocket &other);

  
  /// <summary>
  ///   Destructor.
  /// </summary>
  ~StringSocket(void);
  
  
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
  static int ConnectToRemoteHost(std::string host, std::string service,
      StringSocket **client);
  
  
  /// <summary>
  ///   Gets the string representation of the StringSocket.
  /// </summary>
  virtual std::string ToString(void) const;

  
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
  void BeginSend(std::string msg, sendCallback callback, void *payload);

  
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
  void BeginReceive(recvCallback callback, void *payload);

  
  /// <summary>
  ///   Closes the socket.
  /// </summary>
  void Close(void);
  
  
private:


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
  static void * sendData(void *stringSocket);
  
  
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
  static void * recvData(void *stringSocket);
  
  
  /// <summary>
  ///   Sends received messages in the buffer to the queued receive callbacks.
  /// </summary>
  void recvMessages(void);
  
  
  /// <summary>
  ///   Invokes the send callback on a separate thread.
  /// </summary>
  /// <param name="state">Pointer to the send callback state.</param>
  /// <remarks>
  ///   This method also frees the state from memory.
  /// </remarks>
  static void * invokeSendCallback(void *state);
  
  
  /// <summary>
  ///   Invokes the receive callback on a separate thread.
  /// </summary>
  /// <param name="state">Pointer to the receive callback state.</param>
  /// <remarks>
  ///   This method also frees the state from memory.
  /// </remarks>
  static void * invokeRecvCallback(void *state);
  
  
  /// <summary>
  ///   Appends data to the receive buffer.
  /// </summary>
  /// <param name="buf">The data to append.</param>
  /// <param name="len">The length of the data buffer.</param>
  void appendData(const char * const data, const int len);
  
};


/// <summary>
///   Safely frees a StringSocket object from memory.
/// </summary>
/// <param name="ss">The StringSocket object to free.</param>
extern void freeStringSocket(StringSocket **ss);


#endif
