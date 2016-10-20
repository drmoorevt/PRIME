function [fail, chans, data, time] = runTest23(CommPort, numSweeps, testLen, opDelay, profileList)
    for profListIdx = 1:numel(profileList)
        profIter = profileList(profListIdx);
        [fail, chans, data, time, F1, opDelay] = optimizeDelay(13, CommPort, numSweeps, testLen, opDelay, 1, profIter);
        [fail, chans, data, time, F2, opDelay] = optimizeDelay(13, CommPort, numSweeps, testLen, opDelay, 2, profIter);
        fprintf('\Optimal delays: [ %s]\n', sprintf('%d ', opDelay));
        F = [F1, F2];
        close all
        figure
        movie(gcf,F,1000000,3)
    end
end