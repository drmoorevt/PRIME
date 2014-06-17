function [title,channels,results,time] = rtd(s, test, arg)
    argC = uint8(length(arg));
    testCommand = ['T','e','s','t','0','0'];
    testCommand(5) = test;
    testCommand(6) = argC;
    testCommandWithArgs = strcat(testCommand, arg);
    crc = crc16(testCommandWithArgs, '0000', 'A001');
    testCommandWithArgs(7 + argC) = uint8(bitshift(crc,-8));
    testCommandWithArgs(8 + argC) = uint8(crc);

    fprintf(s,'%s',testCommandWithArgs); % Execute test
    
    % wait for the size of the header, timeout after 30 seconds
    i = 0;
    while((s.BytesAvailable < 2) && (i < 150))
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
        title = {};
        titleRead = fread(s, 62, 'char');
        title = [title, sprintf('%s',titleRead(:,1))];
        timeScaleMicroSec = fread(s, 1, 'uint16');
        bytesPerChannel   = fread(s, 1, 'uint16');
        numChannels       = fread(s, 1, 'uint16');
        fprintf('Got the rest of the test header\n');
        fprintf('Sample period: %dus\n', timeScaleMicroSec);
        fprintf('Bytes per channel: %d\n', bytesPerChannel);
        fprintf('Number of channels: %d\n', numChannels);
        fprintf('Parsing channel headers\n');
        channels = {};
        for i = 1:numChannels
            chanNum = fread(s, 1, 'uint8');
            legend(:,i) = fread(s, 31, 'char');
            channels = [channels, sprintf('%s',legend(:,i))];
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
