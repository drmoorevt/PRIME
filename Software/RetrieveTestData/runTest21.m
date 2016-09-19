function [name, chans, data, time] = runTest21(CommPort, baudRate, numSweeps, testLen, opDelay)
    delete(instrfindall);
    s = openFixtureComms(CommPort, baudRate);
    
    i = 1;
    lowerLim = opDelay(1);
    upperLim = lowerLim * 2;
    while (lowerLim ~= upperLim)
        fprintf('\n\nTesting opDelay: %d\n\n', lowerLim);
        [fail, chans, data, time] = runTest11(CommPort, 921600, numSweeps,  testLen,  [lowerLim, 0, 0, 0]);
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