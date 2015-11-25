/*******************************************************************************
  File: TcpListener.h
  Author: CJ Dimaano
  Team: SegFault
  CS 3505 - Spring 2015
  Date created: April 4, 2015
  Last updated: April 17, 2015
  
  
  Changelog:
  
  April 17, 2015
  - Added mutlithreaded functionality.
  
  April 14, 2015
  - Added ToString method.
  
  April 11, 2015
  - Updated documentation.
  
  April 7, 2015
  - Removed the message terminator requirement for StringSocket.
  
  April 6, 2015
  - Added freeTcpListener global function declaration.
  
  April 4, 2015
  - Created TcpListener.h file.
  - Added class declarations for TcpListener.
  - Added documentation.
*******************************************************************************/


#ifndef __TCPLISTENER_H__
#define __TCPLISTENER_H__


//
// Class headers.
//
#include "ManualResetEvent.h"
#include "StringSocket.h"

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
// TcpListener exception identifiers.
//
#define TL_NO_EXCEPTION        0
#define TL_EXCEPTION          -1
#define TL_EX_GETADDRINFO     -2
#define TL_EX_SETSOCKOPT      -3
#define TL_EX_BINDFAIL        -4


/// <summary>
///   The accept socket callback delegate.  This callback function is called
///   once a socket has been connected.
/// </summary>
/// <param name="ex">
///   An error code indicating whether or not an exception occured while
///   attempting to accept a socket connection.
/// </param>
/// <param name="remoteHost">
///   The StringSocket created as a result from the accepted connection.
/// </param>
/// <param name="payload">
///   An object that uniquely identifies this callback.
/// </param>
/// <remarks>
///   The only possible exception is TL_EXCEPTION.
/// </remarks>
typedef void (*acceptSocketCallback)(int ex, StringSocket* remoteHost,
    void *payload);


/// <summary>
///   Takes care of setting up a TCP connection and listening through a TCP
///   connection.
/// </summary>
/// <remarks>
/// <para>
///   The structure of this class is loosely based off of the .NET TcpListener
///   class.
/// </para>
/// <para>
///   This class is not asynchronous.
/// </para>
/// </remarks>
class TcpListener {
  
private:

  /// <summary>
  ///   Keeps track of the callback for a single accept socket request.
  /// </summary>
  typedef struct acceptSocketCallbackState {
    void *payload;                    // The payload associated with the
                                      //   callback.
    acceptSocketCallback callback;    // The callback to be called once a socket
                                      //   connection has been established.
    StringSocket *socket;             // The StringSocket that was created from
                                      //   the established connection.
    int ex;                           // The exception code that occured while
                                      //   attepmting to accept a connection.
  } acceptSocketCallbackState;

  
  /// <summary>
  ///   The socket descriptor for the listener.
  /// </summary>
  const int sockfd;
  
  
  /// <summary>
  ///   The string representation of the bound socket.
  /// </summary>
  std::string sockstr;
  
  
  /// <summary>
  ///   A queue of BeginAcceptSocket requests.
  /// </summary>
  std::queue<acceptSocketCallbackState *> acceptQueue;
  
  
  /// <summary>
  ///   Mutex handle for locking the accept queue across multiple threads.
  /// </summary>
  pthread_mutex_t acceptQueueMutex;
  
  
  /// <summary>
  ///   Manual reset event for the accept thread.
  /// </summary>
  ManualResetEvent mreAccept;
  
  
  /// <summary>
  ///   Manual reset event to signal that the TcpListener is closing.
  /// </summary>
  ManualResetEvent mreClose;
  
  
  /// <summary>
  ///   Flag to determine whether or not it is safe to join the accept thread.
  /// </summary>
  bool acceptSafeToJoin;
  
  
  /// <summary>
  ///   Keeps track of the accept thread.
  /// </summary>
  pthread_t acceptThread;
  
  
  /// <summary>
  ///   Default constructor.
  /// </summary>
  /// <remarks>
  ///   This class can only be created through the factory method.
  /// </remarks>
  TcpListener(int sockfd);
  
  
public:

  /// <summary>
  ///   Copy constructor.
  /// </summary>
  TcpListener(const TcpListener &other);
  
  
  /// <summary>
  ///   Destructor.
  /// </summary>
  ~TcpListener(void);
  
  
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
  static int CreateTcpListener(std::string host, std::string service,
      TcpListener **listener);
      
      
  /// <summary>
  ///   Gets the string representation of this TcpListener.
  /// </summary>
  std::string ToString(void) const;

  
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
  int Start(void);
  
  
  /// <summary>
  ///   Stops the service for accepting socket connections and closes the
  ///   listener.
  /// </summary>
  void Stop(void);


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
  int AcceptSocket(StringSocket **remoteHost);
  
  
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
  void BeginAcceptSocket(acceptSocketCallback callback, void *payload);
  
  
private:

  /// <summary>
  ///   Begins the asynchronous call for accepting sockets on a TcpListener.
  /// </summary>
  /// <param name="tcpListener">
  ///   The TcpListener object for which this method is intended.
  /// </param>
  static void * acceptSocket(void *tcpListener);
  
  
  /// <summary>
  ///   Begins the asynchronous call for the BeginAcceptSocket callback.
  /// </summary>
  /// <param name="state">
  ///   The callback state for which this method is intended.
  /// </param>
  static void * invokeAcceptSocketCallback(void *state);
  
};


/// <summary>
///   Safely frees a TcpListener object.
/// </summary>
/// <param name="tl">The TcpListener object to free.</param>
extern void freeTcpListener(TcpListener **tl);


#endif
