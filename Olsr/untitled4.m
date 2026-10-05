% olsr_manet_sim.m
% OLSR MPR Selection Simulation — 23CSE343 Wireless & Mobile Networks
% Case Study 2, Part 2 | Team 10
% Requires: FileExchange 71079 (createConnectedNetwork, mpr_select)

clear; clc; close all;
rng(42);

SOURCE  = 1;
N       = 15;
DENSITY = 0.48;

%% Step 1: Generate base network
G0 = createConnectedNetwork(N, DENSITY);
A1 = full(adjacency(G0));
fprintf('Base: N=%d, density=%.2f, avg_deg=%.2f\n\n', ...
    N, DENSITY, mean(sum(A1)));

%% ── DESIGN 1: Normal ──────────────────────────────────────────────────────
G1  = graph(A1);
R1  = mpr_select(G1, SOURCE);       % struct returned by FileExchange 71079
MPR1 = double(R1.selected_MPRs(:));
fprintf('=== DESIGN 1: Normal ===\n');
fprintf('N1=%d | N2=%d | MPRs=%d | Reduction=%.1f%%\n\n', ...
    R1.n_firstHop, R1.n_secondHop, R1.n_mprs, R1.reduction_pct);
fprintf('MPR nodes: '); disp(MPR1');

%% ── DESIGN 2: Link Failure (links 1-3 and 1-5 removed) ───────────────────
A2=A1; A2(1,3)=0; A2(3,1)=0; A2(1,5)=0; A2(5,1)=0;
G2  = graph(A2);
R2  = mpr_select(G2, SOURCE);
MPR2 = double(R2.selected_MPRs(:));
fprintf('=== DESIGN 2: Link Failure ===\n');
fprintf('N1=%d | N2=%d | MPRs=%d | Reduction=%.1f%%\n\n', ...
    R2.n_firstHop, R2.n_secondHop, R2.n_mprs, R2.reduction_pct);

%% ── DESIGN 3: Node Failure (node 3 removed) ───────────────────────────────
A3=A1; A3(3,:)=0; A3(:,3)=0;
G3  = graph(A3);
R3  = mpr_select(G3, SOURCE);
MPR3 = double(R3.selected_MPRs(:));
fprintf('=== DESIGN 3: Node Failure ===\n');
fprintf('N1=%d | N2=%d | MPRs=%d | Reduction=%.1f%%\n\n', ...
    R3.n_firstHop, R3.n_secondHop, R3.n_mprs, R3.reduction_pct);

%% ── DESIGN 4: Both Failures ───────────────────────────────────────────────
A4=A2; A4(3,:)=0; A4(:,3)=0;
G4  = graph(A4);
R4  = mpr_select(G4, SOURCE);
MPR4 = double(R4.selected_MPRs(:));
fprintf('=== DESIGN 4: Link + Node Failure ===\n');
fprintf('N1=%d | N2=%d | MPRs=%d | Reduction=%.1f%%\n\n', ...
    R4.n_firstHop, R4.n_secondHop, R4.n_mprs, R4.reduction_pct);

%% ── MPR sets and hop counts from source for every design ────────────────
graphs = {G1, G2, G3, G4};
mprs   = {MPR1, MPR2, MPR3, MPR4};
names  = {'D1_Normal','D2_Link','D3_Node','D4_Both'};

fprintf('=== MPR SETS ===\n');
for d = 1:4
    fprintf('Design %d MPRs: %s\n', d, num2str(mprs{d}'));
end

H = zeros(4, N);
for d = 1:4
    H(d,:) = distances(graphs{d}, SOURCE);   % hop count from node 1; Inf = unreachable
end

fprintf('\n=== HOP COUNT FROM NODE %d TO EACH NODE ===\n', SOURCE);
T = array2table(H, 'VariableNames', "n"+(1:N), 'RowNames', names);
disp(T);

fprintf('=== ROUTING SUMMARY ===\n');
for d = 1:4
    reachable = isfinite(H(d,:)) & (1:N) ~= SOURCE;
    fprintf('Design %d: reachable=%d/%d | avg hops=%.2f | max hops=%d\n', ...
        d, sum(reachable), N-1, mean(H(d,reachable)), max(H(d,reachable)));
end

%% ── Figure: 4-panel topology ──────────────────────────────────────────────
titles = {'Design 1: Normal', 'Design 2: Link Failure', ...
    'Design 3: Node Failure', 'Design 4: Both Failures'};
mprs   = {MPR1, MPR2, MPR3, MPR4};

figure('Name','OLSR MANET','Color','k', ...
    'Units','normalized','Position',[0.02 0.02 0.96 0.92]);

for d = 1:4
    subplot(2, 2, d);
    G = graphs{d};
    h = plot(G, 'Layout','force', ...
        'NodeColor',[0.2 0.5 0.9], ...
        'EdgeColor',[0.4 0.4 0.7], ...
        'LineWidth',1.0, 'MarkerSize',7);
    set(gca,'Color','k','XColor','w','YColor','w');

    % Source: red
    highlight(h, SOURCE, 'NodeColor','r', 'MarkerSize',10);

    % MPR nodes: green
    mpr_d = mprs{d};
    mpr_d = mpr_d(mpr_d ~= SOURCE);
    if ~isempty(mpr_d)
        highlight(h, mpr_d, 'NodeColor','g', 'MarkerSize',9);
    end

    labelnode(h, 1:numnodes(G), string(1:numnodes(G)));
    title(titles{d}, 'Color','w', 'FontSize',11, 'FontWeight','bold');
end

annotation('textbox',[0.01 0.005 0.5 0.03], ...
    'String','Red = Source Node   |   Green = MPR   |   Blue = Regular Node', ...
    'Color','w','BackgroundColor','k','EdgeColor','none','FontSize',9);