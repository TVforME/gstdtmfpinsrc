project('gstdtmfpinsrc', 'c',
  version : '1.0.0',
  meson_version : '>= 0.56',
  default_options : ['warning_level=1',
                    'c_std=c99',
                    'buildtype=debugoptimized']
)

# Get GStreamer dependencies
gst_version = meson.project_version()
gst_req = '>= 1.20.0'

gstreamer_dep = dependency('gstreamer-1.0', version : gst_req, required : true)
gstbase_dep = dependency('gstreamer-base-1.0', version : gst_req, required : true)
gstaudio_dep = dependency('gstreamer-audio-1.0', version : gst_req, required : true)
spandsp_dep = dependency('spandsp', version : '>= 0.0.6', required : true)

# Get GStreamer plugin directory
plugins_install_dir = get_option('libdir') / 'gstreamer-1.0'

# Plugin sources (standalone - only dtmfpinsrc)
dtmfpinsrc_sources = [
  'gstdtmfpinsrc.c',
  'gstdtmfpinsrc.h',
]

# Build the plugin
gst_dtmfpinsrc = library('gstdtmfpinsrc',
  dtmfpinsrc_sources,
  c_args : [
    '-DHAVE_CONFIG_H',
    '-DGST_USE_UNSTABLE_API',
  ],
  include_directories : include_directories('src'),
  dependencies : [
    gstreamer_dep,
    gstbase_dep,
    gstaudio_dep,
    spandsp_dep,
  ],
  install : true,
  install_dir : plugins_install_dir,
)

# Generate pkg-config file
pkgconfig = import('pkgconfig')
pkgconfig.generate(gst_dtmfpinsrc,
  description : 'GStreamer DTMF PIN detection plugin',
  subdirs : 'gstreamer-1.0',
)

# Install sample configuration file
install_data('codes.pin',
  install_dir : get_option('datadir') / 'gstdtmfpinsrc',
)

# Summary
summary({
  'Version': meson.project_version(),
  'Plugin': 'gstdtmfpinsrc',
  'Install directory': plugins_install_dir,
  'GStreamer version': gst_req,
  'spandsp version': '>= 0.0.6',
}, section: 'Plugin Information')
