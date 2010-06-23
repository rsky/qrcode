<?php
extension_loaded('qr') || dl('qr.so') || exit(1);

$data = 'ＱＲコードは、（株）デンソーウェーブの登録商標です。';
$data = mb_convert_encoding($data, 'SJIS-win', 'UTF-8');

// クラス QRCode
$qr = new QRCode(array('version' => 1, 'maxnum' => 16, 'mode' => QRCode::EM_KANJI));

// データを入力
$qr->addData($data);

// 終了処理、これ以後のデータ入力は不可
// 終了処理をせずに出力しようとするとエラー
$qr->finalize();

// 出力設定は初期化時、終了処理前、終了処理後のいつでもできる
$qr->setFormat(QRCode::FMT_TIFF);
$qr->setMagnify(2);
$qr->setOrder(-1);

// 出力
var_dump($qr->getMimeType());
$qr->outputSymbol('qr.tiff');

// Iterator としてシンボルを一つずつ取得できる
$qr->setFormat(QRCode::FMT_BMP);
foreach ($qr as $pos => $symbol) {
    file_put_contents(sprintf('qr%d.bmp', $pos + 1), $symbol);
}

// 生成したQRコードをイメージリソースとして取得することもできる
// オプションの第一引数 $colors が指定された場合、カラーインデックスが代入される
// $colors[0] には描画色が、$colors[1] には背景色が、それぞれ代入される
$qr->setOrder(0);
$im = $qr->getImageResource($colors);
imagecolorset($im, $colors[0], 255, 255, 0);
imagecolorset($im, $colors[1], 255, 0, 0);
imagejpeg($im, 'qr.jpg', 50);
