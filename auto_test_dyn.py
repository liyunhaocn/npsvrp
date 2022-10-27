import subprocess
import argparse
from time import sleep
import tools
import json
import copy
import sys
import numpy as np
from environment import VRPEnvironment
import os
import re
import random
import threading
from random import randrange
import csv
from concurrent.futures import ThreadPoolExecutor, wait, ALL_COMPLETED, FIRST_COMPLETED


def write_csv(name,data, write_row = True):
    # open the file in the write mode
    with open(name, 'a',encoding='UTF8', newline='') as f:
        # create the csv writer
        writer = csv.writer(f)

        if write_row:
            # write a row to the csv file
            writer.writerow(data)
        else:
            writer.writerows(data)

def file_name(file_dir):
    for root, dirs, files in os.walk(file_dir):
        break
        # 当前目录路径
        #print(root)
        # 当前路径下所有子目录
        #print(dirs)
        # 当前路径下所有非目录子文件
        #print(files)
    return root, dirs, files


cmd_test_g = []
cmd_test_g.append('C:/Users/zhangjunjie/Desktop/ZOOM/VRPTW/venv/Scripts/python')
cmd_test_g.append('solver.py')
# cmd_test_g.append('solver.py')  # 0928
cmd_test_g.append('--epoch_tlim')
cmd_test_g.append('100')
cmd_test_g.append('--run_tag')
cmd_test_g.append('v1_only_delta')
# cmd_test_g.append('--verbose')
# cmd_test_g.append('--instance')
# cmd_test_g.append('instances/ORTEC-VRPTW-ASYM-00c5356f-d1-n258-k12.txt')
def test(instance,rand_num,or_gap,x1,x2,x3,early_time1,early_time2,gap,sol_x1,sol_x2,sol_x3):
    sleep(3)
    return random.randint(10,666)
def mult_process(instance,rand_num,or_gap,x1,x2,x3,early_time1,early_time2,gap,sol_x1,sol_x2,sol_x3):
    cmd_test = copy.copy(cmd_test_g)
    cmd_test.append('--instance')
    cmd_test.append(instance)
    cmd_test.append('--instance_seed')
    cmd_test.append('1')
    cmd_test.append('--solver_seed')
    cmd_test.append('1')

    # cmd_test.append('--rand_num')
    # cmd_test.append(str(rand_num))
    # cmd_test.append('--or_gap')
    # cmd_test.append(str(or_gap))
    # cmd_test.append('--x1')
    # cmd_test.append(str(x1))
    # cmd_test.append('--x2')
    # cmd_test.append(str(x2))
    # cmd_test.append('--x3')
    # cmd_test.append(str(x3))
    # cmd_test.append('--early_time1')
    # cmd_test.append(str(early_time1))
    # cmd_test.append('--early_time2')
    # cmd_test.append(str(early_time2))
    # cmd_test.append('--gap')
    # cmd_test.append(str(gap))
    # cmd_test.append('--sol_x1')
    # cmd_test.append(str(sol_x1))
    # cmd_test.append('--sol_x2')
    # cmd_test.append(str(sol_x2))
    # cmd_test.append('--sol_x3')
    # cmd_test.append(str(sol_x3))

    with subprocess.Popen(cmd_test, stdin=subprocess.PIPE, stdout=subprocess.PIPE, text=True) as p:
        # Set global timeout, bit ugly but this will also interrupt waiting for input

        for line in p.stdout:
            line = line.strip()
            # request = json.loads(line)
            #
            # if line.startswith('zjj_re'):
            #     request = float(request.split(',')[1])
            #     date_dir = './results'
            #     file = 'zjj_wxy.csv'
            #     file = os.path.join(date_dir,file)
            #     data = [instance.split('/')[1], -request]
            #     threadlock.acquire()
            #     write_csv(file, data)
            #     threadlock.release()
            #     return -request

threadlock = threading.Lock()
threadlocks = []

