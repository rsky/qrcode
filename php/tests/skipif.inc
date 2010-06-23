<?php
if (!extension_loaded('qr') && ini_get('enable_dl')) {
    $dlext = !strncasecmp(PHP_OS, 'WIN', 3) ? '.dll' : '.so';
    @dl("qr$dlext");
}
if (!extension_loaded('qr')) {
    die('skip qr extension is not loaded');
}
