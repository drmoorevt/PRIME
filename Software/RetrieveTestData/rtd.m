%RetrieveTestData
s = serial('COM13');
set(s,'BaudRate',115200);
fopen(s);
fprintf(s,'Hi')
s.
fprintf(s.
%out = fscanf(s, '%c', 10);
fclose(s)
delete(s)
clear s