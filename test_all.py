
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
    if 1 <= customers_num <= 299:
        return 3
    elif 300 <= customers_num <= 499:
        return 5
    elif 500 <= customers_num <= 900:
        return 8
    return 0

def get_all_instances_paths():

    path = r"./instances/" #文件夹目录
    ret = []
    for file_path in glob.glob(path + r"*.txt"):
        file_path = file_path.replace("\\", "/")
        ret.append(file_path)
    return ret

def get_all_static_cmds():

    all_paths = get_all_instances_paths()
    cmds = []
    for instance_path in all_paths:
        cmds.append(f"python controller.py --instance {instance_path} "
                    + f"--static --epoch_tlim {get_tlim_of_static(instance_path)} -- ./run.sh")
    # for cmd in cmds:
    #     print(cmd)
    return cmds

if __name__ == "__main__":
    pass


