function [name, chans, data, time] = runTest14(s, numSweeps)
    success = false;
    writeBuffer(1:128) = uint8(0);
    args = argGenTest14(3000, 5, 3000, 0, 1, 1, 0);
    for i = 1:numSweeps
        fprintf('\nExecution %d/%d\n',i,numSweeps);
        [name, chans, data(:,:,i), time, success] = rtd(s, 14, args);
        if (success == true)
            avgData = mean(data, 3);
            plot(time, avgData)
        else
            disp('Test failure ... retrying');
        end
    end
end

