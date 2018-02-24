%% Generate single mesh of points

clc
clear
close all

addpath sphereDivision/
TR0 = IcosahedronMesh;
TR1 = SubdivideSphericalMesh(TR0,1); 
TR2 = SubdivideSphericalMesh(TR0,2);
TR5 = SubdivideSphericalMesh(TR0,5);
classPoints = TR2.X;
camPoints = TR5.X;

%% Forming class quaternions
% idx = 1;
% ang = linspace(0,2*pi,19);
% classQuat = zeros(length(classPoints)*length(ang),4);
% for i = 1:length(classPoints)
%     ax = classPoints(i,:);
%     for j = 1:length(ang)
%         classQuat(idx,:) = axang2quat([ax,ang(j)]);
%         idx = idx + 1;
%     end
% end

%% Forming cam quaternions
% idx = 1;
% ang = linspace(0,2*pi,10);
% camQuat = zeros(length(camPoints)*length(ang),4);
% for i = 1:length(camPoints)
%     ax = camPoints(i,:);
%     for j = 1:length(ang)
%         camQuat(idx,:) = axang2quat([ax,ang(j)]);
%         idx = idx + 1;        
%     end
% end

%% Assigning camQuats to classQuats
% minTheta = zeros(length(camQuat),1);
% camBin = minTheta;
% for i = 1:length(camQuat)
%     q1 = camQuat(i,:);
%     theta = zeros(length(classQuat),1);
%     for j = 1:length(classQuat)
%         q2 = classQuat(j,:);        
%         qDiff = quatmultiply(q1,quatconj(q2));
%         theta(j) = 2 * acos(qDiff(1));
%     end
%     [minTheta(i),camBin(i)] = min(real(theta));
% end
% 
% close all

%% Checking min separation between view classes
% ang = zeros(size(classPoints,1),size(classPoints,1));
% for i = 1:size(classPoints,1)
%     v = classPoints(i,:);
%     v = v / norm(v); % normalize
%     for j = 1:size(classPoints,1)
%         w = classPoints(j,:);
%         w = w / norm(w); % normalize
%         [~,ang(i,j)] = vec2axang(v,w);        
%     end
% end

%% Assigning camPoints to classPoints based on proximity
ang = zeros(size(camPoints,1),size(classPoints,1));
camBin = zeros(size(camPoints,1),1);
viewCamDiff = camBin; % difference from the class the view was binned into
viewQuat = zeros(size(camPoints,1),4);
for i = 1:size(camPoints,1)
    v = camPoints(i,:);
    v = v / norm(v); % normalize
    for j = 1:size(classPoints,1)
        w = classPoints(j,:);
        w = w / norm(w); % normalize
        [~,ang(i,j)] = vec2axang(v,w);
    end
    [sortAng,idx] = min(ang(i,:));
    camBin(i) = idx;
    viewCamDiff(i) = sortAng;
    
    [ax_,ang_] = vec2axang(v,[0,0,1]);
    viewQuat(i,:) = axang2quat([ax_,deg2rad(ang_)]);
end


%% Writing camPoints pose data to file
r = 8:1:11; % specifying sphere radii
counter = 0;
idx = 1;
viewQuatMat = zeros(length(r)*length(camPoints),4);
viewPosMat = zeros(length(r)*length(camPoints),3);
camBinMat = zeros(length(r)*length(camPoints),1);
for i = 1:length(r)
    viewQuatMat(idx:idx+length(viewQuat)-1,:) = viewQuat;
    viewPosMat(idx:idx+length(camPoints)-1,:) = repmat([0 0 r(i)],length(camPoints),1);
    camBinMat(idx:idx+length(camBin)-1) = camBin + counter;
    counter = counter + length(classPoints);
    idx = idx + length(viewQuat);
end

% csvwrite('imagePoseData.csv',[viewQuatMat,viewPosMat,camBinMat])

%% Writing classPoints pose data to file
% Generating class quats
classQuats = zeros(length(classPoints),4);
for i = 1:length(classPoints)
    v = classPoints(i,:);
    v = v / norm(v);    
    [ax_,ang_] = vec2axang(v,[0,0,1]);
    classQuats(i,:) = axang2quat([ax_,deg2rad(ang_)]);
end

r = 8:1:11; % specifying sphere radii
counter = 0;
idx = 1;
classQuatMat = zeros(length(r)*length(classQuats),4);
classPosMat = zeros(length(r)*length(classQuats),3);
classBinMat = zeros(length(r)*length(classQuats),1);
for i = 1:length(r)
    classQuatMat(idx:idx+length(classQuats)-1,:) = classQuats;
    classPosMat(idx:idx+length(classQuats)-1,:) = repmat([0 0 r(i)],length(classQuats),1);
    classBinMat(idx:idx+length(classQuats)-1) = (1:length(classQuats)) + counter;
    counter = counter + length(classQuats);
    idx = idx + length(classQuats);
end

% csvwrite('classPoseData.csv',[classQuatMat,classPosMat,classBinMat])

%% Visualize class and camera view points
close all
figure
hold on
cols = linspecer(size(classPoints,1));
for i = 1:length(classPoints)
    plot3(classPoints(i,1),classPoints(i,2),classPoints(i,3),'.','Color','k','MarkerSize',40)
end
for i = 1:1:length(camPoints)
    plot3(camPoints(i,1),camPoints(i,2),camPoints(i,3),'.','Color',cols(camBin(i),:),'MarkerSize',10)
end

axis equal
view(-34.4,46)
            
            
                

    
