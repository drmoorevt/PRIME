%data1 = voltage
%data2 = inputCurrent
%data3 = outputCurrent
%data4 = state
function [name, chans, data, time] = runTest12(s, numSweeps)
    close all
    writeBuffer(1:128) = uint8(0);
    
    profIter = 0;
    while (profIter < 5) % Won't run XLP test
        loopIter = 1;
        dataIter = 1;
        args = argGenTest12(1000, 28, 0, uint32(profIter), writeBuffer, 0, 128);
        while loopIter <= numSweeps
            fprintf('\nExecution %d/%d\n', loopIter, numSweeps);
            success = false;
            [name, chans, data(:,:,dataIter), time, success] = rtd(s, 12, args);
            if (success == true)
                avgData = mean(data, 3);
                loopIter = loopIter + 1;
                dataIter = dataIter + 1;
            else
                disp('Test failure ... retrying');
            end
        end
        [axes, lines] = ploty4(time, avgData(:,1), time, avgData(:,2), time, avgData(:,3), time, avgData(:,4), chans)
        xlabel('Time (ms)');
        title(name);
        profIter = profIter + 1;
    end
end
