function [name, chans, data, time] = runTest21(CommPort, numSweeps, testLen, opDelay)
    delete(instrfindall);
    s = openFixtureComms(CommPort);
    
    if (opDelay(3) == 0)  % Are we looking for optimal tDelay? Then eDelay will be zero
        i = 1;
        lowerLim = opDelay(1);
        upperLim = lowerLim * 2;
        while (lowerLim ~= upperLim)
            fprintf('\n\nTesting opDelay: %d\n\n', lowerLim);
            [fail, chans, data, time] = runTest11(CommPort, 921600, numSweeps,  testLen,  [lowerLim, opDelay(2), opDelay(3), opDelay(4)]);
            [upperLim, lowerLim] = binSearch(fail, upperLim, lowerLim);
            %set(gcf,'units','normalized','outerposition',[0 0 1 1])
            F(i) = getframe(gcf);
            i = i + 1;
            close all
        end
        fprintf('\n\nOptimal Delay: %d\n\n', lowerLim);
    else
        i = 1;
        lowerLim = opDelay(3);
        upperLim = lowerLim * 2;
        while (lowerLim ~= upperLim)
            fprintf('\n\nTesting opDelay: %d\n\n', lowerLim);
            [fail, chans, data, time] = runTest11(CommPort, 921600, numSweeps,  testLen,  [opDelay(1), opDelay(2), opDelay(3), lowerLim]);
            [upperLim, lowerLim] = binSearch(fail, upperLim, lowerLim);
            %set(gcf,'units','normalized','outerposition',[0 0 1 1])
            F(i) = getframe(gcf);
            i = i + 1;
            close all
        end
        fprintf('\n\nOptimal Delay: %d\n\n', lowerLim);
    end
    close all
    figure
    movie(gcf,F,1000000,3)
end