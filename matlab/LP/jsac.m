%% This script computes the upper bound for a non-caching scenario and for the same scenario using super ultra caching
% The input is a network with edges and their corresponding capacities
% (i.e., bandwidth). For an example network see top-1.csv.

% Copyright Benjamin Rainer and Daniel Posch 2015, Alpen-Adria-Universität
% Klagenurt, Department of Information Technology

clear all;

% parse the graph and compute all possible ways from the clients to their
% corresponding content sources
csv_file = 'top-1.csv';
parser = top_parser(csv_file);
nodes = parser.nodes;
g = parser.mygraph;
cl = parser.clients;

% now compute the average bit rate in the case that there is no caching
% available in the network
% BEWARE: we assume that all clients start to stream at the same point in
% time!
% For further details on the LP see: solveFractionalMultiCommodityFlow.m

[optim_target, solution_clients, solution_graph, avg_bitrate, exitflag] =  solveFractionalMultiCommodityFlow(g, cl);


%% now solve the same scenario but with caching
% therefore, we have to check all combinations of clients that try to
% stream the same content
% BEWARE: there may be many combinations that this script will check until
% it finds the optimal one!!

combs = combvec(parser.servers_clients{:});
fprintf('Going to check %d combinations for an optimal solution for the caching scenario!\n', size(combs,2));

maxV = 0;
iteration = 1;
for c = 1:size(combs, 2)
    fprintf('Computing combination %d...\n', c);
    tmp_cl  = cell(size(combs, 1),1);
    for i=1:size(combs, 1)
        tmp_cl{i} = cl{combs(i,c)};
    end
    [optim_target_caching, solution_clients_caching, solution_graph_caching, avg_bitrate_caching, exitflag] =  solveFractionalMultiCommodityFlow(g, tmp_cl);
    if optim_target_caching > maxV
        maxV = optim_target_caching;
        iteration = c;
    end
    fprintf('Critaria for optimality: %f, for iteration %d.\n', optim_target_caching, c);
end

% finally solve the problem once more for the optimal combination
tmp_cl = cell(size(combs, 1),1);
for i=1:size(combs, 1)
    tmp_cl{i} = cl{combs(i,iteration)};
end
[optim_target_caching, solution_clients_caching, solution_graph_caching, avg_bitrate_caching, exitflag] =  solveFractionalMultiCommodityFlow(g, tmp_cl);