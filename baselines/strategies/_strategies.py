import numpy as np
from environment import State


def _filter_instance(observation: State, mask: np.ndarray):
    res = {}

    for key, value in observation.items():
        if key == 'capacity':
            res[key] = value
            continue

        if key == 'duration_matrix':
            res[key] = value[mask]
            res[key] = res[key][:, mask]
            continue

        res[key] = value[mask]

    return res


def _greedy(observation: State, rng: np.random.Generator):
    return {
        **observation,
        'must_dispatch': np.ones_like(observation['must_dispatch']).astype(np.bool8)
    }

def _greedy_new(observation: State, rng: np.random.Generator):
    mask = np.copy(observation['must_dispatch'])
    mask[0] = True
    for i in range(mask.shape[0]):
        mask[i] = True
    return _filter_instance(observation, mask)


def _lazy(observation: State, rng: np.random.Generator):
    mask = np.copy(observation['must_dispatch'])
    mask[0] = True
    return _filter_instance(observation, mask)


def _random(observation: State, rng: np.random.Generator):
    mask = np.copy(observation['must_dispatch'])
    mask = (mask | rng.binomial(1, p=0.5, size=len(mask)).astype(np.bool8))
    mask[0] = True
    # new_mask = (mask | rng.binomial(1, p=0.5, size=len(mask)).astype()

    return _filter_instance(observation, mask)


def weight_of_nodes(observation,max_epoch=5):
    w = []
    x0 = 0.0
    x1 = 0.6
    x2 = 0.1
    max_epoch0 = max_epoch
    max_epoch = max_epoch + 5
    max_demand = max(observation['demands'])
    w_demands = (max_demand - observation['demands']) * x0 / max_demand / 2

    # self.request_must_dispatch = (planning_starttime + self.EPOCH_DURATION + duration_matrix[0, self.request_customer_index] > self.request_timewi[:, 1]
    w_time_windows_0 = (observation['time_windows'][:, 0] - observation['duration_matrix'][0, :]) / 3600
    w_time_windows_1 = [max(min(i, max_epoch0), 0) * x1 / max_epoch0 / 2 for i in w_time_windows_0]
    w_time_windows_open = np.array(w_time_windows_1)

    w_time_windows_0 = (observation['time_windows'][:, 1] - observation['duration_matrix'][0, :]) / 3600
    w_time_windows_1 = [max(min(i, max_epoch), 0) * x2 / max_epoch / 2 for i in w_time_windows_0]
    w_time_windows_end = np.array(w_time_windows_1)

    w = (w_demands + w_time_windows_open + w_time_windows_end) / 3
    return w

def _weight(observation: State, rng: np.random.Generator, max_epoch):
    mask = np.copy(observation['must_dispatch'])
    # rand p of nodes
    rand_p = rng.random(size=len(mask))
    # probability of deleting nodes
    p = weight_of_nodes(observation, max_epoch)
    mask = (mask | (p<rand_p).astype(np.bool8))
    # mask = (mask | rng.binomial(1, p=0.5, size=len(mask)).astype(np.bool8))
    mask[0] = True
    # new_mask = (mask | rng.binomial(1, p=0.5, size=len(mask)).astype()

    return _filter_instance(observation, mask)

def _new(observation: State, rng: np.random.Generator, sol):
    mask = np.copy(observation['must_dispatch'])
    for route in sol:
        for i in route:
            mask[i] = True
    mask[0] = True
    # print(np.sum(mask))
    return _filter_instance(observation, mask)


