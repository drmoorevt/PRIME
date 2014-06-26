function [success, name, chans, data, time] = TestConsole(CommPort)
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
 
    for i = 1:3
        try
            fprintf('\nExecution #%d/3\n',i);
            [name, chans, data(:,:,i), time] = rtd(s, 13, 'ok!');
            pause(0.5);
        catch ME
            fprintf(ME);
        end
    end
    
    fclose(s);
    delete(s);
    clear s;
    %plot(time,data)
    %http://www.mathworks.com/help/matlab/creating_plots/plotting-with-two-y-axes.html
    stem3(time,chans,data)
    legend(chans)
    xlabel('Time (ms)');
    ylabel('Voltage (V)');
    title(name);
    success = 1;
    
    % determine the demarcations between states of the two writes
    i = 1;
    writing(1) = data(i, 4);
    while (writing(1) == data(i,4))
        i = i + 1;
    end
    writing(1) = i;
    delay(1) = data(i, 4);
    while (delay(1) == data(i,4))
        i = i + 1;
    end
    delay(1) = i;
    readback(1) = data(i, 4);
    while (readback(1) == data(i,4))
        i = i + 1;
    end
    readback(1) = i;
    idle(1) = data(i, 4);
    while (idle(1) == data(i,4))
        i = i + 1;
    end
    idle(1) = i;
    writing(2) = data(i, 4);
    while (writing(2) == data(i,4))
        i = i + 1;
    end
    writing(2) = i;
    delay(2) = data(i, 4);
    while (delay(2) == data(i,4))
        i = i + 1;
    end
    delay(2) = i;
    readback(2) = data(i, 4);
    while (readback(2) == data(i,4))
        i = i + 1;
    end
    readback(2) = i;
    idle(2) = 4096;
    
    voltage = data(1:4096, 1);
    current = data(1:4096, 3);
    power = voltage .* current;
    
    controlWriteDuration = time(writing(1)) % in milliseconds
    controlWritePower = sum(power(1:writing(1)) / writing(1));
    controlWriteEnergy = controlWritePower * controlWriteDuration;
    
    testWriteDuration = time(writing(2)) - time(idle(1)) % in milliseconds
    testWritePower = sum(power(idle(1):writing(2)) / (writing(2) - idle(1)));
    testWriteEnergy = testWritePower * testWriteDuration;
    
    controlDelayDuration = time(delay(1)) - time(writing(1)) % in milliseconds
    controlDelayPower = sum(power(writing(1):delay(1)) / (delay(1) - writing(1)));
    controlDelayEnergy = controlDelayPower * controlDelayDuration;
    
    testDelayDuration = time(delay(2)) - time(writing(2)) % in milliseconds
    testDelayPower = sum(power(writing(2):delay(2)) / (delay(2) - writing(2)));
    testDelayEnergy = testDelayPower * testDelayDuration;
    
    controlReadbackDuration = time(readback(1)) - time(delay(1)) % in milliseconds
    controlReadbackPower = sum(power(delay(1):readback(1)) / (readback(1) - delay(1)));
    controlReadbackEnergy = controlReadbackPower * controlReadbackDuration;
    
    testReadbackDuration = time(readback(2)) - time(delay(2)) % in milliseconds
    testReadbackPower = sum(power(delay(2):readback(2)) / (readback(2) - delay(2)));
    testReadbackEnergy = testReadbackPower * testReadbackDuration;
        
    controlIdleDuration = time(idle(1)) - time(readback(1)) % in milliseconds
    controlIdlePower = sum(power(readback(1):idle(1)) / (idle(1) - readback(1)));
    controlIdleEnergy = controlIdlePower * controlIdleDuration;
    
    testIdleDuration = time(idle(2)) - time(readback(2)) % in milliseconds
    testIdlePower = sum(power(readback(2):idle(2)) / (idle(2) - readback(2)));
    testIdleEnergy = testIdlePower * testIdleDuration;
    
    writeEnergyChange    = 100 * ((testWriteEnergy - controlWriteEnergy) / controlWriteEnergy);
    delayEnergyChange    = 100 * ((testDelayEnergy - controlDelayEnergy) / controlDelayEnergy);
    readbackEnergyChange = 100 * ((testReadbackEnergy - controlReadbackEnergy) / controlReadbackEnergy);
    idleEnergyChange     = 100 * ((testIdleEnergy - controlIdleEnergy) / controlIdleEnergy);
    totalEnergyChange    = 100 * (((testWriteEnergy+testDelayEnergy+testReadbackEnergy+testIdleEnergy) - (controlWriteEnergy+controlDelayEnergy+controlReadbackEnergy+controlIdleEnergy)) / (controlWriteEnergy+controlDelayEnergy+controlReadbackEnergy+controlIdleEnergy));
    
    writeText = sprintf('Write energy change: %f%%',writeEnergyChange);
    delayText = sprintf('Delay energy change: %f%%',delayEnergyChange);
    readbackText = sprintf('Readback energy change: %f%%',readbackEnergyChange);
    idleText = sprintf('Idle energy change: %f%%',idleEnergyChange);
    
    figure(2);
    power(:,2) = data(1:4096, 4);
    plot(time,power);
    newTitle = sprintf('Energy savings: %f,%%',totalEnergyChange);
    title(newTitle);
    xlabel('Time (ms)');
    ylabel('Power (Na)');
    text(2, 3.25, writeText);
    text(2, 3.00, delayText);
    text(2, 2.75, readbackText);
    text(2, 2.50, idleText);
    
    return
    % End selection bypass
    
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
            
        