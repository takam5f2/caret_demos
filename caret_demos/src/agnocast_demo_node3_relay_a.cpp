#include "agnocast/agnocast.hpp"
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"

#include <memory>
#include <string>
#include <utility>

// ==========================================
// [Node3] RelayByPatternANode (agnocast sub/pub by Pattern A)
// ==========================================
int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);

  auto node = std::make_shared<rclcpp::Node>("relay_by_pattern_a_node");

  rclcpp::CallbackGroup::SharedPtr cb_group =
    node->create_callback_group(rclcpp::CallbackGroupType::MutuallyExclusive);
  agnocast::SubscriptionOptions agnocast_opts;
  agnocast_opts.callback_group = cb_group;

  auto agnocast_pub = agnocast::create_publisher<std_msgs::msg::String>(
    node.get(), "/topic3", rclcpp::QoS(10));

  auto agnocast_sub = agnocast::create_subscription<std_msgs::msg::String>(
    node.get(), "/topic2", rclcpp::QoS(1),
    [node, agnocast_pub](const agnocast::ipc_shared_ptr<const std_msgs::msg::String> & msg) {
      RCLCPP_INFO(node->get_logger(),
        "[Node3/RelayByPatternA] agnocast sub /topic2 -> agnocast pub /topic3 (%s)",
        msg->data.c_str());

      auto out = agnocast_pub->borrow_loaned_message();
      out->data = msg->data;
      agnocast_pub->publish(std::move(out));
    },
    agnocast_opts);

  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}