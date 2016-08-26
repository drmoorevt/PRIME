function [name, chans, data, time] = runTest11(CommPort, baudRate, numSweeps, testLen, opDelay)
    delete(instrfindall);
    s = openFixtureComms(CommPort, baudRate);
    
    preTestDelay = 1000;
    postTestDelay = 1000;
    testTime = (preTestDelay + testLen + postTestDelay) / 1000;
    
    profIter = 1;
    while (profIter < 5) % 4 profiles: (profIter = 1,2,3,4) = 0,1,2,3
        sweepIter = 1;
        while sweepIter <= numSweeps
            address = uint32((2^16)*rand/128)*128;
            writeBuffer(1:128) = uint8(rand(1,128)*128);
            args = argGen(1,                    ... // sampRate
                          testLen,              ... // testLen
                          opDelay,              ... // opDelay
                          uint32(profIter - 1), ... // profile
                          preTestDelay,         ... // preTestDelay
                          postTestDelay,        ... // postTestDelay
                          address,              ... // Destination address
                          writeBuffer           ... // Destination data
                          );
            fprintf('\nExecution %d/%d\n', sweepIter, numSweeps);
            success = false;
            try
                [name(:,profIter), chans, data(:,:,sweepIter,profIter), time, success] ...
                    = execTest(s, 11, args);
                avgData = mean(data, 3);
                sweepIter = sweepIter + 1;
            catch
                s = resetFixtureComms(s, CommPort, baudRate);
                disp('Test failure ... retrying');
            end
        end
        filename = sprintf('./results/Test11-Profile%d-%dSweeps.mat', ...
                           profIter, sweepIter-1);
        save(filename,'name','chans','avgData','time')
        testPlot(avgData(:,:,profIter), time, chans, name(:,profIter), testTime);
        profIter = profIter + 2;
    end
end
