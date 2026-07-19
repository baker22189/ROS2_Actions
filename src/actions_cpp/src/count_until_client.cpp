#include "rclcpp/rclcpp.hpp"
#include "rclcpp_action/rclcpp_action.hpp"
#include "my_robot_interfaces/action/count_until.hpp"

using CountUntil = my_robot_interfaces::action::CountUntil;
using CounUntilGoalHandle = rclcpp_action::ClientGoalHandle<CountUntil>;
using namespace std::placeholders;

class ContUntilClientNode : public rclcpp::Node
{
public:
    ContUntilClientNode() : Node("count_until_client") 
    {
        count_until_client = rclcpp_action::create_client<CountUntil>(this, "count_until");
    }

    void send_goal(int target_numer, double period)
    {
        // wait for action server
        count_until_client-> wait_for_action_server();

        //create the goal
        auto goal = CountUntil::Goal();
        goal.target_number = target_numer;
        goal.period = period;

        // add callback
        auto options = rclcpp_action::Client<CountUntil>::SendGoalOptions();
        options.result_callback = std::bind(&ContUntilClientNode::goal_result_callback, this, _1);

        options.goal_response_callback = std::bind(&ContUntilClientNode::goal_respons_callback, this , _1);
        options.feedback_callback = std::bind(&ContUntilClientNode::goal_feedback_callback, this ,_1, _2);

        //send the goal
        RCLCPP_INFO(this->get_logger(), "sending the goal");
        count_until_client->async_send_goal(goal, options);

        //Cancel the goal (test)
        // timer_ = this -> create_wall_timer(
        //     std::chrono::seconds(2), 
        //     std::bind(&ContUntilClientNode::timer_callback, this));

    }

private:
    void timer_callback(){
        RCLCPP_INFO(this->get_logger(), "cancel the goal");
        count_until_client -> async_cancel_goal(goal_handle_);
        timer_ -> cancel();
    }


    // callback to know if the goal is accepted or rejected
    void goal_respons_callback(const CounUntilGoalHandle::SharedPtr &goal_handle)
        {
            if (!goal_handle){
                RCLCPP_INFO(this->get_logger(), "the goal got rejected");
            }
            else{
                this -> goal_handle_ = goal_handle;
                RCLCPP_INFO(this->get_logger(), "the goal got accepted");
            }

        }


    void goal_result_callback(const CounUntilGoalHandle::WrappedResult &result)
        {
            auto status = result.code;
            if(status == rclcpp_action::ResultCode::SUCCEEDED){
                RCLCPP_INFO(this->get_logger(), "Succeeded");
            }
            else if(status == rclcpp_action::ResultCode::ABORTED){
                RCLCPP_ERROR(this->get_logger(), "Aborted");
            }
            else if(status == rclcpp_action::ResultCode::CANCELED){
                RCLCPP_WARN(this->get_logger(), "Canceled");
            }

            int reached_number = result.result->reached_number;
            RCLCPP_INFO(this->get_logger(), "Result: %d", reached_number);
        }
    // receive the feedback
    void goal_feedback_callback(const CounUntilGoalHandle::SharedPtr &goal_handle, 
        const std::shared_ptr<const CountUntil::Feedback> feedback){
            (void) goal_handle;
            int number = feedback->current_number;
            RCLCPP_INFO(this->get_logger(), "Got feedback: %d", number);
    }

        rclcpp_action::Client<CountUntil>::SharedPtr count_until_client;
        rclcpp::TimerBase::SharedPtr timer_;
        CounUntilGoalHandle::SharedPtr goal_handle_;

};

int main(int argc, char **argv)
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<ContUntilClientNode>();
    node->send_goal(6, 1.0);
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}