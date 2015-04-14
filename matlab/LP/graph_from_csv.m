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
%disjoint_path_sets_cell = cell(1);
num_total_path_strats = 1;
for i=1:length(cl)
    % calculate those set of paths which only include disjoint paths
    rc = cl{i}.calcDisjointPahts();
    fprintf('Client %2d has %3d (edge) disjoint path strategies\n',i,rc);
    num_total_path_strats = num_total_path_strats * cl{i}.num_disjoint_path_sets;
    cl{i}.createSingleDimDisjointPathArray();
    %disjoint_path_sets_cell{i} = [1 cl{i}.num_disjoint_path_sets];
end
%%
fprintf('We have %d permutations of strategies to evaluate!\n',num_total_path_strats);


% now we have to get every possible permutation of strategies for each
% client
%combos = allcomb(disjoint_path_sets_cell{:}); //does not work and would
%kill memmory

%%
s_combiner = strategycombiner(cl);
%while(s_combiner.hasNext)
%    strategy = s_combiner.getNextStrategy()
%end

%%
tic;

% Now we solve the optimization problem

% first we get those paths among the selected strategies which are
% (edge) disjoint

g_tmp = copy(g);

strat_for_client = s_combiner.getNextStrategy(); %ones(length(cl),1); %sample strategie
strat = cell(1);
stratsUnion = cell(1);
stratsUnion_cnt = 1;
for i=1:length(cl)
    cl{i}.id = i;
    strat{i} = cell(1,size(cl{i}.disjointPaths_array{strat_for_client(i)},2));
    for j=1:size(cl{i}.disjointPaths_array{strat_for_client(i)},2)
        strat{i}{1,j} = cl{i}.disjointPaths_array{strat_for_client(i)}{1,j};
        stratsUnion{stratsUnion_cnt} = cl{i}.disjointPaths_array{strat_for_client(i)}{1,j};
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
toc
%%
% use g_tmp for any computation on the residuals/capacities
cumulative_bitrate = 0;

for i=1:length(independent_clients)
    toConsume =  1400000;
    
    for j=1:size(independent_clients{i}.disjointPaths_array{strat_for_client(independent_clients{i}.id)},2)
        if toConsume > 0 % we do not care about other possibilities because they do not change anything for any other client (+ it would not influence the maximum)...
            mt = (g_tmp.residuals .* independent_clients{i}.disjointPaths_array{strat_for_client(independent_clients{i}.id)}{1,j}.fulledgeMatrix);
            [r,c,v] = find(mt>0);
            myBitrate = min(min(diag(mt(r,c))), toConsume);
            % now consume the minimum, it is safe to consume it because
            % this client will not intersect any other client
            cumulative_bitrate = cumulative_bitrate + g_tmp.consume(independent_clients{i}.disjointPaths_array{strat_for_client(independent_clients{i}.id)}{1,j}, myBitrate);
            toConsume = toConsume - myBitrate;
        else
            break;
        end
        
    end
end


% now we have just handled those client that do not interfere with other
% clients

%%
% we have to divide the set of dependent clients even more
% we have to be sure that all clients are somehow interconnected
stacki = cell(1);
for i=1:length(dependent_clients)
    stacki{i} = java.util.Stack();
    for k=1:length(dependent_clients)
        disjoint = 1;
        if i ~= k
            for j=1:size(dependent_clients{i}.disjointPaths_array{strat_for_client(dependent_clients{i}.id)},2)
                for h=1:size(dependent_clients{k}.disjointPaths_array{strat_for_client(dependent_clients{k}.id)},2)
                    if checkDisjoint(dependent_clients{i}.disjointPaths_array{strat_for_client(dependent_clients{i}.id)}{1,j}, dependent_clients{k}.disjointPaths_array{strat_for_client(dependent_clients{k}.id)}{1,h}) == 0
                        stacki{i}.push(k);
                        disjoint = 0;
                        break;
                    end
                end
                if disjoint == 0
                    break;
                end
            end
        end
    end
end


%%
% just consume the minbitrate, one constraint is that each client has to
% consume the lowest layer without stalls

for i=1:length(dependent_clients)
    toConsume(i) =  640000;
end
while sum(toConsume(:)) > 0 && iterdiff > 0
    for i=1:length(dependent_clients)
        
        for j=1:size(dependent_clients{i}.disjointPaths_array{strat_for_client(dependent_clients{i}.id)},2)
            if toConsume(i) > 0 % we do not care about other possibilities because they do not change anything for any other client (+ it would not influence the maximum)...
                mt = (g_tmp.residuals .* dependent_clients{i}.disjointPaths_array{strat_for_client(dependent_clients{i}.id)}{1,j}.fulledgeMatrix);
                [r,c,v] = find(mt>0);
                myBitrate = min(min(diag(mt(r,c))), toConsume(i));
                % now consume the minimum, it is safe to consume it because
                % this client will not intersect any other client
                cumulative_bitrate = cumulative_bitrate + g_tmp.consume(dependent_clients{i}.disjointPaths_array{strat_for_client(dependent_clients{i}.id)}{1,j}, myBitrate);
                toConsume(i) = toConsume(i) - myBitrate;
            else
                break;
            end
        end
        
        
        
    end
    % if there are residuals in toConsume, we have to redistribute the sum
    % on those which could consume everything
    
end

% now we look at all intersections

%% algo

A = zeros(nodes, nodes);
for i=1:length(cl)
    A = A + cl{i}.edgeMatrix;
end