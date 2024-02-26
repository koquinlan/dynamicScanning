#include "decs.hpp"

ScanParameters unpackScanParameters(json const& inputParams) {
    ScanParameters scanParameters;

    scanParameters.topLevelParameters.decisionMaking = inputParams["topLevelParams"]["decisionMaking"];
    scanParameters.topLevelParameters.baselinePath = inputParams["topLevelParams"]["baselinePath"];
    scanParameters.topLevelParameters.statePath = inputParams["topLevelParams"]["statePath"];
    scanParameters.topLevelParameters.savePath = inputParams["topLevelParams"]["savePath"];

    scanParameters.dataParameters.maxIntegrationTime = inputParams["dataParams"]["maxIntegrationTime"];
    scanParameters.dataParameters.sampleRate = inputParams["dataParams"]["sampleRate"];
    scanParameters.dataParameters.RBW = inputParams["dataParams"]["RBW"];
    scanParameters.dataParameters.trueCenterFreq = inputParams["dataParams"]["trueCenterFreq"];
    scanParameters.dataParameters.subSpectraAveragingNumber = inputParams["dataParams"]["subSpectraAveragingNumber"];

    scanParameters.filterParameters.cutoffFrequency = inputParams["filterParams"]["cutoffFrequency"];
    scanParameters.filterParameters.poleNumber = inputParams["filterParams"]["poleNumber"];
    scanParameters.filterParameters.stopbandAttenuation = inputParams["filterParams"]["stopbandAttenuation"];

    return scanParameters;
}