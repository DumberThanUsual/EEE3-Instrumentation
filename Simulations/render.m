% Minimal LTspice magnitude/phase reader for lines like:
%  Freq<TAB>(<mag>dB,<phase>°)
% Save as plot_ltspice_minimal.m and run.

fname = ['220u.txt'];  % <-- set your file name


sigma_dB  = 0.1;   % std dev of magnitude noise in dB
sigma_deg = 0.025;   % std dev of phase noise in degrees

% Read all lines
L = readlines(fname);
if numel(L) < 2
    error('File has no data lines.');
end

% Skip header line
L = L(2:end);

% Regex to capture: frequency, magnitude in dB, phase number (deg)
pat = '^\s*([+-]?\d+(?:\.\d+)?(?:[eE][+-]?\d+)?)\s+\(\s*([+-]?\d+(?:\.\d+)?(?:[eE][+-]?\d+)?)dB\s*,\s*([+-]?\d+(?:\.\d+)?(?:[eE][+-]?\d+)?)';

tok = regexp(L, pat, 'tokens', 'once');
keep = ~cellfun('isempty', tok);
tok = tok(keep);

if isempty(tok)
    error('No data matched the expected pattern. Check file format.');
end

T = vertcat(tok{:});
f      = str2double(T(:,1));   % Hz
mag_dB = str2double(T(:,2));   % dB
ph_deg = str2double(T(:,3));   % deg (numeric part only)

% Optional: unwrap phase
ph_deg_unw = unwrap(ph_deg*pi/180)*180/pi;
mag_lin = 10.^(mag_dB/20);      % linear magnitude from dB
mag_lin(mag_lin <= 0) = eps;    % guard against zeros for log plotting
mag_lin = mag_lin + sigma_dB * randn(size(mag_dB));
ph_deg_unw = ph_deg_unw + sigma_deg * randn(size(ph_deg));

figure('Color','w', 'Position', [0 0 600 400]);
tiledlayout(2,1,'TileSpacing','compact','Padding','compact');

% Magnitude: log axis (choose one)

% A) x log (frequency), y log (magnitude) → log-log plot
nexttile;
loglog(f, mag_lin, 'LineWidth',1.3);
grid on; box on;
xlabel('Frequency (Hz)');
ylabel('Magnitude (Ohm)');
xlim([100 100000])
title('Magnitude');

% OR B) x linear, y log (only y is log)
% nexttile; semilogy(f, mag_lin, 'LineWidth',1.3);

% OR C) keep your semilogx (x log) and set y to log via axes property
% nexttile; 
% plot(f, mag_lin, 'LineWidth',1.3);
% set(gca,'XScale','log','YScale','log');

% Phase (unchanged)
nexttile;
semilogx(f, ph_deg_unw, 'LineWidth',1.3); % x log is typical for Bode
grid on; box on;
xlabel('Frequency (Hz)');
ylabel('Phase (deg)');
xlim([100 100000])
title('Phase');

saveas(gca, '220u.eps','epsc');