function [ ret ] = checkDisjoint( first_path, second_path )
% check if the path are (edge) disjoint
            edgeM1 = first_path.edgeMatrix;
            edgeM2 = second_path.edgeMatrix;
            sedgeM1 = sum(edgeM1(:));
            sedgeM2 = sum(edgeM2(:));
            edgeM = xor(edgeM1, edgeM2); % matrix xor, a_{i,j} XOR b_{i,j}
            ret = 0;
            if sum(edgeM(:)) == (sedgeM1 + sedgeM2)
                ret = 1;
            end
        

end

