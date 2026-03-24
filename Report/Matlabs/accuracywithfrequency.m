%% Simple plot of Real vs Sim data
clear all; close all; clc;

% Hardcoded data from table
Current = [0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1, 1.1, 1.2, 1.3, 1.4, 1.5, ...
           1.6, 1.7, 1.8, 1.9, 2, 2.1, 2.2, 2.3, 2.4, 2.5, 2.6, 2.7, 2.8, 2.9, 3];
       
Real = [0.6501428, 0.759392, 0.802812, 0.821621622, 0.849015152, 0.869565217, ...
        0.824166667, 0.826315789, 0.830813008, 0.84, 0.846382979, 0.832679739, ...
        0.837962963, 0.827586207, 0.831327869, 0.83409375, 0.836094527, 0.837941, ...
        0.8423054, 0.844019, 0.845281, 0.8459102, 0.8517212, 0.8523748, 0.8558123, ...
        0.8579123, 0.855123, 0.856123];
    
Sim = [0.705756, NaN, NaN, 0.815822, NaN, NaN, 0.843364, NaN, NaN, 0.893354, ...
       NaN, NaN, 0.847055, NaN, NaN, 0.866871, NaN, NaN, 0.875022, NaN, NaN, ...
       0.881413, NaN, NaN, 0.832854, NaN, NaN, 0.842336];

% Remove NaN values for connected Sim plot
sim_idx = ~isnan(Sim);
Current_sim = Current(sim_idx);
Sim_clean = Sim(sim_idx);

% Create plot
figure;
hold on;

% Plot Real data (solid line)
plot(Current, Real, 'k-x', 'LineWidth', 1.5, 'MarkerSize', 8, 'DisplayName', 'Real');

% Plot Sim data (dotted line)
plot(Current_sim, Sim_clean, 'k--x', 'LineWidth', 1.5, 'MarkerSize', 8, 'DisplayName', 'Sim');

xlabel('Current (A)', 'FontSize', 18);
ylabel('Efficiency', 'FontSize', 18);
title('Simulated vs Observed Efficiency Against Load', 'FontSize', 20);
legend('Location', 'best', 'FontSize', 16);
grid on;
xlim([0.3 3]);
ylim([0 1]);

% Set axes font size
set(gca, 'FontSize', 12);

hold off;