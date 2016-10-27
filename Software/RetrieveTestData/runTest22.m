function [fail, chans, data, time] = runTest22(CommPort, numSweeps, testLen, opDelay, profileList)
    for profListIdx = 1:numel(profileList)
        profIter = profileList(profListIdx);
        [fail, chans, data, time, F1, retDelay] = optimizeDelay(12, CommPort, numSweeps, testLen, opDelay, 1, profIter);
        [fail, chans, data, time, F2, retDelay] = optimizeDelay(12, CommPort, numSweeps, testLen, retDelay, 2, profIter);
        fprintf('\Optimal delays: [ %s]\n', sprintf('%d ', retDelay));
        F = [F1, F2];
        close all
        figure
        movie(gcf,F,1000000,3)
    end
end