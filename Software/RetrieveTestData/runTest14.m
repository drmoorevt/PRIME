function [name, chans, data, time] = runTest14(CommPort, baudRate, numSweeps)
    delete(instrfindall);
    s = openFixtureComms(CommPort, baudRate);
    
    profIter = 1;
    while (profIter < 5)
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
        filename = sprintf('./results/Test14-Profile%d-%dSweeps.mat', ...
                           profIter, sweepIter-1);
        save(filename,'name','chans','avgData','time')
        testPlot(avgData(:,:,profIter), time, chans, name(:,profIter), 50);
        profIter = profIter + 1;
    end
end
