#!/usr/bin/env python3
import rclpy
from rclpy.node import Node
from rclpy.action import ActionServer, GoalResponse, CancelResponse
from rclpy.action.server import ServerGoalHandle
from my_robot_interfaces.action import CountUntil
from rclpy.executors import MultiThreadedExecutor
from rclpy.callback_groups import ReentrantCallbackGroup
import threading
import time


class CountUntilServerNode(Node):
    def __init__(self):
        super().__init__("count_until_server")
        self.goal_handel_: ServerGoalHandle = None
        self.goal_lock_ = threading.Lock()
        self.count_until_server_ = ActionServer(
            self, 
            CountUntil,
            "count_until",
            goal_callback=self.goal_callback,
            cancel_callback=self.cancel_callback,
            execute_callback=self.execute_callback, 
            callback_group=ReentrantCallbackGroup())
        
        self.get_logger().info("server is started")
    
    def goal_callback(self, goal_request: CountUntil.Goal):
        self.get_logger().info("Received a goal")

        #The Policy here is: check if the goal still active and refuse the new if it was
        with self.goal_lock_:
            if self.goal_handel_ is not None and self.goal_handel_.is_active:
                self.get_logger().info("the goal is still active, rejecting the new goal")
                return GoalResponse.REJECT



        if goal_request.target_number < 0:
            self.get_logger().info("Rejecting the goal")
            return GoalResponse.REJECT
        self.get_logger().info("Accepting the goal")
        return GoalResponse.ACCEPT
    
    def cancel_callback(self, goal_handel: ServerGoalHandle):
        self.get_logger().info("Received a cancel request")
        return CancelResponse.ACCEPT #or Reject

    def execute_callback(self, goal_handle: ServerGoalHandle):
        with self.goal_lock_:
            self.goal_handel_ = goal_handle
        #Get the request
        target_number = goal_handle.request.target_number
        period = goal_handle.request.period


        #Execute the Action
        self.get_logger().info("Executing the goal")
        feedback =CountUntil.Feedback()
        result = CountUntil.Result()
        counter =0
        for i in range(target_number):
            if goal_handle.is_cancel_requested:
                self.get_logger().info("canceling the goal")
                goal_handle.canceled()
                result.reached_number = counter
                return result
            counter += 1
            self.get_logger().info(str(counter))
            feedback.current_number = counter
            goal_handle.publish_feedback(feedback)
            time.sleep(period)

        # set the status
        goal_handle.succeed()

        #send result
        
        result.reached_number = counter
        return result

        

def main(args=None):
    rclpy.init(args=args)
    node = CountUntilServerNode()
    rclpy.spin(node, MultiThreadedExecutor())
    rclpy.shutdown()


if __name__ == "__main__":
    main()