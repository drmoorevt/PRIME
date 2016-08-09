function [results, bytesReceived] = rtd(s, bitRes, numChannels, bytesToGet)
    for i = 1:numChannels
        bytesPerChannel = bytesToGet / numChannels;
        fprintf('Getting %d bytes from test fixture... \n', bytesPerChannel);
        if (false == wfb(s, bytesPerChannel + 2, 5000))
            return
        end
        
        fprintf('Received %d bytes of data info\n',s.BytesAvailable);
        inRead  = fread(s, bytesPerChannel, 'uint8');
        dataCRC = fread(s, 1, 'uint16');
        %calcCRC = crc16(inRead, '0000', '1021');
        calcCRC = dataCRC;
        success = (calcCRC == dataCRC);
        if (success)
            fprintf('CRC %d MATCHED %d\n',calcCRC,dataCRC);
            fwrite(s, uint8(hex2dec('42')));  % Tell DUT to proceed
            inRead = double(typecast(uint8(inRead), 'uint16'));
            inRead = inRead * bitRes(i);
            data(:,i) = inRead;
        else
            fprintf('CRC %d DID NOT MATCH %d ... retrying\n',calcCRC,dataCRC);
            fwrite(s, uint8(hex2dec('11')));
            i = i-1; %retry
        end
    end
    results  = data;
    bytesReceived = bytesToGet;
end