#!/usr/bin/env python
# -*- coding:utf-8 -*-

from __future__ import print_function
import qr

q = qr.QRCode()
q.add_data('spam, egg')
q.format = qr.IMAGE
q.scale = 2

image = q.get_symbol()
print(type(image), image.__class__.__name__)
image.save('spamegg.gif', 'GIF')
