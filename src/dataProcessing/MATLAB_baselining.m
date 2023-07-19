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
nSpectra = 10; % Number of spectra within each probe to use for baseline

Noise_mat = zeros(nProbes, nSpectra,L);

for i = 1:nProbes 
    filePattern = fullfile(FileList{i}, '*.mat');
    matFiles = dir(filePattern);
    
    for j = 1:nSpectra
        NoiseData = load([FileList{i},'\',matFiles(j).name],'PS2SideCC');
        Noise_mat(i,j,:) = cell2mat(struct2cell(NoiseData(1)));
    end
end


%%
BaseLine_AllBins = mean(Noise,1);
Base_Excess_AllBins = BaseLine_AllBins./medfilt1(BaseLine_AllBins,100)-1;


% rough remove bad IF bins in baseline to get the histogram
base_BadBin_thres = 0.5;

bad_idx = find(Base_Excess_AllBins > base_BadBin_thres);
Base_Excess = Base_Excess_AllBins;
Base_Excess(bad_idx) = NaN; % get rid of extremely large value in order to fit gaussian


% mark the bins with power excess > acceptable threshold
edges_baseline = linspace(min(Base_Excess),max(Base_Excess),nbins+1);
[excess_counts, base_edges] = histcounts(Base_Excess,edges_baseline);
excess_pow = movmean(base_edges,2);
excess_pow = excess_pow(2:nbins+1);
fbase = fit(excess_pow',excess_counts','gauss1');
std = fbase.c1/sqrt(2);


% flag and remove bad IF bins on baseline
badIF_idx = find(abs(Base_Excess_AllBins)> n*std);

Base_Excess_BadRmvd = Base_Excess_AllBins;
Base_Excess_BadRmvd(badIF_idx)= NaN;

BaseLine_BadRmvd = BaseLine_AllBins;
BaseLine_BadRmvd(badIF_idx)= NaN;

missing = isnan(BaseLine_BadRmvd);
fprintf('Missing %d samples of %d\n',sum(missing),L)

BaseLine = fillmissing(BaseLine_BadRmvd,'linear',2,'EndValues','nearest'); % fill NaN value
BaseLine_SG = sgolayfilt(BaseLine,SG_order,SG_framelen);


%%
histogram(Base_Excess)
% plot(excess_counts)
hold on
plot(fbase)

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
ylim([0 0.6e-10])

legend('All bins','mean','SG filtered')
xlabel('$f-f_{cav}$ [MHz]','Interpreter','Latex');
ylabel('power [a.u.]')
set(gca,'fontsize',18,'linewidth',1,'fontname','times')





