#include <algorithm>
#include <set>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <cstdio>
#include <limits.h>

#include "Params.h"
#include "Matrix.h"
#include "xorshift128.h"
#include "commandline.h"
#include "CircleSector.h"

//#pragma warning(disable:4996)


Params::Params(const CommandLine& cl)
{
	// Read and create some parameter values from the commandline
	config = cl.config;
	nbVehicles = config.nbVeh;
	rng = SmartRandomGenerator(config.seed);
	startWallClockTime = std::chrono::system_clock::now();
	startCPUTime = std::clock();

	// Convert the circle sector parameters from degrees ([0,359]) to [0,65535] to allow for faster calculations
	circleSectorOverlapTolerance = static_cast<int>(config.circleSectorOverlapToleranceDegrees / 360. * 65536);
	minCircleSectorSize = static_cast<int>(config.minCircleSectorSizeDegrees / 360. * 65536);

	// Initialize some parameter values
	std::string content, content2, content3;
	int serviceTimeData = 0;
	int node;
	bool hasServiceTimeSection = false;
	nbClients = 0;
	totalDemand = 0;
	maxDemand = 0;
	durationLimit = INT_MAX;
	vehicleCapacity = INT_MAX;
	isDurationConstraint = false;
	nbMustDispatch = -1;

	// Read INPUT dataset
	//std::ifstream inputFile(config.pathInstance);
	//INFO("cl.config.isNpsRun:", cl.config.isNpsRun);
	if (config.pathInstance == "readstdin") {
	}else{
        auto x = freopen(cl.config.pathInstance.data(), "r", stdin);
        if(x== nullptr){
            throw std::string("x== nullptr = freopen");
        }
    }
	
	if (true)
	{
		// Read the instance name from the first line and remove \r
		getline(std::cin, content);
		
		instanceName = content;
		instanceName.erase( std::remove(instanceName.begin(), instanceName.end(), '\r'), instanceName.end());

		// Read the next lines
		getline(std::cin, content);	// "Empty line" or "NAME : {instance_name}"
		getline(std::cin, content);	// VEHICLE or "COMMENT: {}"

		// Check if the next line has "VEHICLE"
		if (content.substr(0, 7) == "VEHICLE")
		{
			// VRPTW format
			isTimeWindowConstraint = true;

			// Get the number of vehicles and the capacity of the vehicles
			getline(std::cin, content);  // NUMBER    CAPACITY
			std::cin >> nbVehicles >> vehicleCapacity;

			// Skip the next four lines
			getline(std::cin, content);
			getline(std::cin, content);
			getline(std::cin, content);
			getline(std::cin, content);

			// Create a vector where all information on the Clients can be stored and loop over all information in the file
			cli = std::vector<Client>(1001);

			nbClients = 0;
			while (std::cin >> node)
			{
				// Store all the information of the next client
				cli[nbClients].custNum = node;
				std::cin >> cli[nbClients].coordX >> cli[nbClients].coordY >> cli[nbClients].demand >> cli[nbClients].earliestArrival >> cli[nbClients].latestArrival >> cli[nbClients].serviceDuration;
				
				// Scale coordinates by factor 10, later the distances will be rounded so we optimize with 1 decimal distances
				cli[nbClients].coordX *= 10;
				cli[nbClients].coordY *= 10;
				cli[nbClients].earliestArrival *= 10;
				cli[nbClients].latestArrival *= 10;
				cli[nbClients].serviceDuration *= 10;
				cli[nbClients].polarAngle = CircleSector::positive_mod(static_cast<int>(32768. * atan2(cli[nbClients].coordY - cli[0].coordY, cli[nbClients].coordX - cli[0].coordX) / PI));
				
				// Keep track of the max demand, the total demand, and the number of clients
				if (cli[nbClients].demand > maxDemand)
				{
					maxDemand = cli[nbClients].demand;
				}
				totalDemand += cli[nbClients].demand;
				nbClients++;
			}

			// Reduce the size of the vector of clients if possible
			cli.resize(nbClients);

			// Don't count depot as client
			--nbClients;

			// Check if the required service and the start of the time window of the depot are both zero
			if (cli[0].earliestArrival != 0)
			{
				throw std::string("Time window for depot should start at 0");
			}
			if (cli[0].serviceDuration != 0)
			{
				throw std::string("Service duration for depot should be 0");
			}
		}
		else
		{
			// CVRP or VRPTW according to VRPLib format
			for (std::cin >> content; content != "EOF"; std::cin >> content)
			{
				// Read the dimension of the problem (the number of clients)
				if (content == "DIMENSION")
				{
					// Need to substract the depot from the number of nodes
					std::cin >> content2 >> nbClients;
					nbClients--;
                    P.resize(nbClients+1,1);
                }
				// Read the type of edge weights
				else if (content == "EDGE_WEIGHT_TYPE")
				{
					std::cin >> content2 >> content3;
					if (content3 == "EXPLICIT")
					{
						isExplicitDistanceMatrix = true;
					}
				}
				else if (content == "EDGE_WEIGHT_FORMAT")
				{
					std::cin >> content2 >> content3;
					if (!isExplicitDistanceMatrix)
					{
						throw std::string("EDGE_WEIGHT_FORMAT can only be used with EDGE_WEIGHT_TYPE : EXPLICIT");
					}

					if (content3 != "FULL_MATRIX")
					{
						throw std::string("EDGE_WEIGHT_FORMAT only supports FULL_MATRIX");
					}
				}
				else if (content == "CAPACITY")
				{
					std::cin >> content2 >> vehicleCapacity;
				}
				else if (content == "VEHICLES" || content == "SALESMAN")
				{
                    // Set vehicle count from instance only if not specified on CLI.
					std::cin >> content2;
                    if(nbVehicles == INT_MAX) {
						std::cin >> nbVehicles;
                    } else {
                        // Discard vehicle count
                        int _;
						std::cin >> _;
                    }
				}
				else if (content == "DISTANCE")
				{
					std::cin >> content2 >> durationLimit; isDurationConstraint = true;
				}
				// Read the data on the service time (used when the service time is constant for all clients)
				else if (content == "SERVICE_TIME")
				{
					std::cin >> content2 >> serviceTimeData;
				}
				// Read the edge weights of an explicit distance matrix
				else if (content == "EDGE_WEIGHT_SECTION")
				{
					if (!isExplicitDistanceMatrix)
					{
						throw std::string("EDGE_WEIGHT_SECTION can only be used with EDGE_WEIGHT_TYPE : EXPLICIT");
					}
					maxDist = 0;
					timeCost = Matrix(nbClients + 1);
					for (int i = 0; i <= nbClients; i++)
					{
						for (int j = 0; j <= nbClients; j++)
						{
							// Keep track of the largest distance between two clients (or the depot)
							int cost;
							std::cin >> cost;
							if (cost > maxDist)
							{
								maxDist = cost;
							}
							timeCost.set(i, j, cost);
						}
					}
				}
				else if (content == "NODE_COORD_SECTION")
				{
					// Reading client coordinates
					cli = std::vector<Client>(nbClients + 1);
					for (int i = 0; i <= nbClients; i++)
					{
						std::cin >> cli[i].custNum >> cli[i].coordX >> cli[i].coordY;
						
						// Check if the clients are in order
						if (cli[i].custNum != i + 1)
						{
							throw std::string("Clients are not in order in the list of coordinates")
                            + "cli[i].custNum:"+ std::to_string(cli[i].custNum)
                            + "i + 1:" + std::to_string(i + 1);
						}
						cli[i].custNum--;
						cli[i].polarAngle = CircleSector::positive_mod(static_cast<int>(32768. * atan2(cli[i].coordY - cli[0].coordY, cli[i].coordX - cli[0].coordX) / PI));
					}
                    // 将所有的顾客都置为必须配送
                    for (int i = 0; i <= nbClients; i++){
                        cli[i].must_dispatch = 1;
                    }
                    nbMustDispatch = nbClients;
				}
				// Read the demand of each client (including the depot, which should have demand 0)
				else if (content == "DEMAND_SECTION")
				{
					for (int i = 0; i <= nbClients; i++)
					{
						int clientNr = 0;
						std::cin >> clientNr >> cli[i].demand;

						// Check if the clients are in order
						if (clientNr != i + 1)
						{
							throw std::string("Clients are not in order in the list of demands");
						}

						// Keep track of the max and total demand
						if (cli[i].demand > maxDemand)
						{
							maxDemand = cli[i].demand;
						}
						totalDemand += cli[i].demand;
					}
					// Check if the depot has demand 0
					if (cli[0].demand != 0)
					{
						throw std::string("Depot demand is not zero, but is instead: " + std::to_string(cli[0].serviceDuration));
					}
				}
				else if (content == "DEPOT_SECTION")
				{
					std::cin >> content2 >> content3;
					if (content2 != "1")
					{
						throw std::string("Expected depot index 1 instead of " + content2);
					}
				}
                else if (content == "CUSTOMER_WEIGHT")
                {
                    for (int i = 0; i <= nbClients; ++i)
                    {
                        int clientNr = 0;
                        std::cin >> clientNr >> P[i];
                        // Check if the clients are in order
                        if (clientNr != i + 1)
                        {
                            throw std::string("Clients are not in order in the list of CUSTOMER_WEIGHT")
                            + "clientNr:"+ std::to_string(clientNr)
                            + "i + 1:" + std::to_string(i + 1);
                        }
                    }
                    // Check if the service duration of the depot is 0
                    if (P[0] != 0)
                    {
                        throw std::string("P[0] should be 0");
                    }
                }
				else if (content == "MUST_DISPATCH")
				{
					nbMustDispatch = 0;
					for (int i = 0; i <= nbClients; i++)
					{
						int clientNr = 0;
						std::cin >> clientNr >> cli[i].must_dispatch;
						if (cli[i].must_dispatch == 1) {
							++nbMustDispatch;
						}
						// Check if the clients are in order
						if (clientNr != i + 1)
						{
							throw std::string("Clients are not in order in the list of MUST_DISPATCH");
						}
					}
					// Check if the service duration of the depot is 0
					if (cli[0].must_dispatch != 0)
					{
						throw std::string("must_dispatch depot should be 0");
					}
				}
				else if (content == "SERVICE_TIME_SECTION")
				{
					for (int i = 0; i <= nbClients; i++)
					{
						int clientNr = 0;
						std::cin >> clientNr >> cli[i].serviceDuration;

						// Check if the clients are in order
						if (clientNr != i + 1)
						{
							throw std::string("Clients are not in order in the list of service times");
						}
					}
					// Check if the service duration of the depot is 0
					if (cli[0].serviceDuration != 0)
					{
						throw std::string("Service duration for depot should be 0");
					}
					hasServiceTimeSection = true;
				}
				else if (content == "RELEASE_TIME_SECTION")
				{
					for (int i = 0; i <= nbClients; i++)
					{
						int clientNr = 0;
						std::cin >> clientNr >> cli[i].releaseTime;

						// Check if the clients are in order
						if (clientNr != i + 1)
						{
							throw std::string("Clients are not in order in the list of release times");
						}
					}
					// Check if the service duration of the depot is 0
					if (cli[0].releaseTime != 0)
					{
						throw std::string("Release time for depot should be 0");
					}
				}
				// Read the time windows of all the clients (the depot should have a time window from 0 to max)
				else if (content == "TIME_WINDOW_SECTION")
				{
					isTimeWindowConstraint = true;
					for (int i = 0; i <= nbClients; i++)
					{
						int clientNr = 0;
						std::cin >> clientNr >> cli[i].earliestArrival >> cli[i].latestArrival;

						// Check if the clients are in order
						if (clientNr != i + 1)
						{
                            ERROR("Clients are not in order in the list of time windows");
							throw std::string("Clients are not in order in the list of time windows");
						}
					}

					// Check the start of the time window of the depot
					if (cli[0].earliestArrival != 0)
					{
                        ERROR("Time window for depot should start at 0");
                        throw std::string("Time window for depot should start at 0");
					}
				}
				else
				{
                    ERROR("Unexpected data in input file: " + content);
					throw std::string("Unexpected data in input file: " + content);
				}
			}

			if (!hasServiceTimeSection)
			{
				for (int i = 0; i <= nbClients; i++)
				{
					cli[i].serviceDuration = (i == 0) ? 0 : serviceTimeData;
				}
			}

			if (nbClients <= 0)
			{
				throw std::string("Number of nodes is undefined");
			}
			if (vehicleCapacity == INT_MAX)
			{
				throw std::string("Vehicle capacity is undefined");
			}
		}
		
		if (static_cast<int>(cli.size()) < nbClients * 3 + 3) {
			cli.resize(nbClients * 3 + 3);
		}
		for (int i = nbClients + 1; i < static_cast<int>(cli.size()); ++i) {
			cli[i] = cli[0];
			cli[i].custNum = i;
		}
		
		//ERROR("nbMustDispatch:",nbMustDispatch);
	}
	else {
		throw std::invalid_argument("Impossible to open instance file: " + config.pathInstance);
	}

	// Default initialization if the number of vehicles has not been provided by the user
	if (nbVehicles == INT_MAX)
	{
		// Safety margin: 30% + 3 more vehicles than the trivial bin packing LB
		nbVehicles = static_cast<int>(std::ceil(1.3 * totalDemand / vehicleCapacity) + 3.);
		INFO("----- FLEET SIZE WAS NOT SPECIFIED: DEFAULT INITIALIZATION TO ",nbVehicles ," VEHICLES");
	}
	else if (nbVehicles == -1)
	{
		nbVehicles = nbClients;
		INFO("----- FLEET SIZE UNLIMITED: SET TO UPPER BOUND OF ", nbVehicles, " VEHICLES");
	}
	else
	{
		INFO("----- FLEET SIZE SPECIFIED IN THE COMMANDLINE: SET TO ", nbVehicles, " VEHICLES");
	}

	// If the run is a DIMACS run, store the solution in the current folder
	if (config.isDimacsRun)
	{
		config.pathSolution = instanceName + ".sol";
		std::cout << "DIMACS RUN for instance name " << instanceName << ", writing solution to " << config.pathSolution << std::endl;
	}

	// For DIMACS runs, or when dynamic parameters have to be used, set more parameter values
	if (config.isDimacsRun || config.useDynamicParameters)
	{
		// Determine categories of instances based on number of stops/route and whether it has large time windows
		// Calculate an upper bound for the number of stops per route based on capacities
		double stopsPerRoute = vehicleCapacity / (totalDemand / nbClients);
		// Routes are large when more than 25 stops per route
		bool hasLargeRoutes = stopsPerRoute > 20;
		// Get the time horizon (by using the time window of the depot)
		int horizon = cli[0].latestArrival - cli[0].earliestArrival;
		int nbLargeTW = 0;

		// Loop over all clients (excluding the depot) and count the amount of large time windows (greater than 0.7*horizon)
		for (int i = 1; i <= nbClients; i++)
		{
			if (cli[i].latestArrival - cli[i].earliestArrival > 0.7 * horizon)
			{
				nbLargeTW++;
			}
		}
		// Output if an instance has large routes and a large time window
		bool hasLargeTW = nbLargeTW > 0;
		std::cout << "----- HasLargeRoutes: " << hasLargeRoutes << ", HasLargeTW: " << hasLargeTW << std::endl;
		
		// Set the parameter values based on the characteristics of the instance
		if (hasLargeRoutes)
		{
			config.nbGranular = 40;
			// Grow neighborhood and population size
			config.growNbGranularAfterIterations = 10000;
			//config.growNbGranularAfterIterations = 2000;
			config.growNbGranularSize = 5;
			config.growPopulationAfterIterations = 10000;
			//config.growPopulationAfterIterations = 2000;
			config.growPopulationSize = 5;
			// Intensify occasionally
			config.intensificationProbabilityLS = 15;
		}
		else
		{
			// Grow population size only
			// config.growNbGranularAfterIterations = 10000;
			// config.growNbGranularSize = 5;
			if (hasLargeTW)
			{
				// Smaller neighbourhood so iterations are faster
				// So take more iterations before growing population
				config.nbGranular = 20;
				config.growPopulationAfterIterations = 20000;
			}
			else
			{
				config.nbGranular = 40;
				config.growPopulationAfterIterations = 10000;
			}
			config.growPopulationSize = 5;
			// Intensify always
			config.intensificationProbabilityLS = 100;
		}
	}

	
	if (!isExplicitDistanceMatrix)
	{
		// Calculation of the distance matrix
		maxDist = 0;
		timeCost = Matrix(nbClients + 1);
		// Loop over all clients (including the depot)
		for (int i = 0; i <= nbClients; i++)
		{
			// Set the diagonal element to zero (travel to itself)
			timeCost.set(i, i, 0);
			// Loop over all other clients
			for (int j = i + 1; j <= nbClients; j++)
			{
				// Calculate Euclidian distance d
				double d = std::sqrt((cli[i].coordX - cli[j].coordX) * (cli[i].coordX - cli[j].coordX) + (cli[i].coordY - cli[j].coordY) * (cli[i].coordY - cli[j].coordY));
				// Integer truncation
				int cost = static_cast<int>(d);
				// Keep track of the max distance
				if (cost > maxDist)
				{
					maxDist = cost;
				}
				// Save the distances in the matrix
				timeCost.set(i, j, cost);
				timeCost.set(j, i, cost);
			}
		}
	}

	// Compute order proximities once
	orderProximities = std::vector<std::vector<std::pair<double, int>>>(nbClients + 1);
	// Loop over all clients (excluding the depot)
	for (int i = 0; i <= nbClients; i++)
	{
		// Remove all elements from the vector
		auto& orderProximity = orderProximities[i];
		orderProximity.clear();

		// Loop over all clients (excluding the depot and the specific client itself)
		for (int j = 1; j <= nbClients; j++)
		{
			if (i != j)
			{
				// Compute proximity using Eq. 4 in Vidal 2012, and append at the end of orderProximity
				const int timeIJ = timeCost.get(i, j);
				orderProximity.emplace_back(
					timeIJ
					+ std::min(
						proximityWeightWaitTime * std::max(cli[j].earliestArrival - timeIJ - cli[i].serviceDuration - cli[i].latestArrival, 0)
						+ proximityWeightTimeWarp * std::max(cli[i].earliestArrival + cli[i].serviceDuration + timeIJ - cli[j].latestArrival, 0),
						proximityWeightWaitTime * std::max(cli[i].earliestArrival - timeIJ - cli[j].serviceDuration - cli[j].latestArrival, 0)
						+ proximityWeightTimeWarp * std::max(cli[j].earliestArrival + cli[j].serviceDuration + timeIJ - cli[i].latestArrival, 0)),
					j);
			}
		}
		
		// Sort orderProximity (for the specific client)
		std::sort(orderProximity.begin(), orderProximity.end());
	}

	// Calculate, for all vertices, the correlation for the nbGranular closest vertices
	SetCorrelatedVertices();

	// Safeguards to avoid possible numerical instability in case of instances containing arbitrarily small or large numerical values
	if (maxDist < 0.1 || maxDist > 100000)
	{
		throw std::string("The distances are of very small or large scale. This could impact numerical stability. Please rescale the dataset and run again.");
	}
	if (maxDemand < 0.1 || maxDemand > 100000)
	{
		throw std::string("The demand quantities are of very small or large scale. This could impact numerical stability. Please rescale the dataset and run again.");
	}
	if (nbVehicles < std::ceil(totalDemand / vehicleCapacity))
	{
		throw std::string("Fleet size is insufficient to service the considered clients.");
	}

	// A reasonable scale for the initial values of the penalties
	penaltyCapacity = std::max(0.1, std::min(1000., static_cast<double>(maxDist) / maxDemand));

	// Initial parameter values of these two parameters are not argued
	penaltyWaitTime = 0.;
	penaltyTimeWarp = config.initialTimeWarpPenalty;

	// See Vidal 2012, HGS for VRPTW
	proximityWeightWaitTime = 0.2;
	proximityWeightTimeWarp = 1;

//	double min_x = 1e30;
//	double min_y = 1e30;
//	double max_x = -1e30;
//	double max_y = -1e30;
//	for (int i = 0; i <= nbClients; ++i) {
//		min_x = std::min<double>(min_x, cli[i].coordX);
//		min_y = std::min<double>(min_y, cli[i].coordY);
//		max_x = std::max<double>(max_x, cli[i].coordX);
//		max_y = std::max<double>(max_y, cli[i].coordY);
//	}
//	double	delta_x = (max_x - min_x);
//	double delta_y = (max_y - min_y);
//	double ratio1 = std::max<double>(delta_x, delta_y) / std::min<double>(delta_x, delta_y);
//	double min_x1 = 1e30;
//	double min_y1 = 1e30;
//	double max_x1 = -1e30;
//	double max_y1 = -1e30;
//	for (int i = 0; i <= nbClients; ++i) {
//		min_x1 = std::min<double>(cli[i].coordY -cli[i].coordX, min_x1);
//		max_x1 = std::max<double>(cli[i].coordY - cli[i].coordX, max_x1);
//		min_y1 = std::min<double>(cli[i].coordY + cli[i].coordX, min_y1);
//		max_y1 = std::max<double>(cli[i].coordY + cli[i].coordX, max_y1);
//	}
//	double delta_x1 = (max_x1 - min_x1);
//	double delta_y1 = (max_y1 - min_y1);
//	double ratio2 = std::max<double>(delta_x1, delta_y1) / std::min<double>(delta_x1, delta_y1);
//	this->ratio =std::max<double>(ratio1, ratio2);

}

