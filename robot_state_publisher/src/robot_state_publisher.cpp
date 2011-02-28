/*********************************************************************
* Software License Agreement (BSD License)
* 
*  Copyright (c) 2008, Willow Garage, Inc.
*  All rights reserved.
* 
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
* 
*   * Redistributions of source code must retain the above copyright
*     notice, this list of conditions and the following disclaimer.
*   * Redistributions in binary form must reproduce the above
*     copyright notice, this list of conditions and the following
*     disclaimer in the documentation and/or other materials provided
*     with the distribution.
*   * Neither the name of the Willow Garage nor the names of its
*     contributors may be used to endorse or promote products derived
*     from this software without specific prior written permission.
* 
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
*  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
*  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
*  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
*  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
*  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
*  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
*  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
*  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
*  POSSIBILITY OF SUCH DAMAGE.
*********************************************************************/

/* Author: Wim Meeussen */

#include "robot_state_publisher/robot_state_publisher.h"
#include <kdl/frames_io.hpp>
#include <tf_conversions/tf_kdl.h>

using namespace std;
using namespace ros;
using namespace KDL;



namespace robot_state_publisher{

RobotStatePublisher::RobotStatePublisher(const Tree& tree)
   :tree_(tree)
{
  // build tree solver
  solver_.reset(new TreeFkSolverPosFull_recursive(tree_));

  // get parameter to flatter tree or not
  ros::NodeHandle n_private("~");
  n_private.param("flatten_tree", flatten_tree_, false);

  // advertise tf message
  NodeHandle n;
  tf_publisher_ = n.advertise<tf::tfMessage>("/tf", 5);

  // get the root segment of the tree
  SegmentMap::const_iterator root = tree.getRootSegment();
  root_ = root->first;
}



bool RobotStatePublisher::publishTransforms(const map<string, double>& joint_positions, const Time& time)
{
  // calculate transforms 
  map<string, tf::Stamped<Frame> > link_poses;
  solver_->JntToCart(joint_positions, link_poses, flatten_tree_);

  if (link_poses.empty()){
    ROS_ERROR("Could not compute link poses. The tree or the state is invalid.");
    return false;
  }
  transforms_.resize(link_poses.size());

  unsigned int i = 0;
  tf::Transform tf_frame;
  for (map<string, tf::Stamped<Frame> >::const_iterator f=link_poses.begin(); f!=link_poses.end(); f++){
    tf::TransformKDLToTF(f->second, transforms_[i]);
    transforms_[i].stamp_ = time;
    transforms_[i].frame_id_ = f->second.frame_id_;
    transforms_[i].child_frame_id_ = f->first;
    i++;
  }

  tf_broadcaster_.sendTransform(transforms_);

  return true;
}
}


