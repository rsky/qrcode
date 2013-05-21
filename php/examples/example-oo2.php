<?php
// デフォルト値を設定
ini_set('qr.default_format', QR_FMT_PNG);
ini_set('qr.default_magnify', 3);

$qr = new QRCode();
$qr->addData('foo');

// オブジェクトを複製し、途中から異なる内容のQRコードを作成できる
$qr2 = clone $qr;

$qr->addData('bar');
$qr->finalize();
$qr->outputSymbol('foobar.png');

$qr2->addData('baz');
$qr2->finalize();
$qr2->outputSymbol('foobaz.png');
