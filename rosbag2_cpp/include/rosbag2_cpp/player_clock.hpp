// Copyright 2021 Amazon.com, Inc. or its affiliates. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef ROSBAG2_CPP__PLAYER_CLOCK_HPP_
#define ROSBAG2_CPP__PLAYER_CLOCK_HPP_

#include <memory>

#include "rclcpp/clock.hpp"
#include "rclcpp/time.hpp"

namespace rosbag2_cpp
{
using TimePoint = std::chrono::time_point<std::chrono::steady_clock>;
class PlayerClockImpl;

/**
 * Used to control the timing of bag playback.
 * This clock should be used to query times and sleep between message playing,
 * so that the complexity involved around time control and time sources
 * is encapsulated in this one place.
 */
class PlayerClock : public rclcpp::Clock
{
public:
  /**
   * Constructor.
   *
   * \param use_sim_time
   *   If false, starts paused.
   *     Use member functions to configure settings before calling set_paused(false)
   *   If true, subscribe to /clock to provide time
   *     All time control (rate, pause, jump) is disabled, and will not publish to /clock
   * \param starting_time: provides a "first time"
   *    This should probably be the timestamp of the first message in the bag, but is a required argument because it cannot be guessed
   */
  explicit PlayerClock(bool use_sim_time);

  virtual ~PlayerClock();

  // rclcpp::Clock interface
  /**
   * Provides the current time according to the clock's internal model.
   *  if use_sim_time: provides current ROS Time (with optional extrapolation - see "Clock Rate and Time Extrapolation" section)
   *  if !use_sim_time: calculates current "Player Time" based on starting time, playback rate, pause state.
   *    this means that /clock time will match with the recorded messages time, as if we are fully reliving the recorded session
   */
  TimePoint now() const;

  /**
   * Sleeps (non-busy wait) the current thread until the provided time is reached - according to this Clock
   *
   * If time is paused, the requested time may never be reached
   * `real_time_timeout` uses the internal steady clock, if the timeout elapses, return false
   * If jump() is called, return false, allowing the caller to handle the new time
   * Return true if the time has been reached
   */
  bool sleep_until(TimePoint until, rclcpp::Duration real_time_timeout);

  // Time Control interface

  /**
   * Pauses/resumes time. Defaults to true (paused)
   * While paused, `now()` will repeatedly return the same time, until resumed
   *
   * Note: this could have been implemented as `set_rate(0)`, but this interface allows this clock to maintain the
   * clock's rate internally, so that the caller does not have to save it in order to resume.
   */
  void set_paused(bool paused);
  bool get_paused() const;

  /**
    * Changes the rate of playback. Defaults to 1.0
    * \param rate: must be nonzero positive. To pause playback, use \sa set_paused instead
    * \throws std::invalid_argument if rate is <= 0.0
    */
  void set_rate(float rate);
  float get_rate() const;

  /**
   * Set the rate that the clock will be published. Defaults to 0.
   * \param frequency If this is set to <= 0, then the clock will not be published
   */
  void set_clock_publish_frequency(float frequency);
  float get_clock_publish_frequency() const;

  /**
    * Change the current internally maintained offset so that next published time is different.
    *
    * This will trigger any registered JumpHandler callbacks.
    * Call this with the first message timestamp for a bag before starting playback (otherwise this will return current wall time)
    */
  void jump(rclcpp::Time time);

private:
  std::unique_ptr<PlayerClockImpl> impl_;
};

}  // namespace rosbag2_cpp

#endif  // ROSBAG2_CPP__PLAYER_CLOCK_HPP_
