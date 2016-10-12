function [ output_args ] = runTest1(CommPort, numAvgs, testLen, opDelay)
    delete(instrfindall);
    s = openFixtureComms(CommPort);
    
    startVolts   = 1.8;
    endVolts     = 3.3;
    voltIter     = 0.5;
    startCurrent = .1;
    endCurrent   = 10;
    currIter     = 1;
    
    filename = sprintf('./results/Test1_%.3fV-%.3fV(%.3fV)_%.3fmA-%.3fmA(%.3fmA).mat', ...
                       startVolts, endVolts, voltIter, startCurrent, endCurrent, currIter);
                   
    numVoltMeas = round((endVolts - startVolts) / voltIter) + 1;
    numCurrMeas = round((endCurrent - startCurrent) / currIter) + 1;
    
    dutInVolts   = ones(numVoltMeas, numCurrMeas) * 4.50;
    dutOutVolts  = zeros(numVoltMeas,numCurrMeas);
    dutInCur     = zeros(numVoltMeas,numCurrMeas);
    dutOutCur    = zeros(numVoltMeas,numCurrMeas);
    
    voltIdx = 0;
    for outVolts = startVolts:voltIter:endVolts % 1.8 to 3.3 in .1V increments
        voltIdx = voltIdx + 1;
        currIdx = 0;
        for outCurrent = startCurrent:currIter:endCurrent % 0 to 200ma in 1mA increments
            currIdx = currIdx + 1;
            writeBuffer = [typecast(outVolts, 'uint8'),  ...
                           typecast(outCurrent, 'uint8') ...
                           typecast(uint32(numAvgs), 'uint8')];
            args = argGen(1,                    ... // sampRate
                          testLen,              ... // testLen
                          opDelay,              ... // opDelay
                          0,                    ... // profile
                          1000,                 ... // preTestDelay
                          1000,                 ... // postTestDelay
                          0,                    ... // Destination address
                          writeBuffer           ... // Destination data
                          );
            if (s.BytesAvailable > 0)
                fread(s, s.BytesAvailable); % Clear out the serial port!
            end
            % Send the execute test command to a waiting test fixture
            testCommand = ['T','e','s', 't', 1, uint8(length(args)), args];
            crc = crc16(testCommand, '0000', '1021');
            testCommand = [testCommand, typecast(swapbytes(uint16(crc)), 'uint8')];
            fwrite(s, testCommand);
            
            fprintf('Testing %.3fV/%.3fmA ', outVolts, outCurrent);
            
            timeout = 10;
            while ((s.BytesAvailable < 24) && (timeout > 0))
                pause(0.1)
                timeout = timeout - 0.1;
            end
            if (timeout <= 0)
                fprintf('TIMED OUT ');
            end
            
            if (s.BytesAvailable >= 24)
                dutOutVolts(voltIdx, currIdx) = fread(s,  1, 'double');
                dutInCur(voltIdx, currIdx)    = fread(s,  1, 'double');
                dutOutCur(voltIdx, currIdx)   = fread(s,  1, 'double');
                fprintf('SUCCESS: %.3fVin/%.3fmAin %.3fVout/%.3fmAout = %.3fefficiency\n', ...
                         dutInVolts(voltIdx, currIdx),   ...
                         dutInCur(voltIdx, currIdx),     ...
                         dutOutVolts(voltIdx, currIdx),  ...
                         dutOutCur(voltIdx, currIdx),    ...
                         (dutOutVolts(voltIdx, currIdx) * dutOutCur(voltIdx, currIdx)) / ...
                          (dutInVolts(voltIdx, currIdx) * dutInCur(voltIdx, currIdx)));
            else
                fprintf('MISSED\n');
            end
            %pause(.05);
            %s = resetFixtureComms(s, CommPort);
            fwrite(s, uint8(hex2dec('54'))); % Attempt DUT reset
            %pause(.05);
        end
        %pause(1)
        save(filename,'dutInCur','dutInVolts','dutOutCur','dutOutVolts', ...
                      'startCurrent','endCurrent','startVolts','endVolts')
        %pause(1)
    end
    % eff = pOut / pIn = (vOut * iOut) / (vIn * iIn)
    eff = 100 .* ((dutOutVolts .* dutOutCur) ./ (dutInVolts .* dutInCur));
    close all
    %plot(dutOutCur, eff)
    surf(dutOutCur, dutOutVolts, eff)
    %surf(dutOutCur(:,2:end), dutOutVolts(:,2:end), eff(:,2:end))
    xlabel('Output Current (mA)');
    ylabel('Output Voltage (V)');
    zlabel('SMPS Efficiency');
    return
    xlim([startCurrent,endCurrent])
    ylim([startVolts,endVolts])
    
    figure()
    ampMin = 90;
    ampScale = ampMin / currIter;
    ovHi = dutOutVolts(:,ampScale:end);
    ivHi = dutInVolts(:,ampScale:end);
    oiHi = dutOutCur(:,ampScale:end);
    iiHi = dutInCur(:,ampScale:end);
    effHi = 100 .* ((ovHi .* oiHi) ./ (ivHi .* iiHi));
    surf(oiHi, ovHi, effHi)
    xlabel('Output Current (mA)');
    ylabel('Output Voltage (V)');
    zlabel('SMPS Efficiency');
    xlim([startCurrent + ampMin,endCurrent])
    ylim([startVolts,endVolts])
    
end

