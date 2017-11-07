clc
clear

%% Electronic Devices %%

FAN = xlsread('C:\Users\Redwan\Downloads\Dropbox\CalPlug-EMS Team\WaveForms\Appliances with Reference Voltage\FAN.xlsx','C19:C1018');
TV = xlsread('C:\Users\Redwan\Downloads\Dropbox\CalPlug-EMS Team\WaveForms\Appliances with Reference Voltage\TV.xlsx','C19:C1018');
HP = xlsread('C:\Users\Redwan\Downloads\Dropbox\CalPlug-EMS Team\WaveForms\Appliances with Reference Voltage\HP.xlsx','C19:C1018');
STB = xlsread('C:\Users\Redwan\Downloads\Dropbox\CalPlug-EMS Team\WaveForms\Appliances with Reference Voltage\STB.xlsx','C19:C1018');
UNKNOWN_SIGNAL = rand(1000, 1);

ONE_FAN = xlsread('C:\Users\Redwan\Downloads\Dropbox\CalPlug-EMS Team\WaveForms\Multiple Fans\one_Fan1000Samples.xlsx','B19:B1018');
TWO_FANS = xlsread('C:\Users\Redwan\Downloads\Dropbox\CalPlug-EMS Team\WaveForms\Multiple Fans\two_Fans1000Samples.xlsx','B19:B1018');
THREE_FANS = xlsread('C:\Users\Redwan\Downloads\Dropbox\CalPlug-EMS Team\WaveForms\Multiple Fans\three_Fans1000Samples.xlsx','B19:B1018');

%ONE_FAN = xlsread('C:\Users\owner\Dropbox\CalPlug-EMS Team\WaveForms\Multiple Fans\one_Fan1000Samples.xlsx','B19:B1018');
%TWO_FANS = xlsread('C:\Users\owner\Dropbox\CalPlug-EMS Team\WaveForms\Multiple Fans\two_Fans1000Samples.xlsx','B19:B1018');
%THREE_FANS = xlsread('C:\Users\owner\Dropbox\CalPlug-EMS Team\WaveForms\Multiple Fans\three_Fans1000Samples.xlsx','B19:B1018');

HP_FAN = xlsread('C:\Users\Redwan\Downloads\Dropbox\CalPlug-EMS Team\WaveForms\Combined Appliances\HP+FAN.xlsx','B19:B1018');
HP_TV = xlsread('C:\Users\Redwan\Downloads\Dropbox\CalPlug-EMS Team\WaveForms\Combined Appliances\HP+TV.xlsx','B19:B1018');
STB_FAN = xlsread('C:\Users\Redwan\Downloads\Dropbox\CalPlug-EMS Team\WaveForms\Combined Appliances\STB+FAN.xlsx','B19:B1018');
STB_HP = xlsread('C:\Users\Redwan\Downloads\Dropbox\CalPlug-EMS Team\WaveForms\Combined Appliances\STB+HP.xlsx','B19:B1018');
STB_TV = xlsread('C:\Users\Redwan\Downloads\Dropbox\CalPlug-EMS Team\WaveForms\Combined Appliances\STB+TV.xlsx','B19:B1018');
TV_FAN = xlsread('C:\Users\Redwan\Downloads\Dropbox\CalPlug-EMS Team\WaveForms\Combined Appliances\TV+FAN.xlsx','B19:B1018');

% Reference voltage set to Input 0

 Input_1 = TV_FAN/10;
% Input_1 = ONE_FAN;

% Plots the Output Graph. 
plot(Input_1);
grid 'on'
title('Input Waveform')
xlabel('Samples')
ylabel('Current')

Starting = 76;

dlmwrite('Input1.txt', Input_1(Starting:Starting+511), '');