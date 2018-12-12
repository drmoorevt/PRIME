function [numFailures, chans, data, time] = runTest15(CommPort, numSweeps, testLen, opDelay, profileList)
    numFailures = 0;
    delete(instrfindall);
    s = openFixtureComms(CommPort);
    
    preTestDelay = 1000;
    postTestDelay = 1000;
    testTime = (preTestDelay + testLen + postTestDelay) / 1000;
    
    for profListIdx = 1:numel(profileList)
        profIter = profileList(profListIdx);
        sweepIter = 1;
        writeBuffer = [1, 1, 0];  % Measure = 1, Read = 1, Convert = 0
        args = argGen(1,                    ... // sampRate
                      testLen,              ... // testLen
                      opDelay,              ... // opDelay
                      uint32(profIter - 1), ... // profile
                      preTestDelay,         ... // preTestDelay
                      postTestDelay,        ... // postTestDelay
                      0,                    ... // Destination address
                      writeBuffer           ... // Destination data
                      );
        while sweepIter <= numSweeps
            fprintf('\nExecution %d/%d\n', sweepIter, numSweeps);
            success = false;
            try
                [name(:,profIter), chans, data(:,:,sweepIter,profIter), time, success] ...
                    = execTest(s, 15, args);
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
                s = resetFixtureComms(s, CommPort);
                disp('Test failure ... retrying');
            end
        end
        filename = sprintf('./results/%s Test15-Profile%d-%dSweeps.mat', ...
                           datestr(now,'dd-mm-yy HH.MM.SS'), profIter, sweepIter-1);
        save(filename,'name','chans','avgData','time')
        %testPlot(avgData(:,:,profIter), time, chans, name(:,profIter), testTime, 8);
        
        maTitle = strrep(name(:,profIter), 'Passed', 'Passed (50 Sample Moving Average)');
        maTitle = strrep(maTitle, 'Failed', 'Failed (50 Sample Moving Average)');
        movingAverage = avgData(:,:,profIter);
        movingAverage(50:end-50,1) = conv(movingAverage(50:end-50,1), ones(50,1)/50, 'same');
        movingAverage(50:end-50,2) = conv(movingAverage(50:end-50,2), ones(50,1)/50, 'same');
        movingAverage(50:end-50,3) = conv(movingAverage(50:end-50,3), ones(50,1)/50, 'same');
        testPlot(movingAverage(:,:), time, chans, maTitle(:,1), 0, 4);
    end
end