import csv
import datetime
import functools
import math
import time
import argparse
import subprocess
import sys
import os
import uuid
import platform
import numpy as np
import copy
import matplotlib.pyplot as plt

import tools
from environment import VRPEnvironment, ControllerEnvironment
from baselines.strategies import STRATEGIES
from delta import Mydelta
from or_tools import or_main
import area_tool

from environment_virtual import VRPEnvironmentVirtual

global_log_info = True
global_save_current_instance = False
global_log_error = True

global_use_my_delta = 0

# f = open("log/hgs_solver.txt", 'wt')

def write_vrplib_stdin(my_stdin, instance, name="problem", euclidean=False, is_vrptw=True, weight_arg=[]):
    # LKH/VRP does not take floats (HGS seems to do)

    coords = instance['coords']
    demands = instance['demands']
    is_depot = instance['is_depot']
    duration_matrix = instance['duration_matrix']
    capacity = instance['capacity']
    assert (np.diag(duration_matrix) == 0).all()
    assert (demands[~is_depot] > 0).all()

    my_stdin.write("\n".join([
        "{} : {}".format(k, v)
        for k, v in [
                        ("NAME", name),
                        ("COMMENT", "ORTEC"),  # For HGS we need an extra row...
                        ("TYPE", "CVRP"),
                        ("DIMENSION", len(coords)),
                        ("EDGE_WEIGHT_TYPE", "EUC_2D" if euclidean else "EXPLICIT"),
                    ] + ([] if euclidean else [
            ("EDGE_WEIGHT_FORMAT", "FULL_MATRIX")
        ]) + [("CAPACITY", capacity)]
    ]))
    my_stdin.write("\n")

    if not euclidean:
        my_stdin.write("EDGE_WEIGHT_SECTION\n")
        for row in duration_matrix:
            my_stdin.write("\t".join(map(str, row)))
            my_stdin.write("\n")

    my_stdin.write("NODE_COORD_SECTION\n")
    my_stdin.write("\n".join([
        "{}\t{}\t{}".format(i + 1, x, y)
        for i, (x, y) in enumerate(coords)
    ]))
    my_stdin.write("\n")

    my_stdin.write("DEMAND_SECTION\n")
    my_stdin.write("\n".join([
        "{}\t{}".format(i + 1, d)
        for i, d in enumerate(demands)
    ]))
    my_stdin.write("\n")

    my_stdin.write("DEPOT_SECTION\n")
    for i in np.flatnonzero(is_depot):
        my_stdin.write(f"{i + 1}\n")
    my_stdin.write("-1\n")

    if is_vrptw:

        service_t = instance['service_times']

        # Following LKH convention
        my_stdin.write("SERVICE_TIME_SECTION\n")
        my_stdin.write("\n".join([
            "{}\t{}".format(i + 1, s)
            for i, s in enumerate(service_t)
        ]))
        my_stdin.write("\n")

        timewi = instance['time_windows']
        my_stdin.write("TIME_WINDOW_SECTION\n")
        my_stdin.write("\n".join([
            "{}\t{}\t{}".format(i + 1, l, u)
            for i, (l, u) in enumerate(timewi)
        ]))
        my_stdin.write("\n")

        if "must_dispatch" in instance:
            must_dispatch = instance["must_dispatch"]
            my_stdin.write("MUST_DISPATCH\n")
            my_stdin.write("\n".join([
                "{}\t{}".format(i + 1, 1 if s else 0)
                for i, s in enumerate(must_dispatch)
            ]))
            my_stdin.write("\n")

        if 'release_times' in instance:
            release_times = instance['release_times']

            my_stdin.write("RELEASE_TIME_SECTION\n")
            my_stdin.write("\n".join([
                "{}\t{}".format(i + 1, s)
                for i, s in enumerate(release_times)
            ]))
            my_stdin.write("\n")

        if len(weight_arg) > 0:
            my_stdin.write("CUSTOMER_WEIGHT\n")
            my_stdin.write("\n".join([
                "{}\t{}".format(i + 1, s)
                for i, s in enumerate(weight_arg)
            ]))
            my_stdin.write("\n")

    my_stdin.write("EOF\n")
    my_stdin.flush()


def get_cpp_base_cmd():
    executable = os.path.join('dev', 'SmartRouter')
    if platform.system() == 'Windows' and os.path.isfile(executable + '.exe'):
        executable = executable + '.exe'
    assert os.path.isfile(executable), f"HGS executable {executable} does not exist!"

    cmd = [executable, "readstdin", '-veh', '-1', '-useWallClockTime', '1']
    return cmd


def get_initial_weight(instance, seed=1):
    if instance['coords'].shape[0] <= 1:
        yield []
        return

    if instance['coords'].shape[0] <= 2:
        yield [1]
        return

    cmd_arg = ["-call", "getWeight", '-seed', str(seed)]
    cmd = get_cpp_base_cmd() + cmd_arg

    with subprocess.Popen(cmd, stdout=subprocess.PIPE, stdin=subprocess.PIPE, text=True) as p:
        write_vrplib_stdin(p.stdin, instance, is_vrptw=True)
        weight = []
        for line in p.stdout:
            line = line.strip()
            if line.startswith('Weight'):
                weight = [int(x) for x in line.split(" ")[1:]]
                assert len(weight) == instance['coords'].shape[0], "len(weight) == instance['coords'].shape[0]"
                yield weight


# doDynamicWithEjection drop nodes, must_dispatch number
# solve_static_vrptw_lyh(instance,weight_arg,arg_call="hgsAndSmart")

# smallInstance
# solve_static_vrptw_lyh(instance,weight_arg,arg_call="smallInstance")

def solve_static_vrptw_lyh(instance,
                           time_limit=3600,
                           seed=1,
                           initial_solution=None,
                           weight_arg=[],
                           config_str="",
                           arg_call="hgsAndSmart"):
    # Prevent passing empty instances to the static solver, e.g. when
    # strategy decides to not dispatch any requests for the current epoch

    if instance['coords'].shape[0] <= 1:
        yield [], 0
        return

    if instance['coords'].shape[0] <= 2:
        solution = [[1]]
        cost = tools.validate_static_solution(instance, solution)
        yield solution, cost
        return

    cmd = get_cpp_base_cmd() + ["-t", str(max(time_limit - 2, 1)), '-seed', str(seed)]

    cmd += ["-call", arg_call]

    cmd += [x for x in config_str.split("+") if len(x) > 0]
    cmd_str = " ".join(cmd)
    log_info(f"cmd_str:{cmd_str}")

    # if initial_solution is None:
    #     initial_solution = [[i] for i in range(1, instance['coords'].shape[0])]
    # if initial_solution is not None:
    #     cmd += ['-initialSolution', " ".join(map(str, tools.to_giant_tour(initial_solution)))]

    with subprocess.Popen(cmd, stdout=subprocess.PIPE, stdin=subprocess.PIPE, text=True) as p:
        write_vrplib_stdin(p.stdin, instance, is_vrptw=True, weight_arg=weight_arg)
        if global_save_current_instance:
            with open("testInstances/x.txt", 'w+', encoding='UTF8', newline='') as f:
                write_vrplib_stdin(f, instance, is_vrptw=True, weight_arg=weight_arg)
        routes = []
        for line in p.stdout:
            line = line.strip()
            # Parse only lines which contain a route
            if line.startswith('Route'):
                label, route = line.split(": ")
                route_nr = int(label.split("#")[-1])
                assert route_nr == len(routes) + 1, "Route number should be strictly increasing"
                routes.append([int(node) for node in route.split(" ")])
            elif line.startswith('Cost'):
                # End of solution
                solution = routes
                cost = int(line.split(" ")[-1].strip())
                # check_cost = tools.validate_static_solution(instance, solution)
                # assert cost == check_cost, "Cost of HGS VRPTW solution could not be validated"
                yield solution, cost
                # Start next solution
                routes = []
            elif "EXCEPTION" in line:
                raise Exception("HGS failed with exception: " + line)
        assert len(routes) == 0, "HGS has terminated with imcomplete solution (is the line with Cost missing?)"


