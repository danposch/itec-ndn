# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

VERSION='0.1'
APPNAME='template'

from waflib import Build, Logs, Options, TaskGen
import subprocess

def options(opt):
    opt.add_option('--debug',action='store_true',default=False,dest='debug',help='''debugging mode''')
    opt.add_option('--logging',action='store_true',default=True,dest='logging',help='''enable logging in simulation scripts''')
    opt.add_option('--run',
                   help=('Run a locally built program; argument can be a program name,'
                         ' or a command starting with the program name.'),
                   type="string", default='', dest='run')
    opt.add_option('--visualize',
                   help=('Modify --run arguments to enable the visualizer'),
                   action="store_true", default=False, dest='visualize')
    opt.add_option('--mpi',
                   help=('Run in MPI mode'),
                   type="string", default="", dest="mpi")
    opt.add_option('--time',
                   help=('Enable time for the executed command'),
                   action="store_true", default=False, dest='time')

    opt.load("compiler_c compiler_cxx boost ns3")

MANDATORY_NS3_MODULES = ['core', 'network', 'point-to-point', 'applications', 'mobility', 'ndnSIM']
OTHER_NS3_MODULES = ['antenna', 'aodv', 'bridge', 'brite', 'buildings', 'click', 'config-store', 'csma', 'csma-layout', 'dsdv', 'dsr', 'emu', 'energy', 'fd-net-device', 'flow-monitor', 'internet', 'lte', 'mesh', 'mpi', 'netanim', 'nix-vector-routing', 'olsr', 'openflow', 'point-to-point-layout', 'propagation', 'spectrum', 'stats', 'tap-bridge', 'topology-read', 'uan', 'virtual-net-device', 'visualizer', 'wifi', 'wimax']

def configure(conf):
    conf.load("compiler_cxx boost ns3")
    
    #added by dposch
    #import os
    #conf.env['CPPPATH_LIBDAI'] = [os.path.abspath(os.path.join(conf.env['WITH_LIBDAI'],'include'))]
    #conf.env['LIBPATH_LIBDAI'] = [os.path.abspath(os.path.join(conf.env['WITH_LIBDAI'],'lib'))]
    #conf.env.append_value('CPPPATH', conf.env['CPPPATH_LIBDAI'])
    #conf.env.append_value('LIBPATH', conf.env['LIBPATH_LIBDAI'])

    #for mcore24 build env 
    conf.env.LIBPATH = ['/local/users/ndnsim/lib/']
    conf.env.INCLUDES = ['/local/users/ndnsim/include/' '/local/users/ndnsim/include/ns3-dev']

    conf.check(lib='dash', uselib="DASH", define_name='HAVE_DASH')
    conf.check(lib='brite', uselib="BRITE", define_name='HAVE_BRITE')
    #conf.check(lib='ns3-dev-brite-optimized', uselib="BRITE", define_name='HAVE_NS3_BRITE')
    conf.env.append_value('/usr/local/include/libdash/', ['include'])

    #for mcore24 build env    
   # conf.env.append_value('~include/libdash/', ['include'])
   # conf.env.append_value('~include/', ['include'])
   # conf.env.append_value('~include/ns3-dev/', ['include'])
   # conf.env.append_value('~include/ns3-dev/ns3/', ['include'])

    conf.check_boost(lib='system iostreams')
    boost_version = conf.env.BOOST_VERSION.split('_')
    if int(boost_version[0]) < 1 or int(boost_version[1]) < 48:
        Logs.error ("ndnSIM requires at least boost version 1.48")
        Logs.error ("Please upgrade your distribution or install custom boost libraries (http://ndnsim.net/faq.html#boost-libraries)")
        exit (1)

    try:
        conf.check_ns3_modules(MANDATORY_NS3_MODULES)
        for module in OTHER_NS3_MODULES:
            conf.check_ns3_modules(module, mandatory = False)
    except:
        Logs.error ("NS-3 or one of the required NS-3 modules not found")
        Logs.error ("NS-3 needs to be compiled and installed somewhere.  You may need also to set PKG_CONFIG_PATH variable in order for configure find installed NS-3.")
        Logs.error ("For example:")
        Logs.error ("    PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:$PKG_CONFIG_PATH ./waf configure")
        conf.fatal ("")

    if conf.options.debug:
        conf.define ('NS3_LOG_ENABLE', 1)
        conf.define ('NS3_ASSERT_ENABLE', 1)
        conf.define ('_DEBUG', 1)
        conf.env.append_value('CXXFLAGS', ['-O0', '-g3', '-std=c++11'])
    else:
        conf.env.append_value('CXXFLAGS', ['-O3', '-g', '-std=c++11'])

    if conf.env["CXX"] == ["clang++"]:
        conf.env.append_value('CXXFLAGS', ['-fcolor-diagnostics'])

    if conf.env.DEST_BINFMT == 'elf':
        # All ELF platforms are impacted but only the gcc compiler has a flag to fix it.
        if 'gcc' in (conf.env.CXX_NAME, conf.env.CC_NAME):
            conf.env.append_value ('SHLIB_MARKER', '-Wl,--no-as-needed')

    if conf.options.logging:
        conf.define ('NS3_LOG_ENABLE', 1)
        conf.define ('NS3_ASSERT_ENABLE', 1)

