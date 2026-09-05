#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"

#include <memory>
#include <string>

using namespace std::chrono_literals;

// ==========================================
// [Node1] SensorSimulatorNode (ROS 2 talker)
// ==========================================
int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);

  auto node = std::make_shared<rclcpp::Node>("sensor_simulator_node");
  auto pub = node->create_publisher<std_msgs::msg::String>("/topic1", 10);
  auto timer = node->create_wall_timer(1s, [node, pub]() {
    auto msg = std_msgs::msg::String();
    msg.data = "sensor_data";
    RCLCPP_INFO(node->get_logger(), "[Node1/SensorSimulator] pub /topic1 (timer)");
    pub->publish(msg);
  });

  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}