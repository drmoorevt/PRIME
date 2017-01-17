function [newUpper, newLower] = binSearch(fail, upperLim, lowerLim)
    halfDistance = (upperLim - lowerLim) / 2;
    if fail > 0  % test failed, increase lower limit, leave upper
        newLower = round(lowerLim + halfDistance);
        newUpper = round(upperLim);
    else  % test passed, decrease lower limit, save new upper limit
        newLower = round(lowerLim - halfDistance);
        newUpper = round(lowerLim);
    end
end