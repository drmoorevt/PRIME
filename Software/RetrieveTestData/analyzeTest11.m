% power data is (value:state:profile) over the length of time
% energy data (value:state:profile) but scalar
function [inAvePower, outAvePower, inEnergy, outEnergy] = analyzeTest11(name, chans, data, time)
    % determine the input and output power for the test
    % data is (values, (v/i/state), [nothing] -- 1, profile,
    data = data(:,:,1,:);
    inPower(:,:)  = data(:,1,:) .* data(:,2,:);
    outPower(:,:) = data(:,1,:) .* data(:,3,:);
    
    % calculate energy and power consumption for the preTestIdle phase
    preTestIdleStartIdx = [1, 1, 1, 1];
    preTestIdleEndIdx   = findStateDemarcations(data, preTestIdleStartIdx);
    preTestIdleDuration = (time(preTestIdleEndIdx) - time(preTestIdleStartIdx)) / 1000;
    preTestIdleInPower  = accumulate(inPower, preTestIdleStartIdx, preTestIdleEndIdx);
    preTestIdleOutPower = accumulate(outPower, preTestIdleStartIdx, preTestIdleEndIdx);
    preTestIdleInEnergy  = preTestIdleInPower  .* preTestIdleDuration;
    preTestIdleOutEnergy = preTestIdleOutPower .* preTestIdleDuration;
        %store the results in appropriate structures
        inAvePower(1,:)  = preTestIdleInPower;
        outAvePower(1,:) = preTestIdleOutPower;
        inEnergy(1,:)    = preTestIdleInEnergy;
        outEnergy(1,:)   = preTestIdleOutEnergy;
    
    % calculate energy and power consumption for the writing phase
    writeStartIdx  = preTestIdleEndIdx;
    writeEndIdx    = findStateDemarcations(data, writeStartIdx);
    writeDuration  = (time(writeEndIdx) - time(writeStartIdx)) / 1000;
    writeInPower   = accumulate(inPower, writeStartIdx, writeEndIdx);
    writeOutPower  = accumulate(outPower, writeStartIdx, writeEndIdx);
    writeInEnergy  = writeInPower  .* writeDuration;
    writeOutEnergy = writeOutPower .* writeDuration;
        %store the results in appropriate structures
        inAvePower(2,:)  = writeInPower;
        outAvePower(2,:) = writeOutPower;
        inEnergy(2,:)    = writeInEnergy;
        outEnergy(2,:)   = writeOutEnergy;
    
    % calculate energy and power consumption for the delay phase
    delayStartIdx  = writeEndIdx;
    delayEndIdx    = findStateDemarcations(data, delayStartIdx);
    delayDuration  = (time(delayEndIdx) - time(delayStartIdx)) / 1000;
    delayInPower   = accumulate(inPower, delayStartIdx, delayEndIdx);
    delayOutPower  = accumulate(outPower, delayStartIdx, delayEndIdx);
    delayInEnergy  = delayInPower  .* delayDuration;
    delayOutEnergy = delayOutPower .* delayDuration;
        %store the results in appropriate structures
        inAvePower(3,:)  = delayInPower;
        outAvePower(3,:) = delayOutPower;
        inEnergy(3,:)    = delayInEnergy;
        outEnergy(3,:)   = delayOutEnergy;
    
    % calculate energy and power consumption for the readback phase
    readBackStartIdx  = delayEndIdx;
    readBackEndIdx    = findStateDemarcations(data, readBackStartIdx);
    readBackDuration  = (time(readBackEndIdx) - time(readBackStartIdx)) / 1000;
    readBackInPower   = accumulate(inPower, readBackStartIdx, readBackEndIdx);
    readBackOutPower  = accumulate(outPower, readBackStartIdx, readBackEndIdx);
    readBackInEnergy  = readBackInPower  .* readBackDuration;
    readBackOutEnergy = readBackOutPower .* readBackDuration;
        %store the results in appropriate structures
        inAvePower(4,:)  = readBackInPower;
        outAvePower(4,:) = readBackOutPower;
        inEnergy(4,:)    = readBackInEnergy;
        outEnergy(4,:)   = readBackOutEnergy;
    
    postTestDelayStartIdx  = readBackEndIdx;
    postTestDelayEndIdx    = findStateDemarcations(data, postTestDelayStartIdx);
    postTestDelayDuration  = (time(postTestDelayEndIdx) - time(postTestDelayStartIdx)) / 1000;
    postTestDelayInPower   = accumulate(inPower, postTestDelayStartIdx, postTestDelayEndIdx);
    postTestDelayOutPower  = accumulate(outPower, postTestDelayStartIdx, postTestDelayEndIdx);
    postTestDelayInEnergy  = postTestDelayInPower  .* postTestDelayDuration;
    postTestDelayOutEnergy = postTestDelayOutPower .* postTestDelayDuration;
        %store the results in appropriate structures
        inAvePower(5,:)  = postTestDelayInPower;
        outAvePower(5,:) = postTestDelayOutPower;
        inEnergy(5,:)    = postTestDelayInEnergy;
        outEnergy(5,:)   = postTestDelayOutEnergy;
    
    totalStartIdx  = preTestIdleStartIdx;
    totalEndIdx    = postTestDelayEndIdx
    totalDuration  = (time(totalStartIdx) - time(totalEndIdx)) / 1000;
    totalInPower   = accumulate(inPower, postTestDelayStartIdx, postTestDelayEndIdx);
    totalOutPower  = accumulate(outPower, postTestDelayStartIdx, postTestDelayEndIdx);
    totalInEnergy  = totalInPower  .* totalDuration;
    totalOutEnergy = totalOutPower .* totalDuration;
        %store the results in appropriate structures
        inAvePower(6,:)  = totalInPower;
        outAvePower(6,:) = totalOutPower;
        inEnergy(6,:)    = totalInEnergy;
        outEnergy(6,:)   = totalOutEnergy;
    
    %inEnergy  = sum(data(:,:,1)) .* sum(data(:,:,2));
    %outEnergy = sum(data(:,:,1)) .* sum(data(:,:,2));
    
end