function [name, chans, data, time] = runTest21(CommPort, numSweeps, testLen, opDelay, profileList)
    for profListIdx = 1:numel(profileList)
        profIter = profileList(profListIdx);
        frameIdx = 1;
        if (opDelay(1) ~= 0)  % Are we looking for optimal tDelay?
            lowerLim = opDelay(1);
            upperLim = lowerLim * 2;
            while (lowerLim ~= upperLim)
                fprintf('\n\nTesting opDelay: %d\n\n', lowerLim);
                opDelay(1) = lowerLim;
                [fail, chans, data, time] = runTest11(CommPort, numSweeps,  testLen, opDelay, profIter);
                [upperLim, lowerLim] = binSearch(fail, upperLim, lowerLim);
                %set(gcf,'units','normalized','outerposition',[0 0 1 1])
                F(frameIdx) = getframe(gcf);
                frameIdx = frameIdx + 1;
                close all
            end
            fprintf('\n\nOptimal Delay: %d\n\n', lowerLim);
        elseif (opDelay(2) ~= 0) % Are we looking for optimal eDelay?
            lowerLim = opDelay(2);
            upperLim = lowerLim * 2;
            while (lowerLim ~= upperLim)
                fprintf('\n\nTesting opDelay: %d\n\n', lowerLim);
                opDelay(2) = lowerLim;
                [fail, chans, data, time] = runTest11(CommPort, numSweeps,  testLen, opDelay, profIter);
                [upperLim, lowerLim] = binSearch(fail, upperLim, lowerLim);
                %set(gcf,'units','normalized','outerposition',[0 0 1 1])
                F(frameIdx) = getframe(gcf);
                frameIdx = frameIdx + 1;
                close all
            end
            fprintf('\n\nOptimal Delay: %d\n\n', lowerLim);
        elseif (opDelay(3) ~= 0) % Are we looking for optimal dDelay?
            lowerLim = opDelay(3);
            upperLim = lowerLim * 2;
            while (lowerLim ~= upperLim)
                fprintf('\n\nTesting opDelay: %d\n\n', lowerLim);
                opDelay(3) = lowerLim;
                [fail, chans, data, time] = runTest11(CommPort, numSweeps,  testLen, opDelay, profIter);
                [upperLim, lowerLim] = binSearch(fail, upperLim, lowerLim);
                %set(gcf,'units','normalized','outerposition',[0 0 1 1])
                F(frameIdx) = getframe(gcf);
                frameIdx = frameIdx + 1;
                close all
            end
            fprintf('\n\nOptimal Delay: %d\n\n', lowerLim);
        end
        close all
        figure
        movie(gcf,F,1000000,3)
    end
    name = lowerLim; % return the optimal delay result
end