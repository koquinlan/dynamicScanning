%% Visibility Theory Fitting
% Created by Joyce Jiang, Jan/19/22
% Adapted with small modifications by Kyle Quinlan, 8/3/23

%% Define constants
Hz = 1;
kHz = 10^3;
MHz = 10^6;
GHz = 10^9;
hbar = 1.0545718*10^-34;
kb = 1.380649*10^-23;
mK = 1e-3;
K = 1;

%% Parameters
% C.C. & GC: 1st paramater is CC, 2nd is GC
kA = 2*pi*20.56*MHz; % external coupling rates of readout (X) mode
kAl = 2*pi*959*kHz; % internal loss rates of readout (X) mode
kB = 2*pi*1160*Hz; % external coupling rates of axion (Y) mode
kBl = 2*pi*900*kHz; % internal loss rates of axion (Y) mode
gAB = [sqrt(kBl*kA)/2*1.00, 2*pi*7.2*MHz]; % SWAP rate: 1st parameter: C.C; 2nd parameter: GC
hAB = [0, 2*pi*7.2*MHz];
sA = [0, 2*pi*0*MHz]; % SMS rate on A mode
sB = [0, 2*pi*0*MHz];
delta = [0, sA(2)-sB(2)-2*pi*0*kHz]; 
sigma = [0, -sA(2)-sB(2)+2*pi*0*kHz];
N_20mK = 1/(exp(hbar*2*pi*4.985*GHz/(kb*25*mK))-1)+1/2;
N_4K = 1/(exp(hbar*2*pi*4.985*GHz/(kb*4*K))-1)+1/2;
N_HEMT = 15; % HEMT added noise, from HEMT added noise measurement.
eta = 0.9; % transmission btw JPC and JPA, at 20mK stage
alpha = 0.5; % transmissio btw JPA and HEMT, at 4K stage, from HEMT added noise measurement.
JPA_det = 0; % JPA res detuning wrt. GC resonance
phi = 0*pi; % relative phase between G, C pumps

% other parameters
theta = pi/2 - phi/2; % readout quadrature
na = 2*pi*2.3e-6*2.4e7/kB*1.8e6/1.9; % axion occupation
nT = 0; % thermal occupation

% input matrix: noise + axion
Sin_mat  = zeros(6,6);
Sin_mat(1,1) = na;
Sin_mat(2,2) = nT;
Sin_mat(3,3) = nT;
Sin_mat(4,4) = nT + 1;
Sin_mat(5,5) = nT + 1;
Sin_mat(6,6) = na + 1;

% noise input matrix: na = 0
Sin_mat_N  = zeros(6,6);
Sin_mat_N(1,1) = 0;
Sin_mat_N(2,2) = nT;
Sin_mat_N(3,3) = nT;
Sin_mat_N(4,4) = nT + 1;
Sin_mat_N(5,5) = nT + 1;
Sin_mat_N(6,6) = 1;

% Loop Parameters
wmax = 20*2*pi*MHz;
wstep = 0.01*2*pi*MHz;
points = 2*wmax/wstep+1;
ws = -wmax:wstep:wmax;

%% CC: JPA gain profile
% 5.5GHz JPA gain profile taken by ENA
folder = 'C:\Users\Lehnert Lab\Desktop\LehnertLab-Codebase\Axions\TestingScripts\07-Jul-2023\ascii';
JPAdata = importdata([folder,'\s21ampenaf_76.dat']);
JPAfreq = JPAdata.data(:,1);
JPAgain = JPAdata.data(:,2)-mean(JPAdata.data(1:20,2));
res_idx = find(JPAgain == max(JPAgain));
JPA_res = JPAfreq(res_idx);
% figure();
% plot((JPAfreq-JPA_res)*10^3, JPAgain)
% xlim([-100 100])
% xlabel('detuning [MHz]');
% ylabel('JPA gain [dB]');

% fit JPA gain
fit_model = fittype('mag2db(abs(1 -k/(1i*(f-f0)+k/2-p^2/(k/2+1i*(f-f0)))))','coefficients',{'k','p','f0'},'independent','f');
fit_JPA = fit((JPAfreq-JPA_res)*10^3, JPAgain, fit_model, 'StartPoint', [60, 30, 0.2]);
k_JPA = fit_JPA.k;
p_JPA = fit_JPA.p;
f0_JPA = fit_JPA.f0;
% figure() 
% plot(fit_JPA, (JPAfreq-JPA_res)*10^3, JPAgain);
% xlim([-50 50]);

% resample the fitted curve at ws freqs
G_JPA_CC = abs(1 -k_JPA./(1i.*(ws/(2*pi*MHz)-JPA_det)+k_JPA/2-p_JPA^2./(k_JPA/2+1i.*((ws/(2*pi*MHz)-JPA_det))))).^2; % *4 count for single quad. gain

% figure();
% plot((JPAfreq-JPA_res)*10^3, JPAgain)
% hold on
% plot(ws/(2*pi*MHz), pow2db(G_JPA_CC))
% xlim([-50 50]);


