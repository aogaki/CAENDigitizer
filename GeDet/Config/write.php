<?php
$xml = new DOMDocument("1.0", "UTF-8");

$root = $xml->createElement("root");
for ($iModule = 0; $iModule < 8; $iModule++) {
    $modules = $xml->createElement("module");
    $modules->setAttribute("FW", "PHA");
    $modules->setAttribute("ID", "Ge$iModule");

    $RecordLength = $xml->createElement("RecordLength", $_POST['RecordLength']);
    $modules->appendChild($RecordLength);

    $IOLevel = $xml->createElement("IOLevel", $_POST['IOLevel']);
    $modules->appendChild($IOLevel);


    for ($iChannel = 0; $iChannel < 16; $iChannel++) {
        $channels = $xml->createElement("channel");
        $channels->setAttribute("ID", "$iChannel");

        $Using = $xml->createElement("Using", 0);
        if ($iChannel == 0 && $iModule == 0) {
            $Using = $xml->createElement("Using", 1);
        }
        $channels->appendChild($Using);

        $DCOffset = $xml->createElement("DCOffset", $_POST["DCOffset"]);
        $channels->appendChild($DCOffset);

        $Polarity = $xml->createElement("Polarity", $_POST["Polarity"]);
        $channels->appendChild($Polarity);

        $PreTrigger = $xml->createElement("PreTrigger", $_POST["PreTrigger"]);
        $channels->appendChild($PreTrigger);

        $Decimation = $xml->createElement("Decimation", $_POST["Decimation"]);
        $channels->appendChild($Decimation);

        $TrapPoleZero = $xml->createElement("TrapPoleZero", $_POST["TrapPoleZero"]);
        $channels->appendChild($TrapPoleZero);

        $TrapFlatTop = $xml->createElement("TrapFlatTop", $_POST["TrapFlatTop"]);
        $channels->appendChild($TrapFlatTop);

        $TrapRiseTime = $xml->createElement("TrapRiseTime", $_POST["TrapRiseTime"]);
        $channels->appendChild($TrapRiseTime);

        $PeakingTime = $xml->createElement("PeakingTime", $_POST["PeakingTime"]);
        $channels->appendChild($PeakingTime);

        $TTFDelay = $xml->createElement("TTFDelay", $_POST["TTFDelay"]);
        $channels->appendChild($TTFDelay);

        $TTFSmoothing = $xml->createElement("TTFSmoothing", $_POST["TTFSmoothing"]);
        $channels->appendChild($TTFSmoothing);

        $Threshold = $xml->createElement("Threshold", $_POST["Threshold"]);
        $channels->appendChild($Threshold);

        $TrgHoldOff = $xml->createElement("TrgHoldOff", $_POST["TrgHoldOff"]);
        $channels->appendChild($TrgHoldOff);

        $EneCoarseGain = $xml->createElement("EneCoarseGain", $_POST["EneCoarseGain"]);
        $channels->appendChild($EneCoarseGain);

        $EneFineGain = $xml->createElement("EneFineGain", $_POST["EneFineGain"]);
        $channels->appendChild($EneFineGain);

        $NSPeak = $xml->createElement("NSPeak", $_POST["NSPeak"]);
        $channels->appendChild($NSPeak);

        $TrapNSBaseline = $xml->createElement("TrapNSBaseline", $_POST["TrapNSBaseline"]);
        $channels->appendChild($TrapNSBaseline);

        $FakeEventFlag = $xml->createElement("FakeEventFlag", $_POST["FakeEventFlag"]);
        $channels->appendChild($FakeEventFlag);

        $TrapCFDFraction = $xml->createElement("TrapCFDFraction", $_POST["TrapCFDFraction"]);
        $channels->appendChild($TrapCFDFraction);

        $DynamicRange = $xml->createElement("DynamicRange", $_POST["DynamicRange"]);
        $channels->appendChild($DynamicRange);

        $modules->appendChild($channels);
    }
    $root->appendChild($modules);
}
$xml->appendChild($root);

$xml->preserveWhiteSpace = false;
$xml->formatOutput = false;
$xml->save("./test.xml");
$content = $xml->saveXML();
header("Content-Type: text/xml; charset=utf-8");
echo $content;