def run_baseline_lyh(args, env, oracle_solution=None, strategy=None, seed=None):
    strategy = strategy or args.strategy
    strategy = STRATEGIES[strategy] if isinstance(strategy, str) else strategy
    seed = seed or args.solver_seed

    rng = np.random.default_rng(seed)

    total_reward = 0
    done = False
    # Note: info contains additional info that can be used by your solver
    observation, static_info = env.reset()
    epoch_tlim = static_info['epoch_tlim']
    num_requests_postponed = 0
    num_epoch = static_info["end_epoch"] + 1
    current_epoch = observation["current_epoch"]
    if static_info["dynamic_context"] is not None:
        s_start = time.time()
        weight = np.array(list(get_initial_weight(instance=static_info["dynamic_context"], seed=args.solver_seed)))[0]
        time_get_weight = math.ceil(time.time() - s_start)
    else:
        weight = []
        time_get_weight = 0

    while not done:

        epoch_instance = observation['epoch_instance']

        if global_log_info is True:
            log_info(
                f"Epoch {static_info['start_epoch']} <= {observation['current_epoch']} <= {static_info['end_epoch']}",
                newline=False)
            num_requests_open = len(epoch_instance['request_idx']) - 1
            num_new_requests = num_requests_open - num_requests_postponed
            log_info(
                f" | Requests: +{num_new_requests:3d} = {num_requests_open:3d}, {epoch_instance['must_dispatch'].sum():3d}/{num_requests_open:3d} must-go...",
                newline=True, flush=True)

        if oracle_solution is not None:
            request_idx = set(epoch_instance['request_idx'])
            epoch_solution = [route for route in oracle_solution if len(request_idx.intersection(route)) == len(route)]
            cost = tools.validate_dynamic_epoch_solution(epoch_instance, epoch_solution)
        else:
            # Select the requests to dispatch using the strategy
            # Note: DQN strategy requires more than just epoch instance, bit hacky for compatibility with other strategies
            epoch_instance_dispatch = strategy(
                {**epoch_instance, 'observation': observation, 'static_info': static_info}, rng)

            # Run HGS with time limit and get last solution (= best solution found)
            # Note we use the same solver_seed in each epoch: this is sufficient as for the static problem
            # we will exactly use the solver_seed whereas in the dynamic problem randomness is in the instance
            customer_idx = epoch_instance_dispatch['customer_idx']
            if len(weight) > 0:
                # weight_new = np.array([int(x + x/num_epoch*current_epoch) for x in weight])
                weight_new = weight
                request_weight = weight_new[customer_idx]
                # epoch_instance_dispatch["must_dispatch"][:] = True
                # epoch_instance_dispatch["must_dispatch"][0] = False

            else:
                request_weight = []

            solutions = list(solve_static_vrptw_lyh(instance=epoch_instance_dispatch,
                                                    time_limit=epoch_tlim - time_get_weight,
                                                    seed=args.solver_seed,
                                                    weight_arg=request_weight,
                                                    config_str=args.config_str,
                                                    arg_call="hgsAndSmart"
                                                    ))
            assert len(solutions) > 0, f"No solution found during epoch {observation['current_epoch']}"
            epoch_solution, cost = solutions[-1]

            # Map HGS solution to indices of corresponding requests
            epoch_solution = [epoch_instance_dispatch['request_idx'][route] for route in epoch_solution]

        if global_log_info is True:
            num_requests_dispatched = sum([len(route) for route in epoch_solution])
            num_requests_open = len(epoch_instance['request_idx']) - 1
            num_requests_postponed = num_requests_open - num_requests_dispatched
            log_info(
                f" {num_requests_dispatched:3d}/{num_requests_open:3d} dispatched and {num_requests_postponed:3d}/{num_requests_open:3d} postponed | Routes: {len(epoch_solution):2d} with cost {cost:6d}")

        # Submit solution to environment
        observation, reward, done, info = env.step(epoch_solution)
        assert cost is None or reward == -cost, "Reward should be negative cost of solution"
        assert not info['error'], f"Environment error: {info['error']}"

        total_reward += reward

    if global_log_info is True:
        log_info(f"Cost of solution: {-total_reward}")

    return total_reward


def get_datetime_str(style='dt'):
    cur_time = datetime.datetime.now()

    date_str = cur_time.strftime('%y%m%d')
    time_str = cur_time.strftime('%H%M%S')

    if style == 'data':
        return date_str
    elif style == 'time':
        return time_str
    else:
        return date_str + '_' + time_str


if global_log_info:
    f = open("stdcerr/log_" + get_datetime_str() + ".txt", 'wt')

def log_file(obj, newline=True, flush=False):
    # Write logs to stderr since program uses stdout to communicate with controller
    if global_log_info:
        print(str(obj), file=f)


def log_info(obj, newline=True, flush=False):
    if global_log_info is False:
        return
    # Write logs to stderr since program uses stdout to communicate with controller
    sys.stderr.write(str(obj))
    if newline:
        sys.stderr.write('\n')
    if flush:
        sys.stderr.flush()


def log_error(obj, newline=True, flush=False):
    if global_log_error is False:
        return
    # Write logs to stderr since program uses stdout to communicate with controller
    sys.stderr.write(str(obj))
    if newline:
        sys.stderr.write('\n')
    if flush:
        sys.stderr.flush()


def save_results_csv(csv_path, data_dic):
    fieldnames = ["instance", "customers_num", "cost", "route_mum", "epoch_num", "strategy", "solver_seed", "static",
                  "epoch_tlim", "solution"]
    file_exist = os.path.exists(csv_path)
    # log_info(f"fieldnames:{fieldnames}")
    with open(csv_path, 'a', encoding='UTF8', newline='') as f:
        writer = csv.writer(f)
        if not file_exist:
            writer.writerow(fieldnames)
        data_replace_order = []
        for key in fieldnames:
            data_replace_order.append(data_dic[key])
        # log_info(f"data_replace_order:{data_replace_order}")
        writer.writerow(data_replace_order)

