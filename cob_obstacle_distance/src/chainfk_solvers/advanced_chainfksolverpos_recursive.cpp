/*
 * advanced_chainfksolverpos_recursive.h
 *
 *  Created on: May 20, 2015
 *      Author: fxm-mb
 */

#include <ros/ros.h>
#include "cob_obstacle_distance/chainfk_solvers/advanced_chainfksolverpos_recursive.h"

AdvancedChainFkSolverPos_recursive::AdvancedChainFkSolverPos_recursive(const KDL::Chain& _chain):
    chain_(_chain)
{
}

/**
 * Calculates the cartesion positions given to the joints array.
 * This special implementation ensures that the positions are stored in a vector so it is not necessary to call the method for each segment
 * again and again.
 */
int AdvancedChainFkSolverPos_recursive::JntToCart(const KDL::JntArray& q_in, KDL::Frame& p_out, int seg_nr)
{
    unsigned int segmentNr;
    if(seg_nr < 0)
    {
        segmentNr = this->chain_.getNrOfSegments();
    }
    else
    {
        segmentNr = seg_nr;
    }

    p_out = KDL::Frame::Identity();

    if(q_in.rows() != this->chain_.getNrOfJoints())
    {
        return -1;
    }
    else if(segmentNr > this->chain_.getNrOfSegments())
    {
        return -1;
    }
    else
    {
        this->segment_pos_.clear();
        int j=0;
        for(unsigned int i=0;i<segmentNr;i++)
        {
            if(this->chain_.getSegment(i).getJoint().getType() != KDL::Joint::None)
            {
                p_out = p_out * this->chain_.getSegment(i).pose(q_in(j));
                j++;
            }
            else
            {
                p_out = p_out * this->chain_.getSegment(i).pose(0.0);
            }

            this->segment_pos_.push_back(KDL::Frame(p_out)); // store copies not references
        }
        return 0;
    }
}

/**
 * Access previously set segment positions via index.
 */
KDL::Frame AdvancedChainFkSolverPos_recursive::getFrameAtSegment(uint16_t seg_idx) const
{
    KDL::Frame p_out = KDL::Frame::Identity();

    if (seg_idx < this->chain_.getNrOfSegments())
    {
        p_out = this->segment_pos_.at(seg_idx);
    }

    return p_out;
}

/**
 * Output of all previously set segment positions.
 */
void AdvancedChainFkSolverPos_recursive::dumpAllSegmentPostures() const
{
    uint16_t id = 0;
    ROS_INFO_STREAM("=== Dump all Jnt Postures ===");
    for(tFrameVector::const_iterator it = this->segment_pos_.begin(); it != this->segment_pos_.end(); ++it)
    {

        ROS_INFO_STREAM("Segment " << id++ << ". Position: " << std::endl <<
                        it->p.x() << std::endl <<
                        it->p.y() << std::endl <<
                        it->p.z());
        ROS_INFO_STREAM("Rotation: " << std::endl <<
                        it->M.GetRot().x() << std::endl <<
                        it->M.GetRot().y() << std::endl <<
                        it->M.GetRot().z() << std::endl <<
                        "=================================");
    }
}

AdvancedChainFkSolverPos_recursive::~AdvancedChainFkSolverPos_recursive()
{
}
