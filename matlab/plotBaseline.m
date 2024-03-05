function plotBaseline(rootPath)
    % Load the data
    rawData = load(rootPath + "rawData.csv");
    baseline = load(rootPath + "baseline.csv");
    runningAverage = load(rootPath + "runningAverage.csv");
    freq = load(rootPath + "freq.csv");
    outliers = load(rootPath + "outliers.csv");
    
    % Create the plot
    plot(freq, rawData, 'DisplayName', 'Raw Data'); hold on;
    plot(freq, runningAverage, 'DisplayName', 'Running Average');
    plot(freq, baseline, 'DisplayName', 'Baseline');
    plot(freq(outliers), baseline(outliers), 'o', 'DisplayName', 'Outliers', 'Color', 'red');
    hold off;
    
    % Adjust the y-limits
    ylim([0, 1.3 * max(runningAverage)]);
    
    % Add legend
    legend show;
    
    % Display the plot
    title('Baseline Analysis');
    xlabel('Frequency (Hz)');
    ylabel('Amplitude');
end
