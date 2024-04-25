import motion_analysis
import motion_model
import motion_synthesis
import motion_sender
import motion_gui
import motion_control


import torch
from torch.utils.data import Dataset
from torch.utils.data import DataLoader
from torch import nn
from collections import OrderedDict
import networkx as nx
import scipy.linalg as sclinalg
from sklearn.manifold import TSNE
from sklearn.cluster import KMeans
from sklearn.cluster import MeanShift, estimate_bandwidth
from sklearn.cluster import DBSCAN

import os, sys, time, subprocess
import numpy as np
import math

from common import utils
from common import bvh_tools as bvh
from common import mocap_tools as mocap
from common.quaternion import qmul, qrot, qnormalize_np, slerp, qfix
#from common.quaternion_np import slerp
from common.pose_renderer import PoseRenderer


from matplotlib import pyplot as plt
import numpy as np

"""
Mocap Settings
"""

mocap_data_path = "../../data/mocap/Marcella_Dream.bvh"
mocap_valid_frame_range = [ 390, 30000 ]
#mocap_data_path = "../../data/mocap/Marcella_Isabella.bvh"
#mocap_valid_frame_range = [ 350, 29500 ]
#mocap_data_path = "../../data/mocap/Marcella_Nico.bvh"
#mocap_valid_frame_range = [ 300, 26450 ]
#mocap_data_path = "../../data/mocap/Marcella_Jazz.bvh"
#mocap_valid_frame_range = [ 700, 32400 ]

mocap_fps = 50
mocap_seq_window_length = 8
mocap_seq_window_overlap = 4

#mocap_seq_window_length = 50
#mocap_seq_window_overlap = 25

"""
Load Mocap Data
"""

bvh_tools = bvh.BVH_Tools()
mocap_tools = mocap.Mocap_Tools()

bvh_data = bvh_tools.load(mocap_data_path)
mocap_data = mocap_tools.bvh_to_mocap(bvh_data)
mocap_data["motion"]["rot_local"] = mocap_tools.euler_to_quat(mocap_data["motion"]["rot_local_euler"], mocap_data["rot_sequence"])
mocap_data["motion"]["pos_world"], mocap_data["motion"]["rot_world"] = mocap_tools.local_to_world(mocap_data["motion"]["rot_local"], mocap_data["motion"]["pos_local"], mocap_data["skeleton"])

joint_count = mocap_data["motion"]["rot_local"].shape[1]
joint_dim = mocap_data["motion"]["rot_local"].shape[2]
offsets = mocap_data["skeleton"]["offsets"].astype(np.float32)
parents = mocap_data["skeleton"]["parents"]
children = mocap_data["skeleton"]["children"]

joint_weights = np.array([1.0] * joint_count) # todo: get proper joint weights

"""
Mocap Analysis
"""

mocap_data["motion"]["pos_world_m"] = mocap_data["motion"]["pos_world"] / 100.0

