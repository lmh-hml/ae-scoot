/*********************************************************************
 * Software License Agreement (BSD License)
 *
 *  ROS driver for Phidgets high speed encoder
 *  Copyright (c) 2010, Bob Mottram
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

#include <ros/ros.h>
#include <stdio.h>
#include <stdlib.h>
#include <sstream>
#include <std_msgs/String.h>
#include <phidgets_api/phidget.h>
#include "phidgets_motion_control/encoder_params.h"

// Handle
CPhidgetEncoderHandle phid;

// encoder state publisher
ros::Publisher encoder_pub_0;
ros::Publisher encoder_pub_1;

bool initialised = false;

int AttachHandler(CPhidgetHandle phid, void *userptr)
{
    int i, inputcount;
    int serial_number;
    const char *name;
    CPhidget_DeviceID deviceID;

    CPhidget_getDeviceID(phid, &deviceID);
    //CPhidget_getDeviceName (phid, &name);
    //CPhidget_getSerialNumber(phid, &serial_number);
    name="PhidgetEncoder";
    serial_number=341852;
    ROS_INFO("%s Serial number %d attached!", name, serial_number);

    CPhidgetEncoder_getEncoderCount((CPhidgetEncoderHandle)phid, &inputcount);

    //the 1047 requires enabling of the encoder inputs, so enable them if this is a 1047
    if (deviceID == PHIDID_ENCODER_HS_4ENCODER_4INPUT)
    {
            ROS_INFO("Encoder requires Enable. Enabling inputs....\n");
            for (i = 0 ; i < inputcount ; i++)
                    CPhidgetEncoder_setEnabled((CPhidgetEncoderHandle)phid, i, 1);
    }

    return 0;
}

int DetachHandler(CPhidgetHandle phid, void *userptr)
{
    int serial_number;
    const char *name;

    CPhidget_getDeviceName (phid, &name);
    CPhidget_getSerialNumber(phid, &serial_number);
    ROS_INFO("%s Serial number %d detached!",
			 name, serial_number);

    return 0;
}

int ErrorHandler(CPhidgetHandle phid, void *userptr,
				 int ErrorCode, const char *Description)
{
    ROS_INFO("Error handled. %d - %s", ErrorCode, Description);
    return 0;
}

int InputChangeHandler(CPhidgetEncoderHandle ENC,
					   void *usrptr, int Index, int State)
{
    ROS_INFO("Input %d value %d", Index, State);
    return 0;
}

int PositionChangeHandler(CPhidgetEncoderHandle ENC,
						  void *usrptr, int Index,
						  int Time, int RelativePosition)
{
    int Position;
    CPhidgetEncoder_getPosition(ENC, Index, &Position);

    phidgets_motion_control::encoder_params e;
    e.index = Index;
    e.count = Position;
    e.count_change = RelativePosition;
    e.time = Time;

      if (initialised){
        //Motor speed for steering
        if(Index==0){
          encoder_pub_0.publish(e);

        }
        else if(Index==1){  //Speed of wheel
          encoder_pub_1.publish(e);
        }
        ROS_INFO("Encoder %d Count %d", Index, Position);

      }
      //printf("Encoder #%i - Position: %5d -- Relative Change %2d -- Elapsed Time: %5d \n", Index, Position, RelativePosition, Time);

  return 0;
}

int display_properties(CPhidgetEncoderHandle phid)
{
    int serial_number, version, num_encoders, num_inputs;
    const char* ptr;

    CPhidget_getDeviceType((CPhidgetHandle)phid, &ptr);
    CPhidget_getSerialNumber((CPhidgetHandle)phid,
							 &serial_number);
    CPhidget_getDeviceVersion((CPhidgetHandle)phid, &version);

    CPhidgetEncoder_getInputCount(phid, &num_inputs);
    CPhidgetEncoder_getEncoderCount(phid, &num_encoders);

    ROS_INFO("%s", ptr);
    ROS_INFO("Serial Number: %d", serial_number);
    ROS_INFO("Version: %d", version);
    ROS_INFO("Number of encoders %d", num_encoders);

    return 0;
}

bool attach(
			CPhidgetEncoderHandle &phid,
			int serial_number)
{
  // create the object
  CPhidgetEncoder_create(&phid);

  CPhidget_set_OnAttach_Handler((CPhidgetHandle)phid, AttachHandler, NULL);
  CPhidget_set_OnDetach_Handler((CPhidgetHandle)phid, DetachHandler, NULL);
  CPhidget_set_OnError_Handler((CPhidgetHandle)phid, ErrorHandler, NULL);


  CPhidgetEncoder_set_OnInputChange_Handler(phid, InputChangeHandler, NULL);
  CPhidgetEncoder_set_OnPositionChange_Handler (phid, PositionChangeHandler, NULL);

  //open the device for connections
  CPhidget_open((CPhidgetHandle)phid, serial_number);

  // get the program to wait for an encoder device
// to be attached
  if (serial_number == -1) {
      ROS_INFO("Waiting for High Speed Encoder Phidget " \
			 "to be attached....");
  }
  else {
      ROS_INFO("Waiting for High Speed Encoder Phidget " \
			 "%d to be attached....", serial_number);
  }
  int result;
  if((result = CPhidget_waitForAttachment((CPhidgetHandle)phid,10000))) {
		const char *err;
		CPhidget_getErrorDescription(result, &err);
		ROS_ERROR("Problem waiting for attachment: %s", err);
		return false;
	}
  else return true;
}

/*!
 * \brief disconnect the encoder
 */
void disconnect(CPhidgetEncoderHandle &phid)
{
    ROS_INFO("Closing...");
    CPhidget_close((CPhidgetHandle)phid);
    CPhidget_delete((CPhidgetHandle)phid);
}

int main(int argc, char* argv[])
{
    ros::init(argc, argv, "phidgets_high_speed_encoder");
    ros::NodeHandle n;
    ros::NodeHandle nh("~");

    int serial_number = -1;
    nh.getParam("serial", serial_number);
    std::string name = "encoder";
    nh.getParam("name", name);
    if (serial_number==-1) {
        nh.getParam("serial_number", serial_number);
    }
    std::string topic_path = "phidgets/";
    nh.getParam("topic_path", topic_path);

    int frequency = 30;
    nh.getParam("frequency", frequency);

    if (attach(phid, serial_number)) {
		display_properties(phid);

        const int buffer_length = 100;

        std::string topic_steer_enc0 = topic_path + name + "/enc_0";
        std::string topic_steer_enc1 = topic_path + name+ "/enc_1";

        encoder_pub_0 = n.advertise<phidgets_motion_control::encoder_params>(topic_steer_enc0,
												  buffer_length);
        encoder_pub_1 = n.advertise<phidgets_motion_control::encoder_params>(topic_steer_enc1,
                  				buffer_length);
        initialised = true;

        ros::spin();

        disconnect(phid);
    }
    return 0;
}