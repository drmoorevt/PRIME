function [title,channels,results,time,dataBytes] = execTest(s, test, args)
    title = {};
    channels = {};
    results = {};
    time = {};
    dataBytes = 0;
    testBytes = 0;
    
    % Send the execute test command to a waiting test fixture
    testCommand = ['T','e','s', 't', test, uint8(length(args)), args];
    crc = crc16(testCommand, '0000', '1021');
    testCommand = [testCommand, typecast(swapbytes(uint16(crc)), 'uint8')];
    fwrite(s, testCommand);
    
    % Retrieve and acknowledge the test header
    i = 5;
    while (dataBytes == 0)
        try
            if (s.BytesAvailable > 0)
                fread(s, s.BytesAvailable); % Clear out the serial port!
            end
            [title, channels, time, bitRes, numChannels, timeScaleMicroSec, ...
            dataBytes] = rth(s);
            if (dataBytes)   % Header retrieval success, send ACK and proceed
                fwrite(s, uint8(hex2dec('11')));
                break;
            end
        catch
            if (i > 0) % Header retrieval failure, send NAK and retry
                i = i - 1;
                pause(0.100);
                fprintf('Header retrieval failure: retrying\n');
                fwrite(s, uint8(hex2dec('22')));
            else           % Test failure, quit
                fprintf('Header retrieval attempts exhausted: exiting\n');
                fwrite(s, uint8(hex2dec('54')));
                pause(0.500);
                return; 
            end
        end
    end
    
    % Retrieve and acknowledge the test results
    i = 3;
    while (testBytes == 0)
        try
            [results, testBytes] = rtd(s, bitRes, numChannels, dataBytes);
            if (testBytes == dataBytes)   % Success, send ACK and proceed
                fwrite(s, uint8(hex2dec('88')));
                break;
            end
        catch
            pause(0.100);
            if (s.BytesAvailable > 0)
                fread(s, s.BytesAvailable); % Clear out the serial port!
            end
            if (i > 0) % Header retrieval failure, send NAK and retry
                i = i - 1;
                fprintf('Data retrieval failure: retrying\n');
                fwrite(s, uint8(hex2dec('99')));
            else           % Test failure, quit
                fprintf('Data retrieval attempts exhausted: exiting\n');
                fwrite(s, uint8(hex2dec('54')));
                pause(0.500);
                return; 
            end
        end
    end
    
    bytesPerChannel = dataBytes / numChannels;
    time = (1:1:bytesPerChannel/2) * (timeScaleMicroSec / 1000.0);
end
