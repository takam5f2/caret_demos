#include "agnocast/agnocast.hpp"
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"

#include <memory>
#include <string>
#include <utility>

// ==========================================
// [Node2] BridgeToAgnocastNode (ROS 2 -> agnocast bridge)
// ==========================================
int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);

  auto node = std::make_shared<rclcpp::Node>("bridge_to_agnocast_node");

  auto agnocast_pub = agnocast::create_publisher<std_msgs::msg::String>(
    node.get(), "/topic2", rclcpp::QoS(10));

  auto sub = node->create_subscription<std_msgs::msg::String>(
    "/topic1", 10,
    [node, agnocast_pub](const std_msgs::msg::String::SharedPtr msg) {
      RCLCPP_INFO(node->get_logger(),
        "[Node2/BridgeToAgnocast] ROS2 sub /topic1 -> agnocast pub /topic2 (%s)",
        msg->data.c_str());

      auto out = agnocast_pub->borrow_loaned_message();
      out->data = msg->data;
      agnocast_pub->publish(std::move(out));
    });

  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}