function [fail, chans, retData, time] = runTest24(CommPort, numSweeps, testLen, opDelay, profileList)
    for profListIdx = 1:numel(profileList)
        profIter = profileList(profListIdx);
        [fail, chans, data, time, F1, opDelay] = optimizeDelay(14, CommPort, numSweeps, testLen, opDelay, 1, profIter);
        fprintf('Optimal delays: [ %s]\n', sprintf('%d ', opDelay));
        close all
        filename = sprintf('./results/%s Test24-Profile%d-OpDelays[%s].gif', ...
                           datestr(now,'dd-mm-yy HH.MM.SS'), profIter, sprintf('%d ', opDelay));
        movie2gif(F, filename)
        retData(:,:,1,profIter) = data(:,:,1,profIter);
    end
end