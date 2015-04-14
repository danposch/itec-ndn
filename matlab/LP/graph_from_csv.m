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

num_total_path_strats = 1;
for i=1:length(cl)
    % calculate those set of paths which only include disjoint paths
    rc = cl{i}.calcDisjointPahts();
    fprintf('Client %2d has %3d (edge) disjoint path strategies\n',i,rc);
    num_total_path_strats = num_total_path_strats * cl{i}.num_disjoint_path_sets;
    cl{i}.createSingleDimDisjointPathArray();
end

fprintf('We have %d permutations of strategies to evaluate!\n',num_total_path_strats);


% now we have to get every possible permutation of strategies for each
% client

for i=1:length(cl)
    cl{i}.createSingleDimDisjointPathArray();
end
%%
tic;


% Now we solve the optimization problem

% first we get those paths among the selected strategies which are
% (edge) disjoint


% DEBUG: for design issues we just use 1:1:1:1:1:1, TODO: iterate all
% possible permutations

g_tmp = g;

k = 1;
strat_for_client = ones(length(cl),1); %sample strategie
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

disjoint_clients = cell(1);
toRemove = 0;
rem_cnt = 0;
pure_overlapping_paths = stratsUnion;
for i=1:length(stratsUnion)
    inbothsets = 0;
    for j=1:length(disjointPathsAll)
        if comparePaths(stratsUnion{i},disjointPathsAll{j})==1
            rem_cnt = rem_cnt + 1;
            toRemove(rem_cnt) = i;
            break;
        end
    end
end


if rem_cnt > 0
    pure_overlapping_paths(toRemove) = [];
end

dependent_clients = cell(1);
dependent_clients_cnt = 0;

for i=1:length(cl)
    path_found = 0;
    for l=1:size(cl{i}.disjointPaths_array{strat_for_client(i)},2)
        for j=1:length(pure_overlapping_paths)
            if comparePaths(cl{i}.disjointPaths_array{strat_for_client(i)}{1,l}, pure_overlapping_paths{j}) == 1
                dependent_clients_cnt = dependent_clients_cnt + 1;
                dependent_clients{dependent_clients_cnt} = cl{i};
                path_found = 1;
                break;
            end
        end
        if path_found == 1
            break;
        end
    end
end


independent_clients = cl;
toRemove = 0;
rem_cnt = 0;
for i=1:length(independent_clients)
    inbothsets = 0;
    for j=1:length(dependent_clients)
        if independent_clients{i}.id == dependent_clients{j}.id
            rem_cnt = rem_cnt + 1;
            toRemove(rem_cnt) = i;
            break;
        end
    end
end


if rem_cnt > 0
    independent_clients(toRemove) = [];
end


toConsume = maxBitrate;

for i=1:length(independent_clients)
   
    for j=1:size(cl{i}.disjointPaths_array{strat_for_client(i)},2)
    
        min(xor(cl{i}.myGraph.residuals,  cl{i}.disjointPaths_array{strat_for_client(i)}{1,j}.edgeMatrix), toConsume);
            
    end
end


toc

%% algo

A = zeros(nodes, nodes);
for i=1:length(cl)
    A = A + cl{i}.edgeMatrix;
end