#!/usr/bin/env python
# -*- coding:utf-8 -*-

import qr

f = open('spam' + qr.extension(qr.JSON) , 'w')
f.write(qr.qrcode('spam, egg', format = qr.JSON, separator = 0))
f.close()
