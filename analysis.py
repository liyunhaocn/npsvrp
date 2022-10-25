import csv
import os

import numpy as np
import collections
import all_cmds
from operator import itemgetter

def get_csv_data(file_path):

    data = []
    with open(file_path) as csvfile:
        csv_reader = csv.reader(csvfile)  # 使用csv.reader读取csvfile中的文件
        # header = next(csv_reader)        # 读取第一行每一列的标题
        for row in csv_reader:  # 将csv 文件中的数据保存到data中
            # print(f"row:{len(row)}")
            data.append(row)  # 选择某一列加入到data数组中
    return data

def get_diff_of_two_runs(path_a,path_b):
    data_a_instance_name = np.array(get_csv_data(path_a))[:, 0:1]
    data_b_instance_name = np.array(get_csv_data(path_b))[:, 0:1]
    ret = []
    for name in data_a_instance_name:
        if name not in data_b_instance_name:
            ret.append(name)

    for name in data_b_instance_name:
        if name not in data_a_instance_name:
            ret.append(name)
    return ret

def get_more_than_one_times_in_one_run(inst_path):

    data_instance_name = np.array(get_csv_data(inst_path), dtype=object)[:, 0:1]

    arr = []
    ret = []
    for i in data_instance_name:
        if i in arr:
            ret.append(i)
        arr.append(i)
    return ret


def write_csv_by_over_write(path, data):

    with open(path, 'w+') as f:
        csv_write = csv.writer(f)
        for i in data:
            csv_write.writerow(i)
def sort_csv(csv_path):
    data = get_csv_data(csv_path)
    data_0 = data[0]
    data = data[1:]
    data.sort(key=lambda x: x[0])
    # for i in data:
    #     i.insert(1, get_customers_num_from_instance_name(i[0]))
    data = [data_0] + data
    write_csv_by_over_write(csv_path, data)

def get_customers_num_from_instance_name(instance_name):
    return instance_name.split("-")[5].replace("n", "")


if __name__ == "__main__":

    # for root, dirs, files in os.walk(r"results/"):
    #     print(files)
    #     for file in files:
    #         print(r"results/" + file)
    #         sort_csv(r"results/" + file)
    # exit(0)

    sort_csv(r"results/[10_25][static][HGSDefault].csv")
    exit(0)

    # get_more_than_one_times_in_one_run(r"results/[10_08][dynamic][greedy][smartonly].csv")
    # exit(0)

    # more_one = get_more_than_one_times_in_one_run("results/[10_08][dynamic][greedy][smartonly].csv")
    # print(f"one_more:{more_one}")
    # exit(0)

    diff = get_diff_of_two_runs(r"results/[10_15][dynamic][greedy][onlyruin].csv",
                                r"results/[10_04][dynamic][greedy][notag].csv")
    print(diff)
    exit(0)

# The instance ORTEC-VRPTW-ASYM-2e2ef021-d1-n210-k17.txt
# turns out to be infeasible for the static variant.
# You can still use the instance for the dynamic variant.
