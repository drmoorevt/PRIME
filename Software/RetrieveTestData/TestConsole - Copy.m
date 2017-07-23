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
    eeFails  = 0;
    sf1Fails = 0;
    sf2Fails = 0;
    sd1Fails = 0;
    htFails  = 0;
    
    % [[Delay #1 (Erase...)], [Delay #2 (Write...)], [Delay #3 (None yet)]]
    %           ^--- [Time Delay, Energy Delay, Differential Delay]
    eeTimingIODVS  = [[5000,   0,        0], [0,    0,       0], [0,0,0]];
    sf1TimingIODVS = [[150000, 0,        0], [5000, 0,       0], [0,0,0]];
    sf2TimingIODVS = [[25000,  0,        0], [2000, 0,       0], [0,0,0]];
    sd1TimingIODVS = [[175000, 0,        0], [0,    0,       0], [0,0,0]];
    sd2TimingIODVS = [[175000, 0,        0], [0,    0,       0], [0,0,0]];
    sd3TimingIODVS = [[65000,  0,        0], [0,    0,       0], [0,0,0]];
    sd4TimingIODVS = [[5000,   0,        0], [0,    0,       0], [0,0,0]];
    htTimingIODVS  = [[45000,  0,        0], [0,    0,       0], [0,0,0]];

    eeTimingACRT   = [[5000,   0,        0], [0,    0,       0], [0,0,0]];
    sf1TimingACRT  = [[150000, 0,        0], [5000, 0,       0], [0,0,0]];
    sf2TimingACRT  = [[25000,  0,        0], [2000, 0,       0], [0,0,0]];
    sd1TimingACRT  = [[175000, 0,        0], [0,    0,       0], [0,0,0]];
    sd2TimingACRT  = [[175000, 0,        0], [0,    0,       0], [0,0,0]];
    sd3TimingACRT  = [[65000,  0,        0], [0,    0,       0], [0,0,0]];
    sd4TimingACRT  = [[5000,   0,        0], [0,    0,       0], [0,0,0]];
    htTimingACRT   = [[45000,  0,        0], [0,    0,       0], [0,0,0]];

    eeTimingACRE   = [[5000,   200000,   0], [0,    0,       0], [0,0,0]];
    sf1TimingACRE  = [[150000, 7500000,  0], [5000, 1000000, 0], [0,0,0]];
    sf2TimingACRE  = [[25000,  5000000,  0], [2000, 750000,  0], [0,0,0]];
    sd1TimingACRE  = [[175000, 80000000, 0], [0,    0,       0], [0,0,0]];
    sd2TimingACRE  = [[175000, 80000000, 0], [0,    0,       0], [0,0,0]];
    sd3TimingACRE  = [[65000,  0,        0], [0,    0,       0], [0,0,0]];
    sd4TimingACRE  = [[5000,   0,        0], [0,    0,       0], [0,0,0]];
    htTimingACRE   = [[45000,  2000000,  0], [0,    0,       0], [0,0,0]];

    eeTimingACRD   = [[5000,   0,      110], [0,    0,       0], [0,0,0]];
    sf1TimingACRD  = [[150000, 0,      110], [5000, 0,     110], [0,0,0]];
    sf2TimingACRD  = [[25000,  0,      110], [2000, 0,     110], [0,0,0]];
    sd1TimingACRD  = [[175000, 0,      475], [0,    0,       0], [0,0,0]]; %475
    sd2TimingACRD  = [[175000, 0,      110], [0,    0,       0], [0,0,0]];
    sd3TimingACRD  = [[65000,  0,      110], [0,    0,       0], [0,0,0]];
    sd4TimingACRD  = [[15000,  0,      700], [0,    0,       0], [0,0,0]];
    htTimingACRD   = [[45000,  0,      110], [0,    0,       0], [0,0,0]];
    
    %numSweeps = 1024;
    %sd1TimingTest  =  [[0,      0,        0], [0,    0,       0], [0,0,0]];
    %[sdFails,  chans, data, time] = runTest14(CommPort, numSweeps, 100000,  sd1TimingTest,  [3]); % HIH
    %return
   
   %runTest1(CommPort, numSweeps, 7000,  [5000, 0, 0, 0]);
   %return
    
   %IODVS Tests
   %[eeFails,  chans, data, time] = runTest11(CommPort, numSweeps,   6000,  eeTimingIODVS, [1, 3]); % EEPROM
   %[sf1Fails, chans, data, time] = runTest12(CommPort, numSweeps, 250000, sf1TimingIODVS, [3]); % NOR
   %[sf2Fails, chans, data, time] = runTest13(CommPort, numSweeps,  80000, sf2TimingIODVS, [1, 3]); % NAND
   %[sd1Fails, chans, data, time] = runTest14(CommPort, numSweeps, 225000, sd1TimingIODVS, [1, 3]); % SanDisk
   %[sd2Fails, chans, data, time] = runTest14(CommPort, numSweeps, 225000, sd2TimingIODVS, [1, 3]); % Lexar
   %[sd3Fails, chans, data, time] = runTest14(CommPort, numSweeps, 100000, sd3TimingIODVS, [1, 3]); % SwissBit
   %[sd4Fails, chans, data, time] = runTest14(CommPort, numSweeps,  30000, sd4TimingIODVS, [1, 3]); % Kingston
   %[htFails,  chans, data, time] = runTest15(CommPort, numSweeps,  50000,  htTimingIODVS, [1, 3]); % HIH

   % Optimal Time Delay Tests
   %[eeFails,  chans, data, time] = runTest21(CommPort, numSweeps, 6000,  eeTimingACRT,  [3]); % EEPROM
   %[sf1Fails, chans, data, time] = runTest22(CommPort, numSweeps, 250000, sf1TimingACRT, [1, 3]); % NOR
   %[sf2Fails, chans, data, time] = runTest23(CommPort, numSweeps, 80000,  sf2TimingACRT, [1]); % NAND
     % All SDCard Optimal timing tests are invalid due to the presence of onboard caches + SPI polling
   %[sd1Fails, chans, data, time] = runTest24(CommPort, numSweeps, 225000, sd1TimingACRT, [1, 3]); % SanDisk
   %[sd2Fails, chans, data, time] = runTest24(CommPort, numSweeps, 225000, sd2TimingACRT, [1, 3]); % Lexar
   %[sd3Fails, chans, data, time] = runTest24(CommPort, numSweeps, 100000, sd3TimingACRT, [1, 3]); % SwissBit
   %[sd4Fails, chans, data, time] = runTest24(CommPort, numSweeps, 50000,  sd4TimingACRT, [1, 3]); % Kingston
     % End SDCard optimal timing tests
   %[htFails,  chans, data, time] = runTest25(CommPort, numSweeps, 50000,  htTimingACRT,  [1, 3]); % HIH
   
   % Optimal Energy Delay Tests
   %[eeFails,  chans, data, time] = runTest21(CommPort, numSweeps, 6000,   eeTimingACRE,  [1, 3]); % EEPROM
   %[sf1Fails, chans, data, time] = runTest22(CommPort, numSweeps, 250000, sf1TimingACRE, [1, 3]); % NOR
   %[sf2Fails, chans, data, time] = runTest23(CommPort, numSweeps, 80000,  sf2TimingACRE, [1, 3]); % NAND
     % All SDCard Optimal timing tests are invalid due to the presence of onboard caches + SPI polling
   %[sd1Fails, chans, data, time] = runTest24(CommPort, numSweeps, 225000, sd1TimingACRE, [1, 3]); % SanDisk
   %[sd2Fails, chans, data, time] = runTest24(CommPort, numSweeps, 225000, sd2TimingACRE, [1, 3]); % Lexar
   %[sd3Fails, chans, data, time] = runTest24(CommPort, numSweeps, 100000, sd3TimingACRE, [1, 3]); % SwissBit
   %[sd4Fails, chans, data, time] = runTest24(CommPort, numSweeps, 30000,  sd4TimingACRE, [1, 3]); % Kingston
     % End SDCard optimal energy tests
   %[htFails,  chans, data, time] = runTest25(CommPort, numSweeps, 50000,  htTimingACRE,  [1, 3]); % HIH
   
   % Optimal Differential Tests
   %[eeFails,  chans, data, time] = runTest11(CommPort, numSweeps, 6000,   eeTimingACRD,  [1, 3]); % EEPROM
   %[sf1Fails, chans, data, time] = runTest12(CommPort, numSweeps, 250000, sf1TimingACRD, [1, 3]); % NOR
   %[sf2Fails, chans, data, time] = runTest13(CommPort, numSweeps, 80000,  sf2TimingACRD, [1, 3]); % NAND
   %[sd1Fails, chans, data, time] = runTest14(CommPort, numSweeps, 225000, sd1TimingACRD, [1, 3]); % Sandisk
   %[sd2Fails, chans, data, time] = runTest14(CommPort, numSweeps, 250000, sd2TimingACRD, [1, 3]); % Lexar
   %[sd3Fails, chans, data, time] = runTest14(CommPort, numSweeps, 100000, sd3TimingACRD, [1, 3]); % SwissBit
    [sd4Fails, chans, data, time] = runTest14(CommPort, numSweeps, 30000,  sd4TimingACRD, [1, 3]); % Kingston
   %[htFails,  chans, data, time] = runTest15(CommPort, numSweeps, 50000,  htTimingACRD,  [1, 3]); % HIH
   
   %fprintf('Dev\tFails\nEE:\t%d\nSF:\t%d\nSD:\t%d\nHT:\t%d\n', eeFails, sfFails, sdFails, htFails);
   [inEnergy, outEnergy, inEnergyDelta, outEnergyDelta, stateDurations] = analyzeTest(data, time)
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
