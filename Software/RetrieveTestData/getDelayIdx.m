function [delayIdx] = getDelayIdx(opDelay)
    if (opDelay(3) ~= 0)  % Are we looking for optimal dDelay?
        delayIdx = 3;
    elseif (opDelay(2) ~= 0)
        delayIdx = 2;
    elseif (opDelay(1) ~= 0)
        delayIdx = 1;
    else
        delayIdx = 0;
    end
    