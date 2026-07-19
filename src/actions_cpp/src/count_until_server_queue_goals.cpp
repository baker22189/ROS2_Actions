#include <queue>
#include "rclcpp/rclcpp.hpp"
#include "rclcpp_action/rclcpp_action.hpp"
#include "my_robot_interfaces/action/count_until.hpp"

using CountUntil = my_robot_interfaces::action::CountUntil;
using CountUntilGoalHandle = rclcpp_action::ServerGoalHandle<CountUntil>;
using namespace std::placeholders;

class CountUntilServerNode : public rclcpp::Node 
{
public:
    CountUntilServerNode() : Node("count_until_server") 
    {
        goal_queue_thread_ = std:: thread(&CountUntilServerNode::run_goal_queue_thread, this);
        cb_group_ = this -> create_callback_group(rclcpp::CallbackGroupType::Reentrant);
        count_until_server_ = rclcpp_action::create_server<CountUntil>(
            this,
            "count_until",
            std::bind(&CountUntilServerNode::goal_callback, this, _1, _2),
            std::bind(&CountUntilServerNode::cancel_callback, this, _1),
            std::bind(&CountUntilServerNode::handle_accepted_callback, this, _1),
            rcl_action_server_get_default_options(),
            cb_group_
        
        );
        RCLCPP_INFO(this->get_logger(), "Action Server has been started");
    }
    ~CountUntilServerNode(){
        goal_queue_thread_.join();
    }

private:
    rclcpp_action::GoalResponse goal_callback(
        const rclcpp_action::GoalUUID &uuid, std::shared_ptr<const CountUntil::Goal> goal)
        {
            (void) uuid;
            RCLCPP_INFO(this->get_logger(), "Receiving the goal");

            if(goal->target_number <= 0)
            {
                RCLCPP_INFO(this->get_logger(), "Reject the goal");
                return rclcpp_action::GoalResponse::REJECT;
            }


            RCLCPP_INFO(this->get_logger(), "Accepting the goal");
            return rclcpp_action::GoalResponse::ACCEPT_AND_EXECUTE;
        }
    
    rclcpp_action::CancelResponse cancel_callback(
        const std::shared_ptr<CountUntilGoalHandle> goal_handle)
        {
            (void) goal_handle;
            RCLCPP_INFO(this->get_logger(), "receive cancel request");
            return rclcpp_action::CancelResponse::ACCEPT;
        }

    void handle_accepted_callback(
    const std::shared_ptr<CountUntilGoalHandle> goal_handle)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        goal_queue.push(goal_handle);
        RCLCPP_INFO(this->get_logger(), "add goal in the queue");
        RCLCPP_INFO(this->get_logger(), "Queue size: %d", (int)goal_queue.size());
    }

    void run_goal_queue_thread()
    {
        rclcpp::Rate loop_rate(1000.0);
        while(rclcpp::ok()){
            std::shared_ptr<CountUntilGoalHandle> next_goal;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                if(goal_queue.size()>0){
                next_goal = goal_queue.front();
                goal_queue.pop();

                }

            }

            if(next_goal){
                RCLCPP_INFO(this->get_logger(), "executing the next goal in queue");
                execute_goal(next_goal);
            }


            loop_rate.sleep();
        }
    }

    void execute_goal(
    const std::shared_ptr<CountUntilGoalHandle> goal_handle)
    {
        //get the request from the goal
        int target_number = goal_handle->get_goal()->target_number;
        double period = goal_handle->get_goal()->period;

        //Execution the action
        int counter = 0;
        auto result = std::make_shared<CountUntil::Result>();
        auto feedback = std::make_shared<CountUntil::Feedback>();
        rclcpp::Rate loop_rate(1.0/period);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        for (int i=0; i<target_number; i++ ){

            if (goal_handle->is_canceling()){
                result->reached_number = counter;
                goal_handle->canceled(result);
                return;
            }
            counter++;
            RCLCPP_INFO(this->get_logger(), "%d", counter);
            feedback->current_number = counter;
            goal_handle->publish_feedback(feedback);
            loop_rate.sleep();
        }


        //set the final state and return the result
        result->reached_number = counter;
        goal_handle->succeed(result);
        
        
    }



    rclcpp_action::Server<CountUntil>::SharedPtr count_until_server_;
    rclcpp::CallbackGroup::SharedPtr cb_group_;
    std::mutex mutex_;
    std::queue<std::shared_ptr<CountUntilGoalHandle>> goal_queue;
    std::thread goal_queue_thread_;
};

int main(int argc, char **argv)
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<CountUntilServerNode>(); 
    rclcpp::executors::MultiThreadedExecutor executor;
    executor.add_node(node);
    executor.spin();
    rclcpp::shutdown();
    return 0;
}