def solve_static_vrptw_wyx(instance, time_limit=3600, tmp_dir="tmp", seed=1, useDynamicParameters=0, initial_solution=None):
    # Prevent passing empty instances to the static solver, e.g. when
    # strategy decides to not dispatch any requests for the current epoch
    if instance['coords'].shape[0] <= 1:
        yield [], 0
        return

    if instance['coords'].shape[0] <= 2:
        solution = [[1]]
        cost = tools.validate_static_solution(instance, solution)
        yield solution, cost
        return

    os.makedirs(tmp_dir, exist_ok=True)
    instance_filename = os.path.join(tmp_dir, "problem.vrptw")
    tools.write_vrplib(instance_filename, instance, is_vrptw=True)

    executable = os.path.join('baselines', 'hgs_vrptw', 'genvrp')
    # On windows, we may have genvrp.exe
    if platform.system() == 'Windows' and os.path.isfile(executable + '.exe'):
        executable = executable + '.exe'
    assert os.path.isfile(executable), f"HGS executable {executable} does not exist!"
    # Call HGS solver with unlimited number of vehicles allowed and parse outputs
    # Subtract two seconds from the time limit to account for writing of the instance and delay in enforcing the time limit by HGS
    hgs_cmd = [
        executable, instance_filename, str(max(time_limit, 1)),
        '-seed', str(seed), '-veh', '-1', '-useWallClockTime', '1'
        # ,'-useDynamicParameters', str(useDynamicParameters),
        # '-nbGranular', '40', '-doRepeatUntilTimeLimit', '0', '-growPopulationAfterIterations', str(5000),
        # '-minimumPopulationSize','100'
    ]
    if useDynamicParameters == 1:
        hgs_cmd = [
            executable, instance_filename, str(max(time_limit, 1)),
            '-seed', str(seed), '-veh', '-1', '-useWallClockTime', '1',
            '-useDynamicParameters', str(useDynamicParameters),
            # '-nbGranular', '40', '-doRepeatUntilTimeLimit', '0', '-growPopulationAfterIterations', str(20),
            # '-doRepeatUntilTimeLimit', '0',
            # '-it', '5000',  # 20000
            '-minimumPopulationSize', '10',
            '-predictSample', '1',
            '-fractionUse2opt', '10',
            '-quickStop', '1'
        ]
    if initial_solution is None:
        initial_solution = [[i] for i in range(1, instance['coords'].shape[0])]
    if initial_solution is not None:
        hgs_cmd += ['-initialSolution', " ".join(map(str, tools.to_giant_tour(initial_solution)))]

    with subprocess.Popen(hgs_cmd, stdout=subprocess.PIPE, text=True) as p:
        routes = []
        for line in p.stdout:
            # if global_log_info: log_info(line, file=f)
            line = line.strip()
            # Parse only lines which contain a route
            if line.startswith('Route'):
                label, route = line.split(": ")
                route_nr = int(label.split("#")[-1])
                assert route_nr == len(routes) + 1, "Route number should be strictly increasing"
                routes.append([int(node) for node in route.split(" ")])
            elif line.startswith('Cost'):
                log_file(f"wyx:line:{line}")
                # End of solution
                solution = routes
                # cost = int_func(line.split(" ")[-1].strip())
                check_cost = tools.validate_static_solution(instance, solution)
                cost = check_cost
                assert cost == check_cost, "Cost of HGS VRPTW solution could not be validated"
                yield solution, cost
                # Start next solution
                routes = []
            elif line.startswith("Time"):
                log_file(f"wyx:line:{line}")
            elif "EXCEPTION" in line:
                raise Exception("HGS failed with exception: " + line)

        assert len(routes) == 0, "HGS has terminated with imcomplete solution (is the line with Cost missing?)"

def solve_static_vrptw(instance, time_limit=3600, tmp_dir="tmp", seed=1, useDynamicParameters=0, initial_solution=None):
    # Prevent passing empty instances to the static solver, e.g. when
    # strategy decides to not dispatch any requests for the current epoch
    if instance['coords'].shape[0] <= 1:
        yield [], 0
        return

    if instance['coords'].shape[0] <= 2:
        solution = [[1]]
        cost = tools.validate_static_solution(instance, solution)
        yield solution, cost
        return

    os.makedirs(tmp_dir, exist_ok=True)
    instance_filename = os.path.join(tmp_dir, "problem.vrptw")
    tools.write_vrplib(instance_filename, instance, is_vrptw=True)

    executable = os.path.join('baselines', 'hgs_vrptw', 'genvrp')
    # On windows, we may have genvrp.exe
    if platform.system() == 'Windows' and os.path.isfile(executable + '.exe'):
        executable = executable + '.exe'
    assert os.path.isfile(executable), f"HGS executable {executable} does not exist!"
    # Call HGS solver with unlimited number of vehicles allowed and parse outputs
    # Subtract two seconds from the time limit to account for writing of the instance and delay in enforcing the time limit by HGS
    hgs_cmd = [
        executable, instance_filename, str(max(time_limit, 1)),
        '-seed', str(seed), '-veh', '-1', '-useWallClockTime', '1'
        # ,'-useDynamicParameters', str(useDynamicParameters),
        # '-nbGranular', '40', '-doRepeatUntilTimeLimit', '0', '-growPopulationAfterIterations', str(5000),
        # '-minimumPopulationSize','100'
    ]
    if useDynamicParameters == 1:
        hgs_cmd = [
            executable, instance_filename, str(max(time_limit, 1)),
            '-seed', str(seed), '-veh', '-1', '-useWallClockTime', '1',
            '-useDynamicParameters', str(useDynamicParameters),
            # '-nbGranular', '40', '-doRepeatUntilTimeLimit', '0', '-growPopulationAfterIterations', str(20),
            # '-doRepeatUntilTimeLimit', '0',
            # '-it', '5000',  # 20000
            '-minimumPopulationSize', '10'
        ]
    if initial_solution is None:
        initial_solution = [[i] for i in range(1, instance['coords'].shape[0])]
    if initial_solution is not None:
        hgs_cmd += ['-initialSolution', " ".join(map(str, tools.to_giant_tour(initial_solution)))]
    with subprocess.Popen(hgs_cmd, stdout=subprocess.PIPE, text=True) as p:
        routes = []
        for line in p.stdout:
            # if global_log_info: log_info(line, file=f)
            line = line.strip()
            # Parse only lines which contain a route
            if line.startswith('Route'):
                label, route = line.split(": ")
                route_nr = int(label.split("#")[-1])
                assert route_nr == len(routes) + 1, "Route number should be strictly increasing"
                routes.append([int(node) for node in route.split(" ")])
            elif line.startswith('Cost'):
                # End of solution
                solution = routes
                # cost = int_func(line.split(" ")[-1].strip())
                check_cost = tools.validate_static_solution(instance, solution)
                cost = check_cost
                assert cost == check_cost, "Cost of HGS VRPTW solution could not be validated"
                yield solution, cost
                # Start next solution
                routes = []
            elif "EXCEPTION" in line:
                raise Exception("HGS failed with exception: " + line)
        assert len(routes) == 0, "HGS has terminated with imcomplete solution (is the line with Cost missing?)"


