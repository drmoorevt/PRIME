function [channels,results,time] = rtd(s, test)
    testCommand = ['T','e','s','t','0'];
    testCommand(5) = test;
    crc = crc16(testCommand, 'FFFF', 'A001');
    testCommand(6) = uint8(bitshift(crc,-8));
    testCommand(7) = uint8(crc);

    fprintf(s,'%s',testCommand); % Execute test
    
    % wait for the size of the header, timeout after 20 seconds
    i = 0;
    while((s.BytesAvailable < 2) && (i < 200))
        i = i + 1;
        pause(0.1);
    end;
    if (s.BytesAvailable < 2)
        return % No response from DUT, bail out
    else       % got the size of the header
        sizeofHeader = fread(s, 1, 'uint16');
        fprintf('Test indicates that header is %d bytes long\n',sizeofHeader);
    end

    % wait for the rest of the header, timeout after 5 seconds
    i = 0;
    while((s.BytesAvailable < (sizeofHeader  - 2)) && (i < 50))
        i = i + 1;
        pause(0.1);
    end;
    if (s.BytesAvailable < (sizeofHeader  - 2))
        return % Not enough bytes for a header from DUT, bail out
    else       % got the rest of the header, parse it
        timeScaleMicroSec = fread(s, 1, 'uint16');
        bytesPerChannel   = fread(s, 1, 'uint16');
        numChannels       = fread(s, 1, 'uint16');
        fprintf('Got the rest of the test header\n');
        fprintf('Sample period: %dus\n', timeScaleMicroSec);
        fprintf('Bytes per channel: %d\n', bytesPerChannel);
        fprintf('Number of channels: %d\n', numChannels);
        fprintf('Parsing channel headers\n');
        chanNum = {};
        for i = 1:numChannels
            inChan = fread(s, 1, 'uint8');
            chanNum = [chanNum, sprintf('Channel# %u',inChan)];
            title(:,i) = fread(s, 15, 'uint8');
            bitRes(:,i) = fread(s, 1, 'double');
        end
    end
    testBytes = numChannels * bytesPerChannel;
    
    
        %numChannels     = sizeofHeader(1);
        %bytesPerChannel = uint16(bitshift(uint16(sizeofHeader(2)), 8)) + uint16(sizeofHeader(3));
        %totalBytes = (bytesPerChannel + 1 ID byte) * numChannels
        %totalBytes = numChannels * (bytesPerChannel + 1);
    
    % wait for test data, timeout after 5 seconds
    i = 0;
    while((s.BytesAvailable < testBytes) && (i < 50))
        i = i + 1;
        pause(0.1);
    end;
    fprintf('Received %d bytes of data info\n',s.BytesAvailable);

    try
        for i = 1:numChannels
            inRead = fread(s, double(bytesPerChannel/2), 'ushort');
            inRead = inRead * bitRes(i);
            data(:,i) = inRead;
        end
        channels = chanNum;
        results  = data;
        time = (1:1:bytesPerChannel/2) * (timeScaleMicroSec / 1000.0);
        return
    catch ME
        ME
        fclose(s);
        delete(s);
        clear s;
        fprintf('error\n');
        return;
    end
end
