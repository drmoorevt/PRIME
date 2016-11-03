function [numFailures, chans, data, time] = runTest14(CommPort, numSweeps, testLen, opDelay, profileList)
    numFailures = 0;
    delete(instrfindall);
    s = openFixtureComms(CommPort);
    
    preTestDelay = 1000;
    postTestDelay = 1000;
    testTime = (preTestDelay + testLen + postTestDelay) / 1000;
    
    for profListIdx = 1:numel(profileList)
        profIter = profileList(profListIdx);
        sweepIter = 1;
        while sweepIter <= numSweeps
            address = uint32((2^29)*rand/4096)*4096;
            writeBuffer(1:128) = uint8(rand(1,128)*128);
            args = argGen(1,                    ... // sampRate
                          testLen,              ... // testLen
                          opDelay,              ... // opDelay
                          uint32(profIter - 1), ... // profile
                          preTestDelay,         ... // preTestDelay
                          postTestDelay,        ... // postTestDelay
                          address,              ... // Destination address
                          writeBuffer           ... // Destination data
                          );
            fprintf('\nExecution %d/%d\nAddress: %d\n', sweepIter, numSweeps, address);
            fprintf('Data: %s\n', dec2hex(writeBuffer));
            success = false;
            try
                 [name(:,profIter), chans, data(:,:,sweepIter,profIter), time, timeArray(sweepIter,:), success] ...
                     = execTest(s, 14, args);
%                [name(:,profIter), chans, data1, time, timeArray(sweepIter,:), success] ...
%                    = execTest(s, 14, args);
                testPassed = strfind(name(profIter),'Passed');
                if (cellfun('isempty', testPassed))
                    numFailures = numFailures + 1;
                    fprintf('\n******WRITE ERROR DETECTED*****\n');
                    err = MException('Test14:WriteFailure', 'The write did not complete as expected');
                    throw(err);
                end
                avgData = mean(data, 3);
                sweepIter = sweepIter + 1;
            catch
                s = resetFixtureComms(s, CommPort);
                disp('Test failure ... retrying');
            end
        end
        filename = sprintf('./results/%s Test14-Profile%d-%dSweeps.mat', ...
                           datestr(now,'dd-mm-yy HH.MM.SS'), profIter, sweepIter-1);
        save(filename,'name','chans','avgData','time','timeArray')
        %testPlot(avgData(:,:,profIter), time, chans, name(:,profIter), testTime, mean(avgData(:,3))*2);
        
         maTitle = strrep(name(:,profIter), 'Passed', 'Passed (50 Sample Moving Average)');
         maTitle = strrep(maTitle, 'Failed', 'Failed (50 Sample Moving Average)');
         movingAverage = avgData(:,:,profIter);
         movingAverage(50:end-50,1) = conv(movingAverage(50:end-50,1), ones(50,1)/50, 'same');
         movingAverage(50:end-50,2) = conv(movingAverage(50:end-50,2), ones(50,1)/50, 'same');
         movingAverage(50:end-50,3) = conv(movingAverage(50:end-50,3), ones(50,1)/50, 'same');
         testPlot(movingAverage(:,:), time, chans, maTitle(:,1), 0, 56);
        
%         figure('Color', 'white');
%         modStr = strrep(name(profIter), 'Passed', '');
%         modStr = deblank(modStr);
%         histfit(timeArray(:,2),max(24,round(numSweeps/10)),'kernel');
%         t1 = title(sprintf('%s Delay Required (n = %d)',modStr{:}, numSweeps));
%         set(t1,{'FontSize'},{10.0});
%         set(gca, 'XLim', [0,max(timeArray(:,2))])
%         xlabel('SPI Read Attempts Before Success');
%         ylabel('Number of Occurrences');
%         
%         %%% Create the labels
%         mnlabel =  sprintf('Mean -- %3.2f', mean(timeArray(:,2)));
%         melabel =  sprintf('Median -- %3.2f', median(timeArray(:,2)));
%         molabel =  sprintf('Mode -- %3.2f', mode(timeArray(:,2)));
%         stdlabel = sprintf('Std Deviation -- %3.2f', std(timeArray(:,2)));
%         
%         %%% Create the textbox
%         h = annotation('textbox',[0.40 0.80 0.10 0.10]);
%         set(h,'String',{mnlabel,melabel,molabel,stdlabel});
    end
end
