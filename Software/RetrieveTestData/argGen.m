function dataOut = argGen(sampRate,      ...  // Sample rate of the ADCs
                          testLen,    ...  // Operation duration in us
                          opDelay,       ...  // Delay portion of operation
                          profile,       ...  // Profile to specify for op
                          preTestDelay,  ...  // Domain settling time
                          postTestDelay, ...  // Domain hold time
                          pDest,         ...  // Address to write or override
                          buffer         ...  // Data to write
                          )
    dataOut = typecast(uint32(sampRate),                 'uint8');
    dataOut = [dataOut typecast(uint32(testLen),         'uint8')];
    
    %for i = 1 : size(opDelay,2)
    %end
    
    dataOut = [dataOut typecast(uint32(opDelay(1:size(opDelay,2))), 'uint8')];
    dataOut = [dataOut typecast(uint32(profile),                    'uint8')];
    dataOut = [dataOut typecast(uint32(preTestDelay),               'uint8')];
    dataOut = [dataOut typecast(uint32(postTestDelay),              'uint8')];
    dataOut = [dataOut typecast(uint32(pDest),                      'uint8')];
    dataOut = [dataOut typecast(uint32(size(buffer, 2)),            'uint8')];
    dataOut = [dataOut typecast(uint8(buffer),                      'uint8')];
end
