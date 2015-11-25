/*******************************************************************************
  File: ManualResetEvent.h
  Author: CJ Dimaano
  Team: SegFault
  CS 3505 - Spring 2015
  Date created: April 11, 2015
  Last updated: April 12, 2015
  
  
  Changelog:
  
  April 12, 2015
  - Updated documentation.
  
  April 11, 2015
  - Created ManualResetEvent.h file.
  - Added ManualResetEvent class definition.
  - Added documentation.
*******************************************************************************/


#ifndef __MANUALRESETEVENT_H__
#define __MANUALRESETEVENT_H__


//
// Open Group multithreading library.
//
#include <pthread.h>


/// <summary>
///   Provides the functionality of a manual reset event to signal across
///   threads.
/// </summary>
class ManualResetEvent {
  
private:

  /// <summary>
  ///   Handle to the mutex object.
  /// </summary>
  pthread_mutex_t mutex;
  
  
  /// <summary>
  ///   Handle to the condition object.
  /// </summary>
  pthread_cond_t cond;
  
  
  /// <summary>
  ///   Flag determining if the event is set.
  /// </summary>
  bool isSet;


public:

  /// <summary>
  ///   Copy constructor.
  /// </summary>
  ManualResetEvent(const ManualResetEvent &other);
  
  
  /// <summary>
  ///   Default constructor.
  /// </summary>
  ManualResetEvent(void);
  
  
  /// <summary>
  ///   Destructor.
  /// </summary>
  ~ManualResetEvent(void);
  
  
  /// <summary>
  ///   Gets whether or not the event is set.
  /// </summary>
  bool IsSet(void) const;
  
  
  /// <summary>
  ///   Sets the event flag.
  /// </summary>
  void Set(void);
  
  
  /// <summary>
  ///   Resets the event flag.
  /// </summary>
  void Reset(void);
  
  
  /// <summary>
  ///   Waits indefinitely for the event to be set.
  /// </summary>
  void Wait(void);
  
  
  /// <summary>
  ///   Waits a certain amount of time for the event to be set.
  /// </summary>
  /// <param name="timeout">
  ///   The amount of time in seconds to wait for the event to be set.
  /// </param>
  /// <returns>
  ///   True if the event is set; otherwise, false.
  /// </returns>
  /// <remarks>
  /// <para>
  ///   This method will only wait until the timeout has passed.  A timeout
  ///   value of 0 can be specified, which will cause this method to return
  ///   immediately.
  /// </para>
  /// </remarks>
  bool Wait(int timeout);
  
};


#endif
