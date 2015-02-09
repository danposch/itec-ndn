itec-ndn

Install Guide (testet on Ubuntu 14.04 64bit)

# Dependencies:
    * NS3
    * ndnSim
    * Boost C++
		* libcurl
		* cmake
		* BRITE

# Building:

    * mkdir ndnSIM
    * cd ndnSIM

    # Building Brite
		* hg clone http://code.nsnam.org/BRITE
		* cd BRITE
		* make
		* sudo cp libbrite.so /usr/lib/
		

    # fetch NS-3 + ndnSIM
    * sudo apt-get install python-dev python-pygraphviz python-kiwi
    * sudo apt-get install python-pygoocanvas python-gnome2
    * sudo apt-get install python-rsvg ipython

    * git clone https://github.com/named-data/ndn-cxx.git ndn-cxx
    * git clone https://github.com/cawka/ns-3-dev-ndnSIM.git ns-3
    * git clone https://github.com/cawka/pybindgen.git pybindgen
    * git clone https://github.com/named-data/ndnSIM.git ns-3/src/ndnSIM

    * cd ndn-cxx
    * ./waf configure
    * ./waf
    * sudo ./waf install
    * cd ../

    # Build and install NS-3 and ndnSIM
    * cd ns-3
    * ./waf configure -d optimized --with-brite=../BRITE
    * ./waf
    * sudo ./waf install
    * cd ./build
    * sudo cp ./libns3-dev-brite-optimized.so /usr/local/lib/
    * cd ../..

		# Copy BRITE includes manually
		* cd BRITE
		* sudo cp *.h /usr/local/include/ns3-dev/ns3
		* sudo mkdir /usr/local/include/ns3-dev/ns3/Models
		* cd Models/
		* sudo cp *.h /usr/local/include/ns3-dev/ns3/Models
		* cd ../..

    # Build itec-ndn
    * git clone https://github.com/danposch/itec-ndn.git
    * cd itec-ndn
    * ./libdash.sh
    * sudo cp ./libdash/libdash/build/bin/libdash.so  /usr/local/lib/
    * ./waf configure
    * ./waf 
    * ./waf --run example --vis

==============
