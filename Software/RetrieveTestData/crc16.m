function crc = crc16(inBytes, initial, poly)
    poly = uint16(hex2dec(poly));
    crc  = uint16(hex2dec(initial));
    for n = 1:length(inBytes)
        inByte = uint16(inBytes(n));
        for i = 1:8
            xorVal = bitxor(crc,inByte);
            flag   = bitand(xorVal, 1);
            crc    = bitshift(crc, -1);
            inByte = bitshift(inByte, -1);
            if (flag)
                crc = bitxor(crc, poly);
            end
        end
    end
    crc = uint16(crc);
end
