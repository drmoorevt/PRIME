function [name, chans, data, time] = runTest12(s, numSweeps)
    close all
    writeBuffer(1:128) = uint8(0);
    
    profIter = 1;
    while (profIter < 6) % Won't run XLP test
        loopIter = 1;
        dataIter = 1;
        args = argGenTest12(1000, 28, 0, uint32(profIter - 1), writeBuffer, 0, 128);
        while loopIter <= numSweeps
            fprintf('\nExecution %d/%d\n', loopIter, numSweeps);
            success = false;
            try
                [name, chans, data(:,:,profIter,dataIter), time, success] = rtd(s, 12, args);
                avgData = mean(data, 3);
                loopIter = loopIter + 1;
                dataIter = dataIter + 1;
            catch
                disp('Test failure ... retrying');
            end
        end
        testPlot(avgData, time, chans, name, 275);
        profIter = profIter + 1;
    end
end
