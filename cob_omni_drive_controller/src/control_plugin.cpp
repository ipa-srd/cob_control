
#include <controller_interface/controller.h>
#include <hardware_interface/joint_command_interface.h>

#include <pluginlib/class_list_macros.h>

#include <cob_omni_drive_controller/UndercarriageCtrlGeom.h>
#include <geometry_msgs/Twist.h>

#include <boost/scoped_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/thread/mutex.hpp>

#include "GeomController.h"

namespace cob_omni_drive_controller
{

class WheelController: public GeomController<hardware_interface::VelocityJointInterface, UndercarriageCtrl>
{
public:
    WheelController() {}

    virtual bool init(hardware_interface::VelocityJointInterface* hw, ros::NodeHandle &root_nh, ros::NodeHandle& controller_nh){

        if(!GeomController::init(hw, controller_nh)) return false;

        controller_nh.param("max_rot_velocity", max_vel_rot_, 0.0);
        if(max_vel_rot_ < 0){
            ROS_ERROR_STREAM("max_rot_velocity must be non-negative.");
            return false;
        }
        controller_nh.param("max_trans_velocity", max_vel_trans_, 0.0);
        if(max_vel_trans_ < 0){
            ROS_ERROR_STREAM("max_trans_velocity must be non-negative.");
            return false;
        }
        double timeout;
        controller_nh.param("timeout", timeout, 1.0);
        if(timeout < 0){
            ROS_ERROR_STREAM("timeout must be non-negative.");
            return false;
        }
        timeout_.fromSec(timeout);

        wheel_commands_.resize(wheel_states_.size());
        twist_subscriber_ = controller_nh.subscribe("command", 1, &WheelController::topicCallbackTwistCmd, this);

        return true;
  }
    virtual void starting(const ros::Time& time){
        geom_->reset();
        target_.updated = false;
    }
    virtual void update(const ros::Time& time, const ros::Duration& period){

        GeomController::update();

        {
            boost::mutex::scoped_try_lock lock(mutex_);
            if(lock){
                Target target = target_;
                target_.updated = false;

                if(!target.stamp.isZero() && !timeout_.isZero() && (time - target.stamp) > timeout_){
                    target_.stamp = ros::Time(); // only reset once
                    target.state  = UndercarriageCtrl::PlatformState();
                    target.updated = true;
                }
                lock.unlock();

                if(target.updated){
                   geom_->setTarget(target.state);
                }
            }
        }

        geom_->calcControlStep(wheel_commands_, period.toSec(), false);

        for (unsigned i=0; i<wheel_commands_.size(); i++){
            steer_joints_[i].setCommand(wheel_commands_[i].dVelGearSteerRadS);
            drive_joints_[i].setCommand(wheel_commands_[i].dVelGearDriveRadS);
        }
    }
    virtual void stopping(const ros::Time& time) {}

private:
    struct Target {
        UndercarriageCtrl::PlatformState state;
        bool updated;
        ros::Time stamp;
    } target_;

    std::vector<UndercarriageCtrl::WheelCommand> wheel_commands_;

    boost::mutex mutex_;
    ros::Subscriber twist_subscriber_;

    ros::Duration timeout_;
    double max_vel_trans_, max_vel_rot_;

    void topicCallbackTwistCmd(const geometry_msgs::Twist::ConstPtr& msg){
        if(isRunning()){
            boost::mutex::scoped_lock lock(mutex_);
            if(isnan(msg->linear.x) || isnan(msg->linear.y) || isnan(msg->angular.z)) {
                ROS_FATAL("Received NaN-value in Twist message. Reset target to zero.");
                target_.state = UndercarriageCtrl::PlatformState();
            }else{
                target_.state.setVelX(UndercarriageCtrl::limitValue(msg->linear.x, max_vel_trans_));
                target_.state.setVelY(UndercarriageCtrl::limitValue(msg->linear.y, max_vel_trans_));
                target_.state.dRotRobRadS = UndercarriageCtrl::limitValue(msg->angular.z, max_vel_rot_);
            }
            target_.updated = true;
            target_.stamp = ros::Time::now();
        }
    }


};

}

PLUGINLIB_EXPORT_CLASS( cob_omni_drive_controller::WheelController, controller_interface::ControllerBase)
