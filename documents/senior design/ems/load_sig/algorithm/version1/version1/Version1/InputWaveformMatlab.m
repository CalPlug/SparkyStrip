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
Input_1 = Current_HP/10;
Input_2 = zeros(1000,1)/10;

counter = 1;
array_counter = 1;

for i=1:1000
    if counter == 5;
        New_Sampled_Current_Fan(array_counter) = Current_FAN(i);
        array_counter = array_counter + 1;
        counter = 0;
    end
    counter = counter + 1;
end

plot(New_Sampled_Current_Fan)

Starting = 321;

dlmwrite('Input0.txt', Input_0(Starting:Starting+511), '');
dlmwrite('Input1.txt', Input_1(Starting:Starting+511), '');
dlmwrite('Input2.txt', Input_2(Starting:Starting+511), '');