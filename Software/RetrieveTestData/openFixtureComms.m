function [s] = openFixtureComms(CommPort, BaudRate)
    try % Attempt to open the PEGMA comm port
        s = serial(CommPort);
        set(s,'BaudRate',BaudRate);
        s.InputBufferSize = 1048576;    %Allow 1MB input buffer size
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

