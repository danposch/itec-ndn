classdef top_parser < handle
    %PATH Summary of this class goes here
    %   Detailed explanation goes here
    
    properties
        csv_file = ''; %csv_file to parse
        nodes = 0;     %number of nodes for the graph
        graph = 0;     %result graph after parsing
        clients = cell(1);
    end
    
    methods
        
        function obj = top_parser(csv_file)

            obj.csv_file = csv_file;
          
            fid = fopen(obj.csv_file);
          
            %extract number of nodes for graph
            tline = java.lang.String(fgets(fid));
            %fprintf(char(tline));
            while(~tline.isEmpty())
                if(tline.startsWith(java.lang.String('#number of nodes')))
                    tline = java.lang.String(fgets(fid));
                    %tline = tline.substring(0, tline.length()-2);
                    obj.nodes = str2double(char(tline));
                    %obj.nodes = java.lang.Integer.parseInt(tline.toString());
                    break;
                end
                tline = java.lang.String(fgets(fid));
            end
           
            if(obj.nodes == 0)
                fprintf('Invalid Topology!\n');
                return;
            else
                fprintf('Parsed %d nodes.\n', obj.nodes);
            end
            
            %create graph
            obj.graph = graph(obj.nodes);
            
            %extract edge info + bandwidth
            edge_info_str = '';
            while(~tline.isEmpty())
                if(tline.startsWith(java.lang.String('#nodes (n1,n2,bandwidth in bits)')))
                    tline = java.lang.String(fgets(fid));            
                    while(tline.startsWith(java.lang.String('(')))
                        edge_info_str = strcat(edge_info_str, char(tline));
                        tline = java.lang.String(fgets(fid));
                    end
                    break;
                end
                tline = java.lang.String(fgets(fid));
            end
            
            %create edges
            edge = textscan(edge_info_str, '(%d,%d,%d)');
            for i=1:length(edge{1})
                obj.graph.addEdgeWithCapacity(edge{1}(i) + 1, edge{2}(i) + 1, edge{3}(i)); 
            end
            
            fprintf('Parsed %d edges.\n',length(edge{1}));
            
            %extract client / server information
            cs_info_str = '';
            while(~tline.isEmpty())
                if(tline.startsWith(java.lang.String('#properties (Client, Server)')))
                    tline = java.lang.String(fgets(fid));            
                    while(tline.startsWith(java.lang.String('(')))
                        cs_info_str = strcat(cs_info_str, char(tline));
                        tline = java.lang.String(fgets(fid));
                    end
                    break;
                end
                tline = java.lang.String(fgets(fid));
            end
            
            %define client/server start,end
            cs = textscan(cs_info_str, '(%d,%d)');
            fprintf('Parsed %d client/server pairs. Calculating Paths:\n',length(cs{1}));
            for i=1:length(cs{1})
                obj.clients{i} = client(cs{1}(i) + 1, cs{2}(i) + 1, obj.graph);
                n_path = obj.clients{i}.calcPaths(); %calc all paths from client to server
                fprintf('Client %2d has %3d paths to its Server\n',i,n_path);
            end       
            
            fclose(fid);
        end
    end
end