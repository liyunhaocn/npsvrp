# n  is taken from the filename,
# which excludes (排除) the depot.
# Further, the ranges are [1,299], [300, 499], [500, 900].
# 3/5/8 minutes

import os
import glob

import numpy as np

static_cmds = []


def get_tlim_of_static(path_path):
    # return 5
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


def get_tlim_of_dynamic():
    return 100
    # return int(60 * 1807 / 1981)


def get_all_instances_paths():
    path = r"./instances/"  # 文件夹目录
    ret = []
    for file_path in glob.glob(path + r"*.txt"):
        file_path = file_path.replace("\\", "/")
        ret.append(file_path)
    print(f"ret:{ret}")
    return ret


def get_all_instances_names():
    all_paths = get_all_instances_paths()
    return [x.replace("./instances/", "") for x in all_paths]


def get_all_static_nps_cmds():
    all_paths = get_all_instances_paths()
    cmds = []
    for instance_path in all_paths:
        cmds.append(f"python controller.py --instance {instance_path} "
                    + f"--static --epoch_tlim {get_tlim_of_static(instance_path)} -- ./run.sh")
    # for cmd in cmds:
    #     print(cmd)
    return cmds


def get_select_10_instances_path():
    arr = [
        "./instances/ORTEC-VRPTW-ASYM-16b82253-d1-n457-k30.txt",
        "./instances/ORTEC-VRPTW-ASYM-2e0a30ff-d1-n491-k36.txt",
        "./instances/ORTEC-VRPTW-ASYM-3709cf41-d1-n287-k19.txt",
        "./instances/ORTEC-VRPTW-ASYM-798171b0-d1-n273-k21.txt",
        "./instances/ORTEC-VRPTW-ASYM-937b2edc-d1-n234-k15.txt",
        "./instances/ORTEC-VRPTW-ASYM-93ee144d-d1-n688-k38.txt",
        "./instances/ORTEC-VRPTW-ASYM-b384a276-d1-n219-k17.txt",
        "./instances/ORTEC-VRPTW-ASYM-b7ae3aa2-d1-n304-k23.txt",
        "./instances/ORTEC-VRPTW-ASYM-e159d033-d1-n363-k27.txt",
        "./instances/ORTEC-VRPTW-ASYM-e92a0b44-d1-n501-k33.txt",
    ]
    return np.array(arr)


def get_all_static_my_cmds(opt_str="#"):
    all_paths = get_all_instances_paths()
    all_paths = get_select_10_instances_path()
    cmds = []
    for instance_path in all_paths:
        cmds.append(f"python solver.py --instance {instance_path} "
                    + opt_str + " "
                    + f"--static --epoch_tlim {get_tlim_of_static(instance_path)} "
                    + "--verbose")
    return cmds


def get_all_dynamic_my_cmds(tag="notag"):
    all_paths = get_all_instances_paths()
    cmds = []
    for instance_path in all_paths:
        cmds.append(f"python solver.py --instance {instance_path} "
                    + f"--epoch_tlim {get_tlim_of_dynamic()} --run_tag {tag} --verbose")
        # cmds.append(f"./dev/SmartRouter {instance_path} -isNpsRun 0 -t {get_tlim_of_static(instance_path)} "
        #             + f"-seed 1 -veh -1 -useWallClockTime 1  -isNpsRun 0")
    return cmds


if __name__ == "__main__":
    pass
