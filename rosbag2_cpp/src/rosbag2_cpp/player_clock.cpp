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

#include <memory>
#include <thread>

#include "rosbag2_cpp/player_clock.hpp"
#include "rosbag2_cpp/types.hpp"

namespace rosbag2_cpp
{

class PlayerClockImpl
{
public:
  explicit PlayerClockImpl(bool use_sim_time)
  : use_sim_time(use_sim_time)
  {}

  bool use_sim_time;
  rclcpp::Time starting_time;
  bool paused = false;
  float rate = 1.f;
  float clock_publish_frequency = 0.f;
};

PlayerClock::PlayerClock(bool use_sim_time)
: impl_(std::make_unique<PlayerClockImpl>(use_sim_time))
{}

PlayerClock::~PlayerClock()
{}

TimePoint PlayerClock::now() const
{
  return std::chrono::steady_clock::now();
}

bool PlayerClock::sleep_until(TimePoint until, rclcpp::Duration /* real_time_timeout */)
{
  std::this_thread::sleep_until(until);
  return true;
}

void PlayerClock::set_paused(bool paused)
{
  (void)paused;
  throw NotImplementedError();
}

bool PlayerClock::get_paused() const
{
  return impl_->paused;
}

void PlayerClock::set_rate(float rate)
{
  (void)rate;
  throw NotImplementedError();
}

float PlayerClock::get_rate() const
{
  return impl_->rate;
}

void PlayerClock::set_clock_publish_frequency(float frequency)
{
  (void)frequency;
  throw NotImplementedError();
}

float PlayerClock::get_clock_publish_frequency() const
{
  return impl_->clock_publish_frequency;
}

void PlayerClock::jump(rclcpp::Time time)
{
  (void)time;
  throw NotImplementedError();
}

}  // namespace rosbag2_cpp
