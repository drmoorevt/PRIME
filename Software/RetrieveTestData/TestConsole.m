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

    %
    eeTimingIODVS  = [[5000,0,0],   [0,0,0],    [0,0,0]];
    sf1TimingIODVS = [[150000,0,0], [5000,0,0], [0,0,0]];
    sf2TimingIODVS = [[45000,0,0],  [0,0,0],    [0,0,0]];
    sd1TimingIODVS = [[150000,0,0], [0,0,0],    [0,0,0]];
    sd2TimingIODVS = [[150000,0,0],  [0,0,0],    [0,0,0]];
    sd3TimingIODVS = [[65000,0,0],  [0,0,0],    [0,0,0]];
    sd4TimingIODVS = [[2000,0,0],  [0,0,0],    [0,0,0]];
    htTimingIODVS  = [[45000,0,0],  [0,0,0],    [0,0,0]];

    numSweeps = 1;
    eeFails = 0;
    sfFails = 0;
    sdFails = 0;
    htFails = 0;
   
   %runTest1(CommPort, numSweeps, 7000,  [5000, 0, 0, 0]);
   %return
    
   %IODVS Tests
   %[eeFails, chans, data, time] = runTest11(CommPort, numSweeps, 7000,   eeTimingIODVS,  [1, 3]); % EEPROM
   
   %[sfFails, chans, data, time] = runTest12(CommPort, numSweeps, 275000, sf1TimingIODVS, [1, 3]); % NOR
   %[sfFails, chans, data, time] = runTest13(CommPort, numSweeps, 275000, sf2TimingIODVS, [1, 3]); % NAND
   
   %[sdFails, chans, data, time] = runTest13(CommPort, numSweeps, 225000, sd1TimingIODVS, [1, 3]); % Lexar
   %[sdFails, chans, data, time] = runTest13(CommPort, numSweeps, 225000, sd2TimingIODVS, [1, 3]); % SanDisk
   %[sdFails, chans, data, time] = runTest13(CommPort, numSweeps, 100000, sd3TimingIODVS, [1, 3]); % SwissBit
   %[sdFails, chans, data, time] = runTest13(CommPort, numSweeps, 15000,  sd4TimingIODVS, [1, 3]); % Kingston
   %[sdFails, chans, data, time] = runTest13(CommPort, numSweeps, 30000,  sd4TimingIODVS, [1]); % Kingston hack
   
   %[htFails, chans, data, time] = runTest14(CommPort, numSweeps, 50000,  htTimingIODVS,  [1, 3]); % HIH

   % Optimal Time Delay Tests
   %[eeFails, chans, data, time] = runTest21(CommPort, numSweeps, 7000,   [5000, 0, 0, 0], [1, 3]);      % EEPROM
   %[sfFails, chans, data, time] = runTest22(CommPort, numSweeps, 275000, [150000, 5000, 0, 0], [1, 3]); % NOR
   %[sdFails, chans, data, time] = runTest23(CommPort, numSweeps, 225000, [150000, 0, 0, 0], [1, 3]);    % Lexar
   %[htFails, chans, data, time] = runTest24(CommPort, numSweeps, 50000,  [45000, 0, 0, 0], [1]);     % HIH
   
   % Optimal Energy Delay Tests
   %[htFails, chans, data, time] = runTest14(CommPort, numSweeps,  100000,  [0, 0, 0, 70000], [1]);   % HIH
   
   %[htFails, chans, data, time] = runTest24(CommPort, numSweeps,  150000,  [0, 0, 0, 150000], [1, 3]);   % HIH
   
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
