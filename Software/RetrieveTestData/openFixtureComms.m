function [s] = openFixtureComms(CommPort, BaudRate)
    try % Attempt to open the PEGMA comm port
        s = serial(CommPort);
        set(s,'BaudRate',BaudRate);
        s.InputBufferSize = 8388608;    %Allow 8MB input buffer size
        fopen(s);
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

