function dataOut = argGenTest11(preTestDelay,  ...
                                sampleRate,    ...
                                postTestDelay, ...
                                profile,       ...
                                pDest,         ...
                                buffer,        ...
                                length)
    dataOut = typecast(uint32(preTestDelay), 'uint8');
    dataOut = [dataOut typecast(uint32(sampleRate), 'uint8')];
    dataOut = [dataOut typecast(uint32(postTestDelay), 'uint8')];
    dataOut = [dataOut typecast(uint32(profile), 'uint8')];
    dataOut = [dataOut typecast(uint32(pDest), 'uint8')];
    dataOut = [dataOut typecast(uint8(buffer), 'uint8')];
    dataOut = [dataOut typecast(uint32(length), 'uint8')];
end
