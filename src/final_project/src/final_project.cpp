#include "rclcpp/rclcpp.hpp"
#include "rclcpp_action/rclcpp_action.hpp"
#include "turtlesim/srv/kill.hpp"
#include "turtlesim/srv/spawn.hpp"
#include "my_robot_interfaces/action/move_turtle.hpp"
# include "geometry_msgs/msg/twist.hpp"

using Twist = geometry_msgs::msg::Twist;
using MoveTurtle = my_robot_interfaces::action::MoveTurtle;
using MoveTurtleGoalHandle = rclcpp_action::ServerGoalHandle<MoveTurtle>;

using namespace std::placeholders;
using namespace std::chrono_literals;



class TurtleController : public rclcpp::Node
{
public:
    TurtleController() : Node("turtle_controller")
    {
        this->declare_parameter("turtle_name", rclcpp::PARAMETER_STRING);
        turtle_name_ = this->get_parameter("turtle_name").as_string();
        cb_group_ = this->create_callback_group(rclcpp::CallbackGroupType::Reentrant);
        spawn_turtle_client_ = this->create_client<turtlesim::srv::Spawn>(
            "/spawn",rmw_qos_profile_default, cb_group_);
        
        kill_turtle_client_ = this->create_client<turtlesim::srv::Kill>(
            "/kill", rmw_qos_profile_default, cb_group_);

        cmd_vel_publisher_ = this->create_publisher<Twist>("/" + turtle_name_ + "/cmd_vel", 10 );
        spawn_turtle_thread_ = std::thread(std::bind(&TurtleController::spawn_turtle, this));

        move_turtle_server_ = rclcpp_action::create_server<MoveTurtle>(
            this,
            "move_turtle" + turtle_name_,
            std::bind(&TurtleController::goal_callback, this, _1, _2),
            std::bind(&TurtleController::cancel_callback, this, _1),
            std::bind(&TurtleController::handle_accepted_callback, this, _1),
            rcl_action_server_get_default_options(),
            cb_group_
        );
        RCLCPP_INFO(this->get_logger(), "Action has been started");
    }

private:

    rclcpp_action::GoalResponse goal_callback(
        const rclcpp_action::GoalUUID &uuid, std::shared_ptr<const MoveTurtle::Goal> goal)
    {
        (void)uuid;
        (void)goal;
        RCLCPP_INFO(this->get_logger(), "Received a new goal");

        return rclcpp_action::GoalResponse::ACCEPT_AND_EXECUTE;

    }

    rclcpp_action::CancelResponse cancel_callback(
        const std::shared_ptr<MoveTurtleGoalHandle> goal_handle)
    {
        (void)goal_handle;
        RCLCPP_INFO(this->get_logger(), "Received cancel request");
        return rclcpp_action::CancelResponse::ACCEPT;
    }

    void handle_accepted_callback(
        const std::shared_ptr<MoveTurtleGoalHandle> goal_handle)
    {
        execute_goal(goal_handle);
    }

    void execute_goal(
        const std::shared_ptr<MoveTurtleGoalHandle> goal_handle)
    {
        double linear_vel_x = goal_handle->get_goal()->linear_vel_x;
        double angular_vel_z = goal_handle->get_goal()->linear_vel_z;
        int duration_sec = goal_handle->get_goal()->duration_sec;

        auto result = std::make_shared<MoveTurtle::Result>();
        auto start = now();
        auto duration = rclcpp::Duration(duration_sec, 0);
        rclcpp::Rate rate(10);
        while (rclcpp::ok())
        {
            if (now() - start > duration){
                auto msg = Twist();
                msg.linear.x = 0;
                msg.angular.z = 0;
                cmd_vel_publisher_->publish(msg);
                result->success = true;
                result->message = "Success";
                goal_handle->succeed(result);
                return;
            }
            auto msg = Twist();
            msg.linear.x = linear_vel_x;
            msg.angular.z = angular_vel_z;

            cmd_vel_publisher_->publish(msg);
            rate.sleep();
        }
        
    }



    void spawn_turtle(){
        spawn_turtle_client_ -> wait_for_service();
        auto request = std::make_shared<turtlesim::srv::Spawn::Request>();
        request->name = turtle_name_;
        request->x = 5.0;
        request->y = 5.0;
        RCLCPP_INFO(this->get_logger(), "Trying to spawn the turtle");
        auto result = spawn_turtle_client_->async_send_request(request);
        RCLCPP_INFO(this->get_logger(), "New spawned turtle: %s", result.get()->name.c_str());

        std::this_thread::sleep_for(std::chrono::seconds(3));
        kill_turtle_thread_ = std::thread(std::bind(&TurtleController::kill_turtle, this));

    }

    void kill_turtle(){
        kill_turtle_client_ -> wait_for_service();
        auto request = std::make_shared<turtlesim::srv::Kill::Request>();
        request->name = turtle_name_;
        RCLCPP_INFO(this->get_logger(), "Trying to kill the turtle");
        kill_turtle_client_->async_send_request(request);
        RCLCPP_INFO(this->get_logger(), "the turtle has been removen");
    }

    std::string turtle_name_;
    rclcpp::Client<turtlesim::srv::Spawn>::SharedPtr spawn_turtle_client_;
    rclcpp::Client<turtlesim::srv::Kill>::SharedPtr kill_turtle_client_;
    rclcpp::Publisher<Twist>::SharedPtr cmd_vel_publisher_;
    rclcpp_action::Server<MoveTurtle>::SharedPtr move_turtle_server_;
    rclcpp::CallbackGroup::SharedPtr cb_group_;


    std::thread spawn_turtle_thread_;
    std::thread kill_turtle_thread_; 

};

int main(int argc, char **argv)
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<TurtleController>();
    rclcpp::executors::MultiThreadedExecutor executor;
    executor.add_node(node);
    executor.spin();
    rclcpp::shutdown();
    return 0;
}