def _smart(observation: State, rng: np.random.Generator, env_virtual):

    epoch_instance = observation
    observation, static_info = epoch_instance.pop('observation'), epoch_instance.pop('static_info')

    # print()
    # for key, value in observation['epoch_instance'].items():
    #     print(key)

    mask = np.copy(epoch_instance['must_dispatch'])
    mask[0] = True

    if (static_info['is_static'] == True) or (np.all(mask)):
        return _filter_instance(epoch_instance, mask)

    # add to cross-reference
    import sys
    import os
    sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    from solver import solve_static_vrptw

    # Constant
    EPOCH_DURATION = 3600
    MARGIN_DISPATCH = 3600

    # Param
    WAIT_TIME_LIMIT = 3600
    epoch_num = 2
    test_num = 5
    per_5 = np.array([0.005, 0.2, 0.5, 0.7, 0.9, 1])
    per_5_cut = np.array([0, 0, 0, 1, 1, 1])
    per = np.array([0.0, 0.2, 0.25, 0.33, 0.45, 0.57, 0.73, 0.85, 0.94, 0.98, 1])
    per_cut = np.array([0.0, 0.0, 0.0, 0.0, 1, 1, 1, 1, 1, 1, 1])
    current_epoch = observation['current_epoch']

    # Pre calc
    origin_num = len(epoch_instance['request_idx'])
    planning_starttime = EPOCH_DURATION * current_epoch + MARGIN_DISPATCH
    current_num = np.zeros(origin_num, dtype=int)

    distance_delta_all = None
    route_pass = []

    cnt = 0
    while cnt < test_num:
        seed = rng.integers(100000)
        # seed = 1

        env_virtual.import_info(epoch_instance=epoch_instance, virtual_epoch=current_epoch, seed=seed)
        ins = env_virtual.step_num(epoch_num)

        solutions = list(solve_static_vrptw(ins, time_limit=7, seed=seed))
        epoch_solution, cost = solutions[-1]
        route_dispatch_epoch_max = [max(ins['release_epochs'][route]) for route in epoch_solution]

        # for route, route_epoch in zip(epoch_solution, route_dispatch_epoch):
        #     print(route, route_epoch)
        # print(route_dispatch_epoch_max)

        # delay judge
        for route, route_epoch in zip(epoch_solution, route_dispatch_epoch_max):
            if (route_epoch > current_epoch):
                continue
            print('now route : ', route, file=f)
            # calc latest start time
            depot = 0  # For readability, define variable
            earliest_start_depot, latest_arrival_depot = ins['time_windows'][depot]
            current_time = planning_starttime
            longest_wait = latest_arrival_depot

            prev_stop = depot
            for stop in route:
                earliest_arrival, latest_arrival = ins['time_windows'][stop]
                arrival_time = current_time + ins['duration_matrix'][prev_stop, stop]
                longest_wait = min(longest_wait, latest_arrival - arrival_time)
                current_time = max(arrival_time, earliest_arrival)
                current_time += ins['service_times'][stop]
                prev_stop = stop
            current_time += ins['duration_matrix'][prev_stop, depot]
            longest_wait = min(longest_wait, latest_arrival_depot - current_time)

            print('longest_wait : ', longest_wait, file=f)
            # if (longest_wait >= WAIT_TIME_LIMIT):
            #    continue

            route_pass.append(route)
            current_num[route] += 1
        print(current_num, file=f)

        # distance_delta info

        # import math
        # def get_distance(a, b):
        #     return math.sqrt((a[0] - b[0]) ** 2 + (a[1] - b[1]) ** 2)

        # distance_delta = np.zeros(origin_num)
        # for route in epoch_solution:
        #     route.insert(0, 0)
        #     #print(route)
        #     for i in range(0, len(route)):
        #         idx = route[i]
        #         if idx >= origin_num or idx == 0:
        #             continue
        #         idx_lst, idx_nxt = route[i - 1], route[(i + 1) % len(route)]
        #         distance_delta[idx] = get_distance(ins['coords'][idx], ins['coords'][idx_lst]) + get_distance(ins['coords'][idx], ins['coords'][idx_nxt]) - get_distance(ins['coords'][idx_lst], ins['coords'][idx_nxt])
        # distance_delta_all = np.array([distance_delta]) if distance_delta_all is None else np.vstack((distance_delta_all, distance_delta))

        cnt += 1
    if len(route_pass) > 0:
        route_value = [current_num[route].sum() / len(route) for route in route_pass]

        value_order = np.array(route_value).argsort()[::-1]

        flag = np.zeros(origin_num)

        for order_idx in value_order:
            value, route = route_value[order_idx], route_pass[order_idx]
            # print(value, '  ', route, file = f)
            if value < 3.5:
                break
            if flag[route].any():
                continue
            for idx in route:
                flag[idx] = True
                # if ins['time_windows'][idx,1] > planning_starttime + 3600 * 4 and value < 4:
                #     print('pass', file = f)
                #     continue
                mask[idx] = True

    # print(route_pass)
    # print(route_value)

    # prob = current_num / test_num
    # prob = per[current_num]
    prob = per_5_cut[current_num]

    mask = (mask | rng.binomial(1, p=prob, size=len(mask)).astype(np.bool8))
    mask[0] = True

    print(mask, file=f)

    # while True:
    #     x = 1

    # test runtime
    # print()
    # print()
    # tim = [25]
    # tim = [5, 10, 20, 30, 60, 90, 120]
    # tim = [10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20]
    # for t in tim:
    #     print('epoch : ', epoch_num, end = '\t')
    #     print('time : ', t, end = '\t')
    #     solutions = list(solve_static_vrptw(ins, time_limit=t))
    #     turn = len(solutions)
    #     final_solution = solutions[-1]
    #     val = final_solution[-1]
    #     print('turn : ', turn, '\t', 'val : ', val)

    # print(mask)

    return _filter_instance(epoch_instance, mask)


STRATEGIES = dict(
    greedy=_greedy,
    lazy=_lazy,
    random=_random,
    weight=_weight,
    new=_new,
    greedy_new=_greedy_new,
    smart=_smart
)

