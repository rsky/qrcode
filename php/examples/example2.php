<?php
extension_loaded('qr') || dl('qr.so') || exit(1);

$data = 'ＱＲコードをイメージリソースとして取得できます';
$data = mb_convert_encoding($data, 'SJIS-win', 'UTF-8');
$options = array(
    'magnify' => 2,
    'eclevel' => QR_ECL_H,
);
$im = qr_image_resource($data, $options, $colors);

// 描画色 (暗モジュール色) と 背景色 (明モジュール色) を変更
imagecolorset($im, $colors[0], 64, 32, 16);
imagecolorset($im, $colors[1], 255, 255, 216);

// テキスト設定
$text = 'Sample';
$font = 5;
$x = 16;
$y = imagesy($im) / 2 - 8;
$tcolor = imagecolorallocate($im, 0, 0, 255);
$bcolor = imagecolorallocate($im, 255, 255, 255);

// 縁取りを描画
for ($xd = -1; $xd <= 1; $xd++) {
    for ($yd = -1; $yd <= 1; $yd++) {
        imagestring($im, $font, $x + $xd, $y + $yd, $text, $bcolor);
    }
}

// テキストを描画
imagestring($im, $font, $x, $y, $text, $tcolor);

// PNGとして出力
//header('Content-Type: ' . image_type_to_mime_type(IMAGETYPE_PNG));
//imagepng($im);
imagepng($im, 'sample.png');
