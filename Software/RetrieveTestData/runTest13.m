function [name, chans, data, time] = runTest13(s, numSweeps)
    close all
    writeBuffer(1:128) = uint8(0);
    
    profIter = 1;
    while (profIter < 2) % Won't run XLP test -- MAX = 6
        loopIter = 1;
        dataIter = 1;
        args = argGenTest13(1000, 2, 0, uint32(profIter - 1), writeBuffer, 0, 128);
        while loopIter <= numSweeps
            fprintf('\nExecution %d/%d\n', loopIter, numSweeps);
            success = false;
            try
                [name, chans, data(:,:,profIter,dataIter), time, success] = rtd(s, 13, args);
                avgData = mean(data, 3);
                loopIter = loopIter + 1;
                dataIter = dataIter + 1;
            catch
                disp('Test failure ... retrying');
            end
        end
        testPlot(avgData, time, chans, name, 5);
        profIter = profIter + 1;
    end
end
