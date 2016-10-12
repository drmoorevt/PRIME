function [s] = resetFixtureComms(sOrg, CommPort)
    if (sOrg.BytesAvailable > 0)
        fread(sOrg, sOrg.BytesAvailable); % Clear out the serial port!
    end
    closeFixtureComms(sOrg);
    delete(instrfindall);
    s = openFixtureComms(CommPort);
    fwrite(s, uint8(hex2dec('54'))); % Attempt DUT reset
    pause(0.5); % Give the DUT 500ms to process the reset
    closeFixtureComms(s);
    s = openFixtureComms(CommPort);
end