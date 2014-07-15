function [name, chans, data, time] = TestConsole(CommPort)
    close all
    
    numSweeps = 1000;
%    [name, chans, data, time] = runTest11(CommPort, 921600, numSweeps);
%    [name, chans, data, time] = runTest12(CommPort, 921600, numSweeps);
%    [name, chans, data, time] = runTest13(CommPort, 921600, numSweeps);
    [name, chans, data, time] = runTest14(CommPort, 921600, numSweeps);
    return;
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
                %rtd(s, testNum);
            otherwise
        end
    end
end
