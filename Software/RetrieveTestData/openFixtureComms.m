function [s] = openFixtureComms(CommPort, BaudRate)
    try % Attempt to open the PEGMA comm port
        s = serial(CommPort,'BaudRate',BaudRate,'InputBufferSize',33554432);
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

