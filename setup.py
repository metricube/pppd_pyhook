from distutils.core import setup, Extension
import sys

v = sys.version_info

module = Extension(
    'pyhook',
    sources         = ['main.c'],
    extra_link_args = ['-lpython%d.%d' % (v[0], v[1])],
)

setup (
    name        = 'pyhook',
    version     = '1.0',
    description = 'Python hooks for pppd',
    ext_modules = [module],
)
