clc
clear

%% Electronic Devices %%

FAN = xlsread('C:\Users\owner\Desktop\CalPlug\Current vs Time poster\FAN.xlsx','C19:C1018');
TV = xlsread('C:\Users\owner\Desktop\CalPlug\Current vs Time poster\TV.xlsx','C19:C1018');
HP = xlsread('C:\Users\owner\Desktop\CalPlug\Current vs Time poster\HP.xlsx','C19:C1018');
STB = xlsread('C:\Users\owner\Desktop\CalPlug\Current vs Time poster\STB.xlsx','C19:C1018');
UNKNOWN_SIGNAL = rand(1000, 1);

Reference_Voltage = xlsread('C:\Users\owner\Desktop\CalPlug\Current vs Time poster\STB.xlsx','B19:B1018');

% Reference voltage set to Input 0

Input_0 = Reference_Voltage;
Input_1 = STB/10;

% Plots the Output Graph. 
plot(Input_1);
grid 'on'
title('Input Waveform')
xlabel('Samples')
ylabel('Current')

Starting = 1;

dlmwrite('Input0.txt', Input_0(Starting:Starting+511), '');
dlmwrite('Input1.txt', Input_1(Starting:Starting+511), '');