
# n  is taken from the filename,
# which excludes (排除) the depot.
# Further, the ranges are [1,299], [300, 499], [500, 900].
# 3/5/8 minutes

import os
import glob

static_cmds = []

def get_tlim_of_static(path_path):
    # "ORTEC-VRPTW-ASYM-00c5356f-d1-n258-k12.txt"
    customers_num = path_path.split("-")[5].replace("n", "")
    # print(f"num:{customers_num}")
    customers_num = int(customers_num)

    # print(customers_num)
    time_min = 0
    if 1 <= customers_num <= 299:
        time_min = 3
    elif 300 <= customers_num <= 499:
        time_min = 5
    elif 500 <= customers_num <= 900:
        time_min = 8

    # 1807 /1981
    return int(time_min * 60 * 1807 / 1981)

def get_all_instances_paths():

    path = r"./instances/" #文件夹目录
    ret = []
    for file_path in glob.glob(path + r"*.txt"):
        file_path = file_path.replace("\\", "/")
        ret.append(file_path)
    return ret

def get_all_static_nps_cmds():

    all_paths = get_all_instances_paths()
    cmds = []
    for instance_path in all_paths:
        cmds.append(f"python controller.py --instance {instance_path} "
                    + f"--static --epoch_tlim {get_tlim_of_static(instance_path)} -- ./run.sh")
    # for cmd in cmds:
    #     print(cmd)
    return cmds

def get_all_static_my_cmds():

    all_paths = get_all_instances_paths()
    cmds = []
    for instance_path in all_paths:
        cmds.append(f"./dev/SmartRouter {instance_path} -isNpsRun 0 -t {get_tlim_of_static(instance_path)} "
                    + f"-seed 1 -veh -1 -useWallClockTime 1  -isNpsRun 0")
    # for cmd in cmds:
    #     print(cmd)
    return cmds

if __name__ == "__main__":
    pass