mocap_data["motion"]["pos_world_smooth"] = motion_analysis.smooth(mocap_data["motion"]["pos_world_m"], 25)
mocap_data["motion"]["pos_scalar"] = motion_analysis.scalar(mocap_data["motion"]["pos_world_smooth"], "norm")
mocap_data["motion"]["vel_world"] = motion_analysis.derivative(mocap_data["motion"]["pos_world_smooth"], 1.0 / 50.0)
mocap_data["motion"]["vel_world_smooth"] = motion_analysis.smooth(mocap_data["motion"]["vel_world"], 25)
mocap_data["motion"]["vel_world_scalar"] = motion_analysis.scalar(mocap_data["motion"]["vel_world_smooth"], "norm")
mocap_data["motion"]["accel_world"] = motion_analysis.derivative(mocap_data["motion"]["vel_world_smooth"], 1.0 / 50.0)
mocap_data["motion"]["accel_world_smooth"] = motion_analysis.smooth(mocap_data["motion"]["accel_world"], 25)
mocap_data["motion"]["accel_world_scalar"] = motion_analysis.scalar(mocap_data["motion"]["accel_world_smooth"], "norm")
mocap_data["motion"]["jerk_world"] = motion_analysis.derivative(mocap_data["motion"]["accel_world_smooth"], 1.0 / 50.0)
mocap_data["motion"]["jerk_world_smooth"] = motion_analysis.smooth(mocap_data["motion"]["jerk_world"], 25)
mocap_data["motion"]["jerk_world_scalar"] = motion_analysis.scalar(mocap_data["motion"]["jerk_world_smooth"], "norm")
mocap_data["motion"]["qom"] = motion_analysis.quantity_of_motion(mocap_data["motion"]["vel_world_scalar"], joint_weights)
mocap_data["motion"]["bbox"] = motion_analysis.bounding_box(mocap_data["motion"]["pos_world_m"])
mocap_data["motion"]["bsphere"] = motion_analysis.bounding_sphere(mocap_data["motion"]["pos_world_m"])
mocap_data["motion"]["weight_effort"] = motion_analysis.weight_effort(mocap_data["motion"]["vel_world_scalar"], joint_weights, 25)
mocap_data["motion"]["space_effort"] = motion_analysis.space_effort_v2(mocap_data["motion"]["pos_world_m"], joint_weights, 25)
mocap_data["motion"]["time_effort"] = motion_analysis.time_effort(mocap_data["motion"]["accel_world_scalar"], joint_weights, 25)
mocap_data["motion"]["flow_effort"] = motion_analysis.flow_effort(mocap_data["motion"]["jerk_world_scalar"], joint_weights, 25)

"""
Load Model
"""

mocap_features = {"qom": mocap_data["motion"]["qom"],
                  "bsphere": mocap_data["motion"]["bsphere"], 
                  "weight_effort": mocap_data["motion"]["weight_effort"],
                  "space_effort": mocap_data["motion"]["space_effort"],
                  "time_effort": mocap_data["motion"]["time_effort"],
                  "flow_effort": mocap_data["motion"]["flow_effort"]}

motion_model.config = {
    "mocap_data": mocap_data["motion"]["rot_local"],
    "features_data": mocap_features,
    "mocap_window_length": mocap_seq_window_length,
    "mocap_window_offset": mocap_seq_window_overlap,
    "cluster_method": "kmeans",
    "cluster_count": 20,
    "cluster_random_state": 170
    }

clustering = motion_model.createModel(motion_model.config) 

"""
Setup Motion Synthesis
"""


motion_synthesis.config = {"skeleton": mocap_data["skeleton"],
          "model": clustering,
          "seq_window_length": mocap_seq_window_length,
          "seq_window_overlap": mocap_seq_window_overlap
          }

synthesis = motion_synthesis.MotionSynthesis(motion_synthesis.config)

"""
OSC Sender
"""

motion_sender.config["ip"] = "127.0.0.1"
motion_sender.config["port"] = 23456

osc_sender = motion_sender.OscSender(motion_sender.config)


"""
GUI
"""

from PyQt5 import QtWidgets
from PyQt5.QtCore import Qt
import pyqtgraph as pg
import pyqtgraph.opengl as gl
from pathlib import Path

motion_gui.config["synthesis"] = synthesis
motion_gui.config["sender"] = osc_sender

app = QtWidgets.QApplication(sys.argv)
gui = motion_gui.MotionGui(motion_gui.config)

# set close event
def closeEvent():
    QtWidgets.QApplication.quit()
app.lastWindowClosed.connect(closeEvent) # myExitHandler is a callable


"""
OSC Control
"""


motion_control.config["synthesis"] = synthesis
motion_control.config["model"] = clustering
motion_control.config["gui"] = gui
motion_control.config["ip"] = "127.0.0.1"
motion_control.config["port"] = 9002

osc_control = motion_control.MotionControl(motion_control.config)

"""
Start Application
"""

osc_control.start()
gui.show()
app.exec_()

osc_control.stop()
