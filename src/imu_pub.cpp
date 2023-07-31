#include <atomic>
#include <csignal>
#include <thread>
#include <unistd.h>

#include "atk_imu901/imu_pub.h"
#include "rclcpp/rclcpp.hpp"

#include "spdlog/cfg/env.h"
#include "spdlog/fmt/ostr.h"
#include "spdlog/spdlog.h"

ImuPub::ImuPub() : rclcpp::Node("imu901m")
{
    std::string port;
    this->declare_parameter("imu_port", "/dev/ttyS6");
    this->get_parameter("imu_port", port);
    spdlog::info("port = {}", port);

    int baudrate;
    this->declare_parameter("baudrate", 115200);
    this->get_parameter("baudrate", baudrate);
    spdlog::info("baudrate = {}", baudrate);

    std::string topic;
    this->declare_parameter("topic", "imu");
    this->get_parameter("topic", topic);
    spdlog::info("topic = {}", topic);

    atk_ms901_ = std::make_shared<AtkMs901m>();
    atk_ms901_->Init(port, baudrate);

    imu_pub_   = this->create_publisher<sensor_msgs::msg::Imu>(topic, 10);
    imu_timer_ = this->create_wall_timer(
        std::chrono::milliseconds(50), std::bind(&ImuPub::ImuPubCallback, this));
}

ImuPub::~ImuPub()
{
}

void ImuPub::ImuPubCallback()
{
    // 定义IMU数据
    sensor_msgs::msg::Imu imu_msg;

    if (atk_ms901_) {
        // atk_ms901m_attitude_data_t attitude_dat;        /* 姿态角数据 */
        // atk_ms901_->GetAttitude(&attitude_dat, 100);    /* 获取姿态角数据 */
        atk_ms901m_gyro_data_t gyro_dat;                   /* 陀螺仪数据 */
        atk_ms901m_accelerometer_data_t accelerometer_dat; /* 加速度计数据 */
        atk_ms901m_quaternion_data_t quaternion_dat;

        atk_ms901_->GetGyroAccelerometer(&gyro_dat, &accelerometer_dat, 100); /* 获取陀螺仪、加速度计数据 */
        atk_ms901_->GetQuaternion(&quaternion_dat, 100);
        imu_msg.orientation.w         = quaternion_dat.w;
        imu_msg.orientation.x         = quaternion_dat.x;
        imu_msg.orientation.y         = quaternion_dat.y;
        imu_msg.orientation.z         = quaternion_dat.z;
        imu_msg.angular_velocity.x    = gyro_dat.x;
        imu_msg.angular_velocity.y    = gyro_dat.y;
        imu_msg.angular_velocity.z    = gyro_dat.z;
        imu_msg.linear_acceleration.x = accelerometer_dat.x;
        imu_msg.linear_acceleration.y = accelerometer_dat.y;
        imu_msg.linear_acceleration.z = accelerometer_dat.z;
    }

    for (size_t i = 0; i < 9; i++) {
        imu_msg.orientation_covariance[i]         = 0;
        imu_msg.angular_velocity_covariance[i]    = 0;
        imu_msg.linear_acceleration_covariance[i] = 0;
    }

    imu_msg.header.frame_id = "base_link";
    imu_msg.header.stamp    = this->get_clock()->now();
    imu_pub_->publish(imu_msg);
}
