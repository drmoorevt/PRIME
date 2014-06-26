% CCITT_CRC_POLYNOMIAL (0x1021) // Standard polynomial
% CCITT_REV_POLYNOMIAL (0x8408) // Reverse polynomial
% ANSI_CRC_POLYNOMIAL  (0x8005)
% ANSI_REV_POLYNOMIAL  (0xA001) // Bitwise reverse of standard
function crc = crc16(inBytes, initial, poly)
    poly = uint16(hex2dec(poly));
    crc  = uint16(hex2dec(initial));
    for n = 1:length(inBytes)
        crc = bitxor(crc, bitshift(uint16(inBytes(n)), 8));
        for i = 1:8
            flag = bitand(crc, hex2dec('8000'));
            if (flag > 0)
                crc = bitxor(bitshift(crc, 1), poly);
            else
                crc = bitshift(crc, 1);
            end
        end
    end
end