def geatpy_bench(rand_num,or_gap,x1,x2,x3,early_time1,early_time2,gap,sol_x1,sol_x2,sol_x3):
    random.seed(66)
    thread_list = []
    # ###############=======================================##################
    instances_filters = "instances_bench_old"
    instances_filters = "instances_bench_7"
    instances_filters = "instances_bench_8"
    instances_filters = 'instances_classes_info'
    instances_class = '0_0_0'
    # instances_filters = os.path.join(instances_filters, instances_class)
    instances_filters = os.path.join(instances_filters,instances_class,"all")
    # instances_filters = "instances_bench_60"
    instances_filters = "instances"
    root, dirs, files = file_name(instances_filters)
    # files = ['ORTEC-VRPTW-ASYM-b7ae3aa2-d1-n304-k23.txt',
    #          'ORTEC-VRPTW-ASYM-f77bf975-d1-n342-k25.txt',
    #          'ORTEC-VRPTW-ASYM-6f54fdb4-d1-n392-k25.txt'
    # ]
    instances_names = [instances_filters+'/'+i for i in files]
    # print(instances_names)

    rand_num = int(rand_num)
    or_gap = float(or_gap)
    x1 = float(x1)
    x2 = float(x2)
    x3 = float(x3)
    early_time1 = int(early_time1)
    early_time2 = int(early_time2)
    gap = float(gap)
    sol_x1 = float(sol_x1)
    sol_x2 = float(sol_x2)
    sol_x3 = float(sol_x3)
    pool = ThreadPoolExecutor(max_workers=20)
    max_time = 10
    for i in instances_names:
        #rand_num = 100 
        #or_gap = 1000
        # x1 = 0
        # x2 = 0
        # x3 = 0
        # early_time1 = 0
        # early_time2 = 0
        # gap = 600
        # sol_x1 = 0
        # sol_x2 = 0
        # sol_x3 = 0
        #mult_process(i,rand_num,or_gap,x1,x2,x3,early_time1,early_time2,gap,sol_x1,sol_x2,sol_x3)
        thread_list.append(pool.submit(mult_process, i,rand_num,or_gap,x1,x2,x3,early_time1,early_time2,gap,sol_x1,sol_x2,sol_x3))
    wait(thread_list, return_when=ALL_COMPLETED)
    result_all = 0
    # for i in range(len(instances_names)):
    #     r = -111
    #     # r = thread_list[i].result()
    #     if r:
    #         result_all += thread_list[i].result()
    #     else:
    #         result_all += 500000
    #     # print('thread ' + str(i) + ' = ' + str(thread_list[i].result()))
    #     print(str(thread_list[i].result()))
    #print(thread_list[0].result(),"++++",thread_list[1].result())
    pool.shutdown()
    print("========================"+str(result_all)+"==========================")
    return result_all

