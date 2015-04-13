function [spMatrix] = shortestPaths(g)

A = g.residuals;
P = A;
n = nextpow2(length(g.vertices)-1);

for l = 1:n
    for i=1:length(g.vertices)
        for j=1:length(g.vertices)
            
            a=Inf;
            
            for k=1:length(g.vertices) 
                a(k) = A(i,k) + A(k,j);
            end
            
            [P(i,j), b] = min(a);

        end
    end
    
    A = P;

end

spMatrix = A;

end