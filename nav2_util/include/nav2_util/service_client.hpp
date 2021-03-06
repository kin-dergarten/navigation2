// Copyright (c) 2018 Intel Corporation
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

#ifndef NAV2_UTIL__SERVICE_CLIENT_HPP_
#define NAV2_UTIL__SERVICE_CLIENT_HPP_

#include <string>

#include "nav2_util/node_utils.hpp"
#include "rclcpp/rclcpp.hpp"

namespace nav2_util
{

template<class ServiceT>
class ServiceClient
{
public:
  explicit ServiceClient(
    const std::string & service_name,
    const rclcpp::Node::SharedPtr & provided_node = rclcpp::Node::SharedPtr())
  : service_name_(service_name)
  {
    if (provided_node) {
      node_ = provided_node;
    } else {
      node_ = generate_internal_node(service_name + "_Node");
    }
    client_ = node_->create_client<ServiceT>(service_name);
  }

  ServiceClient(const std::string & service_name, const std::string & parent_name)
  : service_name_(service_name)
  {
    node_ = generate_internal_node(parent_name + std::string("_") + service_name + "_client");
    client_ = node_->create_client<ServiceT>(service_name);
  }

  using RequestType = typename ServiceT::Request;
  using ResponseType = typename ServiceT::Response;

  typename ResponseType::SharedPtr invoke(
    typename RequestType::SharedPtr & request,
    const std::chrono::nanoseconds timeout = std::chrono::nanoseconds::max())
  {
    if (!client_->wait_for_service(std::chrono::seconds(1))) {
        throw std::runtime_error(
                service_name_ + " service client not available");
    }

    RCLCPP_DEBUG(
      node_->get_logger(), "%s service client: send async request",
      service_name_.c_str());
    auto future_result = client_->async_send_request(request);

    if (rclcpp::spin_until_future_complete(node_, future_result, timeout) !=
      rclcpp::FutureReturnCode::SUCCESS)
    {
      throw std::runtime_error(service_name_ + " service client: async_send_request failed");
    }

    return future_result.get();
  }

  bool invoke(
    typename RequestType::SharedPtr & request,
    typename ResponseType::SharedPtr & response)
  {
    if (!client_->wait_for_service(std::chrono::seconds(1))) {
      RCLCPP_ERROR(
        node_->get_logger(), "%s service client not available",
        service_name_.c_str());
      return false;
    }

    RCLCPP_DEBUG(
      node_->get_logger(), "%s service client: send async request",
      service_name_.c_str());
    auto future_result = client_->async_send_request(request);

    if (rclcpp::spin_until_future_complete(node_, future_result) !=
      rclcpp::FutureReturnCode::SUCCESS)
    {
      return false;
    }

    response = future_result.get();
    return response.get();
  }

  bool isAvailable(const std::chrono::duration<float> timeout = std::chrono_literals::operator""s(1))
  {
    if (!client_->wait_for_service(timeout)) {
      RCLCPP_INFO(
          node_->get_logger(), "%s service client not available",
          service_name_.c_str());
      return false;
    }
    return true;
  }

  void wait_for_service(const std::chrono::nanoseconds timeout = std::chrono::nanoseconds::max())
  {
    auto sleep_dur = std::chrono::milliseconds(10);
    while (!client_->wait_for_service(timeout)) {
      if (!rclcpp::ok()) {
        throw std::runtime_error(
                service_name_ + " service client: interrupted while waiting for service");
      }
      rclcpp::sleep_for(sleep_dur);
    }
  }

protected:
  std::string service_name_;
  rclcpp::Node::SharedPtr node_;
  typename rclcpp::Client<ServiceT>::SharedPtr client_;
};

}  // namespace nav2_util

#endif  // NAV2_UTIL__SERVICE_CLIENT_HPP_
