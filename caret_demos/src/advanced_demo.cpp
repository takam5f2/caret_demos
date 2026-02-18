#include <chrono>
#include <memory>
#include <vector>
#include <random>
#include <string>
#include <algorithm>

#include "rclcpp/rclcpp.hpp"
#include "rclcpp/generic_publisher.hpp"
#include "rclcpp/generic_subscription.hpp"
#include "sensor_msgs/msg/image.hpp"

#define QOS_HISTORY_SIZE 10

using namespace std::chrono_literals;

std::string spaces(int count) {
  return std::string(std::max(0, count), ' ');
}

std::chrono::milliseconds lognormal_distribution(double max)
{
  static std::random_device seed_gen;
  static std::default_random_engine engine(seed_gen());
  static std::lognormal_distribution<> dist(1.1, 1.7);
  int sleep_ms = std::max(std::min(dist(engine), max), 20.0);
  return std::chrono::milliseconds(sleep_ms);
}


class SensorDummy : public rclcpp::Node {
public:
  SensorDummy(std::string name, std::string pub_t, int period, double delay, int idx, int col, const rclcpp::NodeOptions & opt)
  : Node(name, opt), idx_(idx), col_(col), delay_(delay) {
    pub_ = create_publisher<sensor_msgs::msg::Image>(pub_t, QOS_HISTORY_SIZE);
    timer_ = create_wall_timer(std::chrono::milliseconds(period), [this]() {
        auto msg = std::make_unique<sensor_msgs::msg::Image>();
        msg->header.stamp = now();
        RCLCPP_INFO(this->get_logger(), "%s [Step %d Sensor] Publish >>>", spaces(col_).c_str(), idx_);
        rclcpp::sleep_for(lognormal_distribution(delay_));
        pub_->publish(std::move(msg));
      });
  }
private:
  int idx_, col_; double delay_;
  rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr pub_;
  rclcpp::TimerBase::SharedPtr timer_;
};

class GenericRelayNode : public rclcpp::Node {
public:
  GenericRelayNode(std::string name, std::string sub_t, std::string pub_t, std::string type, int idx, int col, const rclcpp::NodeOptions & opt)
  : Node(name, opt), idx_(idx), col_(col) {
    pub_ = create_generic_publisher(pub_t, type, QOS_HISTORY_SIZE);
    sub_ = create_generic_subscription(sub_t, type, QOS_HISTORY_SIZE,
      [this](std::shared_ptr<rclcpp::SerializedMessage> msg) {
        RCLCPP_INFO(this->get_logger(), "%s [Step %d Generic] Relay", spaces(col_).c_str(), idx_);
        pub_->publish(*msg);
      });
  }
private:
  int idx_, col_;
  rclcpp::GenericPublisher::SharedPtr pub_;
  rclcpp::GenericSubscription::SharedPtr sub_;
};

class NoDependencyNode : public rclcpp::Node {
public:
  NoDependencyNode(std::string name, std::string sub_t, std::string pub_t, double delay, int idx, int col, const rclcpp::NodeOptions & opt)
  : Node(name, opt), idx_(idx), col_(col), delay_(delay) {
    pub_ = create_publisher<sensor_msgs::msg::Image>(pub_t, QOS_HISTORY_SIZE);
    sub_ = create_subscription<sensor_msgs::msg::Image>(sub_t, QOS_HISTORY_SIZE,
      [this](sensor_msgs::msg::Image::UniquePtr msg) {
        RCLCPP_INFO(this->get_logger(), "%s [Step %d Standard] Callback Received", spaces(col_).c_str(), idx_);
        rclcpp::sleep_for(lognormal_distribution(delay_));
        pub_->publish(std::move(msg));
      });
  }
private:
  int idx_, col_; double delay_;
  rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr pub_;
  rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr sub_;
};

class TakePollingNode : public rclcpp::Node {
public:
  TakePollingNode(std::string name, std::string sub_t, std::string pub_t, std::string trigger_t, int ms, double delay, int idx, int col, const rclcpp::NodeOptions & opt)
  : Node(name, opt), idx_(idx), col_(col), delay_(delay), received_(false) {
    pub_ = create_publisher<sensor_msgs::msg::Image>(pub_t, QOS_HISTORY_SIZE);
    auto group = create_callback_group(rclcpp::CallbackGroupType::MutuallyExclusive, false);
    rclcpp::SubscriptionOptions s_opt; s_opt.callback_group = group;
    sub_ = create_subscription<sensor_msgs::msg::Image>(sub_t, QOS_HISTORY_SIZE, [](sensor_msgs::msg::Image::UniquePtr){}, s_opt);
    
    if (trigger_t.empty()) {
      timer_ = create_wall_timer(std::chrono::milliseconds(ms), [this](){ execute(true); });
    } else {
      sub_trig_ = create_subscription<sensor_msgs::msg::Image>(trigger_t, QOS_HISTORY_SIZE, [this](sensor_msgs::msg::Image::UniquePtr){ execute(false); });
    }
  }
private:
  void execute(bool is_timer) {
    sensor_msgs::msg::Image msg; rclcpp::MessageInfo info;
    if (sub_->take(msg, info)) {
      received_ = true;
      RCLCPP_INFO(this->get_logger(), "%s [Step %d Take:%s] Success", spaces(col_).c_str(), idx_, is_timer ? "Timer" : "Topic");
      rclcpp::sleep_for(lognormal_distribution(delay_));
      pub_->publish(std::make_unique<sensor_msgs::msg::Image>(msg));
    } else if (is_timer && received_) {
      RCLCPP_INFO(this->get_logger(), "%s [Step %d Take:Timer] No data", spaces(col_).c_str(), idx_);
    }
  }
  int idx_, col_; double delay_; bool received_;
  rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr pub_;
  rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr sub_, sub_trig_;
  rclcpp::TimerBase::SharedPtr timer_;
};

