clear all;
csv_file = 'top-1.csv';
%parse number of nodes

parser = top_parser(csv_file);

nodes = parser.nodes;
g = parser.graph;
cl = parser.clients;



% create the possible set of paths, we have 2^(sum length(cl{i}.paths))
% sets
% before we do so each client computes all sets of edge disjoint paths ( in
% order to reduce the computational effort ) TODO: easy just compute the
% power set of the path fo a single client, cancel those sets where we have
% overlapping path! - this results in a set of sets of which every set
% represent a node disjoint "strategy"
%%
num_total_path_strats = 1;
for i=1:length(cl)
    % calculate those set of paths which only include disjoint paths
    cl{i}.calcDisjointPahts();
    num_total_path_strats = num_total_path_strats * cl{i}.num_disjoint_path_sets;
    cl{i}.createSingleDimDisjointPathArray();
end

%%
% we will reduce the set of feasibile sets afterwards ( each set should
% include at least a path for each client, otherwise this set is not
% considered )
paths = 0;
for n=1:num_total_paths
    paths(n) = n;
end
%%
% now we have to get every possible permutation of strategies for each
% client

for i=1:length(cl)
    cl{i}.createSingleDimDisjointPathArray();
end


%%
% Now we solve the optimization problem

% first we get those paths among the selected strategies which are
% (edge) disjoint


% DEBUG: for design issues we just use 1:1:1:1:1:1, TODO: iterate all
% possible permutations
k = 1;
strat_for_client = [1,1,1,1,1,1];
strat = cell(1);
stratsUnion = cell(1);
stratsUnion_cnt = 1;
for i=1:length(cl)
    strat{i} = cell(1,size(cl{i}.disjointPaths_array{k},2));
    for j=1:size(cl{i}.disjointPaths_array{k},2)
        strat{i}{1,j} = cl{i}.disjointPaths_array{k}{1,j};
        stratsUnion{stratsUnion_cnt} = cl{i}.disjointPaths_array{k}{1,j};
        stratsUnion_cnt = stratsUnion_cnt + 1;
    end
end


disjointPathsAll = cell(1);
disjointPathsAll_cnt = 1;
for i=1:length(stratsUnion)
    disjoint = 1;
    for j=1:length(stratsUnion)
        if i==j
            continue;
        end
        
        if checkDisjoint(stratsUnion{i}, stratsUnion{j}) == 0
            disjoint = 0;
            break;
        end
        
    end
    if disjoint == 1
        disjointPathsAll{disjointPathsAll_cnt} = stratsUnion{i};
        disjointPathsAll_cnt = disjointPathsAll_cnt + 1;
        
    end
end
% Determine those clients thare only in the disjoint set and those that
% are in both

for i=1:length(cl)
    
end


%% algo

A = zeros(nodes, nodes);
for i=1:length(cl)
    A = A + cl{i}.edgeMatrix;
end