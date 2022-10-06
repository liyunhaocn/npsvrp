
import psutil
import platform
import time
import os

import all_cmds

jobs = []

def run(arr, run=True):

    for i in range(len(arr)):
        arr[i] = f"nohup {arr[i]} &"

    for cmd in arr:
        print(cmd)
    if run == False:
        return 0

    i = 0
    while True:
        
        if i == len(arr):
            break
        # if i < 0:
        #     break
        t = time.localtime()
        # 获取当前时间和cpu的占有率
        cpu_time = '%d:%d:%d' % (t.tm_hour, t.tm_min, t.tm_sec)
        cpu_use = psutil.cpu_percent()

        if cpu_use > 70:
            print("n{0}".format(cpu_use), end=' ')
        else:
            print("y{0}".format(cpu_use))
            cmd = arr[i]
            print(cpu_use)
            ins = cmd.split("/instances/")[1].split(".txt")[0].replace("/", "")+".txt"
            cmd = cmd.replace("&", "> ./stdcerr/{0} 2>&1 &".format(ins))

            print("ins:", ins)
            print("sumit job ", i, cmd)

            os.system(cmd)
            i = i + 1

        time.sleep(1)

if __name__ == "__main__":

    if platform.system() == 'Linux':
        print("run job schedule on Linux")
        os.system("chmod 777 ./dev/SmartRouter")

    # jobs = all_cmds.get_all_static_my_cmds()
    jobs = all_cmds.get_all_dynamic_my_cmds("hgs") + all_cmds.get_all_static_my_cmds("hgs")
    # for job in jobs:
    #     print(job)
    run(jobs, True)
    print(len(jobs))

