#include "agnocast/agnocast.hpp"
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"

#include <memory>
#include <string>

// ==========================================
// [Node6] BridgeFromAgnocastNode (agnocast -> ROS 2 bridge)
// ==========================================
int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);

  auto node = std::make_shared<rclcpp::Node>("bridge_from_agnocast_node");

  rclcpp::CallbackGroup::SharedPtr cb_group =
    node->create_callback_group(rclcpp::CallbackGroupType::MutuallyExclusive);
  agnocast::SubscriptionOptions agnocast_opts;
  agnocast_opts.callback_group = cb_group;

  auto ros2_pub = node->create_publisher<std_msgs::msg::String>("/topic6", 10);

  auto agnocast_sub = agnocast::create_subscription<std_msgs::msg::String>(
    node.get(), "/topic5", rclcpp::QoS(10),
    [node, ros2_pub](const agnocast::ipc_shared_ptr<const std_msgs::msg::String> & msg) {
      RCLCPP_INFO(node->get_logger(),
        "[Node6/BridgeFromAgnocast] agnocast sub /topic5 -> ROS2 pub /topic6 (%s)",
        msg->data.c_str());

      auto out = std_msgs::msg::String();
      out.data = msg->data;
      ros2_pub->publish(out);
    },
    agnocast_opts);

  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}