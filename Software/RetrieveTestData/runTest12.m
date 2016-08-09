function [name, chans, data, time] = runTest12(CommPort, baudRate, numSweeps)
    delete(instrfindall);
    s = openFixtureComms(CommPort, baudRate);
    
    profIter = 1;
    while (profIter < 6) % 5 profiles: (profIter = 1,2,3,4,5) = 0,1,2,3,4
        sweepIter = 1;
        while sweepIter <= numSweeps
            address = uint32((2^21)*rand/4096)*4096;
            writeBuffer(1:128) = uint8(rand(1,128)*128);
            args = argGenTest12(1000, 1, 0, uint32(profIter - 1), writeBuffer, address, 128);
            fprintf('\nExecution %d/%d\n', sweepIter, numSweeps);
            success = false;
            try
                [name(:,profIter), chans, data(:,:,sweepIter,profIter), time, success] ...
                    = execTest(s, 12, args);
                avgData = mean(data, 3);
                sweepIter = sweepIter + 1;
            catch
                s = resetFixtureComms(s, CommPort, baudRate);
                disp('Test failure ... retrying');
            end
        end
        filename = sprintf('./results/Test12-Profile%d-%dSweeps.mat', ...
                           profIter, sweepIter-1);
        save(filename,'name','chans','avgData','time')
        testPlot(avgData(:,:,profIter), time, chans, name(:,profIter), 275);
        profIter = profIter + 3;
    end
end
