import csv


def get_csv_data(file_path):

    data = []
    with open(file_path) as csvfile:
        csv_reader = csv.reader(csvfile)  # 使用csv.reader读取csvfile中的文件
        # header = next(csv_reader)        # 读取第一行每一列的标题
        for row in csv_reader:  # 将csv 文件中的数据保存到data中
            data.append(row)  # 选择某一列加入到data数组中
    return data

if __name__ == "__main__":

    file_path = './results/_Sep_28_2022_20_28_54.csv'
    data = get_csv_data(file_path)
    data = data[1:]
    instance_names = [arr[0] for arr in data]

    # for row in data:
    #     print(row)
    # for i in range(len(data)):
    #     for j in range(i+1, len(data)):
    #         if data[i][0] == data[j][0]:
    #             print(f"data[i]:{data[i]}")
    #             print(f"data[j]:{data[i]}")
    print(len(instance_names))
    print(len(set(instance_names)))
    static_sum_cost = 0
    instance_num = 0
    for row in data:
        instance_num += 1
        static_sum_cost += int(row[2])
        print(f"int(row[2]):{int(row[2])}")
    print(f"static_sum_cost:{static_sum_cost}")
    print(f"instance_num:{instance_num}")