def cul_early_time(route, instance):
    early_lieve = instance['time_windows'][route[-1], 1]
    early_tw = early_lieve
    dur_time = 0
    last_node = route[-1]
    for i in range(len(route) - 2, -1, -1):
        # dur_time = instance['service_times'][last_node]+instance['duration_matrix'][route[i], last_node]
        dur_time = instance['service_times'][last_node] + instance['duration_matrix'][last_node, route[i]]
        early_lieve = early_lieve - dur_time
        early_tw = instance['time_windows'][route[i], 1]
        early_lieve = min(early_lieve, early_tw)
        last_node = route[i]

    #  subtact time of first node
    # dur_time = instance['service_times'][last_node] + instance['duration_matrix'][0, last_node]
    dur_time = instance['service_times'][last_node] + instance['duration_matrix'][last_node, 0]
    early_lieve = early_lieve - dur_time
    return early_lieve


def run_oracle(args, env):
    # Oracle strategy which looks ahead, this is NOT a feasible strategy but gives a 'bound' on the performance
    # Bound written with quotes because the solution is not optimal so a better solution may exist
    # This oracle can also be used as supervision for training a model to select which requests to dispatch

    # First get hindsight problem (each request will have a release time)
    # As a start solution for the oracle solver, we use the greedy solution
    # This may help the oracle solver to find a good solution more quickly
    log_info("Running greedy baseline to get start solution and hindsight problem for oracle solver...")
    run_baseline(args, env, strategy='greedy')
    # Get greedy solution as simple list of routes
    greedy_solution = [route for epoch, routes in env.final_solutions.items() for route in routes]
    hindsight_problem = env.get_hindsight_problem()

    # Compute oracle solution (separate time limit since epoch_tlim is used for greedy initial solution)
    log_info(f"Start computing oracle solution with {len(hindsight_problem['coords'])} requests...")
    oracle_solution = min(solve_static_vrptw(hindsight_problem, time_limit=args.oracle_tlim, tmp_dir=args.tmp_dir,
                                             initial_solution=greedy_solution), key=lambda x: x[1])[0]
    oracle_cost = tools.validate_static_solution(hindsight_problem, oracle_solution)
    log_info(f"Found oracle solution with cost {oracle_cost}")

    # Run oracle solution through environment (note: will reset environment again with same seed)
    total_reward = run_baseline(args, env, oracle_solution=oracle_solution)
    assert -total_reward == oracle_cost, "Oracle solution does not match cost according to environment"
    return total_reward


def plt_test(observation, coords, epoch_instance, ndelta, save=1):
    fig_name = 'FIG'
    plt.figure(fig_name)
    all_x = [i[0] for i in coords]
    all_y = [i[1] for i in coords]
    plt.scatter(all_x, all_y, alpha=0.2, s=2, c='grey')
    new_intance = copy.copy(epoch_instance)
    new_intance['penalty'] = []
    gap = 800
    for i in range(len(epoch_instance['coords'])):
        if epoch_instance['must_dispatch'][i]:
            new_intance['penalty'].append(5000)
            continue
        i_delta = ndelta.cul_delta(epoch_instance['customer_idx'][i])
        new_intance['penalty'].append(i_delta)
    x = epoch_instance['coords'][:, 0]
    y = epoch_instance['coords'][:, 1]
    plt.scatter(x, y, alpha=1, s=10, c=new_intance['penalty'], cmap='Blues_r')
    plt.colorbar()
    filename = './plt/epoch_' + str(observation['current_epoch']) + '.png'
    # plt.show()
    if save:
        if not os.path.exists('plt'):
            os.mkdir('plt')
        plt.savefig(filename, bbox_inches='tight', dpi=300)
    plt.clf()
    #


def plt_vrp(observation, coords_solution, coords, save=0, args=None):
    fig_name = 'FIG'
    plt.figure(fig_name)
    # x_lists = [i[0] for i in coords_solution]
    # y_lists = [i[1] for i in coords_solution]
    # plt.plot(x_lists, y_lists)
    co_lists = []
    w_time_windows = (observation['epoch_instance']['time_windows'][:, 1] - observation['epoch_instance'][
                                                                                'duration_matrix'][0, :]) / 3600
    w_x = observation['epoch_instance']['coords'][:, 0]
    w_y = observation['epoch_instance']['coords'][:, 1]
    w_z = list(range(len(w_y)))
    all_x = [i[0] for i in coords]
    all_y = [i[1] for i in coords]
    plt.scatter(all_x, all_y, alpha=0.2, s=2, c='grey')
    # plt depot
    depot_x = observation['epoch_instance']['coords'][0, 0]
    depot_y = observation['epoch_instance']['coords'][0, 1]
    plt.scatter(depot_x, depot_y, alpha=1, s=15, c='red')
    plt.scatter(w_x[1:], w_y[1:], alpha=1, s=10, c=w_time_windows[1:], cmap='Blues_r')
    plt.colorbar()
    tw_small_x = [w_x[i] for i in range(len(w_time_windows)) if w_time_windows[i] < 1]
    tw_small_y = [w_y[i] for i in range(len(w_time_windows)) if w_time_windows[i] < 1]
    plt.scatter(tw_small_x, tw_small_y, alpha=1, s=15, c='red', marker='*')
    for route in coords_solution:
        # co_lists = observation['epoch_instance']['coords'][route]
        # x_lists = [i[0] for i in co_lists]
        # y_lists = [i[1] for i in co_lists]
        x_lists = [i[0] for i in route]
        y_lists = [i[1] for i in route]
        plt.scatter(x_lists[0], y_lists[0], alpha=1, s=5)
        plt.plot(x_lists, y_lists, linestyle='-', alpha=0.5)

    filename = './plt/epoch_' + str(observation['current_epoch']) + '.png'
    if save:
        if not os.path.exists('plt'):
            os.mkdir('plt')
        ints_name = args.instance.split('/')[1].split('.')[0]
        # ints_name = os.path.basename(args.instance)
        ints_name = 'plt/' + ints_name
        if not os.path.exists(ints_name):
            os.mkdir(ints_name)
        # os.makedirs(ints_name, exist_ok=True)
        filename = './' + ints_name + '/epoch_' + str(observation['current_epoch']) + '.png'
        plt.savefig(filename, bbox_inches='tight', dpi=300)
    plt.clf()
    # plt.show()


# return [[w1,w2,...],[...],[...]] like solution
def cul_weight_sol(sol, instance, args, max_epoch=5):
    x1 = args.sol_x1
    x2 = args.sol_x2
    x3 = args.sol_x3
    max_epoch0 = max_epoch
    max_epoch1 = max_epoch + 5
    avg_dur = args.avg
    tw_open = [instance['time_windows'][route, 0].tolist() for route in sol]
    tw_close = [instance['time_windows'][route, 1].tolist() for route in sol]
    tw_serv = [instance['service_times'][route].tolist() for route in sol]
    tw_dur = [instance['time_windows'][route, 1].tolist() for route in sol]
    tw_all = [instance['time_windows'][route, 1].tolist() for route in sol]
    for i in range(len(tw_open)):
        for j in range(len(tw_open[i])):
            tw_dur[i][j] = (tw_close[i][j] - tw_open[i][j] - tw_serv[i][j]) / 10000
            tw_open[i][j] = (tw_open[i][j] - instance['duration_matrix'][sol[i][j], 0]) / 3600
            tw_close[i][j] = (tw_close[i][j] - instance['duration_matrix'][sol[i][j], 0]) / 3600
            # tw_open[i][j] = max((1 - max(min(tw_open[i][j], max_epoch0), 0) / max_epoch0),0) * x1
            # tw_close[i][j] = max((1 - max(min(tw_close[i][j], max_epoch1), 0) / max_epoch1),0) * x2

            # tw_close[i][j] = max((tw_open[i][j]+tw_close[i][j])/2+1, 1.0)
            # test tmp

            # tw_all[i][j] = (x1 / max(tw_close[i][j] / 3, 0.3) + x2 * tw_dur[i][j]) / 2 + instance['duration_matrix'][sol[i][j], 0] / avg_dur * x3
            tw_all[i][j] = x1 / (tw_dur[i][j] + 0.01) + x2 / max(tw_close[i][j], 0.9) + avg_dur / (
                        instance['duration_matrix'][sol[i][j], 0] + 0.01) * x3
            # tw_all[i][j] = x1/max(tw_open[i][j],0.9) + x2/max(tw_close[i][j],0.9) + instance['duration_matrix'][sol[i][j], 0] / avg_dur * x3
            tw_all[i][j] = 1 / max(tw_close[i][j] / 3, 0.3)
    return tw_all


