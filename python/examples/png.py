#!/usr/bin/env python
# -*- coding:utf-8 -*-

import qr

f = open('spam.png', 'w')
f.write(qr.qrcode('spam, egg', format = qr.FMT_PNG, scale = 8))
f.close()
