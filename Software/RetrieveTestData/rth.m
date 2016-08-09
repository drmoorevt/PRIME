function [title, channels, time, bitRes, numChannels, ...
          timeScaleMicroSec, dataBytes] = rth(s)
    title = {};
    channels = {};
    time = {};
    dataBytes = 0;
    % wait for the size of the header, timeout after 2 seconds
    if (false == wfb(s, 4, 1000))
        fprintf('No header bytes received\n');
        return;
    end
    sizeofHeader = fread(s, 1, 'uint32');
    
    % wait for the rest of the header, (should have already arrived)
    if (false == wfb(s, sizeofHeader - 4, 100))
        fprintf('Not enough header bytes received\n');
        return;
    end
    titleRead         = fread(s, 64, 'uint8');
    timeScaleMicroSec = fread(s,  1, 'uint32');
    bytesPerChannel   = fread(s,  1, 'uint32');
    numChannels       = fread(s,  1, 'uint32');
    for i = 1:numChannels
        chanNum(:,i) = fread(s,  1, 'uint8');
        legend(:,i)  = fread(s, 31, 'uint8');
        bitRes(:,i)  = fread(s,  1, 'double');
        channels = [channels, sprintf('%s',legend(:,i))];
    end
    headerCRC = fread(s, 1, 'uint16');
    
    testHeader = typecast(uint32(sizeofHeader), 'uint8');
    testHeader = [testHeader, uint8(titleRead)'];
    testHeader = [testHeader, typecast(uint32(timeScaleMicroSec), 'uint8')];
    testHeader = [testHeader, typecast(uint32(bytesPerChannel), 'uint8')];
    testHeader = [testHeader, typecast(uint32(numChannels), 'uint8')];
    for i = 1:numChannels
        testHeader = [testHeader, uint8(chanNum(:,i)')];
        testHeader = [testHeader, uint8(legend(:,i)')];
        testHeader = [testHeader, typecast(bitRes(:,i)', 'uint8')];
    end
    calcCRC = crc16(testHeader, '0000', '1021');
    success = (calcCRC == headerCRC);
    
    if (success)
        title = [title, sprintf('%s',titleRead(:,1))];
        fprintf('Test indicates that header is %d bytes long\n',sizeofHeader);
        fprintf('Sample period: %dus\n', timeScaleMicroSec);
        fprintf('Bytes per channel: %d\n', bytesPerChannel);
        fprintf('Number of channels: %d\n', numChannels);
        dataBytes = bytesPerChannel * numChannels;
    else
        fprintf('Found a CRC error in the header!\n');
    end
end