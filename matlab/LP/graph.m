classdef graph < matlab.mixin.Copyable
    
    properties
        edges = 0;
        residuals = 0;
        vertices = 0;
        degreematrix = 0;
        
        nodes_passed = 0;
        current_path = 0;
        paths = 0;
        foundpaths = 0;
        edges_array = java.util.Stack();
    end
    
    methods
        
        function obj = graph( vertices )
           
            for i=1:vertices
               obj.vertices(i) = i; 
            end
            
            obj.edges = zeros(vertices, vertices);
            obj.degreematrix = zeros(vertices, 1);
            for i=1:vertices
                for j=1:vertices
                    if(i==j)
                        obj.residuals(i,j) = 0;
                    else
                        obj.residuals(i,j) = 0;
                    end
                end
            end        
        end
        
        function adjMat = getAdjacencyMatrix(gr)
           
            adjMat = gr.edges;
            
        end
        
        function degMat = getDegreeMatrix(gr)
           
            degMat = gr.degreematrix;
            
        end

        function ret = addEdge(gr, i, j)
            if i > length(gr.vertices) || j > length(gr.vertices)
                ret = -1;
            else
                gr.edges(i,j) = 1;
                gr.residuals(i,j) = 1;
                gr.residuals(j,i) = 1;
                gr.degreematrix(i) = gr.degreematrix(i) + 1;
                gr.degreematrix(j) = gr.degreematrix(j) + 1;
                ret = 0;
            end
        end

        function ret = addEdgeWithCapacity(gr, i, j, capacity)
            if i > length(gr.vertices) || j > length(gr.vertices)
                ret = -1;
            else
                
                gr.edges_array.push( [i j capacity]);
                gr.edges_array.push( [j i capacity]);
                gr.edges(i,j) = 1;
                gr.edges(j,i) = 1;
                gr.residuals(i,j) = capacity;
                gr.residuals(j,i) = capacity;
                gr.degreematrix(i) = gr.degreematrix(i) + 1;
                gr.degreematrix(j) = gr.degreematrix(j) + 1;
                ret = 0;
            end
        end
        
        % calculates shortest paths between each pair of vertices, by using
        % an abitrary matrix multiplication
        function spMatrix = shortestPaths(gr)
            A = gr.residuals;
            P = A;
            n = nextpow2(length(gr.vertices)-1);
            
            for l = 1:n
                for i=1:length(gr.vertices)
                    for j=1:length(gr.vertices)
                        
                        a=Inf;
                        
                        for k=1:length(gr.vertices)
                            a(k) = A(i,k) + A(k,j);
                        end
                        
                        [P(i,j), b] = min(a);
                        
                    end
                end
                
                A = P;
                
            end
            spMatrix = A;
        end
              
        
        function algebraicCon = algebraicConnectivity(gr)
           
            L = (diag(gr.degreematrix) - gr.edges);
            lambda = eig(L);
            lambda = sort(lambda);
            algebraicCon = lambda(2);    % the algebraic connectivity is the second smallest eigenvalue
        end
        
        function list = getAdjacentTo(gr, v)
           
            list = java.util.Stack();
            for j=1:length(gr.vertices)
                if gr.edges(v, j) >= 1
                    list.push(j);
                end
            end
            
        end
        
        function r = calcAllPaths(gr, v, s, t)
            gr.nodes_passed.push(v);
            gr.current_path.push(v);
            
            if v == t
                % we have reached the end, create a path and store it
                pt = path(s, t, length(gr.vertices));
                for i=0:gr.current_path.size()-1
                    pt.addVertex(gr.current_path.get(i));
                end
                gr.paths{gr.foundpaths} = pt;
                gr.foundpaths = gr.foundpaths + 1;
            else
               
                adjacentList = gr.getAdjacentTo(v);
                for i=0:adjacentList.size()-1
                   if gr.nodes_passed.indexOf(adjacentList.get(i)) == -1
                        gr.calcAllPaths(adjacentList.get(i), s, t);
                   end
                end
            end
            
            gr.nodes_passed.pop();
            gr.current_path.pop();
            r = 1;
        end
        
        function ret = consume(gr, p, cap)
            
            for i=1:length(p.myPath)-1
            
                gr.residuals(p.myPath(i), p.myPath(i+1)) = gr.residuals(p.myPath(i), p.myPath(i+1)) - cap;
                
            end
                
            ret = cap;
        end
        
        function ret = calcPaths(gr, s, t)
            gr.nodes_passed = java.util.Stack();
            gr.current_path = java.util.Stack();
            gr.paths = cell(1);
            gr.foundpaths = 1;
            % comupte all paths from s to server(s) for this client
            for i=1:length(t)
                gr.calcAllPaths(s, s, t(i));
            end
            ret = gr.paths;        
        end
        
        
        
    end
    
end

