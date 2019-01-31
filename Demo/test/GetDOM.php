<?php
function GetDOM($file)
{
    $dom = new DOMDocument('1.0', 'UTF-8');
    // $dom->preserveWhiteSpace = false;
    $dom->formatOutput = false;
    $dom->load($file);
    $xml = $dom->saveXML();
    $xml = str_replace(PHP_EOL, "", $xml);

    return $xml;
}
