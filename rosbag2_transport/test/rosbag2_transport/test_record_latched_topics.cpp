// Copyright 2026, Tier IV Inc.
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

#include <gmock/gmock.h>

#include <chrono>
#include <memory>
#include <regex>
#include <string>
#include <utility>

#include "rclcpp/rclcpp.hpp"

#include "rosbag2_test_common/publication_manager.hpp"
#include "rosbag2_test_common/wait_for.hpp"
#include "rosbag2_test_common/client_manager.hpp"

#include "rosbag2_transport/recorder.hpp"

#include "test_msgs/msg/arrays.hpp"
#include "test_msgs/msg/basic_types.hpp"
#include "test_msgs/message_fixtures.hpp"
#include "test_msgs/srv/basic_types.hpp"

#include "mock_recorder.hpp"
#include "record_integration_fixture.hpp"

using namespace std::chrono_literals;  // NOLINT

TEST_F(RecordIntegrationTestFixture, keep_latest_published_transient_local_topics_as_latched_topics)
{
  auto array_message = get_messages_arrays()[0];
  array_message->float32_values = {{40.0f, 2.0f, 0.0f}};
  array_message->bool_values = {{true, false, true}};
  std::string array_topic = "/array_topic";

  auto string_messages = get_messages_strings();
  string_messages[0]->string_value = "Hello World";
  string_messages[1]->string_value = "Transient Local message";
  std::string string_topic = "/string_topic";
  std::string string_topic_transient_local = "/string_topic_transient_local";
  const size_t arbitrary_history = 5;
  auto transient_local_qos = rclcpp::QoS{rclcpp::KeepLast(1)}.transient_local();
  rosbag2_test_common::PublicationManager pub_manager;
  pub_manager.setup_publisher(array_topic, array_message, 2);
  pub_manager.setup_publisher(string_topic, string_messages[0], 2);
  pub_manager.setup_publisher(
    string_topic_transient_local, string_messages[1], 2, transient_local_qos);

  rosbag2_transport::RecordOptions record_options = {false, false, false, true,
    {array_topic, string_topic, string_topic_transient_local}, {}, {}, {}, {}, {}, {},
    "rmw_format", 100ms};
  auto recorder = std::make_shared<rosbag2_transport::Recorder>(
    std::move(writer_), storage_options_, record_options);
  recorder->record();

  start_async_spin(recorder);
  auto cleanup_process_handle = rcpputils::make_scope_exit([&]() {stop_spinning();});

  ASSERT_TRUE(pub_manager.wait_for_matched(array_topic.c_str()));
  ASSERT_TRUE(pub_manager.wait_for_matched(string_topic.c_str()));

  pub_manager.run_publishers();

  auto & writer = recorder->get_writer_handle();
  auto & mock_writer = dynamic_cast<MockSequentialWriter &>(writer.get_implementation_handle());

  constexpr size_t expected_messages = 6;
  auto ret = rosbag2_test_common::wait_until_condition(
    [ =, &mock_writer]() {
      return mock_writer.get_number_of_recorded_messages() >= expected_messages;
    },
    std::chrono::seconds(5));
  EXPECT_TRUE(ret) << "failed to capture expected messages in time";
  auto recorded_messages = mock_writer.get_messages();
  EXPECT_EQ(recorded_messages.size(), expected_messages);

  auto filtered_string_messages0 = filter_messages<test_msgs::msg::Strings>(
    recorded_messages, string_topic);
  auto filtered_string_messages1 = filter_messages<test_msgs::msg::Strings>(
    recorded_messages, string_topic_transient_local);
  auto filtered_array_messages = filter_messages<test_msgs::msg::Arrays>(
    recorded_messages, array_topic);
  ASSERT_THAT(filtered_string_messages0, SizeIs(2));
  ASSERT_THAT(filtered_string_messages1, SizeIs(2));
  ASSERT_THAT(filtered_array_messages, SizeIs(2));
  EXPECT_THAT(filtered_string_messages0[0]->string_value, Eq("Hello World"));
  EXPECT_THAT(filtered_string_messages1[0]->string_value, Eq("Transient Local message"));
  EXPECT_THAT(filtered_array_messages[0]->bool_values, ElementsAre(true, false, true));
  EXPECT_THAT(filtered_array_messages[0]->float32_values, ElementsAre(40.0f, 2.0f, 0.0f));
}

