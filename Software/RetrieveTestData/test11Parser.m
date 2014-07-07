function [totalEnergy, energy, inPower, outPower] = test11ResultParser(data)
    numElemPerSet  = numel(data(:,4));
    % determine the demarcations between states of the write
    preTestIdleIdx   = findStateDemarcation(data, 1);
    writeIdx         = findStateDemarcation(data, preTestIdleIdx);
    delayIdx         = findStateDemarcation(data, writeIdx);
    readBackIdx      = findStateDemarcation(data, delayIdx);
    postTestDelayIdx = findStateDemarcation(data, readBackIdx);
    
    voltage    = data(1:numElemPerSet, 1);
    inCurrent  = data(1:numElemPerSet, 2);
    outCurrent = data(1:numElemPerSet, 3);
    power = voltage .* outCurrent;
    
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
end