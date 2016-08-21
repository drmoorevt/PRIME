function [numFailures, chans, data, time] = runTest13(CommPort, baudRate, numSweeps, testLen, opDelay)
    numFailures = 0;
    delete(instrfindall);
    s = openFixtureComms(CommPort, baudRate);
    
    profIter = 1;
    while (profIter < 5) % 2.1V profile never works
        sweepIter = 1;
        while sweepIter <= numSweeps
            address = uint32((2^29)*rand/4096)*4096;
            writeBuffer(1:128) = uint8(rand(1,128)*128);
            args = argGen(1,                    ... // sampRate
                          testLen,              ... // testLen
                          opDelay,              ... // opDelay
                          uint32(profIter - 1), ... // profile
                          1000,                 ... // preTestDelay
                          1000,                 ... // postTestDelay
                          address,              ... // Destination address
                          writeBuffer           ... // Destination data
                          );
            fprintf('\nExecution %d/%d\nAddress: %d\n', sweepIter, numSweeps, address);
            fprintf('Data: %s\n', dec2hex(writeBuffer));
            success = false;
            try
                [name(:,profIter), chans, data(:,:,sweepIter,profIter), time, success] ...
                    = execTest(s, 13, args);
                testPassed = strfind(name(profIter),'Passed');
                if (cellfun('isempty', testPassed))
                    numFailures = numFailures + 1;
                    fprintf('\n******WRITE ERROR DETECTED*****\n');
                    err = MException('Test13:WriteFailure', 'The write did not complete as expected');
                    throw(err);
                end
                avgData = mean(data, 3);
                sweepIter = sweepIter + 1;
                %close all;
                %testPlot(avgData(:,:,profIter), time, chans, name(:,profIter), 80);
            catch
                s = resetFixtureComms(s, CommPort, baudRate);
                disp('Test failure ... retrying');
            end
        end
        filename = sprintf('./results/Test13-Profile%d-%dSweeps.mat', ...
                           profIter, sweepIter-1);
        save(filename,'name','chans','avgData','time')
        testPlot(avgData(:,:,profIter), time, chans, name(:,profIter), testLen/1000);
        profIter = profIter + 2;
    end
end
