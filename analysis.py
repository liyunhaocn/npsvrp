import csv
import collections


def get_csv_data(file_path):

    data = []
    with open(file_path) as csvfile:
        csv_reader = csv.reader(csvfile)  # 使用csv.reader读取csvfile中的文件
        # header = next(csv_reader)        # 读取第一行每一列的标题
        for row in csv_reader:  # 将csv 文件中的数据保存到data中
            data.append(row)  # 选择某一列加入到data数组中
    return data

if __name__ == "__main__":

    data_my = get_csv_data("./results/_Sep_28_2022_20_28_54.csv")
    data_hgs = get_csv_data("./results/_Sep_28_2022_23_33_24.csv")
    data_my = data_my[1:]
    data_hgs = data_hgs[1:]

    compdic = {}
    for row in data_hgs + data_my:
        if row[0] in compdic:
            compdic[row[0]].append(int(row[2]))
        else:
            compdic[row[0]] = [int(row[2])]

    for key, val in compdic.items():
        if len(val) == 2:
            print(key, val, (val[0]-val[1])/val[0]*100 )
        else:
            print(row[0])
    # for row in data:
    #     print(row)
    # for i in range(len(data)):
    #     for j in range(i+1, len(data)):
    #         if data[i][0] == data[j][0]:
    #             print(f"data[i]:{data[i]}")
    #             print(f"data[j]:{data[i]}")
    instance_names = [arr[0] for arr in data_my]
    # print(len(instance_names))
    # print(len(set(instance_names)))
    # print([item for item, count in collections.Counter(instance_names).items() if count > 1])
    ## [1, 2, 5]

    # static_sum_cost = 0
    # instance_num = 0
    # for row in data:
    #     instance_num += 1
    #     static_sum_cost += int(row[2])
    #     # print(f"int(row[2]):{int(row[2])}")
    # print(f"static_sum_cost:{static_sum_cost}")
    # print(f"instance_num:{instance_num}")

# static_sum_cost:40939732
# instance_num:249

# The instance ORTEC-VRPTW-ASYM-2e2ef021-d1-n210-k17.txt
# turns out to be infeasible for the static variant.
# You can still use the instance for the dynamic variant.
