function [results, bytesReceived] = rtd(s, bitRes, numChannels, bytesToGet)
    fprintf('Getting %d bytes from test fixture... \n', bytesToGet);
    if (false == wfb(s, bytesToGet, 1000))
        return
    end
    fprintf('Received %d bytes of data info\n',s.BytesAvailable);
    
    bytesPerChannel = bytesToGet / numChannels;
    for i = 1:numChannels
        inRead = fread(s, double(bytesPerChannel/2), 'ushort');
        inRead = inRead * bitRes(i);
        data(:,i) = inRead;
    end
    results  = data;
    bytesReceived = bytesToGet;
end