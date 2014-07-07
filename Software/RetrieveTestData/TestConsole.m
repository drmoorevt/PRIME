function [name, chans, data, time] = TestConsole(CommPort)
    s = openFixtureComms(CommPort, 921600);
    [name, chans, data, time] = runTest14(s, 1);
    closeFixtureComms(s);
    
    % Parse the data, call individual parsers to calculate EDP
    
    %http://www.mathworks.com/help/matlab/creating_plots/plotting-with-two-y-axes.html
    %stem3(time,chans,data)
    
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
