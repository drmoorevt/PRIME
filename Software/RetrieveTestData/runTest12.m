function [name, chans, data, time] = runTest12(s, numSweeps)
    success = false;
    writeBuffer(1:128) = uint8(0);
    args = argGenTest12(10000, 28, 10000, 0, writeBuffer, 0, 128);
    for i = 1:numSweeps
        fprintf('\nExecution %d/%d\n',i,numSweeps);
        [name, chans, data(:,:,i), time, success] = rtd(s, 12, args);
        if (success == true)
            avgData = mean(data, 3);
            plot(time, avgData)
        else
            disp('Test failure ... retrying');
            i = i - 1;
        end
    end
end

