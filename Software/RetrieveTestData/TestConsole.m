function [success, name, chans, data, time] = TestConsole(CommPort)
    %s = openFixtureComms(CommPort, 460800);
    s = openFixtureComms(CommPort, 921600);
    [name, chans, data, time] = runTest13(s, 10);
    closeFixtureComms(s);
    
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
            
        