TEST_F(RecordIntegrationTestFixture, latched_regex_topics_recording)
{
  auto test_string_messages = get_messages_strings();
  auto test_array_messages = get_messages_arrays();
  std::string regex = "^/aa$";

  // matching topic
  std::string v1 = "/aa";

  // topics that shouldn't match
  std::string b1 = "/aaa";
  std::string b2 = "/baa";
  std::string b3 = "/baaa";
  std::string b4 = "/aa/aa";

  // checking the test data itself
  std::regex re(regex);
  ASSERT_TRUE(std::regex_match(v1, re));
  ASSERT_FALSE(std::regex_match(b1, re));
  ASSERT_FALSE(std::regex_match(b2, re));
  ASSERT_FALSE(std::regex_match(b3, re));
  ASSERT_FALSE(std::regex_match(b4, re));

  rosbag2_transport::RecordOptions record_options =
  {false, false, false, true, {v1, b1, b2, b3, b4}, {}, {}, {}, {}, {}, {}, "rmw_format", 10ms};
  record_options.latched_regex = regex;

  // TODO(karsten1987) Refactor this into publication manager
  const size_t arbitrary_history = 5;
  auto transient_local_qos = rclcpp::QoS{rclcpp::KeepLast(1)}.transient_local();
  rosbag2_test_common::PublicationManager pub_manager;
  pub_manager.setup_publisher(v1, test_string_messages[0], 3, transient_local_qos);
  pub_manager.setup_publisher(b1, test_string_messages[0], 3, transient_local_qos);
  pub_manager.setup_publisher(b2, test_string_messages[1], 3, transient_local_qos);
  pub_manager.setup_publisher(b3, test_string_messages[0], 3, transient_local_qos);
  pub_manager.setup_publisher(b4, test_string_messages[1], 3);

  auto recorder = std::make_shared<rosbag2_transport::Recorder>(
    std::move(writer_), storage_options_, record_options);
  recorder->record();

  start_async_spin(recorder);
  auto cleanup_process_handle = rcpputils::make_scope_exit([&]() {stop_spinning();});

  ASSERT_TRUE(pub_manager.wait_for_matched(v1.c_str()));

  pub_manager.run_publishers();

  auto & writer = recorder->get_writer_handle();
  MockSequentialWriter & mock_writer =
    static_cast<MockSequentialWriter &>(writer.get_implementation_handle());

  constexpr size_t expected_messages = 3;
  auto ret = rosbag2_test_common::wait_until_condition(
    [ =, &mock_writer]() {
      return mock_writer.get_number_of_recorded_messages() >= expected_messages;
    },
    std::chrono::seconds(5));
  auto recorded_messages = mock_writer.get_messages();
  // We may receive additional messages from rosout, it doesn't matter,
  // as long as we have received at least as many total messages as we expect
  EXPECT_TRUE(ret) << "failed to capture expected messages in time";
  EXPECT_THAT(recorded_messages, SizeIs(Ge(expected_messages)));
  auto recorded_topics = mock_writer.get_topics();
  EXPECT_THAT(recorded_topics, SizeIs(5));
  // EXPECT_TRUE(recorded_topics.find(v1) != recorded_topics.end());
  auto latched_topics = mock_writer.get_latched_topics();
  EXPECT_THAT(latched_topics, SizeIs(1));
  EXPECT_TRUE(std::find(latched_topics.begin(), latched_topics.end(), v1) != latched_topics.end());
}

