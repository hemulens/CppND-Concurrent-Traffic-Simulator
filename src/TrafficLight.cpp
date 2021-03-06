#include "TrafficLight.h"

#include <iostream>
#include <random>

/* Implementation of class "MessageQueue" */
template <typename T>
T MessageQueue<T>::receive() {
  // FP.5a : The method receive should use std::unique_lock<std::mutex> and _condition.wait() to wait for and receive new messages and pull them from the queue using move semantics. The received object should then be returned by the receive function.
  std::unique_lock<std::mutex> uLock(_mtx);
  _cv.wait(uLock, [this] {
    return !_queue.empty();
  });
  T msg = std::move(_queue.front());
  _queue.pop_front();
  return msg;
}

template <typename T>
void MessageQueue<T>::send(T &&msg) {
  // FP.4a : The method send should use the mechanisms std::lock_guard<std::mutex> as well as _condition.notify_one() to add a new message to the queue and afterwards send a notification.
  std::lock_guard<std::mutex> lock(_mtx);
  _queue.emplace_back(std::move(msg));
  _cv.notify_one();
}

/* Implementation of class "TrafficLight" */

TrafficLight::TrafficLight() {
  _currentPhase = TrafficLightPhase::red;
  _msgQueue = std::make_shared<MessageQueue<TrafficLightPhase>>();
}

void TrafficLight::waitForGreen() {
  // FP.5b : add the implementation of the method waitForGreen, in which an infinite while-loop
  // runs and repeatedly calls the receive function on the message queue.
  // Once it receives TrafficLightPhase::green, the method returns.
  while (true) {
    // Checking current traffic light phase
    TrafficLightPhase msg = _msgQueue->receive();  // While loop pauses and waits for a message –> np need to insert a "sleep_for"
    if (msg == TrafficLightPhase::green) {
      return;
    }
  }
}

TrafficLightPhase TrafficLight::getCurrentPhase() {
  return _currentPhase;
}

// Auxilliary function added by Sergei
void TrafficLight::togglePhase() {
  // std::lock_guard<std::mutex> toggleLock(_mutex);  // no monitoring object concept implemented, as locks are added in the cycleThroughPhases()
  if (getCurrentPhase() == TrafficLightPhase::red) {
    _currentPhase = TrafficLightPhase::green;
  } else if (getCurrentPhase() == TrafficLightPhase::green) {
    _currentPhase = TrafficLightPhase::red;
  }
  std::cout << "Traffic light toggled to " << getCurrentPhase() << " in thread " << std::this_thread::get_id() << std::endl;
}

void TrafficLight::simulate() {
  // FP.2b : Finally, the private method „cycleThroughPhases“ should be started in a thread when the public method „simulate“ is called. To do this, use the thread queue in the base class.
  threads.emplace_back(std::thread(&TrafficLight::cycleThroughPhases, this));
}

// virtual function which is executed in a thread
void TrafficLight::cycleThroughPhases() {
  // FP.2a : Implement the function with an infinite loop that measures the time between two loop cycles and toggles the current phase of the traffic light between red and green and sends an update method to the message queue using move semantics. The cycle duration should be a random value between 4 and 6 seconds. Also, the while-loop should use std::this_thread::sleep_for to wait 1ms between two cycles.

  // init stopwatch
  std::chrono::time_point<std::chrono::system_clock> lastUpdate;
  lastUpdate = std::chrono::system_clock::now();
  // init cycle duration variables
  long cycleDuration;
  long t_min{4000};
  long t_max{6000};
  // compute time difference to stop watch
  while (true) {
    // init cycleDuration randomizer
    srand(time(NULL));
    cycleDuration = rand() % (t_max - t_min) + t_min;
    // CPU takes a short nap
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    long timeSinceLastUpdate = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - lastUpdate).count();
    if (timeSinceLastUpdate >= cycleDuration) {
      // lock for a better cout output
      // std::unique_lock<std::mutex> cycleLock(_mutex);
      // toggle between red and green
      togglePhase();
      std::cout << "TrafficLight switched in " << timeSinceLastUpdate << " milliseconds in thread " << std::this_thread::get_id() << std::endl;
      // send update message to queue
      _msgQueue->send(std::move(getCurrentPhase()));
      // reset stopwatch
      lastUpdate = std::chrono::system_clock::now();
      // cycleLock.unlock();
    }
  }
}
