function [name, chans, data, time] = runTest22(CommPort, baudRate, numSweeps, testLen, opDelay)
    delete(instrfindall);
    s = openFixtureComms(CommPort, baudRate);
    i = 1;
    
    % begin by optimizing the erase delay
     lowerLim = opDelay(1);
     upperLim = lowerLim * 2;
    while (lowerLim ~= upperLim)
        fprintf('\n\nTesting [erase delay, write delay]]: [%d,%d]\n\n', lowerLim, opDelay(2));
        [fail, chans, data, time] = runTest12(CommPort, 921600, numSweeps, testLen, [lowerLim, opDelay(2), 0, 0]);
        [upperLim, lowerLim] = binSearch(fail, upperLim, lowerLim);
        %set(gcf,'units','normalized','outerposition',[0 0 1 1])
        F(i) = getframe(gcf);
        i = i + 1;
        close all
    end
    eraseDelay = lowerLim * 1.05;  % increase by 5% for margin of error
    
    % now optimize the write delay
    lowerLim = opDelay(2);
    upperLim = lowerLim * 2;
    while (lowerLim ~= upperLim)
        fprintf('\n\nTesting [erase delay, write delay]]: [%d,%d]\n\n', eraseDelay, lowerLim);
        [fail, chans, data, time] = runTest12(CommPort, 921600, numSweeps, testLen, [eraseDelay, lowerLim, 0, 0]);
        [upperLim, lowerLim] = binSearch(fail, upperLim, lowerLim);
        %set(gcf,'units','normalized','outerposition',[0 0 1 1])
        F(i) = getframe(gcf);
        i = i + 1;
        close all
    end
    writeDelay = lowerLim;
    
    fprintf('\n\nOptimal Erase Delay: %d\n\n', eraseDelay);
    fprintf('\n\nOptimal Write Delay: %d\n\n', writeDelay);
    
    close all
    figure
    movie(gcf,F,1000000,3)
end