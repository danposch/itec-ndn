clear all;

%parameters
csv_file = 'top-1.csv';
minBitrate= 640;
maxBitrate= 1400000

parser = top_parser(csv_file);
nodes = parser.nodes;
g = parser.mygraph;
cl = parser.clients;

%% Primal problem formulation FOR NO CACHING
% solve the linear program that leads to an upper bound of the ILP that is
% np-complete - This is the primal problem
%f = [1];

% the matrix A has g.edges_array.size() + length(cl) columns and 1 +
% sum_{i=1}^{length(cl)} length(cl{i}.paths) rows
num_paths = 0;
for i=1:length(cl)
    num_paths = num_paths + length(cl{i}.paths);
end

f = zeros(length(cl) + num_paths,1);

f(1:length(cl)) = -1;


A = zeros(2*length(cl) + g.edges_array.size(), length(cl) + num_paths);

% set the corresponding entries in A

%for i=1:length(cl)
%    A(1,g.edges_array.size() + i) = -minBitrate; % this sets the min bitrate that has to be consumed
%end

paths_used = length(cl);

for k=1:length(cl)
    for i=1:length(cl{k}.paths)
        paths_used = paths_used + 1;
        A(k, paths_used) = -1;
    end
    A(k, k) = minBitrate;
end

b = zeros(size(A,1),1);

for i=0:g.edges_array.size()-1
    t = g.edges_array.get(i);
    paths_used = 0;
    
    for k=1:length(cl)
        
        for j=1:length(cl{k}.paths)
            
            paths_used = paths_used + 1;
            
            if cl{k}.paths{j}.containsEdge(t(1), t(2)) == 1
                
                A(length(cl) + i + 1, length(cl) + paths_used) = 1;
                
            end
            
        end
        
    end
    
    b(i+1 + length(cl)) = t(3);
end

paths_used = 0;
for i=1:length(cl)
    for j=1:length(cl{i}.paths)
        paths_used = paths_used + 1;
        A(length(cl) + g.edges_array.size() + i, length(cl) + paths_used) = 1;
    end
    
    b(length(cl) + g.edges_array.size() + i) = maxBitrate;
    
end

lb = zeros(size(A,2),1);

[x,fval,exitflag,output,lambda] = linprog(f,A,b,[],[],lb);

%% store the results of the LP for each client and its paths
s = 0;
for i=1:length(cl)
    
    s = s + x(i) * minBitrate;
    cl{i}.maxPossibleBitrateForStreaming = x(i) * minBitrate;
end

s = s/length(cl);

%% Primal problem formulaiton for CACHING


combs = combvec(parser.servers_clients{:});

maxV = 0;
iteration = 1;

