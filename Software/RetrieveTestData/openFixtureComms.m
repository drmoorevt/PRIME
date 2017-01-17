function [s] = openFixtureComms(CommPort)
    try % Attempt to open the PEGMA comm port
        s = serial(CommPort,'BaudRate',115200,'InputBufferSize',4194304, ...
                            'Timeout',1, 'RequestToSend','off');
        fopen(s);
        if (s.BytesAvailable > 0)
            fread(s, s.BytesAvailable); % Clear out the serial port!
        end
        fwrite(s, uint8(hex2dec('54'))); % Attempt DUT reset
        pause(.5);
    catch ME
        ME
        fclose(s);
        delete(s);
        clear s;
        fprintf('Error opening CommPort. Available CommPorts are:\n');
        instrhwinfo ('serial')
        success = 'Exited with ERROR';
        return;
    end
end
