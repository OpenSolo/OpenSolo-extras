% IMPORTANT - This script requires the Matlab symbolic toolbox

clear all;

%% define symbolic variables and constants
syms bodyRateX bodyRateY bodyRateZ real; % body rates relative to Airframe rad/sec
syms azAngle rollAngle elevAngle real; % gimbal joint angles yaw, roll, pitch (rad)
syms azAngleDot rollAngleDot elevAngleDot real; % gimbal joint angle rates yaw, roll, pitch (rad/sec)
syms c1 s1 c2 s2 c3 s3 real;
%% derive equations used to express gimbal rates as a function of airframe relative body rates and  gimbal joint angles
% Define trig quantities
c1 = cos(azAngle);
s1 = sin(azAngle);
c2 = cos(rollAngle);
s2 = sin(rollAngle);
c3 = cos(elevAngle);
s3 = sin(elevAngle);

% Define rotation about azimuth axis
Tyaw = [ c1  s1   0; ...
        -s1  c1   0; ...
          0   0   1];
% Define rotation about roll axis
Troll = [ 1   0   0; ...
          0  c2  s2; ...
          0 -s2  c2];
% Define rotation about elevation axis
Tpitch = [ c3   0  -s3; ...
            0   1    0; ...
           s3   0   c3];

% Calculate matrices for an Euler 312 sequence as used by the gimbal
omega312 = [0;elevAngleDot;0] + Tpitch*[rollAngleDot;0;0]   + Tpitch*Troll*[0;0;azAngleDot];
EulToBodyRates312 = simplify(jacobian(omega312,[rollAngleDot;elevAngleDot;azAngleDot]));
f = matlabFunction(EulToBodyRates312,'file','EulToBodyRates312.m');
fprintf('\n')
fprintf('EulToBodyRates312 = \n')
pretty(EulToBodyRates312)
BodyToEulRates312 = simplify(inv(EulToBodyRates312));
f = matlabFunction(BodyToEulRates312,'file','BodyToEulRates312.m');
fprintf('\n')
fprintf('BodyToEulRates312 = \n')
pretty(BodyToEulRates312)
DCM312 = simplify(Tpitch*Troll*Tyaw);
fprintf('\n')
fprintf('DCM312 = \n')
pretty(transpose(DCM312))

% calculate matrices for a standard aircraft dynamics 321 sequence so that
% methodology can be checked against standard results
omega321 = [rollAngleDot;0;0]   + Troll*[0;elevAngleDot;0] + Troll*Tpitch*[0;0;azAngleDot];
EulToBodyRates321 = simplify(jacobian(omega321,[rollAngleDot;elevAngleDot;azAngleDot]));
f = matlabFunction(EulToBodyRates321,'file','EulToBodyRates321.m');
fprintf('\n')
fprintf('EulToBodyRates321 = \n')
pretty(EulToBodyRates321)
BodyToEulRates321 = simplify(inv(EulToBodyRates321));
f = matlabFunction(BodyToEulRates321,'file','BodyToEulRates321.m');
fprintf('\n')
fprintf('BodyToEulRates321 = \n')
pretty(BodyToEulRates321)
