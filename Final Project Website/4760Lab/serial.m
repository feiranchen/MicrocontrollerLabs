%=====clean up any leftover serial connections==============
clear all
try
    fclose(instrfind) %close any bogus serial connections
end

%=====open a serial connection===============================
%set its rate to 9600 baud
%SR830 terminator character is  (ACSII 13)
%use fprintf(s,['m' mode]) to write
%and fscanf(s) to read the SR830
s = serial('COM1',...
    'baudrate',9600,...
    'terminator',13); 
fopen(s)

%=====open a window and put a quit button in it============
figure(1);clf;
set(gcf,'position',[100,100,300,100])
set(gcf, 'closerequestfcn','quitit=1;fclose(s);delete(gcf)');
axis([0 1 0 1])
axis off
%define the erase button
quitbutton=uicontrol('style','pushbutton',...
   'string','Quit', ...
   'fontsize',12, ...
   'position',[10,10,50,20], ...
   'callback','quitit=1;fclose(s);delete(gcf);');

%=====load two bird songs=============================
% pathname = 'C:\Documents and Settings\bruce land\My Documents\DamianData\Bird Song\';
% filename1 = 'field2_93764_small.wav';
% [song1,Fs1] = wavread([pathname,filename1]);
% filename2 = 'chipping5_15438_small.wav';
% [song2,Fs2] = wavread([pathname,filename2]);

%get a song
[filename1,pathname] = ...
            uigetfile( 'C:\Documents and Settings\*.wav','Song 1');
if isnumeric(filename1) %check to see if Cancel buttion was hit
    quitit=1;fclose(s);delete(gcf)
    error('You MUST choose a song')   %if so, stop program
end
[song1,Fs1] = wavread([pathname,filename1]);
 text(.2,.5,filename1,'Interpreter','none');
 
%get another song
[filename2,pathname] = ...
            uigetfile( 'C:\Documents and Settings\*.wav','Song 2');
if isnumeric(filename2) %check to see if Cancel buttion was hit
    quitit=1;fclose(s);delete(gcf)
    error('You MUST choose a song')   %if so, stop program
end
[song2,Fs2] = wavread([pathname,filename2]);
text(.2,.01,filename2,'Interpreter','none');

%=====Look for sw closing and gen songs======================
quitit=0;
drawnow

%read microcontroller once to throw away setup closeures
fprintf(s,'s')
Switch = fscanf(s,'%d%d')
Switch = [0 0];

while (quitit==0)  
    
    fprintf(s,'s');
    Switch = fscanf(s,'%d%d');
    
    if Switch(1)
        sound(song1,Fs1);
        pause(length(song1)/Fs1 + 0.1);
    end
    
    if Switch(2)
        sound(song2,Fs2);
        pause(length(song2)/Fs2 + 0.1);
    end
    
    drawnow
end

%=====write an excel file===================================
%Note that a Lotus file is actully written because
%Matlab cannot currently write an Excel file.
%Excel will read this format
%This version appends a date/time string to each file name
% wk1write([pathname,filename,datestr(datevec(now),30)], [PlayFreq',PlayAmp',OutAmp',OutPhase'])

%=====close the serial port=================================
fclose(s)
