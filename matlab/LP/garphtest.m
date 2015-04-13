clear all;
[myg, o] = randomGraph(0.5, 10, 5);
shortestPs = myg.shortestPaths();
%%
k = 1:100;
x = 0;
y = 0;
step = 1;

for i=1:10
 for j=1:10   
    coords((i-1)*10 + j,1) = x;
    coords((i-1)*10 + j,2) = y;
    x = x + step;
 end
 y = y + step;
 x = 0;
end

adj = myg.getAdjacencyMatrix();
gplot(adj(k,k), coords(k,:), '-*');
%%
for i=1:length(myg.vertices)
    for j=1:length(myg.vertices)
        if shortestPs(i,j) == Inf
            clearedPs(i,j) = 0;
        else
            clearedPs(i,j) = shortestPs(i,j);
        end
    end
end
    

s = max(max(clearedPs))

%%
test = java.util.Hashtable;

md = java.security.MessageDigest.getInstance('MD5');