if __name__ == '__main__':

    PRAMS = '50	2000	6.73	8.25	1.25	2600	-2600	1700	0	0.5	2.5'  # res = 1380000
    PRAMS = '60	3	3.5	1.14	-0.7	2600	-2600	2	3.5	1.14	-0.7'  # res = 1380000
    PRAMS = '60	100	3.5	1.14	-0.7	-26000	-26000	100	3.5	1.14	-0.7'  # res = 1393235  -- 2749554
    # PRAMS = '60	100	3.5	1.14	-0.7	2600	-26000	100	3.5	1.14	-0.7'  # res = 1380404  -- 2734823
    # PRAMS = '50	0.65	3.5	1.14	-0.7	2600	-2600	0.8	3.5	1.14	-0.7'  # res = 1378500  -- 2713736(repeat 1) 2722842(repeat 3) 2720575(rand 60)
    # PRAMS = '100	0.65	3.5	1.14	-0.7	2600	-2600	3	3.5	1.14	-0.7'  # res = ??  -- 2725014
    # solver_copy_submission.py
    PRAMS = '60	0.65	3.5	1.14	-0.7	2600	-2600	0.65	4.5	3.14	-0.7'  # res = 1377268(delta: avg(repeat 5)) -- 2720578(repeat 1) 2728962(repeat 3) ==>2711912
    PRAMS = '100	0.4	0	3	2	2600	-2600	100	4.5	3.14	-0.7'

    # PRAMS = '100	0.5 0.085	8.01	-3.63	2600	-2600	0.9	0.085	8.01	-3.63'  # good
    # PRAMS = '100	0.31    1.877	3.45	2.8	2600	-2600	0.5	1.877	3.45	2.8'
    # PRAMS = '100	1.74    -4.47	-6.6	0.122	2600	-2600	0.45	-4.47	-6.6	0.122'

    # 1011 dont use ortools and backward to old version
    PRAMS = '50	0    0	0	0	-26000	-26000	0	0	0	0'  # don't use reset_rand_num()
    PRAMS = '50	670    1.177	0	-0.8	-26000	-26000	0.0222	1.177	0	-0.8'  # 1012参数
    PRAMS = '150	600    0	3	0	-26000	-26000	0	0	3	0'  # 0928参数
    PRAMS = '150	0    0	2	0.15	-26000	-26000	0	0	3	0.15'  # 1015 or-tool删点+vrp调优
    PRAMS = '150	500 -10    10	-0.74	-26000	-26000	300	0	3	0.15'  # best prams for most instances 1_1_1 (&largest ins) and 1_0_1 :param1
    # PRAMS = '150	970 -14    9.88	-0.34	-26000	-26000	632	0	3	0.15'  # best prams2
    PRAMS = '150	101    -4.68	9.64	-0.6	-26000	-26000	550	0	3	0.15'  # for 0_0_0 and its remain_ins :prams3
    PRAMS = '150	94    -7.018	14.887	-2.537	-26000	-26000	214	0	3	0.15'  # for 0_1_0 :prams4
    PRAMS = '150	94    -7.018	14.887	-2.537	2600	2600	214	0	3	0.15'  # for 0_1_0 :prams5
    # PRAMS = '150	100    -4.68	9.64	-0.6	3600	3600	550	0	3	0.15'  # best :prams6
    # PRAMS = '150	-139    9.35	6.8	-0.73	2600	2600	573	0	3	0.15'
    PRAMS = '901	94    -7.018	14.887	-2.537	2600	2600	214	0	3	0.15'  # good for 1_1_1 and its remain_ins :prams7 == best_1020
    # PRAMS = '150	101    -4.68	9.64	-0.6	2600	2600	550	0	3	0.15'  # for 0_0_0 and its remain_ins :prams8
    # PRAMS = '150	1000    -19	8	1.34	2600	2600	560	0	3	0.15'
    # PRAMS = '150	177    -16.5	18.2	-0.17	-2000	2600	116	0	3	0.15'
    # PRAMS = '150	101    -4.68	9.64	-0.6	-3000	2400	550	0	3	0.15'  #
    # PRAMS = '150	200    -12	19.65	-1.28	1200	-2000	160	0	3	0.15'
    # PRAMS = '150	105    -4.68	9.64	-0.6	1200	-200	550	0	3	0.15'  # for 0_0_0 and its remain_ins :prams9
    # PRAMS = '150	105    -4.68	9.64	-0.6	1200	-2000	550	0	3	0.15'  # for 0_0_0 and its remain_ins :prams10
    # PRAMS = '150	-530    -11.4	15.4	3	3000	-3600	580	0	3	0.15'  # for 0_0_0 and its remain_ins :prams11
    PRAMS = '901	94    -7.018	14.887	-2.537	1200	-2000	214	0	3	0.15'  # good for 1_1_1 and its remain_ins :prams12
    PRAMS = '150	940    -13	9	-2.37	2100	-2700	460	0	3	0.15'  # prams13
    PRAMS = '150	-26    -6	20	-3.74	3600	-720	157	0	3	0.15'  # prams14

    PRAMS = PRAMS.split()
    geatpy_bench(PRAMS[0], PRAMS[1], PRAMS[2], PRAMS[3], PRAMS[4], PRAMS[5], PRAMS[6], PRAMS[7], PRAMS[8], PRAMS[9],PRAMS[10])