# return weight of i (not requIndx)
def cul_weight_i(n, instance, args, max_epoch=5):
    x1 = args.x1
    x2 = args.x2
    x3 = args.x3
    max_epoch0 = max_epoch
    max_epoch1 = max_epoch + 5
    tw_open = instance['time_windows'][n, 0]
    tw_close = instance['time_windows'][n, 1]
    tw_serv = instance['service_times'][n]
    tw_dur = 0
    tw_all = 0

    tw_dur = (tw_close - tw_open - tw_serv) / 3600
    tw_open = (tw_open - instance['duration_matrix'][n, 0]) / 3600
    tw_close = (tw_close - instance['duration_matrix'][n, 0]) / 3600
    # tw_all = 1 / max(tw_close / 3, 0.3)
    avg_dur = args.avg
    # tw_all = x1 / max(tw_open,0.9) + x2 / max(tw_close,0.9) + instance['duration_matrix'][n, 0]/avg_dur * x3
    # tw_all = x1 / max(tw_open, 0.9) + x2 / max(tw_close, 0.9) + tw_dur * x3
    tw_all = max(x1 / tw_dur + x2 * 3 / max(tw_close, 0.3) + instance['duration_matrix'][n, 0] / avg_dur * x3, 0.1) / 3

    # tw_all = 1 / max(tw_close / 3, 0.3)

    return tw_all


def choose_nodes_new_iter(ndelta, sol, instance, args, max_epoch=5, gap=600):
    sol_copy = [route for route in sol]
    cust_sol = [instance['customer_idx'][route].tolist() for route in sol]
    must_sol = [instance['must_dispatch'][route].tolist() for route in sol]
    w_sol = cul_weight_sol(sol, instance, args, max_epoch)
    new_sol = []
    # gap = 600
    area_gap = args.rand_num
    repeat_time = 0
    del_count = 0
    for re in range(repeat_time):
        index = 0
        for route in cust_sol:
            if len(route) <= 1:
                index += 1
                continue
            max_delta = -10e9
            max_index = -1
            new_route = []
            # For first node
            delta_sol_i = ndelta.cul_delta_three_nodes(route[0], 0, route[1])
            delta_rand_i = ndelta.cul_delta(route[0])
            w_i = w_sol[index][0]
            dis_i = instance['duration_matrix'][sol[index][0], 0]
            if delta_sol_i - w_i * delta_rand_i >= gap * 0 + area_gap and delta_sol_i - delta_rand_i >= max_delta and not \
                    must_sol[index][0]:
                max_delta = delta_sol_i - delta_rand_i
                max_index = 0

            # For 1:n-1
            for i in range(1, len(route) - 1):
                delta_sol_i = ndelta.cul_delta_three_nodes(route[i], route[i - 1], route[i + 1])
                delta_rand_i = ndelta.cul_delta(route[i])
                w_i = w_sol[index][i]
                dis_i = instance['duration_matrix'][sol[index][i], 0]
                if delta_sol_i - w_i * delta_rand_i >= gap * 0 + area_gap and delta_sol_i - delta_rand_i >= max_delta and not \
                        must_sol[index][i]:
                    max_delta = delta_sol_i - delta_rand_i
                    max_index = i
            # For last node
            delta_sol_i = ndelta.cul_delta_three_nodes(route[len(route) - 1], route[len(route) - 2], 0)
            delta_rand_i = ndelta.cul_delta(route[len(route) - 1])
            w_i = w_sol[index][len(route) - 1]
            dis_i = instance['duration_matrix'][sol[index][len(route) - 1], 0]
            if delta_sol_i - w_i * delta_rand_i >= gap * 0 + area_gap and delta_sol_i - delta_rand_i >= max_delta and not \
                    must_sol[index][len(route) - 1]:
                max_delta = delta_sol_i - delta_rand_i
                max_index = len(route) - 1

            # Delete node with max_delta
            if max_index != -1:
                del_count += 1
                sol_copy[index].pop(max_index)
                cust_sol[index].pop(max_index)
                must_sol[index].pop(max_index)
                w_sol[index].pop(max_index)

            new_sol.append(new_route)
            index += 1
    # sol = [instance['request_idx'][route] for route in new_sol]
    if global_log_info is True:
        log_info(f' deleted {del_count} ', newline=False)
    return sol_copy


def delta_weight_instance(epoch_instance, ndelta, args, gap=500):
    new_intance = copy.copy(epoch_instance)
    new_intance['penalty'] = []
    # gap = 500
    gap_w = args.gap
    for i in range(len(epoch_instance['coords'])):
        if epoch_instance['must_dispatch'][i]:
            new_intance['penalty'].append(1000000)
            continue
        # i_delta = ndelta.cul_delta(epoch_instance['customer_idx'][i]) + gap
        # i_delta = cul_weight_i(i, epoch_instance, args) * ndelta.cul_delta(epoch_instance['customer_idx'][i]) + gap * epoch_instance['duration_matrix'][i, 0] + 0
        i_delta = cul_weight_i(i, epoch_instance, args) * gap_w - gap_w + ndelta.cul_delta(
            epoch_instance['customer_idx'][i]) + 0 * epoch_instance['duration_matrix'][i, 0] + gap
        # i_delta = cul_weight_i(i, epoch_instance, args) * ndelta.cul_delta(epoch_instance['customer_idx'][i]) + 300
        new_intance['penalty'].append(i_delta)
    return new_intance


def delta_weight_instance_add_wyx(epoch_instance, ndelta, args, delta_wyx, gap=500):
    new_intance = copy.copy(epoch_instance)
    # new_intance = _filter_instance(new_intance,mask)
    new_intance['penalty'] = []
    # gap = 500
    gap_w = args.gap
    for i in range(len(new_intance['coords'])):
        if new_intance['must_dispatch'][i]:
            new_intance['penalty'].append(1000000)
            continue
        # if mask[i] == False:
        #     new_intance['penalty'].append(-10000)
        #     continue
        # use delta of wxy
        i_delta = cul_weight_i(i, epoch_instance, args)*gap_w - gap_w + delta_wyx[i]*0.5 + 0 * epoch_instance['duration_matrix'][i, 0] + gap
        new_intance['penalty'].append(i_delta)
        # # old version
        # i_delta = cul_weight_i(i, new_intance, args)*gap_w - gap_w + ndelta.cul_delta(new_intance['customer_idx'][i]) + 0 * new_intance['duration_matrix'][i, 0] + gap
        # new_intance['penalty'].append(i_delta)
    # with open('diff_delta.txt', 'a') as f2:
    #     log_info(new_intance['penalty'], file=f2)
    #     log_info(delta_wyx, file=f2)
    return new_intance


