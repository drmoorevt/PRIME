function [name, chans, data, time] = TestConsole(CommPort)
    close all
    
    numSweeps = 100;
    eeFails = 0;
    sfFails = 0;
    sdFails = 0;
    htFails = 0;
    
   [eeFails, chans, data, time] = runTest11(CommPort, 921600, numSweeps);
   %[sfFails, chans, data, time] = runTest12(CommPort, 921600, numSweeps);
   %[sdFails, chans, data, time] = runTest13(CommPort, 921600, numSweeps, 150); % Lexar
   %[sdFails, chans, data, time] = runTest13(CommPort, 921600, numSweeps, 150); % SanDisk
   %[sdFails, chans, data, time] = runTest13(CommPort, 921600, numSweeps, 65);  % SwissBit
   %[sdFails, chans, data, time] = runTest13(CommPort, 921600, numSweeps, 2);   % Kingston
   %[htFails, chans, data, time] = runTest14(CommPort, 921600, numSweeps);
    
   %fprintf('Dev\tFails\nEE:\t%d\nSF:\t%d\nSD:\t%d\nHT:\t%d\n', eeFails, sfFails, sdFails, htFails);
    
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
