function plotExclusions(metricFolderPath, dataFolderPath)
    % Get a list of all .csv files in the metrics folder
    metricFiles = dir(fullfile(metricFolderPath, '*.csv'));
    
    % Load data from each .csv file and store it in the metricArrays list
    metricArrays = {};
    for k = 1:length(metricFiles)
        fileFullPath = fullfile(metricFolderPath, metricFiles(k).name);
        data = load(fileFullPath);
        metricArrays{end+1} = data;
    end

    % Get a list of all .csv files in the data folder
    dataFiles = dir(fullfile(dataFolderPath, '*.csv'));
    
    % Load data from each .csv file and store it in the dataArrays list
    dataArrays = {};
    for k = 1:length(dataFiles)
        fileFullPath = fullfile(dataFolderPath, dataFiles(k).name);
        data = load(fileFullPath);
        dataArrays{end+1} = data;
    end

    % Plot each data array in dataArrays
    target = 4.2e-5;
    figure; % Opens a new figure window

    for i = 1:length(dataArrays)
        data = dataArrays{i};
        label = sprintf('Exclusion Line %d', i);
        plot(data(:,2), data(:,1), 'DisplayName', label); hold on;
    end
    plot(get(gca, 'xlim'), [target target], 'r-', 'DisplayName', 'Target Exclusion'); % Add target exclusion line

    xlim([-0.1, 50.1]);
    ylim([0, 1.3 * target]);

    legend show;
    xlabel('X Label'); % Adjust as necessary
    ylabel('Y Label'); % Adjust as necessary
    title('Data with Target Exclusion');
    hold off;
end
