import math
import os.path
#from split_instances import resd_csv_col
import numpy as np
import tools

class Point():
    def __init__(self,x,y):
        self.x = x
        self.y = y


def GetAreaOfPolyGonbyVector(points):

    area = 0
    if(len(points)<3):

         raise Exception("error")

    for i in range(0,len(points)-1):
        p1 = points[i]
        p2 = points[i + 1]

        triArea = (p1.x*p2.y - p2.x*p1.y)/2
        area += triArea
    return abs(area)

def cul_area(x, y):

    points = []

    # x = [1,2,3,4,5,6,5,4,3,2]
    # y = [1,2,2,3,3,3,2,1,1,1]
    for index in range(len(x)):
        points.append(Point(x[index],y[index]))

    area = GetAreaOfPolyGonbyVector(points)
    return area
    # print(area)
    # print(math.ceil(area))
    # assert math.ceil(area)==1

def cul_m_farthest_k_nearest_dist(epoch_instance, m_farthest=10,k_nearest=10):
    # ins_dir = 'instances'
    # ins_name = 'ORTEC-VRPTW-ASYM-1cd538a9-d1-n400-k25.txt'
    # ins_name = os.path.join(ins_dir, ins_name)
    # instance = tools.read_vrplib(ins_name)
    # # epoch_instance = instance_observation['epoch_instance']
    # epoch_instance = instance
    # Compute some properties used for state representation
    d = epoch_instance['duration_matrix']
    # Add large duration to self
    d = d + int(1e9) * np.eye(d.shape[0])
    # Find k nearest
    ind_nearest = np.argpartition(d, min(k_nearest, d.shape[-1]) - 1, -1)[:, :k_nearest]
    # # Sort by duration
    row_ind = np.arange(d.shape[0])[:, None]
    # ind_nearest_sorted = ind_nearest[row_ind, np.argsort(d[row_ind, ind_nearest], -1)]
    # dist_to_nearest = d[row_ind, ind_nearest_sorted]
    # dist_mean_sorted = np.average(dist_to_nearest, -1)
    dist_mean = np.average(d[row_ind, ind_nearest], -1)
    # Find index of m farthest avg(dist of k nearest)
    # m_farthest = 10  # note: farthest
    ind_m_nearest = np.argpartition(dist_mean, max(-m_farthest, -(dist_mean.shape[-1]) - 1))[-m_farthest:]
    # print(ind_m_nearest)
    # # Find values of m farthest avg
    # val_m_nearest = dist_mean[ind_m_nearest]
    # print(val_m_nearest)
    return ind_m_nearest

def cul_avg_dist_depot2node(epoch_instance):
    return np.mean(epoch_instance['duration_matrix'][0, :])

def cul_max_dist_node2node(epoch_instance):
    return np.max(epoch_instance['duration_matrix'])

def filter_epoch_instance(observation, mask: np.ndarray):
    res = {}

    for key, value in observation.items():
        if key == 'capacity':
            res[key] = value
            continue

        if key == 'duration_matrix':
            res[key] = value[mask]
            res[key] = res[key][:, mask]
            continue

        res[key] = value[mask]

    return res

def delete_farthest_nodes(epoch_instance, ind_nodes):
    mask = np.ones_like(epoch_instance['is_depot'])
    mask = np.array(mask, dtype=np.bool8)
    for i in ind_nodes:
        mask[i] = False
    return filter_epoch_instance(epoch_instance, mask)

def cul_area_xy(x,y):
    min_x = min(x)
    min_y = min(y)
    max_x = max(x)
    max_y = max(y)
    area0 = (max_y - min_y) * (max_x - min_x)
    min_x1 = 1000000
    min_y1 = 1000000
    max_x1 = -1000000
    max_y1 = -1000000
    for i in range(len(x)):
        min_x1 = min(y[i]-x[i], min_x1)
        max_x1 = max(y[i]-x[i], max_x1)
        min_y1 = min(y[i]+x[i], min_y1)
        max_y1 = max(y[i]+x[i], max_y1)
    area1 = (max_x1-min_x1)*(max_y1-min_y1)/2
    return min(area0,area0)

def ratio_xy(x,y):
    min_x = min(x)
    min_y = min(y)
    max_x = max(x)
    max_y = max(y)
    delta_x = (max_x - min_x)
    delta_y = (max_y - min_y)
    ratio = max(delta_x,delta_y) / min(delta_x,delta_y)
    min_x1 = 1000000
    min_y1 = 1000000
    max_x1 = -1000000
    max_y1 = -1000000
    for i in range(len(x)):
        min_x1 = min(y[i]-x[i], min_x1)
        max_x1 = max(y[i]-x[i], max_x1)
        min_y1 = min(y[i]+x[i], min_y1)
        max_y1 = max(y[i]+x[i], max_y1)
    delta_x1 = (max_x1-min_x1)
    delta_y1 = (max_y1-min_y1)
    ratio1 = max(delta_x1,delta_y1) / min(delta_x1,delta_y1)
    return max(ratio,ratio1)

def get_classes(intance):
    m = cul_m_farthest_k_nearest_dist(intance, m_farthest=int(intance['coords'].shape[0] * 0.05))
    processed_int = delete_farthest_nodes(intance, m)
    max_dist_node2node = cul_max_dist_node2node(processed_int)
    # Use origin instance
    mean_dist_depot2node = cul_avg_dist_depot2node(intance)
    # max_area = max_dist_node2node * max_dist_node2node
    bin_data_0 = processed_int['coords'][:, 0]
    bin_data_1 = processed_int['coords'][:, 1]
    area_xy = cul_area_xy(bin_data_0, bin_data_1)
    # area_real = area_tool.cul_area(bin_data_0, bin_data_1)
    # ratio = max_area / area_xy
    # sp_ratio = pow(ratio, 0.5)
    ratioxy = ratio_xy(bin_data_0, bin_data_1)
    dis_ratio = max_dist_node2node/mean_dist_depot2node
    return area_xy, ratioxy, dis_ratio


if __name__ == '__main__':
    # cul_area()
    ins_dir = 'instances'
    ins_name = 'ORTEC-VRPTW-ASYM-1cd538a9-d1-n400-k25.txt'
    ins_name = os.path.join(ins_dir, ins_name)
    instance = tools.read_vrplib(ins_name)
    # epoch_instance = instance_observation['epoch_instance']
    epoch_instance = instance
    m = cul_m_farthest_k_nearest_dist(epoch_instance)
    processed_epoch_int = delete_farthest_nodes(epoch_instance, m)


    print("OK")
