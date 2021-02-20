// Copyright 2018, Bosch Software Innovations GmbH.
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

#include "cdr_converter.hpp"

#include <memory>
#include <string>

#include "ament_index_cpp/get_resources.hpp"
#include "ament_index_cpp/get_package_prefix.hpp"

#include "fastcdr/Cdr.h"
#include "fastcdr/FastBuffer.h"

#include "rosbag2_cpp/types.hpp"

#include "rosidl_runtime_cpp/message_type_support_decl.hpp"

#include "../logging.hpp"

namespace rosbag2_converter_default_plugins
{

CdrConverter::CdrConverter()
{
}

void CdrConverter::deserialize(
  const std::shared_ptr<const rosbag2_storage::SerializedBagMessage> serialized_message,
  const rosidl_message_type_support_t * type_support,
  std::shared_ptr<rosbag2_cpp::rosbag2_introspection_message_t> introspection_message)
{
  if (type_support == nullptr) {
    return;
  }

  rosbag2_cpp::introspection_message_set_topic_name(
    introspection_message.get(), serialized_message->topic_name.c_str());
  introspection_message->time_stamp = serialized_message->time_stamp;

  auto data = serialized_message->serialized_data;
  eprosima::fastcdr::FastBuffer buffer(
    reinterpret_cast<char *>(data->buffer), data->buffer_length);
  eprosima::fastcdr::Cdr deser(buffer, eprosima::fastcdr::Cdr::DEFAULT_ENDIAN,
    eprosima::fastcdr::Cdr::DDS_CDR);

  // auto callbacks = static_cast<const message_type_support_callbacks_t *>(ts->data);
  // MessageTypeSupport_cpp tss{callbacks};
  auto ret = RMW_RET_OK; // tss.deserializeROSmessage(deser, ros_message, callbacks);

  if (ret != RMW_RET_OK) {
    ROSBAG2_CONVERTER_DEFAULT_PLUGINS_LOG_ERROR("Failed to deserialize message.");
  }
}

void CdrConverter::serialize(
  const std::shared_ptr<const rosbag2_cpp::rosbag2_introspection_message_t> introspection_message,
  const rosidl_message_type_support_t * type_support,
  std::shared_ptr<rosbag2_storage::SerializedBagMessage> serialized_message)
{
  serialized_message->topic_name = std::string(introspection_message->topic_name);
  serialized_message->time_stamp = introspection_message->time_stamp;

  if (type_support == nullptr) {
    return;
  }


  auto data = serialized_message->serialized_data;
  eprosima::fastcdr::FastBuffer buffer(
    reinterpret_cast<char *>(data->buffer), data->buffer_length);

  eprosima::fastcdr::Cdr ser(
    buffer, eprosima::fastcdr::Cdr::DEFAULT_ENDIAN, eprosima::fastcdr::Cdr::DDS_CDR);

  // auto callbacks = static_cast<const message_type_support_callbacks_t *>(ts->data);
  // auto tss = std::make_unique<MessageTypeSupport_cpp>(callbacks);
  // auto data_length = tss->getEstimatedSerializedSize(ros_message, callbacks);
  // if (serialized_message->buffer_capacity < data_length) {
  //   if (rmw_serialized_message_resize(serialized_message, data_length) != RMW_RET_OK) {
  //     RMW_SET_ERROR_MSG("unable to dynamically resize serialized message");
  //     return RMW_RET_ERROR;
  //   }
  // }
  //
  // eprosima::fastcdr::FastBuffer buffer(
  //   reinterpret_cast<char *>(serialized_message->buffer), data_length);
  // eprosima::fastcdr::Cdr ser(
  //   buffer, eprosima::fastcdr::Cdr::DEFAULT_ENDIAN, eprosima::fastcdr::Cdr::DDS_CDR);
  //
  // auto ret = tss->serializeROSmessage(ros_message, ser, callbacks);
  // serialized_message->buffer_length = data_length;
  // serialized_message->buffer_capacity = data_length;
  auto ret = RMW_RET_OK;

  if (ret != RMW_RET_OK) {
    ROSBAG2_CONVERTER_DEFAULT_PLUGINS_LOG_ERROR("Failed to serialize message.");
  }
}

}  // namespace rosbag2_converter_default_plugins

#include "pluginlib/class_list_macros.hpp"  // NOLINT
PLUGINLIB_EXPORT_CLASS(
  rosbag2_converter_default_plugins::CdrConverter,
  rosbag2_cpp::converter_interfaces::SerializationFormatConverter)
