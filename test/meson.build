project('dtmfpinsrc_test', 'c',
    version : '1.0.0',
    meson_version : '>= 0.56',
    default_options : ['warning_level=2',
                      'c_std=c99',
                      'buildtype=debugoptimized']
)

# Get GStreamer dependencies
gstreamer_dep = dependency('gstreamer-1.0', version : '>= 1.20.0', required : true)

# Test program sources
test_sources = [
    'test_dtmfpinsrc.c',
]

# Build the test program
test_dtmfpinsrc = executable('test_dtmfpinsrc',
    test_sources,
    c_args : [
        '-DHAVE_CONFIG_H',
    ],
    dependencies : [
        gstreamer_dep,
    ],
    install : false,
    build_by_default : true,
)

# Summary
summary({
    'Test Program': test_dtmfpinsrc.name(),
    'GStreamer version': '>= 1.20.0',
}, section: 'Test Information')
