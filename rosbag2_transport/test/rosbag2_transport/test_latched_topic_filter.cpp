// Copyright 2026, TierIV Inc.
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

#include <algorithm>
#include <future>
#include <map>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "rosbag2_transport/topic_filter.hpp"

using namespace ::testing;  // NOLINT

class TestLatchedTopicFilter : public Test
{
protected:
  std::unordered_map<std::string, std::string> topics_and_types_ {
    {"topic/a", "type_a"},
    {"topic/b", "type_b"},
    {"topic/c", "type_c"},
    {"topic/d", "type_d"},
    {"topic/e", "type_e"},
    {"topic/f", "type_f"},
  };
  std::unordered_map<std::string, std::string> transient_local_topics_ {
    {"topic/a", "type_a"},
    {"topic/c", "type_c"},
    {"topic/e", "type_e"},
  };
};


TEST_F(TestLatchedTopicFilter, latched_topic_filter_all_transient_local_topics) {
  {
    // latched_all_transient_local_topcs = false, latched_topics is empty
    // latched_regex is empty, latched_exclude is empty
    rosbag2_transport::RecordOptions record_options;
    record_options.all_topics = false;
    record_options.latched_all_transient_local = false;
    rosbag2_transport::TopicFilter filter{record_options, nullptr, true};
    auto filtered_topics = filter.filter_latched_topics(topics_and_types_, transient_local_topics_);
    EXPECT_THAT(filtered_topics, SizeIs(0));
  }
  {
    // latched_all_transient_local_topcs = true, latched_topics is empty
    // latched_regex is empty, latched_exclude is empty
    rosbag2_transport::RecordOptions record_options;
    record_options.all_topics = true;
    record_options.latched_all_transient_local = true;
    rosbag2_transport::TopicFilter filter{record_options, nullptr, true};
    auto filtered_topics = filter.filter_latched_topics(topics_and_types_, transient_local_topics_);
    EXPECT_THAT(filtered_topics, SizeIs(3));
    for (const auto & topic :
      {"topic/a", "topic/c", "topic/e"})
    {
      EXPECT_TRUE(
        std::find(filtered_topics.begin(), filtered_topics.end(), topic) != filtered_topics.end());
    }
    for (const auto & topic :
      {"topic/b", "topic/d", "topic/f"})
    {
      EXPECT_FALSE(
        std::find(filtered_topics.begin(), filtered_topics.end(), topic) != filtered_topics.end());
    }
  }
  {
    // latched_all_transient_local_topcs = true, latched_topics is empty
    // latched_regex is specified, latched_exclude is empty
    rosbag2_transport::RecordOptions record_options;
    record_options.all_topics = true;
    record_options.latched_all_transient_local = true;
    record_options.latched_regex = "^topic/a$";
    rosbag2_transport::TopicFilter filter{record_options, nullptr, true};
    auto filtered_topics = filter.filter_latched_topics(topics_and_types_, transient_local_topics_);
    EXPECT_THAT(filtered_topics, SizeIs(1));
    for (const auto & topic :
      {"topic/a"})
    {
      EXPECT_TRUE(
        std::find(filtered_topics.begin(), filtered_topics.end(), topic) != filtered_topics.end());
    }
    for (const auto & topic :
      {"topic/b", "topic/c", "topic/d", "topic/e", "topic/f"})
    {
      EXPECT_FALSE(
        std::find(filtered_topics.begin(), filtered_topics.end(), topic) != filtered_topics.end());
    }
  }
  {
    // latched_all_transient_local_topcs = true, latched_topics is empty
    // latched_regex is empty, latched_exclude is specified
    rosbag2_transport::RecordOptions record_options;
    record_options.all_topics = true;
    record_options.latched_all_transient_local = true;
    record_options.latched_exclude = "^topic/c$";
    rosbag2_transport::TopicFilter filter{record_options, nullptr, true};
    auto filtered_topics = filter.filter_latched_topics(topics_and_types_, transient_local_topics_);
    EXPECT_THAT(filtered_topics, SizeIs(2));
    for (const auto & topic :
      {"topic/a", "topic/e"})
    {
      EXPECT_TRUE(
        std::find(filtered_topics.begin(), filtered_topics.end(), topic) != filtered_topics.end());
    }
    for (const auto & topic :
      {"topic/c", "topic/b", "topic/d", "topic/f"})
    {
      EXPECT_FALSE(
        std::find(filtered_topics.begin(), filtered_topics.end(), topic) != filtered_topics.end());
    }
  }
  {
    // latched_all_transient_local_topcs = true, latched_topics is empty
    // latched_regex is specified, latched_exclude is specified
    rosbag2_transport::RecordOptions record_options;
    record_options.all_topics = true;
    record_options.latched_all_transient_local = true;
    record_options.latched_regex = "^topic/a$|^topic/c$";
    record_options.latched_exclude = "^topic/a$";
    rosbag2_transport::TopicFilter filter{record_options, nullptr, true};
    auto filtered_topics = filter.filter_latched_topics(
      topics_and_types_, transient_local_topics_);
    EXPECT_THAT(filtered_topics, SizeIs(1));
    for (const auto & topic :
      {"topic/c"})
    {
      EXPECT_TRUE(
        std::find(filtered_topics.begin(), filtered_topics.end(), topic) != filtered_topics.end());
    }
    for (const auto & topic :
      {"topic/a", "topic/b", "topic/d", "topic/e", "topic/f"})
    {
      EXPECT_FALSE(
        std::find(filtered_topics.begin(), filtered_topics.end(), topic) != filtered_topics.end());
    }
  }
}

