function [name, chans, data, time] = analyzeTest11(name, chans, data, time)
    % determine the input and output power for the test
    inPower  = data(:,:,1) .* data(:,:,2);
    outPower = data(:,:,1) .* data(:,:,3);
    
    % determine the demarcations between states of the write
    preTestIdleStartIdx   = 1
    preTestIdleEndIdx     = findStateDemarcation(data, preTestIdleStartIdx)
    writeStartIdx         = preTestIdleEndIdx
    writeEndIdx           = findStateDemarcation(data, writeStartIdx)
    delayStartIdx         = writeEndIdx
    delayEndIdx           = findStateDemarcation(data, writeStartIdx)
    readBackStartIdx      = delayEndIdx
    readBackEndIdx        = findStateDemarcation(data, delayEndIdx)
    postTestDelayStartIdx = readBackEndIdx
    postTestDelayEndIdx   = findStateDemarcation(data, readBackEndIdx)
    
    preTestIdleDuration  = time(preTestIdleEndIdx - preTestIdleStartIdx)
    preTestIdleInPower   = sum(inPower(preTestIdleStartIdx : preTestIdleEndIdx))  / ...
                          (preTestIdleEndIdx - preTestIdleStartIdx);
    preTestIdleOutPower  = sum(outPower(preTestIdleStartIdx : preTestIdleEndIdx)) / ...
                          (preTestIdleEndIdx - preTestIdleStartIdx);
    preTestIdleInEnergy  = preTestIdleInPower  * preTestIdleDuration
    preTestIdleOutEnergy = preTestIdleOutPower * preTestIdleDuration
    
    writeDuration = time(writeEndIdx) - time(writeStartIdx)
    writeInPower  = sum(inPower(writeStartIdx : writeEndIdx))  / ...
                    (writeEndIdx - writeStartIdx);
    writeOutPower = sum(outPower(writeStartIdx : writeEndIdx)) / ...
                    (writeEndIdx - writeStartIdx);
    writeInEnergy = preTestIdleInPower  * preTestIdleDuration
    writeOutEnergy = preTestIdleOutPower * preTestIdleDuration
    
    inEnergy  = sum(data(:,:,1)) .* sum(data(:,:,2));
    outEnergy = sum(data(:,:,1)) .* sum(data(:,:,2));
    
end