class ActuatorDummy : public rclcpp::Node {
public:
  ActuatorDummy(std::string name, std::string sub_t, double delay, int idx, int col, const rclcpp::NodeOptions & opt)
  : Node(name, opt), idx_(idx), col_(col), delay_(delay) {
    sub_ = create_subscription<sensor_msgs::msg::Image>(sub_t, QOS_HISTORY_SIZE,
      [this](sensor_msgs::msg::Image::UniquePtr){
        RCLCPP_INFO(this->get_logger(), "%s [Step %d Actuator] Reached <<<", spaces(col_).c_str(), idx_);
        rclcpp::sleep_for(lognormal_distribution(delay_));
      });
  }
private:
  int idx_, col_; double delay_;
  rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr sub_;
};


int main(int argc, char * argv[]) {
  rclcpp::init(argc, argv);
  auto executor = std::make_shared<rclcpp::executors::MultiThreadedExecutor>();
  std::vector<std::shared_ptr<rclcpp::Node>> nodes;

  struct NodeBaseConfig {
    std::string name;
    std::string sub_t;
    std::string pub_t;
    int step;
    double max_delay;
  };

  std::vector<NodeBaseConfig> configs = {
    {"sensor_dummy_node",       "",             "/topic1",       1,  40.0},
    {"generic_relay",           "/topic1",      "/topic_gen",    2,   0.0},
    {"filter_node",             "/topic_gen",   "/topic2",       3, 150.0},
    {"take_polling_node",       "/topic2",      "/topic_trig",   4,  40.0},
    {"take_polling_node_timer", "/topic_trig",  "/topic_polled", 5,  40.0},
    {"actuator_dummy_node",     "/topic_polled", "",             6,  60.0}
  };

  // indentation for log
  size_t max_name_len = 0;
  for (const auto& c : configs) max_name_len = std::max(max_name_len, c.name.length());
  auto get_col = [&](const NodeBaseConfig& c) {
    return static_cast<int>(max_name_len - c.name.length()) + (c.step * 2) + 1;
  };

  rclcpp::NodeOptions intra_on;  intra_on.use_intra_process_comms(true);
  rclcpp::NodeOptions intra_off; intra_off.use_intra_process_comms(false);

  // Step 1: Sensor (Inter-process)
  nodes.emplace_back(std::make_shared<SensorDummy>(
    configs[0].name, configs[0].pub_t, 300, configs[0].max_delay, 
    configs[0].step, get_col(configs[0]), 
    intra_off));

  // Step 2: Generic Relay (Inter-process)
  nodes.emplace_back(std::make_shared<GenericRelayNode>(
    configs[1].name, configs[1].sub_t, configs[1].pub_t, "sensor_msgs/msg/Image", 
    configs[1].step, get_col(configs[1]), 
    intra_off));

  // Step 3: Filter (Inter-process)
  nodes.emplace_back(std::make_shared<NoDependencyNode>(
    configs[2].name, configs[2].sub_t, configs[2].pub_t, configs[2].max_delay, 
    configs[2].step, get_col(configs[2]), 
    intra_off));
  
  // Step 4: Take Topic Trigger (Inter-process)
  nodes.emplace_back(std::make_shared<TakePollingNode>(
    configs[3].name, configs[3].sub_t, configs[3].pub_t, "/topic_gen", 100, 
    configs[3].max_delay, configs[3].step, get_col(configs[3]), 
    intra_off));
  
  // Step 5: Take Polling Timer Trigger (Intra-process)
  // Receive from Step 4(Inter), and send to Step 6(Intra)
  nodes.emplace_back(std::make_shared<TakePollingNode>(
    configs[4].name, configs[4].sub_t, configs[4].pub_t, "", 150, 
    configs[4].max_delay, configs[4].step, get_col(configs[4]), 
    intra_on));
  
  // Step 6: Actuator (Intra-process)
  // Receive from Step 5(Intra)
  nodes.emplace_back(std::make_shared<ActuatorDummy>(
    configs[5].name, configs[5].sub_t, configs[5].max_delay, 
    configs[5].step, get_col(configs[5]), 
    intra_on));

  for (auto & node : nodes) { executor->add_node(node); }
  
  RCLCPP_INFO(rclcpp::get_logger("main"), "===== Final Experiment Configuration (Inter x Intra) Started =====");
  executor->spin();
  rclcpp::shutdown();
  return 0;
}