TEST_F(TestLatchedTopicFilter, latched_topic_filter_latched_topics) {
  {
    // latched_all_transient_local_topcs = false, latched_topics is specified
    // latched_regex is empty, latched_exclude is empty
    rosbag2_transport::RecordOptions record_options;
    record_options.all_topics = true;
    record_options.latched_all_transient_local = false;
    record_options.latched_topics = {"topic/a", "topic/b", "topic/c"};
    rosbag2_transport::TopicFilter filter{record_options, nullptr, true};
    auto filtered_topics = filter.filter_latched_topics(
      topics_and_types_, transient_local_topics_);
    EXPECT_THAT(filtered_topics, SizeIs(3));
    for (const auto & topic :
      {"topic/a", "topic/b", "topic/c"})
    {
      EXPECT_TRUE(
        std::find(filtered_topics.begin(), filtered_topics.end(), topic) != filtered_topics.end());
    }
    for (const auto & topic :
      {"topic/d", "topic/e", "topic/f"})
    {
      EXPECT_FALSE(
        std::find(filtered_topics.begin(), filtered_topics.end(), topic) != filtered_topics.end());
    }
  }
  {
    // latched_all_transient_local_topcs = false, latched_topics is specified
    // latched_regex is specified, latched_exclude is empty
    rosbag2_transport::RecordOptions record_options;
    record_options.all_topics = true;
    record_options.latched_all_transient_local = false;
    record_options.latched_topics = {"topic/a", "topic/b", "topic/c"};
    record_options.latched_regex = "^topic/a$";
    rosbag2_transport::TopicFilter filter{record_options, nullptr, true};
    auto filtered_topics = filter.filter_latched_topics(
      topics_and_types_, transient_local_topics_);
    EXPECT_THAT(filtered_topics, SizeIs(1));
    EXPECT_TRUE(
      std::find(filtered_topics.begin(), filtered_topics.end(), "topic/a") != filtered_topics.end()
    );
  }
  {
    // latched_all_transient_local_topcs = false, latched_topics is specified
    // latched_regex is empty, latched_exclude is specified
    rosbag2_transport::RecordOptions record_options;
    record_options.all_topics = true;
    record_options.latched_all_transient_local = false;
    record_options.latched_topics = {"topic/a", "topic/b", "topic/c"};
    record_options.latched_exclude = "^topic/a$";
    rosbag2_transport::TopicFilter filter{record_options, nullptr, true};
    auto filtered_topics = filter.filter_latched_topics(
      topics_and_types_, transient_local_topics_);
    EXPECT_THAT(filtered_topics, SizeIs(2));
    for (const auto & topic :
      {"topic/b", "topic/c"})
    {
      EXPECT_TRUE(
        std::find(filtered_topics.begin(), filtered_topics.end(), topic) != filtered_topics.end());
    }
    for (const auto & topic :
      {"topic/a", "topic/d", "topic/e", "topic/f"})
    {
      EXPECT_FALSE(
        std::find(filtered_topics.begin(), filtered_topics.end(), topic) != filtered_topics.end());
    }
  }
  {
    // latched_all_transient_local_topcs = false, latched_topics is specified
    // latched_regex is specified, latched_exclude is specified
    rosbag2_transport::RecordOptions record_options;
    record_options.all_topics = true;
    record_options.latched_all_transient_local = false;
    record_options.latched_topics = {"topic/a", "topic/b", "topic/c"};
    record_options.latched_regex = "^topic/a$|^topic/b$";
    record_options.latched_exclude = "^topic/a$";
    rosbag2_transport::TopicFilter filter{record_options, nullptr, true};
    auto filtered_topics = filter.filter_latched_topics(
      topics_and_types_, transient_local_topics_);
    EXPECT_THAT(filtered_topics, SizeIs(1));
    EXPECT_TRUE(
      std::find(filtered_topics.begin(), filtered_topics.end(), "topic/b") != filtered_topics.end()
    );
    for (const auto & topic :
      {"topic/a", "topic/c", "topic/d", "topic/e", "topic/f"})
    {
      EXPECT_FALSE(
        std::find(filtered_topics.begin(), filtered_topics.end(), topic) != filtered_topics.end());
    }
  }
}

