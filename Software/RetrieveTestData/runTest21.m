function [fail, chans, data, time] = runTest21(CommPort, numSweeps, testLen, opDelay, profileList)
    for profListIdx = 1:numel(profileList)
        profIter = profileList(profListIdx);
        [fail, chans, data, time, F1, retDelay] = optimizeDelay(11, CommPort, numSweeps, testLen, opDelay, 1, profIter);
        fprintf('\Optimal delays: [ %s]\n', sprintf('%d ', retDelay));
        close all
        figure
        movie(gcf,F1,1000000,3)
    end
end