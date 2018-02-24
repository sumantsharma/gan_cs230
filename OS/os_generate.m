clear;clc;close all

%% Configurables
experiment = 'exp01';
inputData = [experiment '_os_data.mat'];
screenshotDirectory = fullfile('D:','GAN','OS',experiment);

%% Set-Up
load(inputData);
os = OpticalStimulator.Small;

%% Run
os.GAN(r_Vo2To_vbs, q_vbs2tango, screenshotDirectory);