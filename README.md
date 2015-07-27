itec-ndn

Install Guide (testet on Ubuntu 14.04 64bit)

# Dependencies:
    * NS3
    * (amus)-ndnSim2.x
    * Boost C++
		* libcurl
		* cmake
		* BRITE
		* libdash

# Install pre-compiled libs:
		* sudo apt-get install python-dev python-pygraphviz python-kiwi
		* sudo apt-get install python-pygoocanvas python-gnome2
		* sudo apt-get install python-rsvg ipython
		* sudo apt-get install libsqlite3-dev libcrypto++-dev
		* sudo apt-get install libboost-all-dev
		* sudo apt-get install git-core cmake libxml2-dev libcurl4-openssl-dev
		* sudo apt-get install mercurial

# Building:

    * mkdir ndnSIM
    * cd ndnSIM

    # Fetch and build Brite
		* hg clone http://code.nsnam.org/BRITE
		* cd BRITE
		* make
		* sudo cp libbrite.so /usr/lib/
		* cd ..

		# Fetch and build libdash
		* git clone https://github.com/bitmovin/libdash.git
		* cd libdash/libdash
		* mkdir build
		* cd build
		* cmake ../
		* make dash # only build dash, no need for network stuff
		* cd ../../../
		* sudo cp ./libdash/libdash/build/bin/libdash.so  /usr/local/lib/
		*	sudo mkdir /usr/local/include/libdash
		* sudo cp -r ./libdash/libdash/libdash/include/* /usr/local/include/libdash/

    # Fetch and build ndn-cxx
    * git clone https://github.com/named-data/ndn-cxx.git ndn-cxx
    * cd ndn-cxx
    * ./waf configure
    * ./waf
    * sudo ./waf install
    * cd ../

    # Fetch and build NS-3 + (amus-)ndnSIM2.x
    * git clone https://github.com/cawka/ns-3-dev-ndnSIM.git ns-3
    * git clone https://github.com/cawka/pybindgen.git pybindgen
		* git clone https://github.com/ChristianKreuzberger/amus-ndnSIM.git ns-3/src/ndnSIM
    * cd ns-3
    * ./waf configure -d optimized --with-brite=../BRITE
    * ./waf
    * sudo ./waf install
    * cd ./build
    * sudo cp ./libns3-dev-brite-optimized.so /usr/local/lib/
    * cd ../..

		# Copy BRITE includes manually to your include folder
		* cd BRITE
		* sudo cp *.h /usr/local/include/ns3-dev/ns3
		* sudo mkdir /usr/local/include/ns3-dev/ns3/Models
		* cd Models/
		* sudo cp *.h /usr/local/include/ns3-dev/ns3/Models
		* cd ../..

    # Fetch and build itec-ndn scnearios
    * git clone https://github.com/danposch/itec-ndn.git
    * cd itec-ndn
    * ./waf configure
    * ./waf 
		* cd../

		# Ok now install custom forwarder to enable nacks in NFD
		* cp itec-ndn/extern/forwarder.cpp ns-3/src/ndnSIM/NFD/daemon/fw/forwarder.cpp
		* cp itec-ndn/extern/forwarder.hpp ns-3/src/ndnSIM/NFD/daemon/fw/forwarder.hpp
		* cd ns-3/
		* sudo ./waf install

==============
