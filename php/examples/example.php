<?php
$data = 'ＱＲコードは、（株）デンソーウェーブの登録商標です。';
$data = mb_convert_encoding($data, 'SJIS-win', 'UTF-8');

// 関数 qr_output_symbol
$options = array(
    'mode' => QR_EM_KANJI,
    'format' => QR_FMT_PNG,
    'magnify' => 2,
);
var_dump(qr_mimetype(QR_FMT_PNG));
qr_output_symbol('qr.png', $data, $options);
