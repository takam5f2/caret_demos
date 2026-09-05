#include "agnocast/agnocast.hpp"
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"

#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <utility>

using namespace std::chrono_literals;

// Usage: agnocast_demo <node_name>
//   node1 : sensor_simulator_node
//   node2 : bridge_to_agnocast_node
//   node3 : relay_by_pattern_a_node
//   node6 : bridge_from_agnocast_node
//   node7 : dummy_actuator_node

static void run_node1()
{
  auto node = std::make_shared<rclcpp::Node>("sensor_simulator_node");
  auto pub = node->create_publisher<std_msgs::msg::String>("/topic1", 10);
  auto timer = node->create_wall_timer(1s, [node, pub]() {
    auto msg = std_msgs::msg::String();
    msg.data = "sensor_data";
    RCLCPP_INFO(node->get_logger(), "[Node1/SensorSimulator] pub /topic1 (timer)");
    pub->publish(msg);
  });
  agnocast::SingleThreadedAgnocastExecutor executor;
  executor.add_node(node);
  executor.spin();
}

static void run_node2()
{
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
  agnocast::SingleThreadedAgnocastExecutor executor;
  executor.add_node(node);
  executor.spin();
}

static void run_node3()
{
  auto node = std::make_shared<rclcpp::Node>("relay_by_pattern_a_node");
  rclcpp::CallbackGroup::SharedPtr cb_group =
    node->create_callback_group(rclcpp::CallbackGroupType::MutuallyExclusive);
  agnocast::SubscriptionOptions agnocast_opts;
  agnocast_opts.callback_group = cb_group;

  auto agnocast_pub = agnocast::create_publisher<std_msgs::msg::String>(
    node.get(), "/topic3", rclcpp::QoS(10));
  auto agnocast_sub = agnocast::create_subscription<std_msgs::msg::String>(
    node.get(), "/topic2", rclcpp::QoS(10),
    [node, agnocast_pub](const agnocast::ipc_shared_ptr<const std_msgs::msg::String> & msg) {
      RCLCPP_INFO(node->get_logger(),
        "[Node3/RelayByPatternA] agnocast sub /topic2 -> agnocast pub /topic3 (%s)",
        msg->data.c_str());
      auto out = agnocast_pub->borrow_loaned_message();
      out->data = msg->data;
      agnocast_pub->publish(std::move(out));
    },
    agnocast_opts);
  agnocast::SingleThreadedAgnocastExecutor executor;
  executor.add_node(node);
  executor.spin();
}

static void run_node6()
{
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
  agnocast::SingleThreadedAgnocastExecutor executor;
  executor.add_node(node);
  executor.spin();
}

static void run_node7()
{
  auto node = std::make_shared<rclcpp::Node>("dummy_actuator_node");
  auto sub = node->create_subscription<std_msgs::msg::String>(
    "/topic6", 10,
    [node](const std_msgs::msg::String::SharedPtr msg) {
      RCLCPP_INFO(node->get_logger(),
        "[Node7/DummyActuator] ROS2 sub /topic6 -> complete! (%s)", msg->data.c_str());
    });
  agnocast::SingleThreadedAgnocastExecutor executor;
  executor.add_node(node);
  executor.spin();
}

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);

  if (argc < 2) {
    RCLCPP_ERROR(rclcpp::get_logger("agnocast_demo"),
      "Usage: agnocast_demo <node_name>  (node1|node2|node3|node6|node7)");
    rclcpp::shutdown();
    return 1;
  }

  const std::string node_name(argv[1]);

  if (node_name == "node1") {
    run_node1();
  } else if (node_name == "node2") {
    run_node2();
  } else if (node_name == "node3") {
    run_node3();
  } else if (node_name == "node6") {
    run_node6();
  } else if (node_name == "node7") {
    run_node7();
  } else {
    RCLCPP_ERROR(rclcpp::get_logger("agnocast_demo"),
      "Unknown node: %s  (use node1|node2|node3|node6|node7)", node_name.c_str());
    rclcpp::shutdown();
    return 1;
  }

  rclcpp::shutdown();
  return 0;
}