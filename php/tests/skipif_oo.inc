<?php
include_once 'skipif.inc';
if (version_compare(phpversion(), '5.0.0', '<')) {
    die('skip class QRCode is not available in PHP 4.x');
}
