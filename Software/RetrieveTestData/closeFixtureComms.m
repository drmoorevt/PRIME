function [success] = closeFixtureComms(s)    
    fclose(s);
    delete(s);
    clear s;
    success = 'TRUE';
end
