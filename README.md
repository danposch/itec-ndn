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

    # Fetch and ndn-cxx, ndnsim and itec-ndn scenarios
    * git clone https://github.com/named-data/ndn-cxx.git ndn-cxx
		* git clone https://github.com/cawka/ns-3-dev-ndnSIM.git ns-3
    * git clone https://github.com/cawka/pybindgen.git pybindgen
		* git clone https://github.com/ChristianKreuzberger/amus-ndnSIM.git ns-3/src/ndnSIM
		* git clone https://github.com/danposch/itec-ndn.git

		# (RECOMMENDED) checkout verified compatible versions of ndn-cxx and ndnsim
    * cd ndn-cxx
		* git checkout 9bd4d9832ac16021ba956bb3a0bfb16199572e32
		* cd ..
    * cd ns-3/src/ndnSIM/
		* git checkout 86a881d9898df74fa4cfd8e85684a3ae81ab02e6
		* cd ../../../
		
		# Patch forwarder to enable nacks in NFD; patch content store, patch strategy to enable ompif;
		* cp itec-ndn/extern/forwarder.cpp ns-3/src/ndnSIM/NFD/daemon/fw/forwarder.cpp
		* cp itec-ndn/extern/forwarder.hpp ns-3/src/ndnSIM/NFD/daemon/fw/forwarder.hpp
		* cp itec-ndn/extern/ndn-content-store.hpp ns-3/src/ndnSIM/model/cs/ndn-content-store.hpp
		* cp itec-ndn/extern/content-store-impl.hpp ns-3/src/ndnSIM/model/cs/content-store-impl.hpp
		* cp itec-ndn/extern/content-store-nocache.hpp ns-3/src/ndnSIM/model/cs/content-store-nocache.hpp
		* cp itec-ndn/extern/content-store-nocache.cpp ns-3/src/ndnSIM/model/cs/content-store-nocache.cpp
		* cp itec-ndn/extern/strategy.cpp ns-3/src/ndnSIM/NFD/daemon/fw/strategy.cpp
		* cp itec-ndn/extern/strategy.hpp ns-3/src/ndnSIM/NFD/daemon/fw/strategy.hpp

		# Build ndn-cxx
		* cd ndn-cxx
    * ./waf configure
    * ./waf
    * sudo ./waf install
    * cd ../

    # Build NS-3 + (amus-)ndnSIM2.x
    * cd ns-3.
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

    # Build itec-ndn scnearios
    * cd itec-ndn
    * ./waf configure
    * ./waf 
		* cd../

==============
