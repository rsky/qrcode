#!/usr/bin/env python

import qr

f = open('spam.png', 'w')
f.write(qr.qrcode('spam, egg', format = qr.FMT_PNG, magnify = 2))
f.close()