TEST_F(TestLatchedTopicFilter, latched_topic_filter_without_lached_configurations) {
  {
    // latched_all_transient_local_topcs = true, latched_topics is specified
    // latched_regex is empty, latched_exclude is empty
    rosbag2_transport::RecordOptions record_options;
    record_options.all_topics = true;
    rosbag2_transport::TopicFilter filter{record_options, nullptr, true};
    auto filtered_topics = filter.filter_latched_topics(topics_and_types_, transient_local_topics_);
    EXPECT_THAT(filtered_topics, SizeIs(0));
  }
}

TEST_F(TestLatchedTopicFilter,
  latched_topic_filter_witout_all_transient_local_and_no_local_topics) {
  {
    // latched_all_transient_local_topcs = false, latched_topics is empty
    // latched_regex is specified, latched_exclude is empty
    rosbag2_transport::RecordOptions record_options;
    record_options.all_topics = true;
    record_options.latched_all_transient_local = false;
    // record_options.latched_topics = {"topic/a", "topic/b", "topic/c"};
    record_options.latched_regex = "^topic/a$|^topic/b$";
    rosbag2_transport::TopicFilter filter{record_options, nullptr, true};
    auto filtered_topics = filter.filter_latched_topics(topics_and_types_, transient_local_topics_);
    EXPECT_THAT(filtered_topics, SizeIs(2));
    for (const auto & topic :
      {"topic/a", "topic/b"})
    {
      EXPECT_TRUE(
        std::find(filtered_topics.begin(), filtered_topics.end(), topic) != filtered_topics.end());
    }
    for (const auto & topic :
      {"topic/c", "topic/d", "topic/e", "topic/f"})
    {
      EXPECT_FALSE(
        std::find(filtered_topics.begin(), filtered_topics.end(), topic) != filtered_topics.end());
    }
  }
  {
    // latched_all_transient_local_topcs = false, latched_topics is empty
    // latched_regex is sempty, latched_exclude is specified
    rosbag2_transport::RecordOptions record_options;
    record_options.all_topics = true;
    record_options.latched_all_transient_local = false;
    record_options.latched_topics = {"topic/a", "topic/b", "topic/c", "topic/d", "topic/e"};
    record_options.latched_exclude = "^topic/a$";
    rosbag2_transport::TopicFilter filter{record_options, nullptr, true};
    auto filtered_topics = filter.filter_latched_topics(
      topics_and_types_, transient_local_topics_);
    EXPECT_THAT(filtered_topics, SizeIs(4));
    for (const auto & topic :
      {"topic/b", "topic/c", "topic/d", "topic/e"})
    {
      EXPECT_TRUE(
        std::find(filtered_topics.begin(), filtered_topics.end(), topic) != filtered_topics.end());
    }
    for (const auto & topic :
      {"topic/a", "topic/f"})
    {
      EXPECT_FALSE(
        std::find(filtered_topics.begin(), filtered_topics.end(), topic) != filtered_topics.end());
    }
  }
  {
    // latched_all_transient_local_topcs = false, latched_topics is empty
    // latched_regex is specified, latched_exclude is specified
    rosbag2_transport::RecordOptions record_options;
    record_options.all_topics = true;
    record_options.latched_all_transient_local = false;
    // record_options.latched_topics = {"topic/a", "topic/b", "topic/c"};
    record_options.latched_regex = "^topic/a$|^topic/b$";
    record_options.latched_exclude = "^topic/a$";
    rosbag2_transport::TopicFilter filter{record_options, nullptr, true};
    auto filtered_topics = filter.filter_latched_topics(topics_and_types_, transient_local_topics_);
    EXPECT_THAT(filtered_topics, SizeIs(1));
    for (const auto & topic :
      {"topic/b"})
    {
      EXPECT_TRUE(
        std::find(filtered_topics.begin(), filtered_topics.end(), topic) != filtered_topics.end());
    }
    for (const auto & topic :
      {"topic/a", "topic/c", "topic/d", "topic/e", "topic/f"})
    {
      EXPECT_FALSE(
        std::find(filtered_topics.begin(), filtered_topics.end(), topic) != filtered_topics.end());
    }
  }
}
