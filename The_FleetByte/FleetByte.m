%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% CSC C85 - Fundamentals of Robotics and Automated Systems
% UTSC - Fall 2021
%
% Starter code (c) F. Estrada, August 2021
% 
% Sensors and Signal Processing
%
%  You may have heard there are all kinds of plans to
% send humans to Mars. Eventually, some think we may
% establish long-term habitats on the martian surface
% occupied for long periods by 'martians'.
%
%  One of the challenges of maintaining a long term
% presence on Mars is that martian gravity is much
% weaker than Earth's, and though it's still much
% better than long-term living in space, martian
% explorers would need to keep a serious exercise
% program in order to prevent physical deterioration.
%
%  To help with this task, we've developed the
% FleetByte(tm). A device worn on a person's wrist
% that keeps track of their exercise. It's similar to
% devices you may be familiar with (or indeed which
% you may be wearing). Our goal here is to design
% the sensor and signal processing software that
% will convert the raw measurements provided by
% sensors in the device into accurate estimates
% of what the human wearing it is doing.
%
% Your task is to:
%
% a) Understand the different sensors available,
%    the values they report, and their noise profile.
% b) Implement suitable noise reduction and estimation
%    routines so as to obtain estimates of state variables
%    that are as close as possible to their actual values.
% c) Apply the ideas we discussed in lecture: Noise
%    filtering, consistency, and information redundancy
%    in order to obtain good estimates for state variables.
%
% []=FleetByte(secs, debug)
%
%   secs - number of (virtual) seconds to run the simulation for.
%          Each call to Sim1() returns sensor readings for 1 sec,
%          so this is in effect the numbe or rounds of simulation
%          you want. 
%
%   map - Select map (1 or 2), each is a crop from the global Mars
%         elevation map from NASA - image in public domain. Note
%         that motion on the map is *not to scale*, the map corrsponds
%         to a huge area on Mars, and I want to show motion on this
%         map. So we will pretend it corresponds to an area roughly
%         .5 x .5 Km in size.
%
%  debug - If set to 1, this script will plot the returned hearrate
%          sensor output (so you can see what it looks like and think
%          about how to get a heartrate out of it), and print out
%          the sensor readings returned by Sim1(). You can add your
%          own debug/testing output as well.
%               
% - delta_t - maximum change in rover direction per unit of time, in radians
%
%  On Board Sensors:
%
%  MPS - Martian Positioning System - reports 3D position anywhere on Mars
%        to within a small displacement from actual location. Like its
%        Earthly cousin, MPS has an expected location error. For 
%        a typical wearable device, on Earth, location error is
%        within 5m of the actual location 
%        (https://www.gps.gov/systems/gps/performance/accuracy/)
%        Our FleetByte has a similar receiver, but due to the lower
%        density of Martian atmosphere, distortion due to armospheric
%        effects is lower. Under open sky this means a typical location
%        accuracy of less than 1.5m.
%
%        Note: On Mars we don't have to worry about buildings. On Earth things
%          are more difficult since buildings reflect GPS signals leading
%          to increased error in position estimates.
%
%  Heart Rate Sensor (HRS) - This one is interesting. Modern wearable 
%        HR monitors typically use light reflection from 
%        arterial blood to determine the heart rate - the
%        pulsing blood creates a periodic waveform in the 
%        reflected light. Issues with noise, low signal-to-noise
%        ratio, and effects due to skin colour, thickness, and
%        even ambient light combine to produce a fairly noisy
%        signal. The HR sensor will return an array consisting
%        of the signal measured over the last 10 seconds, from
%        which you will estimate the actual heartrate.
%        If you're very curious, this manufacturer has a
%        very thorough description of how their sensor works and
%        the different technical issues involved in computing a
%        heartrate from it ** YOU ARE NOT EXPECTED TO READ 
%        THROUGH AND IMPLEMENT THIS, IT'S THERE IN CASE YOU 
%        WANT TO LEARN MORE **
%        https://www.maximintegrated.com/en/products/interface/sensor-interface/MAX30102.html#product-details
%
%  Rate gyro (RG) - A fairly standard rate gyro, returns the measured
%              change in angle for the direction of motion (i.e.
%              tells you by how many radians this direction changed
%              in between readings). 
%
%              Somewhat noisy, but this assuming the user doesn't
%              move their arms in weird directions while running
%              it won't be affected by periodic arm motions.
%
%  ** The simulation returns to you the values measured by each
%  ** of these sensors at 1 second intervals. It's up to you to
%  ** decide how best to use/combine/denoise/filter/manipulate
%  ** the sensor readings to produce a good estimate of the actual
%  ** values of the relevant variables (which are also returned
%  ** by the simulation for the purpose of evaluating your
%  ** estimates' accuracy - needless to say, you can't use these
%  ** in any way, shape, or form, to accompish your task.
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

function []=FleetByte(secs, map, debug)

pkg load image;             %%% UNCOMMENT THIS FOR OCTAVE - Octave is doofus and requires this line... arghh!
pkg load statistics;
pkg load optim;
pkg load signal;

close all;
%%%%%%%%%% YOU CAN ADD ANY VARIABLES YOU MAY NEED BETWEEN THIS LINE... %%%%%%%%%%%%%%%%%
 pos_size=5; %5
 MPSpos_size=50;
 pos=ones(3,pos_size)*-1;
 MPSpos=ones(3,MPSpos_size)*-1;
 dir=ones(1,pos_size)*-1;
 prev_xyz=[256 256 .5]; 
 weight_Rg = 0.7; % [0 1], 0: Estimate from delta xy, 1: Estimate from prev_di and Rg
 prev_di = [1 0];
 prev_vel=10;
 vel_conv = 1000/3600;
 prev_hr_estimates = []; 
 
%%%%%%%%%% ... AND THIS LINE %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

idx=1;
while(idx<=secs)               %% Main simulation loop

 [MPS,HRS,Rg]=Sim1(map);       % Simulates 1-second of running and returns the sensor readings
                         
 %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 % TO DO:
 %  Sim1() returns noisy readings for global position (x,y,z), a heart-rate
 %    sensor signal array for the last 10 seconds, and a value for the rate
 %    gyro (the amount of rotation in radians by which the running direction
 %    changed over the last second).
 %
 %    In the space below, write code to:
 %
 %    - Estimate as closely as possible the actual 3D position of the jogger
 %
 %    - Compute the current hear-rate (this will require some thought, make
 %      sure to look closely at the plot of HRS, and think of ways in which
 %      you can determine the heart rate from this). Remember the data 
 %      in the plot corresponds to the last 10 seconds. And, just FYI, it's
 %      based on what the actual data returned from a typical wrist-worn
 %      heart rate monitor returns. So it's fairly realistic in terms of what
 %      you'd need to process if you were actually implementing a FleetByte
 %
 %    - Estimate the running direction (huh? but the rate gyro only returns
 %      the change in angle over the last second! we don't know the initial
 %      running direction right?) - well, you don't, but you can figure it
 %      out :) - that's part of the exercise.
 %      * REFERENCE: - given a direction vector, if you want to apply a
 %         rotation by a particular angle to this vector, you simply 
 %         multiply the vector by the corresponding rotation matrix:
 %
 %            d1=R*d;
 %
 %         Where d is the input direction vector (a unit-length, column
 %         vector with 2 components). R is the rotation matrix for 
 %         the amount of rotation you want:
 %
 %           R=[cos(theta) -sin(theta)
 %              sin(theta) cos(theta)];
 %
 %         'theta' is in radians. Finally, d1 is the resulting direction vector.
 %
 %    - Estimate the running speed in Km/h - This is *not* returned by any
 %      of the sensor readings, so you have to estimate it (carefully). 
 %
 %    Goal: To get the estimates as close as possible to the real value for
 %          the relevant quantities above. The last part of the script calls
 %          the imulation code to plot the real values against your estimares
 %          so you can see how well you're doing. In particular, you want the
 %          RMS of each measurement to be as close to 0 as possible.
 %          RMS is a common measure of error, and corresponds to the square
 %          root of the average squared error between a measurement and the
 %          corresponding estimate, taken over time. 
 %    
 %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

 xyz=[256 256 .5]; %prev_xyz;       % Replace with your computation of position, the map is 512x512 pixels in size
 hr=82;                  % Replace with your computation of heart rate
 di=[0 1];               % Replace with your computation for running direction, this should be a 2D unit vector
 vel=5;                  % Replace with your computation of running velocity, in Km/h
 
 %%%%%%%%%%%%%% Calculating Heartrate %%%%%%%%%%%%%%%%
 % Smooth the HRS signal (shift up so no negative values)
 L = 1200;
 t = (0 : L - 1);
 smooth_HRS = 3 * (sgolayfilt(HRS, 4, 101) + 1);
 
 % find peaks
 [amplitude, location] = findpeaks(smooth_HRS, 'MinPeakDistance', 15);
 amplitude = amplitude(2:end - 1);
 location = location(2:end - 1);
 num_peaks = length(amplitude);
 peak_deltas = [];
 location_temp = location;
 amplitude_threshold = sum(amplitude) / length(amplitude);
 % calculate spacing between peaks (exlude small amplitude peaks)
 for i = 1:num_peaks
   if (amplitude(i) < (amplitude_threshold - 0.5)) %too noisy
     location_temp(i) = -1; % disregard this peak
   endif
   % add spacing if both current and previous peaks are valid
   if (i > 1 && location_temp(i-1) != -1 && location_temp(i) != -1)
     peak_deltas(end + 1) = location_temp(i) - location_temp(i-1);
   endif
 endfor

 % reset num_peaks
 num_delta = length(peak_deltas);
 
 % Generate weight_array
 weight_increment = 1;
 weight_array = [1:weight_increment:((weight_increment) * (num_delta - 1)) + 1];
 wad = dot(weight_array, peak_deltas) / sum(weight_array);
 
 % identify where we are on the curve
 hr = 7200 / wad;
 prev_hr_estimates(end+1) = hr;
 if (length(prev_hr_estimates) > 5)
   if (hr <= 130)
     if (prev_hr_estimates(end) - prev_hr_estimates(end-4) > 7)
       hr = hr + 15;
     endif
   endif
 endif
 
 % -------------------------------------------------------------------
 
 posInd=idx;
 MPSposInd=idx;
 dirInd=idx;

 if (idx>pos_size)
   posInd=pos_size+1;
 endif
 
 if (idx>MPSpos_size)
   MPSposInd=MPSpos_size+1;
 endif
 
 if (posInd==pos_size+1)
   pos(1,:)=circshift(pos(1,:),-1);
   pos(2,:)=circshift(pos(2,:),-1);
   pos(3,:)=circshift(pos(3,:),-1);
   dir(1,:)=circshift(dir(1,:),-1);
   posInd=pos_size;
 endif
 
 if (MPSposInd==MPSpos_size+1)
   MPSpos(1,:)=circshift(MPSpos(1,:),-1);
   MPSpos(2,:)=circshift(MPSpos(2,:),-1);
   MPSposInd = MPSpos_size;
 endif
 
 pos(1,posInd)=MPS(1,1);
 pos(2,posInd)=MPS(1,2);
 pos(3,posInd)=MPS(1,3);
 MPSpos(1,MPSposInd)=MPS(1,1);
 MPSpos(2,MPSposInd)=MPS(1,2);
 
  deg = 1;
  px = polyfit((1:1:posInd), pos(1,1:posInd), deg);
  py = polyfit((1:1:posInd), pos(2,1:posInd), deg);
  polyx = polyval(px,posInd);
  polyy = polyval(py,posInd);

  xyz(1)=polyx;
  xyz(2)=polyy;
  xyz(3)=0.5;
  
  d = sqrt((xyz(2)-MPS(2))^2 + (xyz(1)-MPS(1))^2);
  if (d>1.5)
    best_x = (d-1.5)/d * MPS(1) + 1.5/d*xyz(1);
    best_y = (d-1.5)/d * MPS(2) + 1.5/d*xyz(2);
    xyz(1)=best_x;
    xyz(2)=best_y;
  endif
 
 MPSpos(1,MPSposInd)=xyz(1);
 MPSpos(2,MPSposInd)=xyz(2);
 
 totalVel=0;
 samples=15.0;
 if ~(idx>samples&idx>16);
   vel = 10;
 else
   for (i=0:samples)
    distance = ((MPSpos(1,MPSposInd) - MPSpos(1,MPSposInd-i-1))^2 + (MPSpos(2,MPSposInd) - MPSpos(2,MPSposInd-i-1))^2)^0.5;
    distance = distance/(i+1);
    totalVel=totalVel+distance/vel_conv;
   endfor
   
   vel=totalVel/samples;
   vel= 0.2*(totalVel/samples) + 0.8*prev_vel;
 endif
 
## ---------------------------------
 delta_xyz = xyz - prev_xyz;
 di = [delta_xyz(1) delta_xyz(2)];
 total = di(1) + di(2);
 pos_est_di = di./total;
 % 2. estimate from Rg
 R = [cos(Rg) -sin(Rg); sin(Rg) cos(Rg)];
 Rg_est_di = (R*prev_di.').';
 % 3. Take weighted average
 di = weight_Rg*Rg_est_di + (1-weight_Rg)*pos_est_di;

 
 totalDir=0;
 samples=15.0;
 if ~(idx>samples&idx>16);

 else
   for (i=0:samples)
      delta_x = MPSpos(1,MPSposInd-i)-MPSpos(1,MPSposInd-i-1);
      delta_y = MPSpos(2,MPSposInd-i)-MPSpos(2,MPSposInd-i-1);
      delta_z = MPSpos(3,MPSposInd-i)-MPSpos(3,MPSposInd-i-1);
      
      di = [delta_x delta_y];
      total = di(1) + di(2);

   endfor
   
   vel=totalVel/samples;
   vel= 0.2*(totalVel/samples) + 0.8*prev_vel;
   %vel=prev_vel+0.3*(totalVel/(samples+4) - prev_vel);
   %vel = prev_vel - xyz(3) - prev_xyz(3);
 endif
 
 
 
 prev_xyz = xyz;
 prev_di=di;
 prev_vel=vel;
 xyz(3)=0.5;
 
 if (debug==1)
     figure(5);clf;plot(HRS);
     fprintf(2,'****** For this frame: *******\n');
     fprintf(2,'MPS=[%f %f %f]\n',MPS(1),MPS(2),MPS(3));
     fprintf(2,'Rate gyro=%f\n',Rg);
     fprintf(2,'---> Press [ENTER] on the Matlab/Octave terminal to continue...\n');
     drawnow;
     pause;
 end;
 
 %%% SOLUTION:   
  
 %%%%%%%%%%%%%%%%%%  DO NOT CHANGE ANY CODE BELOW THIS LINE %%%%%%%%%%%%%%%%%%%%%

 % Let's use the simulation script to plot your estimates against the real values
 % of the quantities of interest
 Sim1(map, xyz,hr,di,vel);
 idx=idx+1; 
end;

%%%%% Interesting links you may want to browse - I used these while designing this exercise.
% https://www.rohm.com/electronics-basics/sensor/pulse-sensor
% https://valencell.com/blog/optical-heart-rate-monitoring-what-you-need-to-know/
% https://www.maximintegrated.com/en/products/interface/sensor-interface/MAX30102.html#product-details
