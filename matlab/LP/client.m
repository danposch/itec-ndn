classdef client < handle
    %PATH Summary of this class goes here
    %   Detailed explanation goes here
    
    properties
        paths = cell(1);
        myGraph = 0;
        start_vertex = 0;
        end_vertex = 0;
        intersected = cell(1);
        edgeMatrix = 0;
        disjointPaths = 0;
        num_disjoint_path_sets = 0;
    end
    
    methods
        function obj = client( this_start_vertex, this_end_vertex, this_graph)
            
            obj.myGraph = this_graph;
            obj.start_vertex = this_start_vertex;
            obj.end_vertex = this_end_vertex;
            obj.edgeMatrix = zeros(length(obj.myGraph.vertices), length(obj.myGraph.vertices));
            
        end
        
        
        function ret = calcPaths(cl)
            cl.paths = cl.myGraph.calcPaths(cl.start_vertex, cl.end_vertex);
            ret = length(cl.paths);
            
            % prepare matrices for calculating bandwidth shares on graph
            % edges
            for i=1:length(cl.paths)
                for k=1:length(cl.paths{i}.myPath)-1
                    cl.edgeMatrix(cl.paths{i}.myPath(k), cl.paths{i}.myPath(k+1)) = 1;
                end
                
            end
        end
        
        function ret = printPaths(cl)
            
            for i=1:length(cl.paths)
                cl.paths{i}.printMe();
            end
        end
        
        function ret = createSpecialEdgeMatrix(cl, p)
            % special because we do not take into account the first edge
            % and the last edge
            ret = zeros(length(p.vertices), length(p.vertices));
            if length(p.myPath) > 3
                for k=2:length(p.myPath)-2
                    ret(p.myPath(k), p.myPath(k+1)) = 1;
                end
            end
            
        end
        
        function ret = checkDisjoint(cl, first_path, second_path)
            
            edgeM1 = cl.createSpecialEdgeMatrix(first_path);
            edgeM2 = cl.createSpecialEdgeMatrix(second_path);
            sedgeM1 = sum(edgeM1(:));
            sedgeM2 = sum(edgeM2(:));
            edgeM = xor(edgeM1, edgeM2); % matrix xor, a_{i,j} XOR b_{i,j}
            ret = 0;
            if sum(edgeM(:)) == (sedgeM1 + sedgeM2)
                ret = 1;
            end
        end
        
        function ret = calcDisjointPahts(cl)
            % by definition, every single paths is a disjoint path
            % therefore, just add every single path to our list
            
            cl.disjointPaths = cell(length(cl.paths));
            cl.disjointPaths{1} = cell(1);
           
            for i=1:length(cl.paths)
            
                cl.disjointPaths{1}{i,1} = cl.paths{i};
                
            end
            
            cl.num_disjoint_path_sets = cl.num_disjoint_path_sets + (size(cl.disjointPaths{1},1));
            
            if length(cl.paths) > 1
                % create all combinations and test if the selected paths
                % are disjoint
                for i=2:length(cl.paths)
                    cl.disjointPaths{i} = combnk(cl.paths, i);
                    clear rowsToDelete;
                    rowsToDelete = 0;
                    rowsTDC = 0;
                    for j=1:size(cl.disjointPaths{i},1)
                        % check every strategy for disjoint paths
                        disjoint = 1;
                        for n=1:size(cl.disjointPaths{i},2)-1
                            if disjoint == 0
                                break
                            end
                            for k=(n+1):size(cl.disjointPaths{i},2)
                                if cl.checkDisjoint(cl.disjointPaths{i}{j,n}, cl.disjointPaths{i}{j, k}) == 0
                                    disjoint = 0;
                                end
                            end
                        end
                        if disjoint == 0
                            rowsTDC = rowsTDC + 1;
                            rowsToDelete(rowsTDC) = j;
                        end
                        
                    end
                    if rowsTDC > 0
                        cl.disjointPaths{i}(rowsToDelete, :) = [];
                    end
                    
                    if size(cl.disjointPaths{i},1) == 0
                        break;
                    end
                    
                    cl.num_disjoint_path_sets = cl.num_disjoint_path_sets + (size(cl.disjointPaths{i},1));
                end
            end
        end
        
    end
    
end

