<?php
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
