#include <chrono>
#include <memory>
#include <string>
#include <vector>
#include <thread>

#include "rclcpp/rclcpp.hpp"
#include "rclcpp/generic_publisher.hpp"
#include "rclcpp/generic_subscription.hpp"
#include "sensor_msgs/msg/image.hpp"

using namespace std::chrono_literals;

#define ENABLE_INTRA_PROCESS(options) \
    options.use_intra_process_comm = rclcpp::IntraProcessSetting::Enable

int main(int argc, char * argv[]) {
    rclcpp::init(argc, argv);

    auto transient_local_qos = rclcpp::QoS(1).reliable().transient_local();

    // --- Node 1: Sensor Simulator ---
    auto sensor_simulator_node = std::make_shared<rclcpp::Node>("sensor_simulator_node");
    auto pub_raw = sensor_simulator_node->create_publisher<sensor_msgs::msg::Image>("/image_raw", 10);
    auto timer_300ms = sensor_simulator_node->create_wall_timer(300ms, [&](){
        RCLCPP_INFO(sensor_simulator_node->get_logger(), "[Step 1] Sensor Start: Pub /image_raw (Inter-process) >>>");
        pub_raw->publish(std::make_unique<sensor_msgs::msg::Image>());
    });

    // --- Node 2: Intra-process Relay ---
    auto intra_relay_node = std::make_shared<rclcpp::Node>(
        "intra_relay_node", rclcpp::NodeOptions().use_intra_process_comms(true));
    auto pub_filtered = intra_relay_node->create_publisher<sensor_msgs::msg::Image>("/image_filtered_intra", 10);
    auto sub_raw = intra_relay_node->create_subscription<sensor_msgs::msg::Image>(
        "/image_raw", 10, [&](sensor_msgs::msg::Image::UniquePtr msg){
            RCLCPP_INFO(intra_relay_node->get_logger(),
                "        [Step 2] Intra Relay: Recv /image_raw -> Pub /image_filtered_intra (Intra-process)");
            pub_filtered->publish(std::move(msg));
        });

    // --- Node 3: Data Serializer ---
    auto data_serializer_node = std::make_shared<rclcpp::Node>("data_serializer_node");
    auto pub_serialized = data_serializer_node->create_generic_publisher(
        "/image_serialized", "sensor_msgs/msg/Image", 10);
    auto sub_filtered = data_serializer_node->create_subscription<sensor_msgs::msg::Image>(
        "/image_filtered_intra", 10, [&](sensor_msgs::msg::Image::UniquePtr msg){
            RCLCPP_INFO(data_serializer_node->get_logger(),
                "    [Step 3] Data Serializer: Recv /image_filtered_intra -> Pub Generic /image_serialized");
            auto ser_msg = std::make_shared<rclcpp::SerializedMessage>();
            rclcpp::Serialization<sensor_msgs::msg::Image> ser;
            ser.serialize_message(msg.get(), ser_msg.get());
            pub_serialized->publish(*ser_msg);
        });

    // --- Node 4: Generic Relay ---
    auto generic_relay_node = std::make_shared<rclcpp::Node>("generic_relay_node");
    auto pub_relay = generic_relay_node->create_publisher<sensor_msgs::msg::Image>("/image_relay", 10);
    auto group_relay = generic_relay_node->create_callback_group(
        rclcpp::CallbackGroupType::MutuallyExclusive, false);
    auto opt_relay = rclcpp::SubscriptionOptions();
    opt_relay.callback_group = group_relay;
    
    auto sub_data_gen = generic_relay_node->create_generic_subscription(
        "/image_serialized", "sensor_msgs/msg/Image", 10,
        [](std::shared_ptr<rclcpp::SerializedMessage>){}, opt_relay);
    auto sub_trig_gen = generic_relay_node->create_generic_subscription(
        "/image_serialized", "sensor_msgs/msg/Image", 10, [&](std::shared_ptr<rclcpp::SerializedMessage>){
            RCLCPP_INFO(generic_relay_node->get_logger(), "      [Step 4] Generic Relay: Triggered by /image_serialized.");
            auto msg = sub_data_gen->create_serialized_message();
            rclcpp::MessageInfo info;
            if (sub_data_gen->take_serialized(*msg, info)) {
                RCLCPP_INFO(generic_relay_node->get_logger(), "      [Step 4] SUCCESS: Pub /image_relay");
                pub_relay->publish(*msg);
            }
        });

    // --- Node 5: Inter Take Node ---
    auto inter_take_node = std::make_shared<rclcpp::Node>("inter_take_node");
    auto pub_buffered = inter_take_node->create_publisher<sensor_msgs::msg::Image>("/image_inter_buffered", 10);
    auto group_inter = inter_take_node->create_callback_group(rclcpp::CallbackGroupType::MutuallyExclusive, false);
    auto opt_inter = rclcpp::SubscriptionOptions();
    opt_inter.callback_group = group_inter;
    
    auto sub_data_relay = inter_take_node->create_subscription<sensor_msgs::msg::Image>(
        "/image_relay", 10, [](const sensor_msgs::msg::Image::SharedPtr){}, opt_inter);
    auto timer_150ms = inter_take_node->create_wall_timer(150ms, [inter_take_node, pub_buffered, sub_data_relay](){
        sensor_msgs::msg::Image msg; rclcpp::MessageInfo info;
        if (sub_data_relay->take(msg, info)) {
            RCLCPP_INFO(inter_take_node->get_logger(),
                "         [Step 5] Inter Take Node: Timer triggered take(/image_relay) -> Pub /image_inter_buffered");
            pub_buffered->publish(msg);
        }
    });

    // --- Node 6: Intra Take Node ---
    auto intra_take_node = std::make_shared<rclcpp::Node>(
        "intra_take_node", rclcpp::NodeOptions().use_intra_process_comms(true));
    auto pub_latched = intra_take_node->create_publisher<sensor_msgs::msg::Image>(
        "/image_transient_local", transient_local_qos);
    auto group_intra = intra_take_node->create_callback_group(rclcpp::CallbackGroupType::MutuallyExclusive, false);
    auto opt_intra = rclcpp::SubscriptionOptions();
    opt_intra.callback_group = group_intra;
    ENABLE_INTRA_PROCESS(opt_intra);

    auto sub_data_intra = intra_take_node->create_subscription<sensor_msgs::msg::Image>(
        "/image_filtered_intra", 10, 
        [pub_latched](const sensor_msgs::msg::Image::SharedPtr msg){ pub_latched->publish(*msg); }, opt_intra);

    auto sub_trig_inter = intra_take_node->create_subscription<sensor_msgs::msg::Image>(
        "/image_inter_buffered", 10, [intra_take_node, sub_data_intra](sensor_msgs::msg::Image::UniquePtr){
            RCLCPP_INFO(intra_take_node->get_logger(),
                "         [Step 6] Intra Take Node: Topic triggered take(/image_filtered_intra).");
            auto ipw = sub_data_intra->get_intra_process_waitable();
            if (ipw && ipw->is_ready(nullptr)) {
                auto data = ipw->take_data();
                if (data && (ipw->execute(data), true)) {
                    RCLCPP_INFO(intra_take_node->get_logger(), "         [Step 6] SUCCESS: Pub /image_transient_local");
                }
            }
        });

    // --- Node 7: Timer Take Node ---
    auto timer_take_node = std::make_shared<rclcpp::Node>("timer_take_node");
    auto pub_final = timer_take_node->create_publisher<sensor_msgs::msg::Image>("/image_final", 10);
    auto group_timer = timer_take_node->create_callback_group(rclcpp::CallbackGroupType::MutuallyExclusive, false);
    auto opt_timer = rclcpp::SubscriptionOptions();
    opt_timer.callback_group = group_timer;
    
    auto sub_data_latched = timer_take_node->create_subscription<sensor_msgs::msg::Image>(
        "/image_transient_local", transient_local_qos, [](const sensor_msgs::msg::Image::SharedPtr){}, opt_timer);
    auto timer_cons_100ms = timer_take_node->create_wall_timer(200ms, [timer_take_node, pub_final, sub_data_latched](){
        auto msg = std::make_shared<sensor_msgs::msg::Image>();
        rclcpp::MessageInfo info;
        if (sub_data_latched->take(*msg, info)) {
            RCLCPP_INFO(timer_take_node->get_logger(),
                "         [Step 7] Timer Take Node: Timer triggered take(/image_transient_local) -> Pub /image_final");
            pub_final->publish(*msg);
        }
    });

    // --- Node 8: Dummy Actuator ---
    auto dummy_actuator_node = std::make_shared<rclcpp::Node>("dummy_actuator_node");
    auto sub_final = dummy_actuator_node->create_subscription<sensor_msgs::msg::Image>(
        "/image_final", 10, [&](const sensor_msgs::msg::Image::SharedPtr){
            RCLCPP_INFO(dummy_actuator_node->get_logger(), "     [Step 8] Dummy Actuator: Recv /image_final. Path Complete! <<<");
        });

    // --- Execution ---
    std::vector<std::shared_ptr<rclcpp::Node>> nodes = {
        sensor_simulator_node, intra_relay_node, data_serializer_node,
        generic_relay_node, inter_take_node, intra_take_node,
        timer_take_node, dummy_actuator_node
    };
    std::vector<std::thread> threads;
    for (auto node : nodes) { threads.emplace_back([node]() { rclcpp::spin(node); }); }
    for (auto & thread : threads) { thread.join(); }

    rclcpp::shutdown();
    return 0;
}
