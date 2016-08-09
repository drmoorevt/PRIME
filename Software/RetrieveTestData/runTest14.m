function [name, chans, data, time] = runTest14(CommPort, baudRate, numSweeps)
    delete(instrfindall);
    s = openFixtureComms(CommPort, baudRate);
    
    profIter = 1;
    while (profIter < 5) % 4 profiles: (profIter = 1,2,3,4) = 0,1,2,3
        sweepIter = 1;
        args = argGenTest14(1000, 5, 0, uint32(profIter - 1), 1, 1, 0);
        while sweepIter <= numSweeps
            fprintf('\nExecution %d/%d\n', sweepIter, numSweeps);
            success = false;
            try
                [name(:,profIter), chans, data(:,:,sweepIter,profIter), time, success] ...
                    = execTest(s, 14, args);
                avgData = mean(data, 3);
                sweepIter = sweepIter + 1;
            catch
                s = resetFixtureComms(s, CommPort, baudRate);
                disp('Test failure ... retrying');
            end
        end
        filename = sprintf('./results/Test14-Profile%d-%dSweeps.mat', ...
                           profIter, sweepIter-1);
        save(filename,'name','chans','avgData','time')
        testPlot(avgData(:,:,profIter), time, chans, name(:,profIter), 50);
        profIter = profIter + 2;
    end
end
