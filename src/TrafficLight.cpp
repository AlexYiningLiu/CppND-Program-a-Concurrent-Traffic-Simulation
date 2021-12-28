#include <iostream>
#include <random>
#include "TrafficLight.h"

/* Implementation of class "MessageQueue" */

template <typename T>
T MessageQueue<T>::receive()
{
    // perform queue modification under the lock
    std::unique_lock<std::mutex> uLock(_mutex);
    // pass unique lock to condition variable
    _cond.wait(uLock, [this] { return !_queue.empty(); });

    // remove last vector element from queue
    T msg = std::move(_queue.back());
    _queue.pop_back();

    // will not be copied due to return value optimization (RVO) in C++
    return msg; 
}

template <typename T>
void MessageQueue<T>::send(T &&msg)
{
    // perform vector modification under the lock
    std::lock_guard<std::mutex> uLock(_mutex);
    // flush out the message queue
    _queue.clear();
    // add vector to queue
    _queue.emplace_back(std::move(msg));
    // notify client after pushing new Vehicle into vector
    _cond.notify_one();
}

/* Implementation of class "TrafficLight" */

TrafficLight::TrafficLight()
{
    _currentPhase = red;
}

void TrafficLight::waitForGreen()
{
    // function is called by a different thread, inside the Intersection::addVehicleToQueue call
    // hence we need to protect _queue using mutex, because send is called by the thread that runs cycleThroughPhases
    while (true)
    {
        if (_messageQueue.receive() == green)
        {
            return;
        }
    }
}

TrafficLight::TrafficLightPhase TrafficLight::getCurrentPhase()
{
    return _currentPhase;
}

void TrafficLight::simulate()
{
    threads.emplace_back(std::thread(&TrafficLight::cycleThroughPhases, this));
}

// virtual function which is executed in a thread
void TrafficLight::cycleThroughPhases()
{
    double cycleDuration; // duration of a cycle, will be a random value between 4 and 6 seconds
    std::chrono::time_point<std::chrono::system_clock> lastUpdate;
    lastUpdate = std::chrono::system_clock::now();
    std::random_device rd;
    std::mt19937 eng(rd());
    std::uniform_real_distribution<double> distr(4.0, 6.0);
    
    double timeSinceLastUpdate;

    // generate the cycle duration randomly
    cycleDuration = distr(eng) * 1000;
    while (true)
    {
        // sleep at every iteration to reduce CPU usage
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        // compute time difference to stop watch
        timeSinceLastUpdate = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - lastUpdate).count();
        if (timeSinceLastUpdate >= cycleDuration)
        {
            // reset stop watch for next cycle
            lastUpdate = std::chrono::system_clock::now();

            // regenerate the cycle duration
            cycleDuration = distr(eng) * 1000;

            // toggle the current phase between red and green
            _currentPhase = (_currentPhase == red) ? green : red;

            // send update method to the message queue using move semantics
            _messageQueue.send(std::move(_currentPhase));
        }
    } // eof simulation loop
}

