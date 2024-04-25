import numpy as np

from common import utils
from common import bvh_tools as bvh
from common import mocap_tools as mocap

config = { 
    "file_name": "data/mocap/accumulation_fullbody_take1.bvh",
    "fps": 50,
    "hip_joint_index": 0,
    "remove_hip_offset": True,
    "joint_pos_scale": 1.2
    }

class MotionPlayer():
    def __init__(self, config):

        self.file_name = config["file_name"]
        
        self.load(self.file_name)
        
    def load(self, file_name):
        
        # load mocap data
        bvh_tools = bvh.BVH_Tools()
        mocap_tools = mocap.Mocap_Tools()

        bvh_data = bvh_tools.load(file_name)
        self.mocap_data = mocap_tools.bvh_to_mocap(bvh_data)
        
        self.fps = self.mocap_data["frame_rate"]
        
        rot_sequence = self.mocap_data["rot_sequence"]
        offsets = self.mocap_data["skeleton"]["offsets"]
        pos_local = self.mocap_data["motion"]["pos_local"]
        rot_local_euler = self.mocap_data["motion"]["rot_local_euler"]

        # convert from left handed bvh coordinate system to right handed standard coordinate system
        rot_sequence_2 = [1, 0, 2]

        offsets_2 = np.stack(np.stack([offsets[:, 0], -offsets[:, 2], offsets[:, 1]]), axis=1) 
        pos_local_2 = np.stack(np.stack([pos_local[:, :, 0], -pos_local[:, :, 2], pos_local[:, :, 1]]), axis=2) 
        rot_local_euler_2 = np.stack(np.stack([rot_local_euler[:, :, 0], -rot_local_euler[:, :, 2], rot_local_euler[:, :, 1]]), axis=2) 


        self.mocap_data["rot_sequence"] = rot_sequence_2
        self.mocap_data["skeleton"]["offsets"] = offsets_2
        self.mocap_data["motion"]["pos_local"] = pos_local_2
        self.mocap_data["motion"]["rot_local_euler"] = rot_local_euler_2

        # compute local joint rotations and world joint positions
        self.mocap_data["motion"]["rot_local"] = mocap_tools.euler_to_quat(self.mocap_data["motion"]["rot_local_euler"] , self.mocap_data["rot_sequence"])
        
        # compute world positions scaled to compensate for different dancers' heights and with hip position subtracted
        pos_world, rot_world  = mocap_tools.local_to_world(self.mocap_data["motion"]["rot_local"], self.mocap_data["motion"]["pos_local"], self.mocap_data["skeleton"])
        
        self.mocap_data["motion"]["rot_world"] = rot_world
 
        # remove hip offset
        if config["remove_hip_offset"] == True:
            hip_pos_world = pos_world[:, config["hip_joint_index"], :]
            hip_pos_world = np.expand_dims(hip_pos_world,axis=1)
            hip_pos_world = np.repeat(hip_pos_world, pos_world.shape[1], axis=1)
            pos_world = pos_world - hip_pos_world


        # swap coordinates
        pos_world_x = np.copy(pos_world[:,:,0])
        pos_world_y = np.copy(pos_world[:,:,1])
        pos_world_z = np.copy(pos_world[:,:,2])

        pos_world[:,:,0] = pos_world_x
        pos_world[:,:,1] = pos_world_z
        pos_world[:,:,2] = pos_world_y
            
        pos_world *= config["joint_pos_scale"]
            
        self.mocap_data["motion"]["pos_world"] = pos_world
        
 
        # update start, end, and play position
        self.play_frame = 0
        self.start_play_frame = 0
        self.end_play_frame = self.mocap_data["motion"]["pos_world"].shape[0] - 1

    def update(self):

        if self.play_frame >= self.end_play_frame:
            self.play_frame = self.start_play_frame
        
        self.play_frame += 1
        
    def get_file_name(self):
        return self.file_name
        
    def get_fps(self):
        return self.fps
    
    def set_fps(self, fps):
        self.fps = fps
        
    def get_play_frame(self):
        return self.play_frame
        
    def set_play_frame(self, frame):

        if frame >= self.end_play_frame:
            frame = self.end_play_frame - 1
            
        if frame <= self.start_play_frame:
             frame = self.start_play_frame + 1
             
        self.play_frame = frame
        
    def get_start_play_frame(self):
        return self.start_play_frame
        
    def set_start_play_frame(self, frame):
        
        if frame >= self.end_play_frame:
            frame = self.end_play_frame - 1

        self.start_play_frame = frame
        self.play_frame  = self.start_play_frame
    
    def get_end_play_frame(self):
        return self.end_play_frame
        
    def set_end_play_frame(self, frame):
        
        if frame <= self.start_play_frame:
             frame = self.start_play_frame + 1

        self.end_play_frame = frame
        self.play_frame  = self.end_play_frame

    def get_skeleton(self):
        return self.mocap_data["skeleton"]

    def get_pose(self, pose_feature):
        
        return self.mocap_data["motion"][pose_feature][self.play_frame, ...]
