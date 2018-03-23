clc
clearvars -EXCEPT files
close all

if ~exist('files','var')
    files = dir(fullfile('D:','GAN','pix2pix','experiments','TangoGanExp05_train','images','*.png'));
end

vidFile = VideoWriter(fullfile('D:','GAN','pix2pix','experiments','TangoGanExp05_train','progress.mp4'),'MPEG-4');
vidFile.FrameRate = 3;
open(vidFile);

i = 1;
while i < length(files)
    imInput = imread(fullfile(files(i).folder, files(i).name));
    imOutput = imread(fullfile(files(i+1).folder, files(i+1).name));
    imTrue = imread(fullfile(files(i+2).folder, files(i+2).name));
    
    figure(1)
    montage([imInput, imOutput, imTrue]);
    
    writeVideo(vidFile, getframe(figure(1)));
    
    if i < 30000
        i = i + 300;
    else
        i = i + 150;
    end
    
    fprintf('progress = %1.4f percent \n', i/length(files) * 100)
    
end

close(vidFile);
