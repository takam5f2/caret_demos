#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"

#include <memory>
#include <string>

// ==========================================
// [Node7] DummyActuatorNode (ROS 2 listener)
// ==========================================
int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);

  auto node = std::make_shared<rclcpp::Node>("dummy_actuator_node");

  auto sub = node->create_subscription<std_msgs::msg::String>(
    "/topic6", 10,
    [node](const std_msgs::msg::String::SharedPtr msg) {
      RCLCPP_INFO(node->get_logger(),
        "[Node7/DummyActuator] ROS2 sub /topic6 -> complete! (%s)", msg->data.c_str());
    });

  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}