function [ ret ] = comparePaths( first_path, second_path )
ret = 0;
if length(first_path.myPath) == length(second_path.myPath)
    cumMatrix = xor(first_path.edgeMatrix, second_path.edgeMatrix);
    if sum(cumMatrix(:)) == 0
        ret = 1;
    end
else
    ret = 0;
end

end

