%% Simple plot of Real vs Sim data - Error vs Impedance
clear all; close all; clc;

% Hardcoded impedance values in decades (ohms)
Impedance = [1, 10, 100, 1000, 10000, 100000, 1000000, 10000000];

% Hardcoded experimental error data (%) at corresponding impedance decades
Experimental_Error = [2, 0.12, 0.12, 0.14, 0.15, 0.17, 0.21, 0.98];

% Hardcoded simulation error data (%) at corresponding impedance decades
Simulation_Error = [0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1];

% Create plot
figure;
hold on;

% Plot Experimental error data (solid line)
plot(Impedance, Experimental_Error, 'k-x', 'LineWidth', 1.5, 'MarkerSize', 8, 'DisplayName', 'Theoretical');

% Plot Simulation error data (dotted line)
plot(Impedance, Simulation_Error, 'k--x', 'LineWidth', 1.5, 'MarkerSize', 8, 'DisplayName', 'Simulation');

xlabel('Impedance (\Omega)', 'FontSize', 18);
ylabel('Error (%)', 'FontSize', 18);
title('Error against Impedance', 'FontSize', 20);
legend('Location', 'best', 'FontSize', 16);
grid on;
set(gca, 'XScale', 'log');
xlim([1 10000000]);
ylim([0 1]);

% Set y-axis ticks from 0% to 1% in 0.1% increments
yticks(0:0.1:1);
ytickformat('%.1f%%');

% Set axes font size
set(gca, 'FontSize', 12);

hold off;