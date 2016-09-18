function [name, chans, data, time] = TestConsole(CommPort)
    close all
    
%    interval = 1;
%    delete(instrfindall);
%    s = openFixtureComms(CommPort, 115200);   
%     while (1)
%         pause(interval);
%         KBps = s.BytesAvailable / (1024 * interval);
%         flushinput(s);
%         fprintf('\nKBps: %10.4f, Kbps: %10.4f', KBps, KBps * 8);
%     end

    numSweeps = 1;
    eeFails = 0;
    sfFails = 0;
    sdFails = 0;
    htFails = 0;
   
   %runTest1(CommPort, 921600, numSweeps, 7000,  [5000, 0, 0, 0]);
   %return
    
   %[eeFails, chans, data, time] = runTest11(CommPort, 921600, numSweeps, 7000,  [5000, 0, 0, 0]);    % EEPROM
   %[sfFails, chans, data, time] = runTest12(CommPort, 921600, numSweeps, 275000, [150000, 9, 0, 0]);   % NOR
   %[sdFails, chans, data, time] = runTest13(CommPort, 921600, numSweeps, 225000, [150000, 0, 0, 0]);  % Lexar
   %[sdFails, chans, data, time] = runTest13(CommPort, 921600, numSweeps, 225000, [150000, 0, 0, 0]);  % SanDisk
   %[sdFails, chans, data, time] = runTest13(CommPort, 921600, numSweeps, 100000, [65000, 0, 0, 0]);   % SwissBit
   %[sdFails, chans, data, time] = runTest13(CommPort, 921600, numSweeps, 15000,  [2000, 0, 0, 0]);    % Kingston
   %[htFails, chans, data, time] = runTest14(CommPort, 921600, numSweeps,  50000,  [45000, 0, 0, 0]);   % HIH

   %[eeFails, chans, data, time] = runTest21(CommPort, 921600, numSweeps, 7000,   [5000, 0, 0, 0]);   % EEPROM
   [sfFails, chans, data, time] = runTest22(CommPort, 921600, numSweeps, 275000, [150000, 5000, 0, 0]);   % NOR
   %[htFails, chans, data, time] = runTest24(CommPort, 921600, numSweeps,  50000,  [45000, 0, 0, 0]);   % HIH
   
   %fprintf('Dev\tFails\nEE:\t%d\nSF:\t%d\nSD:\t%d\nHT:\t%d\n', eeFails, sfFails, sdFails, htFails);
   %[inEnergy, outEnergy, inEnergyDelta, outEnergyDelta] = analyzeTest(data, time)
   return
   
   % End selection bypass
    
    while(1)
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
