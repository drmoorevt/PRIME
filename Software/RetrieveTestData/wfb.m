function gotBytes = wfb(s, numBytes, timeoutMS)
    i = 0;
    while((s.BytesAvailable < numBytes) && (i < timeoutMS / 10))
        i = i + 1;
        pause(0.01);
    end;
    gotBytes = (s.BytesAvailable >= numBytes);
end