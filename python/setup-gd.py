#!/usr/bin/env python

from distutils.core import setup, Extension
import os

def get_include_dirs():
    includedir = os.popen('gdlib-config --includedir').read().split()
    includedir.append('./libqr')
    return includedir

def get_library_dirs():
    return os.popen('gdlib-config --libdir').read().split()

module1 = Extension('qr',
        define_macros = [('QR_PYTHON_MODULE', 1),
                         ('QR_STATIC_BUILD', 1),
                         ('QR_ENABLE_TIFF', 1),
                         ('QR_ENABLE_GD', 1),
                         ('QR_GD_DISABLE_GIFANIM', 1)],
        include_dirs = get_include_dirs(),
        libraries = ['gd', 'z'],
        library_dirs = get_library_dirs(),
        sources = ['qrmodule.c', 'libqr/qr.c', 'libqr/qrcnv.c',
                   'libqr/qrcnv_bmp.c', 'libqr/qrcnv_gd.c',
                   'libqr/qrcnv_svg.c', 'libqr/qrcnv_tiff.c'])

setup(name = 'qr',
        version = '0.2.1',
        description = 'QR Code Module for Python',
        ext_modules = [module1])
