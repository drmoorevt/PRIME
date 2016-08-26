function [numFailures, chans, data, time] = runTest14(CommPort, baudRate, numSweeps, testLen, opDelay)
    numFailures = 0;
    delete(instrfindall);
    s = openFixtureComms(CommPort, baudRate);
    
    profIter = 1;
    while (profIter < 5) % 4 profiles: (profIter = 1,2,3,4) = 0,1,2,3
        sweepIter = 1;
        writeBuffer = [1, 1, 0];  % Measure = 1, Read = 1, Convert = 0
        args = argGen(1,                    ... // sampRate
                      testLen,              ... // testLen
                      opDelay,              ... // opDelay
                      uint32(profIter - 1), ... // profile
                      1000,                 ... // preTestDelay
                      1000,                 ... // postTestDelay
                      0,                    ... // Destination address
                      writeBuffer           ... // Destination data
                      );
        while sweepIter <= numSweeps
            fprintf('\nExecution %d/%d\n', sweepIter, numSweeps);
            success = false;
            try
                [name(:,profIter), chans, data(:,:,sweepIter,profIter), time, success] ...
                    = execTest(s, 14, args);
                testPassed = strfind(name(profIter),'Passed');
                if (cellfun('isempty', testPassed))
                    numFailures = numFailures + 1;
                    fprintf('\n******TEST ERROR DETECTED*****\n');
                    %err = MException('Test:Failure', 'The operation did not complete as expected');
                    %throw(err);
                end
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
        testPlot(avgData(:,:,profIter), time, chans, name(:,profIter), testLen/1000);
        %profIter = profIter + 2;
        profIter = profIter + 5;  % skipping the low power test now
    end
end
