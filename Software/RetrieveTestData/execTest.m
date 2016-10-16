function [title,channels,results,time,timing,dataBytes] = execTest(s, test, args)
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
    while (dataBytes == 0)
        try
            if (s.BytesAvailable > 0)
                fread(s, s.BytesAvailable); % Clear out the serial port!
            end
            [title, channels, time, bitRes, numChannels, timeScaleMicroSec, ...
             timing, dataBytes] = rth(s);
            if (dataBytes)   % Header retrieval success, send ACK and proceed
                fwrite(s, uint8(hex2dec('11')));
                break;
            end
        catch
            return;
        end
    end
    
    % Retrieve and acknowledge the test results
    while (testBytes == 0)
        try
            [results, testBytes] = rtd(s, bitRes, numChannels, dataBytes);
            if (testBytes >= dataBytes)   % Success, send ACK and proceed
                fwrite(s, uint8(hex2dec('88')));
                break;
            end
        catch
            return;
        end
    end
    
    bytesPerChannel = dataBytes / numChannels;
    time = (1:1:bytesPerChannel/2) * (timeScaleMicroSec / 1000.0);
end
