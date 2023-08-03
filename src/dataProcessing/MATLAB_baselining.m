%% Set parameters %%
nProbes = 10; % Only use first 10 probes for testing

nbins = 200; % number of bins to calc histogram
n = 5; % power excess >4.7*std determined as bad bins

SG_order = 4; % d of SG filter
SG_framelen = 4001; % 2w + 1 of SG filter

% Alazar card
Fs = 30e6; % sampling rate
L = 15e4; % number of samples
RBW = Fs/L;
n_avg = 32;
t_aq = L/Fs;
digitsPrec = 7;
f2Side = (-L/2:L/2-1)*(Fs/L)/1e6; % zero-centered frequency range


%% Retrieve some practice CC data from the bench test run %%
date = '17-May-2022';

folder = ['S:\lehnert\common\Quantum Metrology for Dark Matter\Joyce Jiang\measurement\',date,'\Alazartimetrace'];
FileNames = dir([folder,'\CC_initProbe_*']);

FileList = cellfun(@(x) fullfile(folder,x),{FileNames.name},'UniformOutput',false);

FileList = FileList(1:nProbes);


%% Baselining process %%
nSpectraBaseline = 10; % Number of spectra within each probe to use for baseline

Noise_mat = zeros(nProbes, nSpectraBaseline,L);

for i = 1:nProbes
    filePattern = fullfile(FileList{i}, '*.mat');
    matFiles = dir(filePattern);
    
    for j = 1:nSpectraBaseline
        NoiseData = load([FileList{i},'\',matFiles(j).name],'PS2SideCC');
        Noise_mat(i,j,:) = cell2mat(struct2cell(NoiseData(1)));
    end
end

Noise = reshape(Noise_mat, [], size(Noise_mat, 3));

%%
BaseLine_AllBins = mean(Noise,1);
Base_Excess_AllBins = BaseLine_AllBins./medfilt1(BaseLine_AllBins,100)-1;

% rough remove bad IF bins in baseline to get the histogram
[Base_Excess_BadRmvd, badIF_idx] = filloutliers(Base_Excess_AllBins, NaN, 'quartiles', ThresholdFactor=5);

BaseLine_BadRmvd = BaseLine_AllBins;
BaseLine_BadRmvd(badIF_idx)= NaN;

missing = isnan(BaseLine_BadRmvd);
fprintf('Missing %d samples of %d\n',sum(missing),L)

BaseLine = fillmissing(BaseLine_BadRmvd,'linear',2,'EndValues','nearest'); % fill NaN value
BaseLine_SG = sgolayfilt(BaseLine,SG_order,SG_framelen);

%%
figure();
plot(f2Side, Base_Excess_AllBins)
hold on
plot(f2Side, Base_Excess_BadRmvd)
hold off

%%
figure();
plot(f2Side, BaseLine_AllBins,'color',[0.25 0.25 0.25],'Linewidth',1.5)
hold on
plot(f2Side, BaseLine,'color',[0 0.4470 0.7410])
hold on
plot(f2Side, BaseLine_SG)
hold off

xlim([-15 15])
ylim([0 0.4e-10])

legend('All bins','mean','SG filtered')
xlabel('$f-f_{cav}$ [MHz]','Interpreter','Latex');
ylabel('power [a.u.]')
set(gca,'fontsize',18,'linewidth',1,'fontname','times')




%% save bad bin removed data for processing tests
nSpectra = 1000; % = numel(matFiles);

% Before the for loop, initialize a cell array to store the raw_data from each iteration
all_raw_data = cell(1, nProbes);

for i = 1:nProbes
    raw_data = zeros(nProbes,L);
    
    filePattern = fullfile(FileList{i}, '*.mat');
    matFiles = dir(filePattern);
    
    for j = 1:nSpectra
        baseFileName = matFiles(j).name;
        FAxionData = load([FileList{i},'\',baseFileName],'PS2SideCC');
        raw_data(j,:) = cell2mat(struct2cell(FAxionData(1)));
    end
    
    % ====================== remove the bad bins ======================
    % roughly remove the bad bins
    intermediate_spec_AllBins = raw_data./BaseLine_SG;
    
    [foo, badbin_idx] = filloutliers(mean(intermediate_spec_AllBins), NaN, 'quartiles', ThresholdFactor=5);
    
    raw_data(:,badbin_idx)= NaN;
    raw_data = fillmissing(raw_data,'linear',2,EndValues='nearest'); % fill NaN value
    
    intermediate_spec_AllBins(:,badbin_idx)= NaN;
    intermediate_spec_mat = fillmissing(intermediate_spec_AllBins,'linear',2,EndValues='nearest'); % fill NaN value
    
    % Store the raw_data for each probe in the cell array
    all_raw_data{i} = raw_data;
    
    csvFileName = sprintf('raw_data_probe_%d.csv', i);
    writematrix(raw_data, csvFileName);
end

% concatenated_raw_data = cat(1, all_raw_data{:});
% writematrix(concatenated_raw_data, 'all_raw_data.csv');

% 	% ================ get processed spectra ==========
%     Filter_SG = sgolayfilt(intermediate_spec_mat,SG_order,SG_framelen,[],2);
%     processed_spec_mat = intermediate_spec_mat./Filter_SG-1;
%     sigma_processed_expc= 1/sqrt(n_avg);
%     sigma_processed_mat = sigma_processed_expc.*ones(nProbes,L+ShiftPoints*(nProbes-1));
%
%     % ================ shift the spectrum ==========
%     rescaled_spec_mat = processed_spec_mat./vis;
%     shifted_spec_mat =  nan(nProbes,L+ShiftPoints*(nProbes-1));
%     shifted_vis_mat = nan(nProbes,L+ShiftPoints*(nProbes-1));
%     for i = 1:nProbes
%         shifted_spec_mat(i,(nProbes-i)*ShiftPoints+1:(nProbes-i)*ShiftPoints+L) = rescaled_spec_mat(i,:);
%         shifted_vis_mat(i,(nProbes-i)*ShiftPoints+1:(nProbes-i)*ShiftPoints+L) = vis;
%     end
%
%     % =========================== get combined spectrum =====================
%     % get weight matrix
%     sigma_rescaled_mat = sigma_processed_mat./shifted_vis_mat;
%     sum_rows = nansum((1./sigma_rescaled_mat).^2);
%     weight_mat = (1./sigma_rescaled_mat).^2./sum_rows; % Brubaker thesis eq 7.8 normalize the vis^2 mat in each column, has 0 in matrix element
%     combined_spec(j,:) = nansum(weight_mat.*shifted_spec_mat); %  Brubaker thesis eq 7.9
%
%     % get std of combined spec
%     sigma_combined_spec(j,:) = sum_rows.^(-1/2);
%
%     % get normalized combine spectrum
%     norm_combined_spec(j,:) = combined_spec(j,:)./sigma_combined_spec(j,:); % normalized combined spec Brubaker thesis eq 7.11
%     edges_norm_combined = linspace(min(norm_combined_spec),max(norm_combined_spec),nbins+1);
%     [norm_combined_counts, norm_combined_edges]= histcounts(norm_combined_spec(j,:), edges_norm_combined);
%     norm_combined_excess = movmean(norm_combined_edges, 2);
%     norm_combined_excess = norm_combined_excess(2:nbins+1);
%     fnc = fit(norm_combined_excess',norm_combined_counts','gauss1'); % f(x) =  a1*exp(-((x-b1)/c1)^2)
%     sigma_norm_combined(j) = fnc.c1/sqrt(2);
%     mean_norm_combined(j) = fnc.b1;
%
%     savedir = ['S:\lehnert\common\Quantum Metrology for Dark Matter\Joyce Jiang\measurement\',dates{1},'\DataProcessed'];
%     save(fullfile(savedir,['\',CC_or_GC,'_FAxion',num2str(j)]),'ProbeInit','f2Side','BaseLine','BaseLine_SG','combined_spec','sigma_combined_spec',...
%     'norm_combined_spec','sigma_norm_combined','mean_norm_combined')
% end
%
%
%
