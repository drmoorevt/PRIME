function args = test11Args(commonArgs, profile, pDest, buffer, len)
    args.commonArgs = commonArgs;
    args.profile    = uint32(profile);
    args.pDest      = uint32(pDest);
    args.buffer     = uint32(buffer);
    args.len        = uint32(len);
    
    %args = struct2cell(commonArgs)';
    %args = [args, uint32(profile), uint32(pDest)]
    %args = [args, [1:128]]
    %args = [args, uint16(len)]
end