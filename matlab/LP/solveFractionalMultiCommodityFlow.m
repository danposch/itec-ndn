function [ optim_target, clients_, graph_with_residuals, average_bitrate, exitflag ] = solveFractionalMultiCommodityFlow(graph_, clients_, content_popularity)
%solveFractionalMultiCommodityFlow This function solves a fractional multi-commodity flow problem. Thus, only providing an upper bound to multiple flow problems (especially if the flows shall be from a set of values, for such a solution see: solveMultiCommodityFlow).

if nargin < 3
    
    content_popularity = ones(length(clients_));
    
end
% The linear optimization problem for solving fractional multi-commodity
% flow problems looks like follows. We introduce a fractional variable(s) y
% for each client.
%   min -||y||_{1}
%   subject to
%   y_{i} * minBitrate_{i}-\sum_{p \in P_{i}} x_{p} \leq 0, \forall i \in C
%   \sum_{(u,v) \in p} x_p \leq c(u,v),  \forall (u,v) \in E_{G}, p \in
%   (\cup_{i}^{|C|} P_{i})
%   \sum_{p \in P_{i}} x_{p} \leq maxBitrate_{i}, \forall i \in C

fprintf('Going to solve multi-commodity flow for %d clients and flows.\n', length(clients_));

graph_with_residuals = copy(graph_);
num_paths = 0;
for i=1:length(clients_)
    num_paths = num_paths + length(clients_{i}.paths);
end

f = zeros(length(clients_) + num_paths,1);

%   min -||y||_{1}
for i=1:length(clients_)
        f(i) = -1*content_popularity(i);
end

A = zeros(2*length(clients_) + graph_.edges_array.size(), length(clients_) + num_paths);

%   y_{i} * minBitrate_{i}-\sum_{p \in P_{i}} x_{p} \leq 0, \forall i \in C
paths_used = length(clients_);
for k=1:length(clients_)
    for i=1:length(clients_{k}.paths)
        paths_used = paths_used + 1;
        A(k, paths_used) = -1;
    end
    A(k, k) = clients_{k}.minBitrate;
end

b = zeros(size(A,1),1);


%   \sum_{(u,v) \in p} x_p \leq c(u,v)  \forall (u,v) \in E_{G}, p \in
%   (\cup_{i} P_{i})

for i=0:graph_.edges_array.size()-1
    t = graph_.edges_array.get(i);
    paths_used = 0;
    
    for k=1:length(clients_)
        
        for j=1:length(clients_{k}.paths)
            
            paths_used = paths_used + 1;
            
            if clients_{k}.paths{j}.containsEdge(t(1), t(2)) == 1
                
                A(length(clients_) + i + 1, length(clients_) + paths_used) = 1;
                
            end
            
        end
        
    end
    
    b(i+1 + length(clients_)) = t(3);
end

% \sum_{p \in P_{i}} x_{p} \leq maxBitrate_{i}, \forall i \in C
paths_used = 0;
for i=1:length(clients_)
    for j=1:length(clients_{i}.paths)
        paths_used = paths_used + 1;
        A(length(clients_) + graph_.edges_array.size() + i, length(clients_) + paths_used) = 1;
    end
    
    b(length(clients_) + graph_.edges_array.size() + i) = clients_{i}.maxBitrate;
    
end

lb = zeros(size(A,2),1);

[x,fval,exitflag,output,lambda] = linprog(f,A,b,[],[],lb);


average_bitrate = 0;
for i=1:length(clients_)
    
    average_bitrate = average_bitrate + x(i) * clients_{i}.minBitrate;
    clients_{i}.maxPossibleBitrateForStreaming = x(i) * clients_{i}.minBitrate;
end

average_bitrate = average_bitrate/length(clients_);

fprintf('Average bitrate for each client is: %f bps!\n', average_bitrate);

edge_consumption = A*x;

for i=0:graph_.edges_array.size()-1
    t = graph_.edges_array.get(i);
    graph_with_residuals.residuals(t(1), t(2)) = t(3) - edge_consumption(length(clients_) + i + 1);
    
end

optim_target = (-1) * (f'*x);

% finally assign each path the consumed bitrate
paths_used = length(clients_);

for i=1:length(clients_)
    for j=1:length(clients_{i}.paths)
        paths_used = paths_used + 1;
        clients_{i}.paths{j}.bitrate = x(paths_used);
    end    
end

end

