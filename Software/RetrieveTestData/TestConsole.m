function [success, name, chans, data, time] = TestConsole(CommPort)
    s = openFixtureComms(CommPort, 460800); % Open the comm port...
%   s = openFixtureComms(CommPort, 115200); % Open the comm port...
    success = false;
    numSweeps = 100; % Run the test...
    args = argGenTest11(250, 1, 250, 0, [0:127], 0, 128);
    for i = 1:numSweeps
%        try
            fprintf('\nExecution %d/%d\n',i,numSweeps);
            [name, chans, data(:,:,i), time, success] = rtd(s, 11, args);
            if (success == true)
                avgData = mean(data,3);
                plot(time, avgData)
            else
                disp('Test failure ... retrying');
                numSweeps = numSweeps - 1;
            end
%        catch ME
%            delete(instrfindall)
%            fprintf(ME);
%        end
    end
    
    closeFixtureComms(s); % Close the comm port...
    
    % Parse the data, call individual parsers to calculate EDP
    
    
    %http://www.mathworks.com/help/matlab/creating_plots/plotting-with-two-y-axes.html
    %stem3(time,chans,data)
    legend(chans)
    xlabel('Time (ms)');
    ylabel('Voltage (V)');
    title(name);
    success = 1;
    
    return
    % End selection bypass
    
    while (1)
        fprintf('PEGMA Test Console\n');
        fprintf('0: Close serial port and exit program\n');
        fprintf('1: Run test and retrieve test data\n');
        inString = input('Input command: ');
        switch (inString)
            case 0
                fclose(s);
                delete(s);
                clear s;
                success = 'Exited Successfully';
                return;
            case 1
                testNum = input('  Test to run: ');
                rtd(s, testNum);
            otherwise
        end
    end
end
            
        