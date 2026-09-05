#include "agnocast/agnocast.hpp"
#include "std_msgs/msg/string.hpp"

#include <memory>
#include <string>
#include <utility>

using namespace std::chrono_literals;

// ==========================================
// [Node4] RelayByPatternBNode (agnocast sub/pub by Pattern B)
// ==========================================
// Pattern B (Stage 2): agnocast::Node + this->create_subscription + this->create_publisher
// Runs with AgnocastOnlySingleThreadedExecutor
class RelayByPatternBNode : public agnocast::Node
{
public:
  RelayByPatternBNode() : agnocast::Node("relay_by_pattern_b_node")
  {
    agnocast_sub_ = this->create_subscription<std_msgs::msg::String>(
      "/topic3", rclcpp::QoS(10),
      [this](const agnocast::ipc_shared_ptr<const std_msgs::msg::String> & msg) {
        RCLCPP_INFO(
          get_logger(),
          "[Node4/RelayByPatternB] agnocast sub /topic3 -> agnocast pub /topic4 (%s)",
          msg->data.c_str());

        auto out = agnocast_pub_->borrow_loaned_message();
        out->data = msg->data;
        agnocast_pub_->publish(std::move(out));
      });

    agnocast_pub_ = this->create_publisher<std_msgs::msg::String>("/topic4", rclcpp::QoS(1));
  }

private:
  agnocast::Subscription<std_msgs::msg::String>::SharedPtr agnocast_sub_;
  agnocast::Publisher<std_msgs::msg::String>::SharedPtr agnocast_pub_;
};

// ==========================================
// Main Function
// ==========================================
int main(int argc, char ** argv)
{
  agnocast::init(argc, argv);
  agnocast::AgnocastOnlySingleThreadedExecutor executor;
  auto node = std::make_shared<RelayByPatternBNode>();
  executor.add_node(node);
  executor.spin();
  return 0;
}