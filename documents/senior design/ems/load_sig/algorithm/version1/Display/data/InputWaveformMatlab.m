clc
clear

%% Noise Removal %%

Current_FAN = xlsread('C:\Users\owner\Desktop\CalPlug\Current vs Time poster\FAN.xlsx','C19:C1018');
Current_TV = xlsread('C:\Users\owner\Desktop\CalPlug\Current vs Time poster\TV.xlsx','C19:C1018');
Current_HP = xlsread('C:\Users\owner\Desktop\CalPlug\Current vs Time poster\HP.xlsx','C19:C1018');
Current_STB = xlsread('C:\Users\owner\Desktop\CalPlug\Current vs Time poster\STB.xlsx','C19:C1018');

Reference_Voltage = xlsread('C:\Users\owner\Desktop\CalPlug\Current vs Time poster\STB.xlsx','B19:B1018');

% Reference voltage set to Input 0

Input_0 = Reference_Voltage;
Input_1 = Current_STB/10;

% Find the ranges of the FFTs

counter = 1;
array_counter = 1;

%for i=1:489
%    fan_reference = fft(Current_FAN(i:511+i));
%    tv_reference = fft(Current_TV(i:511+i));
%    stb_reference = fft(Current_STB(i:511+i));
%    hp_reference = fft(Current_HP(i:511+i));
    
%    FAN_180(i) = abs(fan_reference(19))/abs(fan_reference(7));
%    FAN_300(i) = abs(fan_reference(32))/abs(fan_reference(7));
%    TV_180(i) = abs(tv_reference(19))/abs(tv_reference(7));
%    TV_300(i) = abs(tv_reference(32))/abs(tv_reference(7));
%    STB_180(i) = abs(stb_reference(19))/abs(stb_reference(7));
%    STB_300(i) = abs(stb_reference(32)/abs(stb_reference(7)));
%    HP_180(i) = abs(hp_reference(19))/abs(hp_reference(7));
%    HP_300(i) = abs(hp_reference(32))/abs(hp_reference(7));
%        Output(array_counter) = Input_1(i);
%        array_counter = array_counter + 1;
%        counter = 0;
%    end
%    counter = counter + 1;
%end

%counter = 1;
%array_counter = 1;

%for i=1:1000
%    if counter == 5;
%        Output(array_counter) = Input_1(i);
%        array_counter = array_counter + 1;
%        counter = 0;
%    end
%    counter = counter + 1;
%end

%% plot(Output)

Starting = 112;

dlmwrite('Input0.txt', Input_0(Starting:Starting+511), '');
dlmwrite('Input1.txt', Input_1(Starting:Starting+511), '');