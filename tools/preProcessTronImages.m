%% 
clc
clear
close all

%% Change naming convention 
% files = dir('*.png');
% for i = 1:length(files)
%     name = files(i).name
%     imNum = str2num(files(i).name(strfind(files(i).name,'_')+1 : strfind(files(i).name,'.')-1));
%     
%     im = im2double(imread(name));
%     imwrite(im, fullfile('new',sprintf('img%06d.jpg',imNum)))
% end
%     
    
%% Detect TANGO ROI to create a mask

tronImageDir = fullfile('D:','GAN','pix2pix','images','TangoGanExp04','rawReal50');
files = dir(fullfile(tronImageDir,'*.png'));

widthOffsetRight = 170;
heightOffsetRight = 200;
yOffset = -50;
xOffset = -10;

for i = 1:length(files)
    rgb = im2double(imread(fullfile(files(i).folder,files(i).name)));
    I = adapthisteq(rgb2gray(rgb));
%     imshow(I)

    background = imopen(I,strel('disk',15));
    I2 = I - background;
    I3 = imadjust(I2);
    bw = imbinarize(I3);
    bw = bwareaopen(bw, 50);
    cc = bwconncomp(bw, 4);
    graindata = regionprops(cc,'basic');
    grain_areas = [graindata.Area];
    [max_area, idx] = max(grain_areas);

    grain = false(size(bw));
    grain(cc.PixelIdxList{idx}) = true;
%     imshow(grain);

    [ii,jj] = find(grain);

    xmin = min(jj) + xOffset;
    ymin = min(ii) + yOffset;
    xmax = max(jj);
    ymax = max(ii);
    width = xmax - xmin + widthOffsetRight;
    height = ymax - ymin + heightOffsetRight;
    xmax = min(xmin + width, 1920);
    ymax = min(ymin + height, 1200);
   
    figure(1)
%     imshow(I); hold on;
%     rectangle('Position',[xmin,ymin,width,height],'EdgeColor','w');
    
    % Determine background color
    bgColor = 33/255;
    
    % Create new image
    newIm = ones(size(I)) * bgColor;
    newIm(ymin:ymax, xmin:xmax) = I(ymin:ymax, xmin:xmax);
    imshowpair(I,newIm,'montage')
    
    % Save new image
    imNum = str2num(files(i).name(strfind(files(i).name,'_')+1 : strfind(files(i).name,'.')-1));
    imwrite(newIm, fullfile('D:','GAN','pix2pix','images','TangoGanExp04','real50',sprintf('img%06d.jpg',imNum)))
    
    

end


