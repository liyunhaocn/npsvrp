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
    print(len(jobs))

    if run == False:
        return 0

    os.system("rm stdcerr/*.txt")
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
            pass
            # print("n{0}".format(cpu_use), end=' ')
        else:
            print("y{0}".format(cpu_use))
            cmd = arr[i]
            print(cpu_use)
            ins = cmd.split("/instances/")[1].split(".txt")[0].replace("/", "")+".txt"
            cmd = cmd.replace("&", ">> ./stdcerr/{0} 2>&1 &".format(ins))

            print("ins:", ins)
            print("sumit job ", i, cmd)

            os.system(cmd)
            i = i + 1

        time.sleep(1)

if __name__ == "__main__":

    if platform.system() == 'Linux':
        print("run job schedule on Linux")
        os.system("chmod 777 ./dev/SmartRouter")

    # jobs = all_cmds.get_all_static_my_cmds("testruin")
    # jobs = all_cmds.get_all_dynamic_my_cmds("testruin")

    # cmd = [
    #     "--init", "101010",
    #     "--iter", "50"
    # ]
    # s1 = "#".join(cmd)
    # print(s1)
    # print(s1.split("#"))
    # s = "#"
    # print([x for x in s.split("#") if len(x) > 0])

    # print([x for x in s.split("#") if len(x) > 0])
    fractions_default = [
        "-fractionGeneratedNearest", "0.05",
        "-fractionGeneratedSmart", "0.0",
        "-fractionGeneratedFurthest", "0.05",
        "-fractionGeneratedSweep", "0.05",
        "-fractionGeneratedRandomly", "0.85",
    ]

    fractions_all_random = [
        "-fractionGeneratedNearest", "0.00",
        "-fractionGeneratedSmart", "0.0",
        "-fractionGeneratedFurthest", "0.00",
        "-fractionGeneratedSweep", "0.00",
        "-fractionGeneratedRandomly", "1.00",
    ]

    grow_population_argv = [
        "-minimumPopulationSize", "10",
        "-growPopulationSize", "5",
        "-growPopulationAfterNonImprovementIterations", "500"
    ]

    grow_nb_granular_argv = [
        "-nbGranular", "5",
        "-growNbGranularSize", "5",
        "-growNbGranularAfterNonImprovementIterations", "500"
    ]

    init_small_tolerated = [
        "-maxToleratedCapacityViolation", "20",
        "-maxToleratedTimeWarp", "20"
    ]

    use_eax_ox_star = ["-useEaxAndOXStar", "1"]

    nagatama_before_restart = ["-nagataMaBeforeRestart", "1"]
    ruin_before_restart = ["-ruinBeforeRestart", "1"]
    reset_with_random = ["-resetPopulationWithAllRandom", "1"]
    ruin_when_get_bks = ["-ruinWhenGetBKS", "1"]

    config_string_arr = [
        # "--run_tag HGSDefault --config_str + ",

        # done
        "--run_tag nb5000InitDefaultResetRandomRuinBeforeReset --config_str " + "+" + "+".join(
            ["-nbIter", "5000"] + reset_with_random + ruin_before_restart),

        "--run_tag nb5000InitDefaultResetRandomRuinGetBKS --config_str " + "+" + "+".join(
            ["-nbIter", "5000"] + reset_with_random + ruin_when_get_bks),

        "--run_tag nb5000InitDefaultResetRandomRuinNaMABeforeReset --config_str " + "+" + "+".join(
            ["-nbIter", "5000"] + reset_with_random + ruin_before_restart + nagatama_before_restart),

        "--run_tag nb5000InitDefault --config_str " + "+" + "+".join(
            ["-nbIter", "5000"]),

        "--run_tag nb5000InitAllRandom --config_str " + "+" + "+".join(
            ["-nbIter", "5000"] + fractions_all_random),

        "--run_tag nb5000InitDefaultReset_with_random --config_str " + "+" + "+".join(
            ["-nbIter", "5000"] + reset_with_random),

        # "--run_tag nb20000InitDefault --config_str " + "+" + "+".join(
        #     ["-nbIter", "20000"]),
        # "--run_tag nb20000InitAllRandom --config_str " + "+" + "+".join(
        #     ["-nbIter", "20000"] + fractions_all_random),
        #
        # "--run_tag nb2000InitDefault --config_str " + "+" + "+".join(
        #     ["-nbIter", "2000"]),
        # "--run_tag nb2000InitAllRandom --config_str " + "+" + "+".join(
        #     ["-nbIter", "2000"] + fractions_all_random),

        # "--run_tag initSmallTolerate --config_str " + "+" + "+".join(init_small_tolerated),

        # "--run_tag smallNBGranular --config_str " + "+" + "+".join(["-nbGranular", "20"]),
        # "--run_tag smallTargetFeasible --config_str " + "+" + "+".join(["-targetFeasible", "0.2"]),
        # "--run_tag bigTargetFeasible --config_str " + "+" + "+".join(["-targetFeasible", "0.5"]),
        # "--run_tag growNbGranular --config_str " + "+" + "+".join(grow_nb_granular_argv),
        # "--run_tag growPopulation --config_str " + "+" + "+".join(grow_population_argv),

    ]

    jobs = all_cmds.get_all_dynamic_my_cmds()
    print(len(jobs))
    run(jobs, True)


