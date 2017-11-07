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
Input_1 = Current_TV/10;
Input_2 = Current_HP/10;
Input_3 = Current_STB/10;
Input_4 = Current_FAN/10;
Input_5 = rand(1000,1);

plot(Current_TV)

Starting = 321;

dlmwrite('Input0.txt', Input_0(Starting:Starting+511), '');
dlmwrite('Input1.txt', Input_1(Starting:Starting+511), '');
dlmwrite('Input2.txt', Input_2(Starting:Starting+511), '');
dlmwrite('Input3.txt', Input_3(Starting:Starting+511), '');
dlmwrite('Input4.txt', Input_4(Starting:Starting+511), '');
dlmwrite('Input5.txt', Input_5(Starting:Starting+511), '');