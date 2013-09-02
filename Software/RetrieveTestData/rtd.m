function [channels,results] = rtd(s, test)
    testCommand = ['T','e','s','t','0'];
    testCommand(5) = test;
    crc = crc16(testCommand, 'FFFF', 'A001');
    testCommand(6) = uint8(bitshift(crc,-8));
    testCommand(7) = uint8(crc);

    fprintf(s,'%s',testCommand); % Execute test
    
    % wait for test data, timeout after 5 seconds
    i = 0;
    while((s.BytesAvailable < 3) && (i < 50))
        i = i + 1;
        pause(0.1);
    end;
    fprintf('Received %d bytes of header info\n',s.BytesAvailable);
    if (s.BytesAvailable < 3)
        return
    else
        header = fread(s, 3, 'uint8');
        numChannels     = header(1);
        bytesPerChannel = uint16(bitshift(uint16(header(2)), 8)) + uint16(header(3));
        %totalBytes = (bytesPerChannel + 1 ID byte) * numChannels
        totalBytes = numChannels * (bytesPerChannel + 1);
    end

    % wait for test data, timeout after 5 seconds
    i = 0;
    while((s.BytesAvailable < totalBytes) && (i < 50))
        i = i + 1;
        pause(0.1);
    end;
    fprintf('Received %d bytes of data info\n',s.BytesAvailable);

    chanNum = {};
    try
        for i = 1:numChannels
            inChan = fread(s, 1, 'uint8');
            chanNum = [chanNum, sprintf('Channel# %u',inChan)];
            inRead = fread(s, double(bytesPerChannel/2), 'ushort');
            data(:,i) = inRead;
        end
        channels = chanNum;
        results = data;
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