%% GC: JPA gain profile
% 5.5GHz JPA gain profile taken by ENA
folder = 'C:\Users\Lehnert Lab\Desktop\LehnertLab-Codebase\Axions\TestingScripts\07-Jul-2023\ascii';
JPAdata = importdata([folder,'\s21ampenaf_76.dat']);
JPAfreq = JPAdata.data(:,1);
JPAgain = JPAdata.data(:,2)-mean(JPAdata.data(1:20,2));
res_idx = find(JPAgain == max(JPAgain));
JPA_res = JPAfreq(res_idx);
% figure();
% plot((JPAfreq-JPA_res)*10^3, JPAgain)
% xlim([-100 100])
% xlabel('detuning [MHz]');
% ylabel('JPA gain [dB]');

% fit JPA gain
fit_model = fittype('mag2db(abs(1 -k/(1i*(f-f0)+k/2-p^2/(k/2+1i*(f-f0)))))','coefficients',{'k','p','f0'},'independent','f');
fit_JPA = fit((JPAfreq-JPA_res)*10^3, JPAgain, fit_model, 'StartPoint', [60, 30, 0.2]);
k_JPA = fit_JPA.k;
p_JPA = fit_JPA.p;
f0_JPA = fit_JPA.f0;
% figure() 
% plot(fit_JPA, (JPAfreq-JPA_res)*10^3, JPAgain);
% xlim([-50 50]);

% resample the fitted curve at ws freqs
JPA_det = 0;
G_JPA_GC = abs(1-k_JPA./(1i.*(ws/(2*pi*MHz)-JPA_det)+k_JPA/2-p_JPA^2./(k_JPA/2+1i.*((ws/(2*pi*MHz)-JPA_det))))).^2;% *4 count for single quad. gain
% figure();
% plot(ws/(2*pi*MHz), pow2db(G_JPA_GC))
% xlim([-50 50]);

%% Amplified quadrature readout: cascading amplifier chain
% Linear equations
SR_SQ = zeros(1,numel(gAB));
vis_mat = zeros(numel(gAB),numel(ws));
S_a = zeros(numel(gAB),numel(ws));
S_N = zeros(numel(gAB),numel(ws));

