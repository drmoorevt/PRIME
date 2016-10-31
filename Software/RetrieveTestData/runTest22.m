function [fail, chans, retData, time] = runTest22(CommPort, numSweeps, testLen, opDelay, profileList)
    for profListIdx = 1:numel(profileList)
        profIter = profileList(profListIdx);
        [fail, chans, data, time, F1, retDelay] = optimizeDelay(12, CommPort, numSweeps, testLen, opDelay,  1, profIter);
        [fail, chans, data, time, F2, retDelay] = optimizeDelay(12, CommPort, numSweeps, testLen, retDelay, 2, profIter);
        fprintf('Optimal delays: [ %s]\n', sprintf('%d ', retDelay));
        F = [F1, F2];
        close all
        filename = sprintf('./results/%s Test22-Profile%d-OpDelays[%s].gif', ...
                           datestr(now,'dd-mm-yy HH.MM.SS'), profIter, sprintf('%d ', opDelay));
        movie2gif(F, filename)
        retData(:,:,1,profIter) = data(:,:,1,profIter);
    end
end