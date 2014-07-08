function [name, chans, data, time] = runTest14(s, numSweeps)
    close all
    writeBuffer(1:128) = uint8(0);
    
    profIter = 1;
    while (profIter < 6) % Won't run XLP test
        loopIter = 1;
        dataIter = 1;
        args = argGenTest14(1000, 5, 0, uint32(profIter - 1), 1, 1, 0);
        while loopIter <= numSweeps
            fprintf('\nExecution %d/%d\n', loopIter, numSweeps);
            success = false;
            try
                [name, chans, data(:,:,profIter,dataIter), time, success] = rtd(s, 14, args);
                avgData = mean(data, 3);
                loopIter = loopIter + 1;
                dataIter = dataIter + 1;
            catch
                disp('Test failure ... retrying');
            end
        end
        testPlot(avgData, time, chans, name, 50);
        profIter = profIter + 1;
    end
end
