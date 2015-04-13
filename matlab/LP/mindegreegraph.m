function [retgr] = mindegreegraph(vertices)

retgr = graph(vertices); 

for i=1:vertices
    if i+1 <= vertices
        retgr.addEdge(i,i+1);
    end
end

end