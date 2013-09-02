function success = TestConsole(CommPort)
    % Attempt to open the PEGMA comm port
    try
        s = serial(CommPort);
        set(s,'BaudRate',115200);
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
 
    [chans,data] = rtd(s, 9);
    fclose(s);
    delete(s);
    clear s;
    plot(data)
    legend(chans)
    return
                
    while (1)
        fprintf('PEGMA Test Console\n');
        fprintf('0: Close serial port and exit program\n');
        fprintf('1: Run test and retrieve test data\n');
        inString = input('Input command: ');
        switch (inString)
            case 0
                fclose(s);
                delete(s);
                clear s;
                success = 'Exited Successfully';
                return;
            case 1
                testNum = input('  Test to run: ');
                rtd(s, testNum);
            otherwise
        end
    end
end
            
        