def find_class(area_xy, ratioxy, dis_ratio, args):
    # tmp params

    mean_area = 16243198.3
    mean_rarioxy = 2.710696626
    mean_dis_ratio = 2.242701354
    class_1 = area_xy < 10e8 and ratioxy < mean_rarioxy and mean_dis_ratio > mean_dis_ratio
    class_2 = area_xy > mean_area and ratioxy > mean_rarioxy
    class_3 = area_xy > mean_area and ratioxy < mean_rarioxy and mean_dis_ratio > mean_dis_ratio
    # if class_1 or class_2 or class_3:
    #     args.or_gap = 109
    #     args.x1 = -4.68
    #     args.x2 = 9.64
    #     args.x3 = -0.6
    #     args.early_time1 = 2600
    #     args.early_time2 = 2600
    #     args.gap = 550
    #     return
    class_4 = area_xy < 4e6 and ratioxy < 2
    if class_4:
        args.or_gap = 109
        args.x1 = -4.68
        args.x2 = 9.64
        args.x3 = -0.6
        args.early_time1 = 1200
        args.early_time2 = -2000
        args.gap = 550
        return
    class_5 = area_xy < 7e6 and ratioxy > 2
    # '901	94    -7.018	14.887	-2.537	2600	2600	214	0	3	0.15'
    if class_5:
        args.or_gap = 94
        args.x1 = -7.018
        args.x2 = 14.887
        args.x3 = -2.537
        args.early_time1 = 2600
        args.early_time2 = 2600
        args.gap = 214


# add by YxuanwKeith
# f2 = open("log/predict_info.txt", 'wt')
def predict_info(epoch_instance, current_epoch, rng, env_virtual, tmp, tlim):
    predict_start_time = time.time()

    epoch_num = 2
    test_num = 5
    per_5_cut = np.array([0, 0, 0, 1, 1, 1])
    origin_num = len(epoch_instance['request_idx'])
    current_num = np.zeros(origin_num, dtype=int)

    distance_delta_all = None
    distance_delta_ave = np.zeros(origin_num)
    mask_num = np.zeros(origin_num)
    route_pass = []
    max_time_once = 15

    cnt = 0
    while time.time() - predict_start_time + max_time_once < tlim:
        seed = rng.integers(100000000)
        # seed = 1

        env_virtual.import_info(epoch_instance=epoch_instance, virtual_epoch=current_epoch, seed=seed)
        ins = env_virtual.step_num(epoch_num)
        # with open('ins.txt','a') as f2:
        #     log_info(ins, file = f2)

        solutions = list(solve_static_vrptw_wyx(ins, time_limit=max_time_once, tmp_dir=tmp, seed=seed, useDynamicParameters=1))
        if len(solutions) == 0:
            if global_log_info: log_info('pass a predict sample')
            continue

        epoch_solution, cost = solutions[-1]

        # distance_delta info
        distance_delta = np.zeros(origin_num)
        for route in epoch_solution:
            route.insert(0, 0)
            for i in range(1, len(route)):
                idx = route[i]
                if idx >= origin_num or idx == 0:
                    continue
                idx_lst, idx_nxt = route[i - 1], route[(i + 1) % len(route)]
                distance_delta[idx] = ins['duration_matrix'][idx_lst, idx] + ins['duration_matrix'][idx, idx_nxt] - \
                                      ins['duration_matrix'][idx_lst, idx_nxt]

        distance_delta_ave += distance_delta
        distance_delta_all = np.array([distance_delta]) if distance_delta_all is None else np.vstack(
            (distance_delta_all, distance_delta))

        cnt += 1

    log_info(f"predict cnt : {cnt}")
    distance_delta_ave /= cnt

    return distance_delta_ave, distance_delta_all


