#!/usr/bin/env python
# -*- coding:utf-8 -*-

import qr

f = open('spam' + qr.extension(qr.SVG) , 'w')
f.write(qr.qrcode('spam, egg', format = qr.SVG, separator = 0))
f.close()
