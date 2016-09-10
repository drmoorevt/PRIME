function [name, chans, data, time] = runTest24(CommPort, baudRate, numSweeps, testLen, opDelay)
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

    delete(instrfindall);
    s = openFixtureComms(CommPort, baudRate);
    
    i = 1;
    lowerLim = 50000;
    upperLim = lowerLim * 2;
    while (lowerLim ~= upperLim)
        fprintf('\n\nTesting opDelay: %d\n\n', lowerLim);
        [fail, chans, data, time] = runTest14(CommPort, 921600, numSweeps,  50000,  [lowerLim, 0, 0, 0]);
        [upperLim, lowerLim] = binSearch(fail, upperLim, lowerLim);
        %set(gcf,'units','normalized','outerposition',[0 0 1 1])
        F(i) = getframe(gcf);
        i = i + 1;
        close all
    end
    fprintf('\n\nOptimal Delay: %d\n\n', lowerLim);
    close all
    figure
    movie(gcf,F,1000000,3)
end