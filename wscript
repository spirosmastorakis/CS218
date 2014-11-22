# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

# def options(opt):
#     pass

# def configure(conf):
#     conf.check_nonfatal(header_name='stdint.h', define_name='HAVE_STDINT_H')

def build(bld):
    module = bld.create_ns3_module('social-network', ['internet','core'])
    module.source = [
        'model/social-network.cc',
        'model/relationship.cc',
        'model/pkt-header.cc',
        'model/content-manager.cc',
        'model/interest-manager.cc',
        'model/community-manager.cc',
        'helper/social-network-helper.cc',
        'model/kmean_code/KCtree.cpp',
        'model/kmean_code/KCutil.cpp',
        'model/kmean_code/KM_ANN.cpp',
        'model/kmean_code/KMcenters.cpp',
        'model/kmean_code/KMdata.cpp',
        'model/kmean_code/KMeans.cpp',
        'model/kmean_code/KMfilterCenters.cpp',
        'model/kmean_code/KMlocal.cpp',
        'model/kmean_code/kmlsample.cpp',
        'model/kmean_code/KMrand.cpp',
        'model/kmean_code/KMterm.cpp',
	'model/qhs.cc'
        ]

    module_test = bld.create_ns3_module_test_library('social-network')
    module_test.source = bld.path.ant_glob ('test/*.cc')

    #header = bld(features='ns3header')
    #header.module_test = 'social-network' 
    #header.source = [
	#'test/test-qhs.h'
        #]

    headers = bld(features='ns3header')
    headers.module = 'social-network'
    headers.source = [
        'model/social-network.h',
        'model/relationship.h',
        'model/pkt-header.h',
        'model/content-manager.h',
        'model/interest-manager.h',
        'model/community-manager.h',
        'helper/social-network-helper.h',
        'model/kmean_code/KCtree.h',
        'model/kmean_code/KCutil.h',
        'model/kmean_code/KM_ANN.h',
        'model/kmean_code/KMcenters.h',
        'model/kmean_code/KMdata.h',
        'model/kmean_code/KMeans.h',
        'model/kmean_code/KMfilterCenters.h',
        'model/kmean_code/KMlocal.h',
        'model/kmean_code/KMrand.h',
        'model/kmean_code/KMterm.h',
        'model/kmean_code/kmlsample.h',
	'model/qhs.h'
        ]

    if bld.env.ENABLE_EXAMPLES:
        bld.recurse('examples')

    # bld.ns3_python_bindings()

