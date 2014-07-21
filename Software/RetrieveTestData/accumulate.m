function [accumulations] = accumulate(data, startIdx, endIdx)
    for i = 1:length(data(1,:))
        accumulations(i) = sum(data(startIdx : endIdx, i)) ./ (endIdx(i) - startIdx(i));
    end
end