mexPath = genpath('../build/src/release/');
addpath(mexPath);

scanDir = "C:/Users/Lehnert Lab/Desktop/mexDynamicScanning/scanDir/";

baselineParams = struct(...
    'repeats', 3, ...
    'subSpectra', 32, ...
    'savePlots', 0 ...
);

scanParams = struct(...
    'topLevelParams', struct(...
        'decisionMaking', true, ...
        'baselinePath', scanDir + "baseline/", ...
        'statePath', scanDir + "state/", ...
        'savePath', scanDir + "save/", ...
        'visPath', scanDir + "vis/", ...
        'wisdomPath', scanDir + "wisdom/" ...
    ), ...
    'dataParams', struct(...
        'maxIntegrationTime', 1.234, ...
        'sampleRate', 32e6, ...
        'RBW', 100, ...
        'trueCenterFreq', 1, ...
        'subSpectraAveragingNumber', 20 ...
    ), ...
    'filterParams', struct(...
        'cutoffFrequency', 10e3, ...
        'poleNumber', 3, ...
        'stopbandAttenuation', 15.0 ...
    )...
);


foo = mexBaseline(jsonencode(scanParams), jsonencode(baselineParams));
ans = mexScanRunner(jsonencode(scanParams));


clear all