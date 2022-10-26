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


STRATEGIES = dict(
    greedy=_greedy,
    lazy=_lazy,
    random=_random,
    weight=_weight,
    new=_new,
    greedy_new=_greedy_new
)

