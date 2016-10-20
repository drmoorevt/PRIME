function [fail, chans, data, time, F, opDelay] = optimizeDelay(testToRun, CommPort, numSweeps, testLen, opDelay, opSet, profile)
    frameIdx = 1;
    idxLo = ((opSet - 1) * 3) + 1;        
    idxHi = opSet * 3;
    delayIdx = getDelayIdx(opDelay(idxLo:idxHi)) + ((opSet - 1) * 3);
    lowerLim = opDelay(delayIdx);
    upperLim = lowerLim * 2;
    while (lowerLim ~= upperLim)
        opDelay(delayIdx) = lowerLim;
        fprintf('\nTesting delays: [ %s]\n', sprintf('%d ', opDelay));
        if (11 == testToRun)
            [fail, chans, data, time] = runTest11(CommPort, numSweeps, testLen, opDelay, profile);
        elseif (12 == testToRun)
            [fail, chans, data, time] = runTest12(CommPort, numSweeps, testLen, opDelay, profile);
        elseif (13 == testToRun)
            [fail, chans, data, time] = runTest13(CommPort, numSweeps, testLen, opDelay, profile);
        elseif (14 == testToRun)
            [fail, chans, data, time] = runTest14(CommPort, numSweeps, testLen, opDelay, profile);
        elseif (15 == testToRun)
            [fail, chans, data, time] = runTest15(CommPort, numSweeps, testLen, opDelay, profile);
        end
        [upperLim, lowerLim] = binSearch(fail, upperLim, lowerLim);
        %set(gcf,'units','normalized','outerposition',[0 0 1 1])
        F(frameIdx) = getframe(gcf);
        frameIdx = frameIdx + 1;
        close all
    end
    opDelay(delayIdx) = round(lowerLim * 1.05);  % increase erase delay by 5% for margin of error