def run_baseline(args, env, oracle_solution=None, strategy=None):
    strategy = strategy or args.strategy

    rng = np.random.default_rng(args.solver_seed)

    total_reward = 0
    done = False

    # Note: info contains additional info that can be used by your solver
    observation, static_info = env.reset()
    epoch_tlim = static_info['epoch_tlim']

    num_requests_postponed = 0
    if static_info['num_epochs'] > 1:
        # add by YxuanwKeith
        env_virtual = VRPEnvironmentVirtual(seed=args.instance_seed, instance=static_info['dynamic_context'],
                                            epoch_tlim=args.epoch_tlim, is_static=args.static)

        ndelta = Mydelta(static_info['dynamic_context'], args.solver_seed, args.rand_num)
        bin_data_0 = static_info['dynamic_context']['coords'][:, 0]
        bin_data_1 = static_info['dynamic_context']['coords'][:, 1]
        area_xy, ratioxy, dis_ratio = area_tool.get_classes(static_info['dynamic_context'])

        find_class(area_xy, ratioxy, dis_ratio, args)

    while not done:
        start_time = time.time()
        if static_info['num_epochs'] > 1:
            max_epoches = static_info['end_epoch'] - static_info['start_epoch'] + 1
            # tmp_k = max(115 - (observation['current_epoch'] - static_info['start_epoch'] - 1) * 20, 50)
            tmp_k = max(150 - (observation['current_epoch'] - static_info['start_epoch']) * int(100 / max_epoches), 30)
            # log_info(f" k = {tmp_k} ")
            ndelta.reset_rand_num(tmp_k)
        best_sol = list()
        best_coords_solution = list()
        epoch_instance = observation['epoch_instance']
        min_each_dis = 10e8

        args.avg = np.mean(epoch_instance['duration_matrix'][:, 0])
        # args.avg = np.mean(np.square(static_info['dynamic_context']['duration_matrix'][:, 0]))
        # maxc = np.max(static_info['dynamic_context']['duration_matrix'][:, 0])
        # numc = len(static_info['dynamic_context']['duration_matrix'][:, 0])
        # log_info(f'===== num= {numc} ============avg==== {maxc} ====== area= {area_bin} ======')
        # log_info(f"mean = {args.avg}")

        if global_log_info is True:
            log_info(
                f"Epoch {static_info['start_epoch']} <= {observation['current_epoch']} <= {static_info['end_epoch']}",
                newline=False)
            num_requests_open = len(epoch_instance['request_idx']) - 1
            num_new_requests = num_requests_open - num_requests_postponed
            log_info(
                f" | Requests: +{num_new_requests:3d} = {num_requests_open:3d}, {epoch_instance['must_dispatch'].sum():3d}/{num_requests_open:3d} must-go...",
                newline=False, flush=True)

        if oracle_solution is not None:
            request_idx = set(epoch_instance['request_idx'])
            epoch_solution = [route for route in oracle_solution if len(request_idx.intersection(route)) == len(route)]
            cost = tools.validate_dynamic_epoch_solution(epoch_instance, epoch_solution)
        else:
            # Select the requests to dispatch using the strategy
            # TODO improved better strategy (machine learning model?) to decide which non-must requests to dispatch
            # strategy = "weight"
            strategy = "greedy"  # if use "greedy", the """epoch_instance_dispatch['must_dispatch']""" can not be used.
            use_dyn = 1
            if static_info['end_epoch'] < 2:
                use_dyn = 0
            repeat_time = 1
            # # log_info("Time ", i)
            epoch_instance = observation['epoch_instance']
            max_epoch = static_info['end_epoch']

            if strategy == "weight":
                epoch_instance_dispatch = STRATEGIES[strategy](epoch_instance, rng, max_epoch)
            if strategy == "greedy":
                epoch_instance_dispatch = STRATEGIES[strategy](epoch_instance, rng)

            if use_dyn == 0:
                solutions = list(
                    solve_static_vrptw_lyh(epoch_instance, time_limit=int(epoch_tlim),
                                           seed=args.solver_seed))
                epoch_solution, cost = solutions[-1]
            else:
                use_ortools = 1
                if use_ortools:
                    #  test OR_tools
                    # use delta of wyx
                    if global_use_my_delta == 0:
                        distance_delta_ave, distance_delta_all = predict_info(epoch_instance, observation['current_epoch'], rng, env_virtual, args.tmp_dir, epoch_tlim - 22)
                        or_epoch_instance = delta_weight_instance_add_wyx(epoch_instance, ndelta, args, distance_delta_ave, gap=args.or_gap)
                        running_time = 10
                    else:
                        or_epoch_instance = delta_weight_instance(epoch_instance, ndelta, args, gap=args.or_gap)
                        running_time = int(epoch_tlim/2 - (time.time()-start_time))
                    # use lyh_solver
                    # weight_arg = [int(i) for i in or_epoch_instance['penalty']]
                    # weight_arg[0] = 0
                    # or_epoch_instance.pop('penalty')
                    # solutions = list(solve_static_vrptw_lyh(or_epoch_instance, weight_arg=weight_arg, time_limit=int(running_time),seed=args.solver_seed,arg_call = "hgsAndSmart"))
                    # sol, reward = solutions[-1]

                    sol = or_main(or_epoch_instance, running_time, global_log_info)
                    sol_id = [epoch_instance['request_idx'][route] for route in sol]
                    num_requests_dispatched = sum([len(route) for route in sol_id])
                    if num_requests_dispatched == 0:
                        reward = 0
                    else:
                        try:
                            reward = tools.validate_dynamic_epoch_solution(epoch_instance, sol_id)
                        except Exception as e:
                            reward = -1

                    if global_log_info is True:
                        log_info(f' OR-Tools reward: {reward} ', newline=False)
                    epoch_solution = sol

                    coords_solution = [epoch_instance['coords'][route] for route in epoch_solution]
                    # plt_vrp(observation, coords_solution, static_info['dynamic_context']['coords'],1, args)
                else:
                    # wyx
                    if args.strategy == 'smart':
                        epoch_instance_dispatch = strategy(
                            {**epoch_instance, 'observation': observation, 'static_info': static_info}, rng)

                    # OLD VERSION
                    solutions = list(
                        solve_static_vrptw(epoch_instance_dispatch, time_limit=int(epoch_tlim / 2),
                                           tmp_dir=args.tmp_dir,
                                           seed=args.solver_seed, useDynamicParameters=use_dyn))
                    assert len(
                        solutions) > 0, f"No solution found during epoch {observation['current_epoch']} and time_lim={epoch_tlim}"
                    epoch_solution, cost = solutions[-1]

                new_epoch_solution = []
                if strategy == "greedy":
                    for route in epoch_solution:
                        if epoch_instance['must_dispatch'][route].any():
                            new_epoch_solution.append(route)
                            continue
                        # early_time = epoch_instance['time_windows'][route[0], 0]/2+epoch_instance['time_windows'][route[0], 1]/2-epoch_instance['duration_matrix'][0, route[0]]
                        new_earlyest_time = cul_early_time(route, epoch_instance)
                        # log_info(f"early_time = {early_time}, new_earlyest_time = {new_earlyest_time} ")
                        if new_earlyest_time < args.early_time1:
                            new_epoch_solution.append(route)
                            if global_log_info is True:
                                log_info(f"ROUTE NOTICE--{new_earlyest_time} ", newline=False)
                epoch_solution = new_epoch_solution
            # assert len(solutions) > 0, f"No solution found during epoch {observation['current_epoch']} and time_lim={epoch_tlim}"

            # Delete route without must-go
            del_must = 1

            # record coords of solution
            coords_solution = [epoch_instance_dispatch['coords'][route] for route in epoch_solution]
            # record must-go of solution
            must_solution = [epoch_instance_dispatch['must_dispatch'][route] for route in epoch_solution]

            # Map HGS solution to indices of corresponding requests
            # epoch_solution = [epoch_instance_dispatch['request_idx'][route] for route in epoch_solution]

            # Distance of Each node
            num_requests_dispatched = sum([len(route) for route in epoch_solution])

            # Choose nodes with delta
            epoch_solution_cust = [epoch_instance_dispatch['customer_idx'][route] for route in epoch_solution]
            # choose_nodes(ndelta, epoch_solution_cust, must_solution)

            use_new_strategy = 1

            for count in range(repeat_time):
                if use_new_strategy and use_dyn == 1:
                    epoch_solution_id = choose_nodes_new_iter(ndelta, epoch_solution, epoch_instance, args, max_epoch,
                                                              args.gap)
                    epoch_solution = [epoch_instance_dispatch['request_idx'][route] for route in epoch_solution_id]
                    strategy = 'new'
                    epoch_instance_dispatch_2 = STRATEGIES[strategy](epoch_instance, rng, epoch_solution_id)
                    running_time = int(epoch_tlim - (time.time() - start_time))

                    # solutions = list(solve_static_vrptw_lyh(epoch_instance_dispatch_2, time_limit=int(running_time),seed=args.solver_seed,arg_call = "hgsAndSmart"))

                    solutions = list(
                        solve_static_vrptw(epoch_instance_dispatch_2, time_limit=int(running_time),
                                           tmp_dir=args.tmp_dir, seed=args.solver_seed, useDynamicParameters=use_dyn))
                    assert len(
                        solutions) > 0, f"No solution found during epoch {observation['current_epoch']} and time_lim={epoch_tlim}"
                    epoch_solution, cost = solutions[-1]
                    # Delete route without must-go
                    if del_must:
                        new_epoch_solution = []
                        if strategy == "new":
                            for route in epoch_solution:
                                # if epoch_instance_dispatch_2['must_dispatch'][route].any():
                                #     new_epoch_solution.append(route)
                                if epoch_instance_dispatch_2['must_dispatch'][route].any():
                                    new_epoch_solution.append(route)
                                    continue

                                new_earlyest_time = cul_early_time(route, epoch_instance_dispatch_2)
                                # log_info(early_time)
                                if new_earlyest_time < args.early_time2:
                                    new_epoch_solution.append(route)
                                    if global_log_info is True:
                                        log_info(f"ROUTE NOTICE--{new_earlyest_time} ", newline=False)

                        epoch_solution_id = new_epoch_solution
                    else:
                        epoch_solution_id = epoch_solution
                    # record coords of solution
                    coords_solution = [epoch_instance_dispatch_2['coords'][route] for route in epoch_solution_id]
                    # record must-go of solution
                    must_solution = [epoch_instance_dispatch_2['must_dispatch'][route] for route in epoch_solution_id]
                    epoch_solution = epoch_solution_id

                else:
                    epoch_solution = [epoch_instance_dispatch['request_idx'][route] for route in epoch_solution]

            if use_new_strategy and use_dyn == 1:
                # Map HGS solution to indices of corresponding requests
                epoch_solution = [epoch_instance_dispatch_2['request_idx'][route] for route in epoch_solution_id]
                # Distance of Each node
                num_requests_dispatched = sum([len(route) for route in epoch_solution])

            if num_requests_dispatched == 0:
                reward = 0
            else:
                reward = tools.validate_dynamic_epoch_solution(epoch_instance, epoch_solution)
            each_node_dis = reward / num_requests_dispatched if num_requests_dispatched != 0 else 0
            # min_each_dis = min(each_node_dis, min_each_dis)

            # Delete route without must-go
            # new_epoch_solution = []
            # for route in epoch_solution:
            #     if epoch_instance['must_dispatch'][route].any():
            #         new_epoch_solution.append(route)
            # epoch_solution = new_epoch_solution
            # epoch_solution =best_sol
        # plt
        # plt_vrp(observation, coords_solution, static_info['dynamic_context']['coords'],1, args)
        # Submit solution to environment
        observation, reward, done, info = env.step(epoch_solution)

        if global_log_info is True:
            num_requests_dispatched = sum([len(route) for route in epoch_solution])
            num_requests_open = len(epoch_instance['request_idx']) - 1
            num_requests_postponed = num_requests_open - num_requests_dispatched
            each_node_dis = -reward / num_requests_dispatched if num_requests_dispatched != 0 else 0
            log_info(
                f" {num_requests_dispatched:3d}/{num_requests_open:3d} dispatched and {num_requests_postponed:3d}/{num_requests_open:3d} postponed | Routes: {len(epoch_solution):2d} with cost {-reward:6d} "
                f"| Each node: {int(each_node_dis):4d}")

        # assert cost is None or reward == -cost, "Reward should be negative cost of solution"
        assert not info['error'], f"Environment error: {info['error']}"

        total_reward += reward

    if global_log_info is True:
        log_info(f"Cost of solution: {-total_reward}")
        # log_info(f"Cost of solution: {sum(env.final_costs.values())}")

    return total_reward


