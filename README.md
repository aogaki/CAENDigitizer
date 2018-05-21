# CAENDigitizer and DAQ
CAEN digitizer handling classes and DAQ test system

How to install all necessary libraries.  
1. Install CAEN digitizer libraries and drivers.  
  
2. Install ROOT  
In the case of handling CAEN digitizer, till here is enough.  
  
3. Install DAQ-middleware  
For Scientific Linux 6 or 7  
Download rpm files from http://daqmw.kek.jp/rpm/  
or  
Download a script file from http://daqmw.kek.jp/src/daqmw-rpm  
**sh daqmw-rpm install** as root  
That,s all  

For Ubuntu 16.04  
Install omniORB  
**sudo apt install omniorb omniidl libomniorb4-dev omniorb-\***  
  
and stop nameserver service  
**sudo systemctl disable omniorb4-nameserver.service**
  
Install OpenRTM-aist  
Now there WEB page is beeing moved and modified.  Check http://openrtm.org/
  
Install xerces-c  
**sudo apt install libxerces-c\***  
  
Install xalan-c  
**sudo apt install libxalan-c\***  
  
Install boost  
**sudo apt install libboost-all\***  
  
Install swig  
**sudo apt install swig**  
  
Install UUID  
**sudo apt install uuid-dev**  
  
Install XML utilities  
**sudo apt install libxml2-utils**  
  
Install DAQ-middleware  
Download file from http://daqmw.kek.jp/src/  
Now (1 / 18 / 2018), the last version is http://daqmw.kek.jp/src/DAQ-Middleware-1.4.2.tar.gz  
I found some problems with gcc version 7.  It comes from conflicting the boost::function and std::function(C++11).  The solution is simple.  In the file src/lib/json_spirit_v2.06/json_spirit/json_spirit_reader.cpp, replace "typedef function" with "typedef boost::function".  Need patch?  DIY!    
  
Now you can use DAQ system without networking.  
Let's go to the network configuration.  
Simply, and wish to increase the network speed, I don't use any firewall.  In the case of use it, please open some ports described in the manual of DAQ-middleware.  
  
Install xinetd  
**sudo yum install xinetd** or **sudo apt install xinetd**  
And startup the xinetd service. You don't know how to do it? I recomend to know it, because you should manage your PC.  
Adding the line described in /usr/share/daqmw/etc/remote-boot/services.sample to /etc/services  
And **cp /usr/share/daqmw/etc/remote-boot/bootComps-xinetd.sample /etc/xinetd.d/bootComps** as root user.  
In the bootCamps, there is a user name definition. Change it to fit for your system.  