TEST_F(RecordIntegrationTestFixture, latced_regex_and_exclude_regex_topic_recording)
{
  auto test_string_messages = get_messages_strings();
  auto test_array_messages = get_messages_arrays();
  std::string regex = "/[a-z]+_nice(_.*)";
  std::string topics_regex_to_exclude = "/[a-z]+_nice_[a-z]+/(.*)";

  // matching topics - the only ones that should be recorded
  std::string v1 = "/awesome_nice_topic";
  std::string v2 = "/still_nice_topic";

  // excluded topics
  std::string e1 = "/quite_nice_namespace/but_it_is_excluded";

  // topics that shouldn't match
  std::string b1 = "/numberslike1arenot_nice";
  std::string b2 = "/namespace_before/not_nice";

  // checking the test data itself
  std::regex re(regex);
  std::regex exclude(topics_regex_to_exclude);

  ASSERT_TRUE(std::regex_match(v1, re));
  ASSERT_FALSE(std::regex_match(v1, exclude));

  ASSERT_TRUE(std::regex_match(v2, re));
  ASSERT_FALSE(std::regex_match(v2, exclude));

  ASSERT_FALSE(std::regex_match(b1, re));
  ASSERT_FALSE(std::regex_match(b1, exclude));

  ASSERT_FALSE(std::regex_match(b2, re));
  ASSERT_FALSE(std::regex_match(b2, exclude));

  // this example matches both regexes - should be excluded
  ASSERT_TRUE(std::regex_match(e1, re));
  ASSERT_TRUE(std::regex_match(e1, exclude));

  rosbag2_transport::RecordOptions record_options =
  {false, false, false, true, {v1, v2, b1, b2, e1}, {}, {}, {}, {}, {}, {}, "rmw_format", 10ms};
  // record_options.regex = regex;
  record_options.latched_regex = regex;
  record_options.latched_exclude = topics_regex_to_exclude;

  // TODO(karsten1987) Refactor this into publication manager
  const size_t arbitrary_history = 5;
  auto transient_local_qos = rclcpp::QoS{rclcpp::KeepLast(1)}.transient_local();
  rosbag2_test_common::PublicationManager pub_manager;
  pub_manager.setup_publisher(v1, test_string_messages[0], 3, transient_local_qos);
  pub_manager.setup_publisher(v2, test_string_messages[1], 3, transient_local_qos);
  pub_manager.setup_publisher(b1, test_string_messages[0], 3, transient_local_qos);
  pub_manager.setup_publisher(b2, test_string_messages[1], 3, transient_local_qos);
  pub_manager.setup_publisher(e1, test_string_messages[0], 3, transient_local_qos);

  auto recorder = std::make_shared<rosbag2_transport::Recorder>(
    std::move(writer_), storage_options_, record_options);
  recorder->record();

  start_async_spin(recorder);
  auto cleanup_process_handle = rcpputils::make_scope_exit([&]() {stop_spinning();});

  ASSERT_TRUE(pub_manager.wait_for_matched(v1.c_str()));
  ASSERT_TRUE(pub_manager.wait_for_matched(v2.c_str()));

  pub_manager.run_publishers();

  auto & writer = recorder->get_writer_handle();
  MockSequentialWriter & mock_writer =
    static_cast<MockSequentialWriter &>(writer.get_implementation_handle());

  constexpr size_t expected_messages = 3;
  auto ret = rosbag2_test_common::wait_until_condition(
    [ =, &mock_writer]() {
      return mock_writer.get_number_of_recorded_messages() >= expected_messages;
    },
    std::chrono::seconds(5));
  auto recorded_messages = mock_writer.get_messages();
  // We may receive additional messages from rosout, it doesn't matter,
  // as long as we have received at least as many total messages as we expect
  EXPECT_TRUE(ret) << "failed to capture expected messages in time";
  EXPECT_THAT(recorded_messages, SizeIs(Ge(expected_messages)));

  auto recorded_topics = mock_writer.get_topics();
  EXPECT_THAT(recorded_topics, SizeIs(5));
  EXPECT_TRUE(recorded_topics.find(v1) != recorded_topics.end());
  EXPECT_TRUE(recorded_topics.find(v2) != recorded_topics.end());
  EXPECT_TRUE(recorded_topics.find(b1) != recorded_topics.end());
  EXPECT_TRUE(recorded_topics.find(b2) != recorded_topics.end());
  EXPECT_TRUE(recorded_topics.find(e1) != recorded_topics.end());

  auto latched_topics = mock_writer.get_latched_topics();
  EXPECT_THAT(latched_topics, SizeIs(2));
  EXPECT_TRUE(std::find(latched_topics.begin(), latched_topics.end(), v1) != latched_topics.end());
  EXPECT_TRUE(std::find(latched_topics.begin(), latched_topics.end(), v2) != latched_topics.end());
}

