#!/usr/bin/env python
# -*- coding:utf-8 -*-

import qr, cStringIO
from PIL import Image

buf = cStringIO.StringIO()
qr.qrcode('spam, egg', buf, format=qr.BMP, magnify=2)
buf.seek(0)
Image.open(buf).save('spam.png', 'PNG')