if __name__ == "__main__":

    parser = argparse.ArgumentParser()
    parser.add_argument("--strategy", type=str, default='lazy',
                        help="Baseline strategy used to decide whether to dispatch routes")
    # Note: these arguments are only for convenience during development, during testing you should use controller.py
    parser.add_argument("--instance", help="Instance to solve")
    parser.add_argument("--instance_seed", type=int, default=1, help="Seed to use for the dynamic instance")
    parser.add_argument("--solver_seed", type=int, default=1, help="Seed to use for the solver")
    parser.add_argument("--static", action='store_true',
                        help="Add this flag to solve the static variant of the problem (by default dynamic)")
    parser.add_argument("--epoch_tlim", type=int, default=20, help="Time limit per epoch")
    parser.add_argument("--oracle_tlim", type=int, default=120, help="Time limit for oracle")
    parser.add_argument("--tmp_dir", type=str, default=None,
                        help="Provide a specific directory to use as tmp directory (useful for debugging)")
    parser.add_argument("--verbose", action='store_true', help="Show verbose output")
    parser.add_argument("--rand_num", type=int, default=150, help="rand_num")
    parser.add_argument("--or_gap", type=float, default=94, help="or_gap")
    parser.add_argument("--x1", type=float, default=-7.018, help="x1")
    parser.add_argument("--x2", type=float, default=14.887, help="x2")
    parser.add_argument("--x3", type=float, default=-2.537, help="x3")
    parser.add_argument("--early_time1", type=int, default=1200, help="early_time1")
    parser.add_argument("--early_time2", type=int, default=-2000, help="early_time2")
    parser.add_argument("--gap", type=float, default=214, help="gap")
    parser.add_argument("--sol_x1", type=float, default=0, help="sol_x1")
    parser.add_argument("--sol_x2", type=float, default=3, help="sol_x2")
    parser.add_argument("--sol_x3", type=float, default=-0.15, help="sol_x3")
    parser.add_argument("--avg", type=int, default=2000, help="sol_x3")

    # liyunhao argv
    parser.add_argument("--config_str", default="+", help="config_str is needed")
    parser.add_argument("--run_tag", default="notag", help="run_tag")

    args = parser.parse_args()

    if args.tmp_dir is None:
        # Generate random tmp directory
        args.tmp_dir = os.path.join("tmp", str(uuid.uuid4()))
        log_info(args.tmp_dir)
        cleanup_tmp_dir = True
    else:
        # If tmp dir is manually provided, don't clean it up (for debugging)
        cleanup_tmp_dir = False

    try:
        h = time.strftime("%H", time.localtime())
        m = time.strftime("%M", time.localtime())
        s = time.strftime("%S", time.localtime())
        args.solver_seed = int(h) * 3600 + int(m) * 60 + int(s)

        if args.instance is not None:
            env = VRPEnvironment(seed=args.instance_seed, instance=tools.read_vrplib(args.instance),
                                 epoch_tlim=args.epoch_tlim, is_static=args.static)
            args_dic = vars(args).copy()
        else:
            assert args.strategy != "oracle", "Oracle can not run with external controller"
            # Run within external controller
            env = ControllerEnvironment(sys.stdin, sys.stdout)
            args_dic = {}

        # Make sure these parameters are not used by your solver
        args.instance = None
        args.instance_seed = None
        args.static = None
        args.epoch_tlim = None

        if args.strategy == 'oracle':
            run_oracle(args, env)
        else:
            re_all = run_baseline(args, env)
            log_info('zjj_re,'+str(re_all))
        if args.instance is not None:
            log_info(tools.json_dumps_np(env.final_solutions))

        if "instance" in args_dic:

            result_dic = args_dic
            ymd = time.strftime("%m_%d", time.localtime())
            # log_info(f"ymd:{ymd}")
            result_dic["cost"] = -re_all
            result_dic["route_mum"] = 0
            for key, value in env.final_solutions.items():
                result_dic["route_mum"] += len(value)

            result_dic["instance"] = result_dic["instance"].replace("instances/", "")
            result_dic["solution"] = tools.json_dumps_np(env.final_solutions)
            result_dic["epoch_num"] = len(env.final_solutions)
            is_static = args_dic["static"]
            run_type = "static" if is_static else "dynamic"
            strategy = args_dic["strategy"]
            run_tag = args_dic["run_tag"]
            customers_num = result_dic["instance"].split("-")[5].replace("n", "")
            result_dic["customers_num"] = customers_num

            save_results_csv(f"./results/[{ymd}][{run_type}][{run_tag}].csv", result_dic)
            log_info(tools.json_dumps_np(env.final_solutions))
    finally:
        if cleanup_tmp_dir:
            tools.cleanup_tmp_dir(args.tmp_dir)

