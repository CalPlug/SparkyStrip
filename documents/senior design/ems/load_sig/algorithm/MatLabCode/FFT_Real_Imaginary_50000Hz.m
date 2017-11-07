clc
clear

% Read Files on Excel

Time = xlsread('C:\Users\owner\Dropbox\CalPlug-EMS Team\WaveForms\Different Computers\Lenovo.xlsx','A19:A10018');
Load_Signature = xlsread('C:\Users\owner\Dropbox\CalPlug-EMS Team\WaveForms\Different Computers\Lenovo.xlsx','B19:B10018');

% Make 2 sinusoid waves, one with double frequency

F_s = 50000; % Sampling Frequency
F_1 = 60; % Frequency of Device #1
F_2 = 120; % Frequency of Device #2
F_3 = 180; % Frequency of Device #3


wave_1 = sin(2*pi*Time*F_1);
wave_2 = sin(2*pi*Time*F_1);
%wave_2 = .5*sin(2*pi*Time*F_2);
%wave_3 = .5*sin(2*pi*Time*F_3);

%Load_Signature = wave_2 + wave_1;
% 512 samples of loadsig

start_sample = 1;
for i=1:5120;
    Load_Sig(i) = Load_Signature(i+start_sample);
    Time_512(i) = Time(i);
end


% Debugging phase

% N = 1:1000;
% Load_Sig = cos(2*pi*.05*N);



% Setting the time to start at time = 0

Time = Time - Time(1);
Time_512 = Time_512 - Time_512(1);

% Computes the dtft
% Note: requires the dtft.m file in MatLab Directory!

L = length(Time);
[H,W] = dtft( Load_Sig, L*2 );

% Normalizes the Frequency response.  Un-normalized Values, set max = 1;
Max = max(abs(H));

H = H/Max;
Real_H = real(H);
Imaginary_H = imag(H);
H = abs(H);

% Our sampling frequency
Sampling_Frequency = 50000;

PI = 3.14159265359;

% the Interval that we are sampling from
W = W*Sampling_Frequency/(2*PI);

% Plotting the graph

figure(1);
plot(W, H);
title('Normalized Magnitude/Frequency');
xlabel('Frequency');
ylabel('Normalized Magnitude');
grid on;

figure(2);
plot(W, Real_H);
title('Real Frequency');
xlabel('Frequency');
ylabel('Normalized Magnitude');
grid on;

figure(3)
plot(W, Imaginary_H);
title('Imaginary Frequency');
xlabel('Frequency');
ylabel('Normalized Magnitude');
grid on;

figure(4)
plot(Time_512, Load_Sig);
title('HP-Laptop ProBook 4530s');
xlabel('Time');
ylabel('Amplitude');
grid on;

Phase = atan( Imaginary_H./Real_H);

% debug. Normalized Phase values
for i=1:100;
   Total = 0;
   for j=1:20;
       Total = Total + Phase((i-1)*20+j);
   end
   for k=1:20;
       Phase_Average((i-1)*20+k) = Total/10;
   end
end


figure(5)
plot(W, Phase_Average);
title('Phase vs Frequency');
xlabel('Frequency');
ylabel('Phase');
grid on;

[Phase(1025) Phase(1073) Phase(1121)]
[H(1025) H(1073) H(1121)]
[Real_H(1025) Real_H(1073) Real_H(1121)]
[Imaginary_H(1025) Imaginary_H(1073) Imaginary_H(1121)]