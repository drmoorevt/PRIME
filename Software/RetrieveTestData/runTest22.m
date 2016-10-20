function [name, chans, data, time] = runTest22(CommPort, numSweeps, testLen, opDelay, profileList)
    for profListIdx = 1:numel(profileList)
        profIter = profileList(profListIdx);
        frameIdx = 1;
        % Determine the optimal erase delay first:
        if (opDelay(1) ~= 0)  % Are we looking for optimal tDelay?
            lowerLim = opDelay(1); % begin by optimizing the erase delay
            upperLim = lowerLim * 2;
            while (lowerLim ~= upperLim)
                fprintf('\n\nTesting [erase delay, write delay]]: [%d,%d]\n\n', lowerLim, opDelay(4));
                opDelay(1) = lowerLim;
                [fail, chans, data, time] = runTest12(CommPort, numSweeps, testLen, opDelay, profIter);
                [upperLim, lowerLim] = binSearch(fail, upperLim, lowerLim);
                %set(gcf,'units','normalized','outerposition',[0 0 1 1])
                F(frameIdx) = getframe(gcf);
                frameIdx = frameIdx + 1;
                close all
            end
            opDelay(1) = lowerLim * 1.05;  % increase erase delay by 5% for margin of error

            lowerLim = opDelay(4);  % now optimize the write delay
            upperLim = lowerLim * 2;
            while (lowerLim ~= upperLim)
                fprintf('\n\nTesting [erase delay, write delay]]: [%d,%d]\n\n', opDelay(1), lowerLim);
                opDelay(4) = lowerLim;
                [fail, chans, data, time] = runTest12(CommPort, numSweeps, testLen, opDelay, profIter);
                [upperLim, lowerLim] = binSearch(fail, upperLim, lowerLim);
                %set(gcf,'units','normalized','outerposition',[0 0 1 1])
                F(frameIdx) = getframe(gcf);
                frameIdx = frameIdx + 1;
                close all
            end

            fprintf('\n\nOptimal Erase Delay: %d\n\n', opDelay(1));
            fprintf('\n\nOptimal Write Delay: %d\n\n', opDelay(4));

            close all
            figure
            movie(gcf,F,1000000,3)
            
        elseif (opDelay(2) ~= 0)  % Are we looking for optimal eDelay?
            lowerLim = opDelay(2); % begin by optimizing the erase delay
            upperLim = lowerLim * 2;
            while (lowerLim ~= upperLim)
                fprintf('\n\nTesting [erase delay, write delay]]: [%d,%d]\n\n', lowerLim, opDelay(4));
                opDelay(1) = lowerLim;
                [fail, chans, data, time] = runTest12(CommPort, numSweeps, testLen, opDelay, profIter);
                [upperLim, lowerLim] = binSearch(fail, upperLim, lowerLim);
                %set(gcf,'units','normalized','outerposition',[0 0 1 1])
                F(frameIdx) = getframe(gcf);
                frameIdx = frameIdx + 1;
                close all
            end
            opDelay(1) = lowerLim * 1.05;  % increase erase delay by 5% for margin of error

            lowerLim = opDelay(4);  % now optimize the write delay
            upperLim = lowerLim * 2;
            while (lowerLim ~= upperLim)
                fprintf('\n\nTesting [erase delay, write delay]]: [%d,%d]\n\n', opDelay(1), lowerLim);
                opDelay(4) = lowerLim;
                [fail, chans, data, time] = runTest12(CommPort, numSweeps, testLen, opDelay, profIter);
                [upperLim, lowerLim] = binSearch(fail, upperLim, lowerLim);
                %set(gcf,'units','normalized','outerposition',[0 0 1 1])
                F(frameIdx) = getframe(gcf);
                frameIdx = frameIdx + 1;
                close all
            end

            fprintf('\n\nOptimal Erase Delay: %d\n\n', opDelay(1));
            fprintf('\n\nOptimal Write Delay: %d\n\n', opDelay(4));

            close all
            figure
            movie(gcf,F,1000000,3)
        
        elseif (opDelay(3) ~= 0) % Are we looking for optimal dDelay?
            lowerLim = opDelay(1); % begin by optimizing the erase delay
            upperLim = lowerLim * 2;
            while (lowerLim ~= upperLim)
                fprintf('\n\nTesting [erase delay, write delay]]: [%d,%d]\n\n', lowerLim, opDelay(4));
                opDelay(1) = lowerLim;
                [fail, chans, data, time] = runTest12(CommPort, numSweeps, testLen, opDelay, profIter);
                [upperLim, lowerLim] = binSearch(fail, upperLim, lowerLim);
                %set(gcf,'units','normalized','outerposition',[0 0 1 1])
                F(frameIdx) = getframe(gcf);
                frameIdx = frameIdx + 1;
                close all
            end
            opDelay(1) = lowerLim * 1.05;  % increase erase delay by 5% for margin of error

            lowerLim = opDelay(4);  % now optimize the write delay
            upperLim = lowerLim * 2;
            while (lowerLim ~= upperLim)
                fprintf('\n\nTesting [erase delay, write delay]]: [%d,%d]\n\n', opDelay(1), lowerLim);
                opDelay(4) = lowerLim;
                [fail, chans, data, time] = runTest12(CommPort, numSweeps, testLen, opDelay, profIter);
                [upperLim, lowerLim] = binSearch(fail, upperLim, lowerLim);
                %set(gcf,'units','normalized','outerposition',[0 0 1 1])
                F(frameIdx) = getframe(gcf);
                frameIdx = frameIdx + 1;
                close all
            end

            fprintf('\n\nOptimal Erase Delay: %d\n\n', opDelay(1));
            fprintf('\n\nOptimal Write Delay: %d\n\n', opDelay(4));

            close all
            figure
            movie(gcf,F,1000000,3)
        end
    end
    name = lowerLim; % return the optimal delay result
end