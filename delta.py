import numpy as np

class Mydelta:

    def __init__(self, all_instance, seed, k=50):
        # Choose N nodes randomly in ALL intance
        self.all = all_instance
        self.seed = seed
        self.rng = np.random.default_rng(self.seed)
        self.k = k

        # Sample uniformly
        num_customers = len(self.all['coords']) - 1  # Exclude depot

        # Sample data uniformly from customers (1 to num_customers)
        def sample_from_customers(k=self.k):
            return self.rng.integers(num_customers, size=k) + 1

        k_nodes = sample_from_customers()  # customer IDs
        self.n_nodes = np.append(k_nodes, 0)  # add depot

        self.x_delta = (num_customers+1) * [-10e9]

    def cul_delta(self, customer_id):
        if self.x_delta[customer_id] > -10e8:
            return self.x_delta[customer_id]
        distance_customer_id = self.all['duration_matrix'][self.n_nodes, customer_id]
        distance_index_sort = np.argsort(distance_customer_id)
        # dis_a = self.all['duration_matrix'][self.n_nodes[distance_index_sort[0]], customer_id]
        # dis_b = self.all['duration_matrix'][self.n_nodes[distance_index_sort[1]], customer_id]
        # dis_c = self.all['duration_matrix'][self.n_nodes[distance_index_sort[0]], self.n_nodes[distance_index_sort[1]]]
        # self.x_delta[customer_id] = dis_a + dis_b - dis_c
        temp_d = 0
        repeat_time = 3
        temp_min = 10000000
        for i in range(1, repeat_time+1):

            dis_a = self.all['duration_matrix'][self.n_nodes[distance_index_sort[0]], customer_id]
            dis_b = self.all['duration_matrix'][self.n_nodes[distance_index_sort[i]], customer_id]
            dis_c = self.all['duration_matrix'][self.n_nodes[distance_index_sort[0]], self.n_nodes[distance_index_sort[i]]]
            temp_d += dis_a + dis_b - dis_c
            temp_min = min(temp_min, dis_a + dis_b - dis_c)
        self.x_delta[customer_id] = temp_d/repeat_time
        self.x_delta[customer_id] = temp_min
        return self.x_delta[customer_id]

    def cul_delta_three_nodes(self, customer, customer_before, customer_after):
        dis_a = self.all['duration_matrix'][customer_before, customer]
        dis_b = self.all['duration_matrix'][customer, customer_after]
        dis_c = self.all['duration_matrix'][customer_before, customer_after]
        return dis_a + dis_b - dis_c

    def reset_rand_num(self, k):
        self.k = k

        # Sample uniformly
        num_customers = len(self.all['coords']) - 1  # Exclude depot

        # Sample data uniformly from customers (1 to num_customers)
        def sample_from_customers(k=self.k):
            return self.rng.integers(num_customers, size=k) + 1

        k_nodes = sample_from_customers()  # customer IDs
        self.n_nodes = np.append(k_nodes, 0)  # add depot

        self.x_delta = (num_customers + 1) * [-10e9]
