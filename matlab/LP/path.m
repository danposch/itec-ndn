classdef path < handle
    %PATH Summary of this class goes here
    %   Detailed explanation goes here
    
    properties
        edges = 0;
        start_vertex = 0;
        end_vertex = 0;
        vertices = 0;
        myPath = -1;
        edgeMatrix = 0;
    end
    
    methods
         function obj = path( this_start_vertex, this_end_vertex, total_graph_vertices)
           
            obj.start_vertex = this_start_vertex;
            obj.end_vertex = this_end_vertex;
            
            for i=1:total_graph_vertices
               obj.vertices(i) = i; 
            end
            
            obj.edges = zeros(total_graph_vertices, total_graph_vertices);
                    
        end

        function ret = addVertex(p, i)
            if p.myPath(length(p.myPath)) == -1
                p.myPath(length(p.myPath)) = i;
            else
                p.myPath(length(p.myPath) + 1) = i;
            end
        end
           
             
        function ret = printMe(p)
            for i=1:length(p.myPath)-1
                fprintf('%d->', p.myPath(i));
            end
            fprintf('%d\n', p.myPath(length(p.myPath)));
        end
        
    end
    
end

