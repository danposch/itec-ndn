clear all;
nodes = 33;
g = graph(nodes);

fid = fopen('edges.csv');
k = textscan(fid, '(%d,%d,%d)');
fclose(fid);

for i=1:length(k{1})

    g.addEdgeWithCapacity(k{1}(i) + 1, k{2}(i) + 1, k{3}(i));
    
end

fid = fopen('clients_servers.csv');
cs = textscan(fid, '(%d,%d)');
fclose(fid);
cl = cell(1);

%
for i=1:length(cs{1})
    cl{i} = client(cs{1}(i) + 1, cs{2}(i) + 1, g);
    % enumerate all paths
    cl{i}.calcPaths();
end


% create the possible set of paths, we have 2^(sum length(cl{i}.paths))
% sets
% before we do so each client computes all sets of node disjoint paths ( in
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
power_set_paths = cell(1);
for i=1:length(paths)
    power_set_paths{i} = combnk(paths, i); 
end


% algo

A = zeros(nodes, nodes);
for i=1:length(cl)
    A = A + cl{i}.edgeMatrix; 
end