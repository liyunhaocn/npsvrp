# Solver for Dynamic VRPTW, baseline strategy is to use the static solver HGS-VRPTW repeatedly
import argparse
import csv
import math
import subprocess
import sys
import os
import time
import platform
import numpy as np

import tools
from environment import VRPEnvironment, ControllerEnvironment
from strategies import STRATEGIES

global_log_info = False
global_save_current_instance = False
global_log_error = True

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
                assert len(weight) == instance['coords'].shape[0] , "len(weight) == instance['coords'].shape[0]"
                yield weight

def solve_static_vrptw(instance, time_limit=3600, seed=1, initial_solution=None, weight_arg=[], configKind = ""):
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

    # cmd += ["-call", "smartOnly"]
    cmd += ["-call", "hgsAndSmart"]
    cmd += ["-configKind", configKind]
    cmd_str = " ".join(cmd)
    log_info(f"cmd_str:{cmd_str}")

    if initial_solution is None:
        initial_solution = [[i] for i in range(1, instance['coords'].shape[0])]
    if initial_solution is not None:
        cmd += ['-initialSolution', " ".join(map(str, tools.to_giant_tour(initial_solution)))]

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
    oracle_solution = \
    min(solve_static_vrptw(hindsight_problem, time_limit=args.oracle_tlim, initial_solution=greedy_solution),
        key=lambda x: x[1])[0]
    oracle_cost = tools.validate_static_solution(hindsight_problem, oracle_solution)
    log_info(f"Found oracle solution with cost {oracle_cost}")

    # Run oracle solution through environment (note: will reset environment again with same seed)
    total_reward = run_baseline(args, env, oracle_solution=oracle_solution)
    assert -total_reward == oracle_cost, "Oracle solution does not match cost according to environment"
    return total_reward


def run_baseline(args, env, oracle_solution=None, strategy=None, seed=None):
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
            log_info(f"Epoch {static_info['start_epoch']} <= {observation['current_epoch']} <= {static_info['end_epoch']}",
                newline=False)
            num_requests_open = len(epoch_instance['request_idx']) - 1
            num_new_requests = num_requests_open - num_requests_postponed
            log_info(f" | Requests: +{num_new_requests:3d} = {num_requests_open:3d}, {epoch_instance['must_dispatch'].sum():3d}/{num_requests_open:3d} must-go...",
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

            solutions = list(solve_static_vrptw(instance=epoch_instance_dispatch,
                                                time_limit=epoch_tlim-time_get_weight,
                                                seed=args.solver_seed,
                                                weight_arg=request_weight,
                                                configKind=args.solver_configKind
                                                ))
            assert len(solutions) > 0, f"No solution found during epoch {observation['current_epoch']}"
            epoch_solution, cost = solutions[-1]

            # Map HGS solution to indices of corresponding requests
            epoch_solution = [epoch_instance_dispatch['request_idx'][route] for route in epoch_solution]

        if global_log_info is True:
            num_requests_dispatched = sum([len(route) for route in epoch_solution])
            num_requests_open = len(epoch_instance['request_idx']) - 1
            num_requests_postponed = num_requests_open - num_requests_dispatched
            log_info(f" {num_requests_dispatched:3d}/{num_requests_open:3d} dispatched and {num_requests_postponed:3d}/{num_requests_open:3d} postponed | Routes: {len(epoch_solution):2d} with cost {cost:6d}")

        # Submit solution to environment
        observation, reward, done, info = env.step(epoch_solution)
        assert cost is None or reward == -cost, "Reward should be negative cost of solution"
        assert not info['error'], f"Environment error: {info['error']}"

        total_reward += reward

    if global_log_info is True:
        log_info(f"Cost of solution: {-total_reward}")

    return total_reward


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

    fieldnames = ["instance", "customers_num", "cost", "route_mum", "epoch_num", "strategy", "solver_seed", "static", "epoch_tlim", "solution"]
    file_exist = os.path.exists(csv_path)
    # print(f"fieldnames:{fieldnames}")
    with open(csv_path, 'a', encoding='UTF8', newline='') as f:
        writer = csv.writer(f)
        if not file_exist:
            writer.writerow(fieldnames)
        data_replace_order = []
        for key in fieldnames:
            data_replace_order.append(data_dic[key])
        # print(f"data_replace_order:{data_replace_order}")
        writer.writerow(data_replace_order)

# --instance ./instances/ORTEC-VRPTW-ASYM-2e2ef021-d1-n210-k17.txt --static --epoch_tlim 164 --run_tag notag --verbose
if __name__ == "__main__":

    parser = argparse.ArgumentParser()
    parser.add_argument("--strategy", type=str, default='greedy',
                        help="Baseline strategy used to decide whether to dispatch routes")
    # Note: these arguments are only for convenience during development, during testing you should use controller.py
    parser.add_argument("--instance", help="Instance to solve")
    parser.add_argument("--instance_seed", type=int, default=1, help="Seed to use for the dynamic instance")
    parser.add_argument("--solver_seed", type=int, default=1, help="Seed to use for the solver")
    parser.add_argument("--static", action='store_true',
                        help="Add this flag to solve the static variant of the problem (by default dynamic)")
    parser.add_argument("--epoch_tlim", type=int, default=120, help="Time limit per epoch")
    parser.add_argument("--oracle_tlim", type=int, default=120, help="Time limit for oracle")
    # parser.add_argument("--tmp_dir", type=str, default=None, help="Provide a specific directory to use as tmp directory (useful for debugging)")
    # parser.add_argument("--model_path", type=str, default=None, help="Provide the path of the machine learning model to be used as strategy (Path must not contain `model.pth`)")
    parser.add_argument("--verbose", action='store_true', help="Show verbose output")
    parser.add_argument("--run_tag", default="", help="Show verbose output")
    parser.add_argument("--solver_configKind", default="", help="Show verbose output")
    args = parser.parse_args()

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
            strategy = STRATEGIES[args.strategy]
            # now.strftime("%Y-%m-%d, ")
            reward = run_baseline(args, env, strategy=strategy)

        if "instance" in args_dic:

            result_dic = args_dic
            ymd = time.strftime("%m_%d", time.localtime())
            # print(f"ymd:{ymd}")
            result_dic["cost"] = -reward
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
        pass
