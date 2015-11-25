/*******************************************************************************
  File: ManualResetEvent.cpp
  Author: CJ Dimaano
  Team: SegFault
  CS 3505 - Spring 2015
  Date created: April 11, 2015
  Last updated: April 12, 2015
  
  
  Compile with:
  g++ -pthread - lrt -c ManualResetEvent.cpp
  
  
  Resources:
  - http://stackoverflow.com/questions/178114/pthread-like-windows-manual-reset-event
    Post by ceztko, last edited on January 22, 2014
    Accessed on April 11, 2015
  - http://pubs.opengroup.org/onlinepubs/9699919799/
    Accessed on April 12, 2015
  
  
  Changelog:
  
  April 12, 2015
  - Updated documentation.
  - Updated Wait(int) method.
  
  April 11, 2015
  - Created ManualResetEvent.cpp file.
  - Added ManualResetEvent class implementation.
*******************************************************************************/


//
// Class header file.
//
#include "ManualResetEvent.h"

//
// System libraries.
//
#include <errno.h>
#include <time.h>


/// <summary>
///   Copy constructor.
/// </summary>
ManualResetEvent::ManualResetEvent(const ManualResetEvent &other)
    : mutex(other.mutex), cond(other.cond), isSet(other.isSet) {
  //
  // Do nothing.
  //
}


/// <summary>
///   Default constructor.
/// </summary>
ManualResetEvent::ManualResetEvent(void)
    : isSet(false) {
  
  pthread_mutex_init(&this->mutex, NULL);
  pthread_cond_init(&this->cond, NULL);
  
}


/// <summary>
///   Destructor.
/// </summary>
ManualResetEvent::~ManualResetEvent(void) {
  
  pthread_mutex_destroy(&this->mutex);
  pthread_cond_destroy(&this->cond);
  
}


/// <summary>
///   Gets whether or not the event is set.
/// </summary>
bool ManualResetEvent::IsSet(void) const {
  return this->isSet;
}


/// <summary>
///   Sets the event flag.
/// </summary>
void ManualResetEvent::Set(void) {
  
  pthread_mutex_lock(&this->mutex); {
    
    this->isSet = true;
    pthread_cond_signal(&this->cond);
    
  } pthread_mutex_unlock(&this->mutex);
  
}


/// <summary>
///   Resets the event flag.
/// </summary>
void ManualResetEvent::Reset(void) {
  
  pthread_mutex_lock(&this->mutex); {
    
    this->isSet = false;
  
  } pthread_mutex_unlock(&this->mutex);
  
}


/// <summary>
///   Waits indefinitely for the event to be set.
/// </summary>
void ManualResetEvent::Wait(void) {

  pthread_mutex_lock(&this->mutex); {
    
    while(!this->isSet)
      pthread_cond_wait(&this->cond, &this->mutex);
  
  } pthread_mutex_unlock(&this->mutex);

}


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
bool ManualResetEvent::Wait(int timeout) {
  
  pthread_mutex_lock(&this->mutex); {
    
    struct timespec wait;
    
    clock_gettime(CLOCK_REALTIME, &wait);
    
    wait.tv_sec += timeout;
    wait.tv_nsec += ((unsigned long)timeout * 1000000UL);
    
    int ret = 0;
    while(!this->isSet && ret == 0)
      ret = pthread_cond_timedwait(&this->cond, &this->mutex, &wait);
    
  } pthread_mutex_unlock(&this->mutex);
  
  return this->isSet;
  
}
