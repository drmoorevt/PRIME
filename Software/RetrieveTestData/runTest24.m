function [name, chans, data, time] = runTest24(CommPort, numSweeps, testLen, opDelay, profileList)
    for profListIdx = 1:numel(profileList)
        profIter = profileList(profListIdx);
        frameIdx = 1;
        if (opDelay(3) == 0)  % Are we looking for optimal tDelay? Then eDelay will be zero
            lowerLim = opDelay(1);
            upperLim = lowerLim * 2;
            while (lowerLim ~= upperLim)
                fprintf('\n\nTesting tDelay: %d\n\n', lowerLim);
                [fail, chans, data, time] = runTest14(CommPort, numSweeps,  testLen,  [lowerLim, opDelay(2), opDelay(3), opDelay(4)], profIter);
                [upperLim, lowerLim] = binSearch(fail, upperLim, lowerLim);
                %set(gcf,'units','normalized','outerposition',[0 0 1 1])
                F(frameIdx) = getframe(gcf);
                frameIdx = frameIdx + 1;
                close all
            end
            fprintf('\n\nOptimal tDelay: %d\n\n', lowerLim);
        else  % otherwise we are looking for the optimal eDelay
            lowerLim = opDelay(3);
            upperLim = lowerLim * 2;
            while (lowerLim ~= upperLim)
                fprintf('\n\nTesting eDelay: %d\n\n', lowerLim);
                [fail, chans, data, time] = runTest14(CommPort, numSweeps,  testLen,  [opDelay(1), opDelay(2), lowerLim, opDelay(4)], profIter);
                [upperLim, lowerLim] = binSearch(fail, upperLim, lowerLim);
                %set(gcf,'units','normalized','outerposition',[0 0 1 1])
                F(frameIdx) = getframe(gcf);
                frameIdx = frameIdx + 1;
                close all
            end
            fprintf('\n\nOptimal eDelay: %d\n\n', lowerLim);
        end
        close all
        figure
        movie(gcf,F,1000000,3)
    end
end