function [fail, chans, data, time] = runTest25(CommPort, numSweeps, testLen, opDelay, profileList)
    for profListIdx = 1:numel(profileList)
        profIter = profileList(profListIdx);
        [fail, chans, data, time, F1, opDelay] = optimizeDelay(15, CommPort, numSweeps, testLen, opDelay, 1, profIter);
        fprintf('\Optimal delays: [ %s]\n', sprintf('%d ', opDelay));
        close all
        figure
        movie(gcf,F1,1000000,3)
    end
end