for c = 1:size(combs, 2)
    
    tmp_cl  = cell(size(combs, 1),1);
    
    for i=1:size(combs, 1)
        tmp_cl{i} = cl{combs(i,c)};
    end
    
    num_paths = 0;
    for i=1:length(tmp_cl)
        num_paths = num_paths + length(tmp_cl{i}.paths);
    end
    
    f = zeros(length(tmp_cl) + num_paths,1);
    
    f(1:length(tmp_cl)) = -1;
    
    
    A = zeros(2*length(tmp_cl) + g.edges_array.size(), length(tmp_cl) + num_paths);
    
    % set the corresponding entries in A
    
    %for i=1:length(cl)
    %    A(1,g.edges_array.size() + i) = -minBitrate; % this sets the min bitrate that has to be consumed
    %end
    
    paths_used = length(tmp_cl);
    
    for k=1:length(tmp_cl)
        for i=1:length(tmp_cl{k}.paths)
            paths_used = paths_used + 1;
            A(k, paths_used) = -1;
        end
        A(k, k) = minBitrate;
    end
    
    b = zeros(size(A,1),1);
    
    for i=0:g.edges_array.size()-1
        t = g.edges_array.get(i);
        paths_used = 0;
        
        for k=1:length(tmp_cl)
            
            for j=1:length(tmp_cl{k}.paths)
                
                paths_used = paths_used + 1;
                
                if tmp_cl{k}.paths{j}.containsEdge(t(1), t(2)) == 1
                    
                    A(length(tmp_cl) + i + 1, length(tmp_cl) + paths_used) = 1;
                    
                end
                
            end
            
        end
        
        b(i+1 + length(tmp_cl)) = t(3);
    end
    
    paths_used = 0;
    for i=1:length(tmp_cl)
        for j=1:length(tmp_cl{i}.paths)
            paths_used = paths_used + 1;
            A(length(tmp_cl) + g.edges_array.size() + i, length(tmp_cl) + paths_used) = 1;
        end
        
        b(length(tmp_cl) + g.edges_array.size() + i) = maxBitrate;
        
    end
    
    lb = zeros(size(A,2),1);
    
    [x,fval,exitflag,output,lambda] = linprog(f,A,b,[],[],lb);
    if (f'*x)*(-1) > maxV
        maxV = (-1) * (f'*x);
        iteration = c;
    end
end

%% now calculate the best iteration again

tmp_cl = cell(size(combs, 1),1);

for i=1:size(combs, 1)
    tmp_cl{i} = cl{combs(i,iteration)};
end

num_paths = 0;
for i=1:length(tmp_cl)
    num_paths = num_paths + length(tmp_cl{i}.paths);
end

f = zeros(length(tmp_cl) + num_paths,1);

f(1:length(tmp_cl)) = -1;


A = zeros(2*length(tmp_cl) + g.edges_array.size(), length(tmp_cl) + num_paths);

% set the corresponding entries in A

%for i=1:length(cl)
%    A(1,g.edges_array.size() + i) = -minBitrate; % this sets the min bitrate that has to be consumed
%end

paths_used = length(tmp_cl);

for k=1:length(tmp_cl)
    for i=1:length(tmp_cl{k}.paths)
        paths_used = paths_used + 1;
        A(k, paths_used) = -1;
    end
    A(k, k) = minBitrate;
end

b = zeros(size(A,1),1);

for i=0:g.edges_array.size()-1
    t = g.edges_array.get(i);
    paths_used = 0;
    
    for k=1:length(tmp_cl)
        
        for j=1:length(tmp_cl{k}.paths)
            
            paths_used = paths_used + 1;
            
            if tmp_cl{k}.paths{j}.containsEdge(t(1), t(2)) == 1
                
                A(length(tmp_cl) + i + 1, length(tmp_cl) + paths_used) = 1;
                
            end
            
        end
        
    end
    
    b(i+1 + length(tmp_cl)) = t(3);
end

paths_used = 0;
for i=1:length(tmp_cl)
    for j=1:length(tmp_cl{i}.paths)
        paths_used = paths_used + 1;
        A(length(tmp_cl) + g.edges_array.size() + i, length(tmp_cl) + paths_used) = 1;
    end
    
    b(length(tmp_cl) + g.edges_array.size() + i) = maxBitrate;
    
end

lb = zeros(size(A,2),1);

[x,fval,exitflag,output,lambda] = linprog(f,A,b,[],[],lb);


s = 0;
for i=1:length(tmp_cl)
    
    s = s + x(i) * minBitrate;
    tmp_cl{i}.maxPossibleBitrateForStreaming = x(i) * minBitrate;
end

s = s/length(tmp_cl);



%%
% solve the linear program that leads to an upper bound of the ILP that is
% np-complete - This is the dual of the primal
f = [];
for i=0:g.edges_array.size()-1
    t = double(g.edges_array.get(i));
    f = [f t(3)];
end

% the matrix A has g.edges_array.size() + length(cl) columns and 1 +
% sum_{i=1}^{length(cl)} length(cl{i}.paths) rows
num_paths = 0;
for i=1:length(cl)
    num_paths = num_paths + length(cl{i}.paths);
    f = [f 0];
end

A = zeros(1 + num_paths, g.edges_array.size() + length(cl));

% set the corresponding entries in A

for i=1:length(cl)
    A(1,g.edges_array.size() + i) = -minBitrate; % this sets the min bitrate that has to be consumed
end

paths_used = 0;

for k=1:length(cl)
    for i=1:length(cl{k}.paths)
        paths_used = paths_used + 1;
        for j=0:g.edges_array.size()-1
            t = g.edges_array.get(j);
            if cl{k}.paths{i}.containsEdge(t(1), t(2)) == 1
                A(1 + paths_used, j+1) = -1;
            end
        end
        
        A(1 + paths_used, g.edges_array.size() + k) = 1;
        
    end
end

b = zeros(size(A,1),1);
b(1) = -1;

lb = zeros(size(A,2),1);

[x,fval,exitflag,output,lambda] = linprog(f,A,b,[],[],lb);

upperBoundLP = (f*x) * minBitrate;


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
    toConsume =  maxBitrate;
    
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


% now we have just handled those clients that do not interfere with other
% clients

%%
% we have to divide the set of dependent clients even more
% we have to be sure that all clients are somehow interconnected
stacki = cell(1);
stacki{1} = [];
for i=1:length(dependent_clients)
    
    found = 0;
    for t=1:length(stacki)
        
        [found, elem] = ismember(i,stacki{t});
        if found == 1
            mySetIndex = t;
            break;
        end
    end
    if found == 0
        mySetIndex = i;
        stacki{mySetIndex} = [];
    end
    
    cnt = 0;
    for k=1:length(dependent_clients)
        disjoint = 1;
        if i ~= k
            for j=1:size(dependent_clients{i}.disjointPaths_array{strat_for_client(dependent_clients{i}.id)},2)
                for h=1:size(dependent_clients{k}.disjointPaths_array{strat_for_client(dependent_clients{k}.id)},2)
                    if checkDisjoint(dependent_clients{i}.disjointPaths_array{strat_for_client(dependent_clients{i}.id)}{1,j}, dependent_clients{k}.disjointPaths_array{strat_for_client(dependent_clients{k}.id)}{1,h}) == 0
                        stacki{mySetIndex} = [stacki{mySetIndex}, k];
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

% create for each client the list of clients that it is able to "reach" using
% intersecting edges


for i=1:length(stacki)
    stacki{i} = unique(stacki{i});
end

for i=1:length(stacki)
    for j=i+1:length(stacki)
        [a, b, c] = intersect(stacki{i}, stacki{j});
        if ~isempty(a)
            stacki{i} = [stacki{i}, stacki{j}];
            stacki{i} = unique(stacki{i});
        end
    end
end




%%
% for each set in stacki
%HALBFERTIG
for s = 1:length(stacki)
    
    % consume the lowest layer without stalls
    toConsume = []; %erase old values..
    for i=1:length(stacki{s}{i})
        toConsume(i) =  minBitrate;
    end
    
    while sum(toConsume(:)) > 0
        for i=1:length(stacki{s}) %THIS REQUIRES cl TO BE ORDED BY client_ids IN ASCENDING ORDER
            
            %for j=1:size(dependent_clients{i}.disjointPaths_array{strat_for_client(dependent_clients{i}.id)},2)
            for j=1:size(cl{stacki{s}(i)}.disjointPaths_array{strat_for_client(stacki{s}(i))},2)
                if toConsume(i) > 0 % we do not care about other possibilities because they do not change anything for any other client (+ it would not influence the maximum)...
                    mt = (g_tmp.residuals .* cl{stacki{s}(i).disjointPaths_array{strat_for_client(stacki{s}(i))}{1,j}.fulledgeMatrix);%error
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
    
end

%% algo

A = zeros(nodes, nodes);
for i=1:length(cl)
    A = A + cl{i}.edgeMatrix;
end