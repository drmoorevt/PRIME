function [fail, chans, retData, time] = runTest25(CommPort, numSweeps, testLen, opDelay, profileList)
    for profListIdx = 1:numel(profileList)
        profIter = profileList(profListIdx);
        [fail, chans, data, time, F1, opDelay] = optimizeDelay(15, CommPort, numSweeps, testLen, opDelay, 1, profIter);
        fprintf('Optimal delays: [ %s]\n', sprintf('%d ', opDelay));
        close all
        filename = sprintf('./results/%s Test25-Profile%d-OpDelays[%s].gif', ...
                           datestr(now,'dd-mm-yy HH.MM.SS'), profIter, sprintf('%d ', opDelay));
        movie2gif(F, filename)
        retData(:,:,1,profIter) = data(:,:,1,profIter);
    end
end