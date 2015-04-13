function [retgr, addedEedges] = randomGraph( p, vertices, max_degree)
%% Erdos - Rheny random graphs
% p in [0,1]
% use the inversion method to generate the set pdf

addedEedges = 0;
retgr = graph(vertices); 

for i=1:vertices
    for j=1:vertices
        if i~=j
            if rand(1) <= p
                if retgr.degreematrix(i) < max_degree && retgr.degreematrix(j) < max_degree
                retgr.addEdge(i,j);
                addedEedges = addedEedges + 1;
                end
            end
        end
    end
end

end