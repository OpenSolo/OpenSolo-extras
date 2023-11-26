%% plot gyro bias estimates
figure;
plot(statesLog(1,:),statesLog(9:11,:)/dt*180/pi);
grid on;
ylabel('Gyro Bias Estimate (deg/sec)');
xlabel('time (sec)');

%% plot velocity
figure;
plot(statesLog(1,:),statesLog(6:8,:));
grid on;
ylabel('Velocity (m/sec)');
xlabel('time (sec)');