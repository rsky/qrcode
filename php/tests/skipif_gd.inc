<?php
include_once 'skipif.inc';
if (!defined('QR_GD_ENABLED') || QR_GD_ENABLED == 0) {
    die('skip GD support is not enabled');
}
