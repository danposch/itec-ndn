classdef strategycombiner < handle
    %PATH Summary of this class goes here
    %   Detailed explanation goes here
    
    properties
      clients = cell(1);
      nextStrategy = 0;
      hasNext = true;
    end
    
    methods
        
        function obj = strategycombiner(clients)
            %init obj
            obj.clients = clients;
            obj.reset();
        end
        
        function strategy = getNextStrategy(scombiner)
            if(scombiner.hasNext)
                strategy = scombiner.nextStrategy;
                scombiner.hasNext = false;
                
                %calc nextStrategy
                for i=1:size(scombiner.nextStrategy)
                    %check if we can increase index i of strategy
                    
                    if (scombiner.nextStrategy(i) < scombiner.clients{i}.num_disjoint_path_sets)
                        scombiner.nextStrategy(i) = scombiner.nextStrategy(i) + 1;
                        scombiner.hasNext = true;
                        
                        if (i > 1)
                            for k=1:i-1 % reset strategy bevor index i
                                scombiner.nextStrategy(k) = 1;
                            end
                        end
                        break;
                    end                     
                end
            else
                strategy = zeros(length(scombiner.clients),1);    
            end
        end
        
        function ret = reset(scombiner)
            scombiner.nextStrategy = ones(length(scombiner.clients),1);
            scombiner.hasNext = true;
        end
               
    end
end