double Params::getTimeElapsedSeconds(){
	if (config.useWallClockTime)
	{
		std::chrono::duration<double> wctduration = (std::chrono::system_clock::now() - startWallClockTime);
		return wctduration.count();
	}
	return (std::clock() - startCPUTime) / (double)CLOCKS_PER_SEC;
}

bool Params::isTimeLimitExceeded(){
	return getTimeElapsedSeconds() >= config.timeLimit;
}

void Params::SetCorrelatedVertices(){
	// Calculation of the correlated vertices for each client (for the granular restriction)
	correlatedVertices = std::vector<std::vector<int>>(nbClients + 1);

	// First create a set of correlated vertices for each vertex (where the depot is not taken into account)
	std::vector<std::set<int>> setCorrelatedVertices = std::vector<std::set<int>>(nbClients + 1);

	// Loop over all clients (excluding the depot)
	for (int i = 1; i <= nbClients; i++)
	{
		auto& orderProximity = orderProximities[i];

		// Loop over all clients (taking into account the max number of clients and the granular restriction)
		for (int j = 0; j < std::min(config.nbGranular, nbClients - 1); j++)
		{
			// If i is correlated with j, then j should be correlated with i (unless we have asymmetric problem with time windows)
			// Insert vertices in setCorrelatedVertices, in the order of orderProximity, where .second is used since the first index correponds to the depot
			setCorrelatedVertices[i].insert(orderProximity[j].second);
			
			// For symmetric problems, set the other entry to the same value
			if (config.useSymmetricCorrelatedVertices)
			{
				setCorrelatedVertices[orderProximity[j].second].insert(i);
			}
		}
	}

	// Now, fill the vector of correlated vertices, using setCorrelatedVertices
	for (int i = 1; i <= nbClients; i++)
	{
		for (int x : setCorrelatedVertices[i])
		{
			// Add x at the end of the vector
			correlatedVertices[i].push_back(x);
		}
	}
}
