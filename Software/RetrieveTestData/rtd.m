%RetrieveTestData
s = serial('COM13');
set(s,'BaudRate',115200);
s.InputBufferSize = 65536;
fopen(s);

fprintf(s,'Test09'); % Execute test

% wait for test data, timeout after 5 seconds, print byteReceived as it
% comes in
i = 0;
while((s.BytesAvailable < 24576) && (i < 50))
    i = i + 1;
    pause(0.1);
    s.BytesAvailable
end;

try
    for i = 1:3
        inRead = fread(s, 4096, 'ushort');
        data(:,i) = inRead;
    end
catch ME
    ME
    fclose(s);
    delete(s);
    clear s;
    fprintf('error\n');
    return;
end

plot(data)

fclose(s);
delete(s);
clear s;