TEST_F(RecordIntegrationTestFixture, latched_regex_and_exclude_topic_topic_recording)
{
  auto test_string_messages = get_messages_strings();
  auto test_array_messages = get_messages_arrays();
  std::string regex = "/[a-z]+_nice(_.*)";
  std::string exclude = "/quite_nice_namespace/but_it_is_excluded";

  // matching topics - the only ones that should be recorded
  std::string v1 = "/awesome_nice_topic";
  std::string v2 = "/still_nice_topic";

  // excluded topics
  std::string e1 = "/quite_nice_namespace/but_it_is_excluded";

  // topics that shouldn't match
  std::string b1 = "/numberslike1arenot_nice";
  std::string b2 = "/namespace_before/not_nice";

  // checking the test data itself
  std::regex re(regex);
  ASSERT_TRUE(std::regex_match(v1, re));
  ASSERT_TRUE(std::regex_match(v2, re));
  ASSERT_FALSE(std::regex_match(b1, re));
  ASSERT_FALSE(std::regex_match(b2, re));
  ASSERT_TRUE(std::regex_match(e1, re));

  std::regex ex(exclude);
  ASSERT_FALSE(std::regex_match(v1, ex));
  ASSERT_FALSE(std::regex_match(v2, ex));
  ASSERT_FALSE(std::regex_match(b1, ex));
  ASSERT_FALSE(std::regex_match(b2, ex));
  ASSERT_TRUE(std::regex_match(e1, ex));  // this example matches both regexes - should be excluded

  rosbag2_transport::RecordOptions record_options =
  {false, false, false, true, {v1, v2, b1, b2, e1}, {}, {}, {}, {}, {}, {}, "rmw_format", 10ms};
  record_options.latched_regex = regex;
  record_options.latched_exclude = exclude;

  // TODO(karsten1987) Refactor this into publication manager
  const size_t arbitrary_history = 5;
  auto transient_local_qos = rclcpp::QoS{rclcpp::KeepLast(1)}.transient_local();
  rosbag2_test_common::PublicationManager pub_manager;
  pub_manager.setup_publisher(v1, test_string_messages[0], 3, transient_local_qos);
  pub_manager.setup_publisher(v2, test_string_messages[1], 3, transient_local_qos);
  pub_manager.setup_publisher(b1, test_string_messages[0], 3, transient_local_qos);
  pub_manager.setup_publisher(b2, test_string_messages[1], 3, transient_local_qos);
  pub_manager.setup_publisher(e1, test_string_messages[0], 3, transient_local_qos);

  auto recorder = std::make_shared<rosbag2_transport::Recorder>(
    std::move(writer_), storage_options_, record_options);
  recorder->record();

  start_async_spin(recorder);
  auto cleanup_process_handle = rcpputils::make_scope_exit([&]() {stop_spinning();});

  ASSERT_TRUE(pub_manager.wait_for_matched(v1.c_str()));
  ASSERT_TRUE(pub_manager.wait_for_matched(v2.c_str()));

  pub_manager.run_publishers();

  auto & writer = recorder->get_writer_handle();
  MockSequentialWriter & mock_writer =
    static_cast<MockSequentialWriter &>(writer.get_implementation_handle());

  constexpr size_t expected_messages = 3;
  auto ret = rosbag2_test_common::wait_until_condition(
    [ =, &mock_writer]() {
      return mock_writer.get_number_of_recorded_messages() >= expected_messages;
    },
    std::chrono::seconds(5));
  auto recorded_messages = mock_writer.get_messages();
  // We may receive additional messages from rosout, it doesn't matter,
  // as long as we have received at least as many total messages as we expect
  EXPECT_TRUE(ret) << "failed to capture expected messages in time";
  EXPECT_THAT(recorded_messages, SizeIs(Ge(expected_messages)));

  auto recorded_topics = mock_writer.get_topics();
  EXPECT_THAT(recorded_topics, SizeIs(5));
  EXPECT_TRUE(recorded_topics.find(v1) != recorded_topics.end());
  EXPECT_TRUE(recorded_topics.find(v2) != recorded_topics.end());
  EXPECT_TRUE(recorded_topics.find(b1) != recorded_topics.end());
  EXPECT_TRUE(recorded_topics.find(b2) != recorded_topics.end());
  EXPECT_TRUE(recorded_topics.find(e1) != recorded_topics.end());

  auto latched_topics = mock_writer.get_latched_topics();
  EXPECT_THAT(latched_topics, SizeIs(2));
  EXPECT_TRUE(std::find(latched_topics.begin(), latched_topics.end(), v1) != latched_topics.end());
  EXPECT_TRUE(std::find(latched_topics.begin(), latched_topics.end(), v2) != latched_topics.end());
}
