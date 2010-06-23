<?php
include_once 'skipif.inc';
if (!defined('QR_TIFF_ENABLED') || QR_TIFF_ENABLED == 0) {
    die('skip TIFF support is not enabled');
}
