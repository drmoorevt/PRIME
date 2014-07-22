% power data is (value:state:profile) over the length of time
% energy data (value:state:profile) but scalar
function [inAvePower, outAvePower, inEnergy, outEnergy] = analyzeTest(name, chans, data, time)
    % determine the input and output power for the test
    % data is (values, (v/i/state), [nothing] -- 1, profile,
    data = data(:,:,1,:);
    inPower(:,:)  = data(:,1,:) .* data(:,2,:);
    outPower(:,:) = data(:,1,:) .* data(:,3,:);
    
    STATE_MAX = 6;
    stateRange = (3.3 * (4096.0 / STATE_MAX) / 4096.0) *.9;
    
    % calculate energy and power consumption for the preTestIdle phase
    i = 1;
    stateStartIdx = ones(1, length(data(1,1,1,:)));
    while (stateStartIdx < length(data(:,1,1,1)))
        stateEndIdx = findStateDemarcations(data(:,4,1,:), stateStartIdx, stateRange);
        stateDuration = (time(stateEndIdx) - time(stateStartIdx)) / 1000; %ms -> s
        inAvePower(i,:)  = accumulate(inPower, stateStartIdx, stateEndIdx);
        outAvePower(i,:) = accumulate(outPower, stateStartIdx, stateEndIdx);
        inEnergy(i,:)    = inAvePower(i,:)  .* stateDuration;
        outEnergy(i,:)   = outAvePower(i,:) .* stateDuration;
        stateStartIdx = stateEndIdx;
        i = i + 1;
    end    
    totalInEnergy  = sum(inEnergy);
    totalOutEnergy = sum(outEnergy);

    % Calculate normalized energy savings per profile
    deltaInEnergy  = 100 * (1 - (totalInEnergy  ./ totalInEnergy(1)))
    deltaOutEnergy = 100 * (1 - (totalOutEnergy ./ totalOutEnergy(1)))
    
end