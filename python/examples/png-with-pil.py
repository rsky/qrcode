#!/usr/bin/env python
# -*- coding:utf-8 -*-

import qr, cStringIO
from PIL import Image

buf = cStringIO.StringIO()
buf.write(qr.qrcode('spam, egg', format=qr.BMP, magnify=2))
buf.seek(0)
Image.open(buf).save('spam.png', 'PNG')