def build (bld):
    deps = 'BOOST BOOST_IOSTREAMS ' + ' '.join (['ns3_'+dep for dep in MANDATORY_NS3_MODULES + OTHER_NS3_MODULES]).upper ()

    common = bld.objects (
        target = "extensions",
        features = ["cxx"],
        source = bld.path.ant_glob(['extensions/**/*.cc']),
        use = deps,
        cxxflags = [bld.env.CXX11_CMD],
        includes = ' libdash/libdash/qtsampleplayer/libdashframework/ /usr/local/include/libdash/ /usr/local/include/ns3-dev/ns3/ndnSIM/NFD/ /usr/local/include/ns3-dev/ns3/ndnSIM/NFD/daemon/ /usr/local/include/ns3-dev/ns3/ndnSIM/NFD/core/ /local/users/ndnsim/include/ /local/users/ndnsim/include/ns3-dev/'
        )

    #framework = bld.objects (
        #target = "libdashframework",
        #features = ["cxx"],
        #source = bld.path.ant_glob(['libdash/libdash/qtsampleplayer/libdashframework/Adaptation/*.cpp']),
        #use = deps,
        #cxxflags = [bld.env.CXX11_CMD],
        #includes = ' /usr/local/include/libdash/ '
        #)

    for scenario in bld.path.ant_glob (['scenarios/*.cc']):
        name = str(scenario)[:-len(".cc")]
        app = bld.program (
            target = name,
            features = ['cxx'],
            source = [scenario],        #added by dposch
            use = deps + " extensions DASH BRITE",
            includes = " extensions libdash/libdash/qtsampleplayer/libdashframework/ /usr/local/include/libdash/ /usr/local/include/ns3-dev/ns3/ndnSIM/NFD/ /usr/local/include/ns3-dev/ns3/ndnSIM/NFD/daemon/ /usr/local/include/ns3-dev/ns3/ndnSIM/NFD/core/ /local/users/ndnsim/include/ /local/users/ndnsim/include/ns3-dev/"
            )

def shutdown (ctx):
    if Options.options.run:
        visualize=Options.options.visualize
        mpi = Options.options.mpi

        if mpi and visualize:
            Logs.error ("You cannot specify --mpi and --visualize options at the same time!!!")
            return

        argv = Options.options.run.split (' ');
        argv[0] = "build/%s" % argv[0]

        if visualize:
            argv.append ("--SimulatorImplementationType=ns3::VisualSimulatorImpl")

        if mpi:
            argv.append ("--SimulatorImplementationType=ns3::DistributedSimulatorImpl")
            argv.append ("--mpi=1")
            argv = ["openmpirun", "-np", mpi] + argv
            Logs.error (argv)

        if Options.options.time:
            argv = ["time"] + argv

        return subprocess.call (argv)
