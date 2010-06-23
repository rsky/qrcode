#!/usr/bin/env python

import qr

print('Content-Type: ' + qr.mimetype(qr.FMT_ASCII))

aa = qr.qrcode('spam, egg', format = qr.FMT_ASCII)
# for console
print(unicode(aa).replace(u'  ', u'\u2588').replace(u'**', u'\u3000'))
# for text
#print(unicode(aa).replace(u'  ', u'\u3000').replace(u'**', u'\u2588'))
# for HTML
#print(aa.replace('  ', '&nbsp;&nbsp;').replace('**', '&#2588;&#2588;'))
