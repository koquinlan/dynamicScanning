mexPath = genpath('../build/src/release/');
addpath(mexPath);

scanDir = "C:/Users/Lehnert Lab/Desktop/mexDynamicScanning/scanDir/";

% Baseline Parameters
baselineParams = struct(...
    'repeats', 3, ...
    'subSpectra', 32, ...
    'savePlots', 1 ...
);


% Static Run Parameters
integrationTimes = [20];
stepSize = 0.1;
numSteps = 2;
targetCoupling = 3.4e-5;

scanParams = struct(...
    'topLevelParams', struct(...
        'decisionMaking', false, ...
        'baselinePath', scanDir + "baseline/", ...
        'statePath', scanDir + "state/", ...
        'savePath', scanDir + "save/", ...
        'visPath', scanDir + "vis/", ...
        'wisdomPath', scanDir + "wisdom/" ...
    ), ...
    'dataParams', struct(...
        'maxIntegrationTime', 7.5, ... % This will get changed based on which scan we're on   
        'sampleRate', 32e6, ...
        'RBW', 100, ...
        'trueCenterFreq', 1, ...
        'subSpectraAveragingNumber', 15 ...
    ), ...
    'filterParams', struct(...
        'cutoffFrequency', 10e3, ...
        'poleNumber', 3, ...
        'stopbandAttenuation', 15.0 ...
    )...
);
%% Baselining
fprintf('Beginning Baselining')

baselinePerf = jsondecode(mexBaseline(jsonencode(scanParams), jsonencode(baselineParams)));

fprintf('Baselining Complete')
%% Scan Logic
fprintf('Beginning Scans\n')

performances = cell(length(integrationTimes), numSteps);

for idx = 1:length(integrationTimes)
    scanParams.dataParams.maxIntegrationTime = integrationTimes(idx);
    
    for step = 1:numSteps
        fullSave = (mod(step, 10) == 1);

        % Run the scan
        tic
        performances{idx, step} = jsondecode(mexScanRunner(jsonencode(scanParams), fullSave));
        toc

        scanParams.dataParams.trueCenterFreq = scanParams.dataParams.trueCenterFreq + stepSize;
    end
end

fprintf('Scans Complete\n')
%% 

clear all