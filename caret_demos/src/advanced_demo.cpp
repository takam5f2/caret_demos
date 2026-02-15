#include <chrono>
#include <memory>
#include <vector>
#include <random>

#include "rclcpp/rclcpp.hpp"
#include "rclcpp/generic_publisher.hpp"
#include "rclcpp/generic_subscription.hpp"
#include "sensor_msgs/msg/image.hpp"

#define QOS_HISTORY_SIZE 10

using namespace std::chrono_literals;

std::chrono::milliseconds lognormal_distribution(double max)
{
  static std::random_device seed_gen;
  static std::default_random_engine engine(seed_gen());
  static std::lognormal_distribution<> dist(1.1, 1.7);

  int sleep_ms = std::max(std::min(dist(engine), max), 20.0);
  return std::chrono::milliseconds(sleep_ms);
}

class NoDependencyNode : public rclcpp::Node
{
public:
  NoDependencyNode(std::string node_name, std::string sub_topic_name, std::string pub_topic_name)
  : Node(node_name)
  {
    pub_ = create_publisher<sensor_msgs::msg::Image>(pub_topic_name, QOS_HISTORY_SIZE);
    sub_ = create_subscription<sensor_msgs::msg::Image>(
      sub_topic_name, QOS_HISTORY_SIZE, [&](sensor_msgs::msg::Image::UniquePtr msg)
      {
        rclcpp::sleep_for(lognormal_distribution(200));
        pub_->publish(std::move(msg));
      });
  }

private:
  rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr pub_;
  rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr sub_;
};

class ActuatorDummy : public rclcpp::Node
{
public:
  ActuatorDummy(std::string node_name, std::string sub_topic_name)
  : Node(node_name)
  {
    sub_ = create_subscription<sensor_msgs::msg::Image>(
      sub_topic_name, QOS_HISTORY_SIZE,
      [this](sensor_msgs::msg::Image::UniquePtr msg)
      {
        rclcpp::sleep_for(lognormal_distribution(80));
        (void)msg;
      }
    );
  }

private:
  rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr sub_;
};

class SensorDummy : public rclcpp::Node
{
public:
  SensorDummy(std::string node_name, std::string topic_name, int period_ms)
  : Node(node_name)
  {
    this->declare_parameter<bool>("use_rosbag", false);
    bool use_rosbag = false;
    this->get_parameter("use_rosbag", use_rosbag);
    if (use_rosbag) return;

    pub_ = create_publisher<sensor_msgs::msg::Image>(topic_name, QOS_HISTORY_SIZE);
    timer_ = create_wall_timer(std::chrono::milliseconds(period_ms), [&]() {
        auto msg = std::make_unique<sensor_msgs::msg::Image>();
        rclcpp::sleep_for(lognormal_distribution(50));
        msg->header.stamp = now();
        pub_->publish(std::move(msg));
      });
  }

private:
  rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr pub_;
  rclcpp::TimerBase::SharedPtr timer_;
};

class GenericRelayNode : public rclcpp::Node {
public:
  GenericRelayNode(std::string node_name, std::string sub_topic, std::string pub_topic, std::string type)
  : Node(node_name) {
    pub_ = create_generic_publisher(pub_topic, type, QOS_HISTORY_SIZE);
    sub_ = create_generic_subscription(sub_topic, type, QOS_HISTORY_SIZE,
      [this](std::shared_ptr<rclcpp::SerializedMessage> msg) {
        pub_->publish(*msg);
      });
  }
private:
  rclcpp::GenericPublisher::SharedPtr pub_;
  rclcpp::GenericSubscription::SharedPtr sub_;
};

// TakePollingNode
class TakePollingNode : public rclcpp::Node {
public:
  TakePollingNode(std::string node_name, std::string sub_topic, std::string trigger_topic, std::string pub_topic, int period_ms) 
  : Node(node_name) {
    pub_ = create_publisher<sensor_msgs::msg::Image>(pub_topic, QOS_HISTORY_SIZE);
    polling_group_ = create_callback_group(rclcpp::CallbackGroupType::MutuallyExclusive, false);

    rclcpp::SubscriptionOptions options;
    options.callback_group = polling_group_;
    
    sub_ = create_subscription<sensor_msgs::msg::Image>(
      sub_topic, QOS_HISTORY_SIZE, [](sensor_msgs::msg::Image::UniquePtr) {}, options);

    if (trigger_topic.empty()) {
      timer_ = create_wall_timer(std::chrono::milliseconds(period_ms), [this]() { this->execute_take(); });
    } else {
      sub_trigger_ = create_subscription<sensor_msgs::msg::Image>(
        trigger_topic, QOS_HISTORY_SIZE, [this](sensor_msgs::msg::Image::UniquePtr) { this->execute_take(); });
    }
  }

private:
  void execute_take() {
    sensor_msgs::msg::Image msg; 
    rclcpp::MessageInfo msg_info;
    if (sub_->take(msg, msg_info)) {
      auto pub_msg = std::make_unique<sensor_msgs::msg::Image>(msg);
      rclcpp::sleep_for(lognormal_distribution(45));
      pub_->publish(std::move(pub_msg));
    }
  }
  rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr pub_;
  rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr sub_, sub_trigger_;
  rclcpp::TimerBase::SharedPtr timer_;
  rclcpp::CallbackGroup::SharedPtr polling_group_;
};

int main(int argc, char * argv[]) {
  rclcpp::init(argc, argv);
  auto executor = std::make_shared<rclcpp::executors::MultiThreadedExecutor>();
  std::vector<std::shared_ptr<rclcpp::Node>> nodes;

  rclcpp::NodeOptions intra_proc_options;
  intra_proc_options.use_intra_process_comms(true);

  nodes.emplace_back(std::make_shared<SensorDummy>(
    "sensor_dummy_node", "/topic1", 100, intra_proc_options)
  );

  nodes.emplace_back(std::make_shared<GenericRelayNode>(
    "generic_relay", "/topic1", "/topic_gen", "sensor_msgs/msg/Image", intra_proc_options)
  );

  nodes.emplace_back(std::make_shared<NoDependencyNode>("filter_node", "/topic_gen", "/topic2"));

  // take: topic trigger
  nodes.emplace_back(std::make_shared<TakePollingNode>(
    "take_polling_node", "/topic2", "/topic2", "/topic_trig", 100));

  // take: timer trigger
  nodes.emplace_back(std::make_shared<TakePollingNode>(
    "take_polling_node_timer", "/topic_trig", "", "/topic_polled", 100));

  nodes.emplace_back(std::make_shared<ActuatorDummy>("actuator_dummy_node", "/topic_polled"));

  for (auto & node : nodes) { executor->add_node(node); }
  executor->spin();
  rclcpp::shutdown();
  return 0;
}