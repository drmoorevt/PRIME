function [name, chans, data, time] = runTest11(s, numSweeps)
    success = false;
    writeBuffer(1:128) = uint8(0);
    args = argGenTest11(250, 1, 250, 0, writeBuffer, 0, 128);
    for i = 1:numSweeps
        fprintf('\nExecution %d/%d\n',i,numSweeps);
        [name, chans, data(:,:,i), time, success] = rtd(s, 11, args);
        if (success == true)
            avgData = mean(data, 3);
            plot(time, avgData)
        else
            disp('Test failure ... retrying');
            i = i - 1;
        end
    end
end