% For the GC case as well, vary k
% for k = 1:numel(gAB)
k=1;
    chiALlst = zeros(1,numel(ws));
    chiAAlst = zeros(1,numel(ws));
    vis_SQ = zeros(1,numel(ws));
    Sout_amp_SQ = zeros(1,numel(ws));
    Sout_amp_a_SQ = zeros(1,numel(ws));
    Sout_amp_N_SQ = zeros(1,numel(ws));
    for i = 1:numel(ws)
        % A
        denom0 = 1i*(ws(i)+(delta(k)-sigma(k))/2)+kA/2+kAl/2;
        AAin = sqrt(kA)/denom0;
        ALAin = sqrt(kAl)/denom0;
        AA = -1;
        AAdag = -1i*sA(k)*exp(-1i*phi)/denom0;
        AB = -1i*gAB(k)/denom0;
        ABdag = -1i*hAB(k)*exp(-1i*phi)/denom0;
        A = [AA AAdag AB ABdag];

        % Adag
        denom1 = 1i*(ws(i)-(delta(k)-sigma(k))/2)+kA/2+kAl/2;
        AdagAindag = sqrt(kA)/denom1;
        AdagLAindag = sqrt(kAl)/denom1;
        AdagA = 1i*sA(k)*exp(1i*phi)/denom1;
        AdagAdag = -1;
        AdagB = 1i*hAB(k)*exp(1i*phi)/denom1;
        AdagBdag = 1i*gAB(k)/denom1;
        Adag = [AdagA AdagAdag AdagB AdagBdag];

        % B
        denom = 1i*(ws(i)-(delta(k)+sigma(k))/2)+kB/2+kBl/2;
        BBin = sqrt(kB)/denom;
        BLBin = sqrt(kBl)/denom;
        BB = -1;
        BBdag = -1i*sB(k)*exp(-1i*phi)/denom;
        BA = -1i*gAB(k)/denom;
        BAdag = -1i*hAB(k)*exp(-1i*phi)/denom;
        B = [BA BAdag BB BBdag];

        % Bdag
        denom3 = 1i*(ws(i)+(delta(k)+sigma(k))/2)+kB/2+kBl/2;
        BdagBindag = sqrt(kB)/denom3;
        BdagLBindag = sqrt(kBl)/denom3;
        BdagB = 1i*sB(k)*exp(1i*phi)/denom3;
        BdagBdag = -1;
        BdagAdag = 1i*gAB(k)/denom3;
        BdagA = 1i*hAB(k)*exp(1i*phi)/denom3; 
        Bdag = [BdagA BdagAdag BdagB BdagBdag];

        eqs = [A; Adag; B; Bdag];
        XA = linsolve(eqs,[-AAin; zeros(3,1)]); % X represents A, Adag, B, Bdag
        XL = linsolve(eqs,[0; 0; -BLBin; 0]);
        XB = linsolve(eqs,[0; 0; -BBin; 0]);
        XAdag = linsolve(eqs,[0; -AdagAindag; 0; 0]);
        XLdag = linsolve(eqs,[0; 0; 0; -BdagLBindag]);
        XBdag = linsolve(eqs,[0; 0; 0; -BdagBindag]);

        chiAAlst(i) = 1 - sqrt(kA)*XA(1); % Aout is the 1st solution in eqs
        chiABlst(i) = -sqrt(kA)*XB(1);
        chiALlst(i) = -sqrt(kA)*XL(1);
        chiAAdaglst(i) = -sqrt(kA)*XAdag(1); 
        chiABdaglst(i) = -sqrt(kA)*XBdag(1);
        chiALdaglst(i) = -sqrt(kA)*XLdag(1);

        chiAdagAlst(i) = -sqrt(kA)*XA(2); % Adagout is the 2nd solution in eqs
        chiAdagBlst(i) = -sqrt(kA)*XB(2);
        chiAdagLlst(i) = -sqrt(kA)*XL(2);
        chiAdagAdaglst(i) = 1 - sqrt(kA)*XAdag(2); 
        chiAdagBdaglst(i) = -sqrt(kA)*XBdag(2);
        chiAdagLdaglst(i) = -sqrt(kA)*XLdag(2);
        
        chi_mat = zeros(6,6);
        chi_mat(3,1) = chiABlst(i);
        chi_mat(3,2) = chiALlst(i);
        chi_mat(3,3) = chiAAlst(i);
        chi_mat(3,4) = chiAAdaglst(i);
        chi_mat(3,5) = chiALdaglst(i);
        chi_mat(3,6) = 0;    %faxion signal is not sending through A+ port
        % chi_mat(3,6) = chiABdaglst(i);
        chi_mat(4,1) = chiAdagBlst(i);
        chi_mat(4,2) = chiAdagLlst(i);
        chi_mat(4,3) = chiAdagAlst(i);
        chi_mat(4,4) = chiAdagAdaglst(i);
        chi_mat(4,5) = chiAdagLdaglst(i);
        chi_mat(4,6) = 0;
        % chi_mat(4,6) = chiAdagBdaglst(i);

        Sout_mat = conj(chi_mat)*Sin_mat*(chi_mat.');
        Sout_mat_N = conj(chi_mat)*Sin_mat_N*(chi_mat.');
        
        % SINGLE QUADRATURE READOUT
        Sout_amp_SQ(i) = ((Sout_mat(3,3)+Sout_mat(4,4))+(Sout_mat(4,3)+Sout_mat(3,4))*(cos(theta)^2-sin(theta)^2))/2 ...
            -1i*(Sout_mat(4,3)-Sout_mat(3,4))*cos(theta)*sin(theta);
        Sout_amp_N_SQ(i) = ((Sout_mat_N(3,3)+Sout_mat_N(4,4))+(Sout_mat_N(4,3)+Sout_mat_N(3,4))*(cos(theta)^2-sin(theta)^2))/2 ...
            -1i*(Sout_mat_N(4,3)-Sout_mat_N(3,4))*cos(theta)*sin(theta);
        Sout_amp_a_SQ(i) = Sout_amp_SQ(i) - Sout_amp_N_SQ(i);
        if k == 1 % k=1, C.C case; else k=2, GC case
            vis_SQ(i) = Sout_amp_a_SQ(i)./(Sout_amp_N_SQ(i) + (1-eta)/eta*N_20mK + (1-alpha)*N_4K/(alpha*eta*4*G_JPA_CC(i)) + N_HEMT/(alpha*eta*4*G_JPA_CC(i))); % 4 accouts for single quadrature gain
        else
            G_JPC(i) = 4*abs(chiALlst(i))^2; % times 4 accout for single quadrature gain
            vis_SQ(i) = Sout_amp_a_SQ(i)./(Sout_amp_N_SQ(i) + (1-eta)/eta*N_20mK + (1-alpha)*N_4K/(alpha*eta*4*G_JPA_GC(i)) + N_HEMT/(alpha*eta*4*G_JPA_GC(i)));
        end
    end 
S_a(k,:) = Sout_amp_a_SQ;
S_N(k,:) = Sout_amp_N_SQ;
vis_mat(k,:) = vis_SQ;
SR_SQ(k) = sum(abs(vis_SQ).^2)*wstep;

SRE_SQ = SR_SQ(2)/SR_SQ(1);

% % save visibility theory matrix
% savefile = ['S:\lehnert\common\Quantum Metrology for Dark Matter\Joyce Jiang\measurement\02-May-2022\Vis_Theory.mat'];
% save(fullfile(savefile),'ws','vis_mat')

experimentalVisCurve = readmatrix('visCurve.csv');
experimentalFreqs = readmatrix('trueProbeFreqs.csv');
figure();
plot(ws/(2*pi*MHz), vis_mat(1,:));
hold on
plot(experimentalFreqs, experimentalVisCurve);
xlim([-10 10]);
xlabel('Probe Det. [MHz]');
ylabel('Visibility');
legend('CC')
