function [numFailures, chans, data, time] = runTest13(CommPort, baudRate, numSweeps, writeWait)
    numFailures = 0;
    delete(instrfindall);
    s = openFixtureComms(CommPort, baudRate);
    
    profIter = 1;
    while (profIter < 5) % 2.1V profile never works
        sweepIter = 1;
        while sweepIter <= numSweeps
            address = uint32((2^29)*rand/4096)*4096;
            writeBuffer(1:128) = uint8(rand(1,128)*128);
            args = argGenTest13(4000, 2, 0, uint32(profIter - 1), ... 
                                writeBuffer, address, 128, writeWait);
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
                closeFixtureComms(s);
                delete(instrfindall);
                pause(1);
                s = openFixtureComms(CommPort, baudRate);
                fwrite(s, uint8(hex2dec('54'))); % Attempt DUT reset
                closeFixtureComms(s);
                disp('Test failure ... retrying');
                pause(0.5); % Give the DUT 500ms to process the reset
                s = openFixtureComms(CommPort, baudRate);
            end
        end
        filename = sprintf('./results/Test13-Profile%d-%dSweeps.mat', ...
                           profIter, sweepIter-1);
        save(filename,'name','chans','avgData','time')
        testPlot(avgData(:,:,profIter), time, chans, name(:,profIter), 20);
        profIter = profIter + 2;
    end
end
