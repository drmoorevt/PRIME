function [name, chans, data, time] = runTest22(CommPort, numSweeps, testLen, opDelay, profileList)
    for profListIdx = 1:numel(profileList)
        profIter = profileList(profListIdx);
        frameIdx = 1;
        if (opDelay(3) == 0)  % Are we looking for optimal tDelay? Then eDelay will be zero
            lowerLim = opDelay(1); % begin by optimizing the erase delay
            upperLim = lowerLim * 2;
            while (lowerLim ~= upperLim)
                fprintf('\n\nTesting [erase delay, write delay]]: [%d,%d]\n\n', lowerLim, opDelay(2));
                [fail, chans, data, time] = runTest12(CommPort, numSweeps, testLen, [lowerLim, opDelay(2), opDelay(3), opDelay(4)], profIter);
                [upperLim, lowerLim] = binSearch(fail, upperLim, lowerLim);
                %set(gcf,'units','normalized','outerposition',[0 0 1 1])
                F(frameIdx) = getframe(gcf);
                frameIdx = frameIdx + 1;
                close all
            end
            eraseDelay = lowerLim * 1.05;  % increase by 5% for margin of error

            lowerLim = opDelay(2);  % now optimize the write delay
            upperLim = lowerLim * 2;
            while (lowerLim ~= upperLim)
                fprintf('\n\nTesting [erase delay, write delay]]: [%d,%d]\n\n', eraseDelay, lowerLim);
                [fail, chans, data, time] = runTest12(CommPort, numSweeps, testLen, [eraseDelay, lowerLim, opDelay(3), opDelay(4)], profIter);
                [upperLim, lowerLim] = binSearch(fail, upperLim, lowerLim);
                %set(gcf,'units','normalized','outerposition',[0 0 1 1])
                F(frameIdx) = getframe(gcf);
                frameIdx = frameIdx + 1;
                close all
            end
            writeDelay = lowerLim;

            fprintf('\n\nOptimal Erase Delay: %d\n\n', eraseDelay);
            fprintf('\n\nOptimal Write Delay: %d\n\n', writeDelay);

            close all
            figure
            movie(gcf,F,1000000,3)
        end
    end
    name = lowerLim; % return the optimal delay result
end