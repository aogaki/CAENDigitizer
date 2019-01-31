<?php

class ParReader
{
    private $dom;
    private $fileName;
    public function __construct($file)
    {
        $this->fileName = $file;
        $this->dom = new DOMDocument('1.0', 'UTF-8');
        $this->dom->load($this->fileName);
        $this->GenModArray();
        $this->GenChArray();

        $this->xpath = new DOMXPath($this->dom);
    }

    public function Save($name)
    {
        $this->dom->save($name);
    }

    private $xpath;
    public function GetVal($mod, $ch, $key)
    {
        // Like a hard coding.  Think better way!!
        if ($key == "RecordLength") {
            $key = "../RecordLength";
        } elseif ($key == "IOLevel") {
            $key = "../IOLevel";
        }
        return $this->xpath->query("//module[@ID = \"$mod\"]/channel[@ID = \"$ch\"]/$key")->item(0)->nodeValue;
    }
    public function SetVal($mod, $ch, $key, $val)
    {
        // Like a hard coding.  Think better way!!
        if ($key == "RecordLength") {
            $key = "../RecordLength";
        } elseif ($key == "IOLevel") {
            $key = "../IOLevel";
        }
        $this->xpath->query("//module[@ID = \"$mod\"]/channel[@ID = \"$ch\"]/$key")->item(0)->nodeValue = $val;
    }

    private $modArray = array();
    public function GetModArray()
    {
        return $this->modArray;
    }
    private function GenModArray()
    {
        foreach ($this->dom->getElementsByTagName("module") as $node) {
            $this->modArray[] = $node->getAttribute("ID");
        }
    }

    private $chArray = array();
    public function GetChArray()
    {
        return $this->chArray;
    }
    private function GenChArray()
    {
        foreach ($this->dom->getElementsByTagName("module") as $module) {
            $modID = $module->getAttribute("ID");
            $tmpArray = array();
            foreach ($module->getElementsByTagName("channel") as $channel) {
                $chID = $channel->getAttribute("ID");
                $tmpArray[] = $chID;
            }

            $this->chArray[$modID] = $tmpArray;
        }
    }

    public function MakeRecordLength($mod, $ch)
    {
        $val = $this->GetVal($mod, $ch, "RecordLength");
        echo "Record Length:\t";
        echo "<input type=\"number\" name=\"RecordLength\" value=\"$val\" min=\"$val\" max=\"$val\"> samples";
    }

    public function MakeIOLevel($mod, $ch)
    {
        $val = $this->GetVal($mod, $ch, "IOLevel");
        echo "IO Level:\t";
        if ($val == "TTL") {
            echo "<input type=\"radio\" name=\"IOLevel\" value=\"NIM\">NIM";
            echo "<input type=\"radio\" name=\"IOLevel\" value=\"TTL\" checked=\"checked\">TTL";
        } else {
            echo "<input type=\"radio\" name=\"IOLevel\" value=\"NIM\" checked=\"checked\">NIM";
            echo "<input type=\"radio\" name=\"IOLevel\" value=\"TTL\">TTL";
        }
    }

    public function MakeUsing($mod, $ch)
    {
        $val = $this->GetVal($mod, $ch, "Using");
        echo "Channel:\t";
        if ($val == "0") {
            echo "<input type=\"radio\" name=\"Using\" value=\"1\">Enable";
            echo "<input type=\"radio\" name=\"Using\" value=\"0\" checked=\"checked\">Disable";
        } else {
            echo "<input type=\"radio\" name=\"Using\" value=\"1\" checked=\"checked\">Enable";
            echo "<input type=\"radio\" name=\"Using\" value=\"0\">Disable";
        }
    }

    public function MakeDCOffset($mod, $ch)
    {
        $val = $this->GetVal($mod, $ch, "DCOffset");
        echo "DC Offset:\t";
        echo "<input type=\"number\" name=\"DCOffset\" value=\"$val\" min=\"0\" max=\"100\"> %";
    }

    public function MakePolarity($mod, $ch)
    {
        $val = $this->GetVal($mod, $ch, "Polarity");
        echo "Polarity:\t";
        if ($val == "Negative") {
            echo "<input type=\"radio\" name=\"Polarity\" value=\"Positive\">Positive";
            echo "<input type=\"radio\" name=\"Polarity\" value=\"Negative\" checked=\"checked\">Negative";
        } else {
            echo "<input type=\"radio\" name=\"Polarity\" value=\"Positive\" checked=\"checked\">Positive";
            echo "<input type=\"radio\" name=\"Polarity\" value=\"Negative\">Negative";
        }
    }

    public function MakePreTrigger($mod, $ch)
    {
        $val = $this->GetVal($mod, $ch, "PreTrigger");
        echo "Pre-Trigger Size:\t";
        echo "<input type=\"number\" name=\"PreTrigger\" value=\"$val\" min=\"0\" max=\"1024\"> ns";
    }

    public function MakeDecimation($mod, $ch)
    {
        $val = $this->GetVal($mod, $ch, "Decimation");
        $nChoice = 4; // Check how to use constant
        $choices = array("Disabled", "2 samples (50 MS/s)", "4 samples (25 MS/s)", "8 samples (12.5 MS/s)");

        echo "Decimation:\t";
        echo "<select name=\"Decimation\">";
        for ($i = 0; $i < $nChoice; $i++) {
            $startTag = "<option value=\"$i\">";
            if ($i == $val) {
                $startTag = "<option value=\"$i\" selected>";
            }
            echo $startTag.$choices[$i]."</option>";
        }
        echo "</select>";
    }

    public function MakeTrapPoleZero($mod, $ch)
    {
        $val = $this->GetVal($mod, $ch, "TrapPoleZero");
        echo "Trapezoid Pole Zero:\t";
        echo "<input type=\"number\" name=\"TrapPoleZero\" value=\"$val\" min=\"0\" max=\"650000\"> ns";
    }

    public function MakeTrapFlatTop($mod, $ch)
    {
        $val = $this->GetVal($mod, $ch, "TrapFlatTop");
        echo "Trapezoid Flat Top:\t";
        echo "<input type=\"number\" name=\"TrapFlatTop\" value=\"$val\" min=\"0\" max=\"650000\"> ns";
    }

    public function MakeTrapRiseTime($mod, $ch)
    {
        $val = $this->GetVal($mod, $ch, "TrapRiseTime");
        echo "Trapezoid Rise Time:\t";
        echo "<input type=\"number\" name=\"TrapRiseTime\" value=\"$val\" min=\"0\" max=\"650000\"> ns";
    }

    public function MakePeakingTime($mod, $ch)
    {
        $val = $this->GetVal($mod, $ch, "PeakingTime");
        echo "Peaking Time:\t";
        echo "<input type=\"number\" name=\"PeakingTime\" value=\"$val\" min=\"0\" max=\"650000\"> ns (This should be less than 80% of Flat top.)";
    }

    public function MakeTTFDelay($mod, $ch)
    {
        $val = $this->GetVal($mod, $ch, "TTFDelay");
        echo "TTF Delay (Same as input rise time):\t";
        echo "<input type=\"number\" name=\"TTFDelay\" value=\"$val\" min=\"0\" max=\"650000\"> ns";
    }

    public function MakeTTFSmoothing($mod, $ch)
    {
        $val = $this->GetVal($mod, $ch, "TTFSmoothing");
        $nChoice = 5; // Check how to use constant
        $choices = array("Disabled", "2 samples", "4 samples", "8 samples", "16 samples");

        echo "TTFSmoothing:\t";
        echo "<select name=\"TTFSmoothing\">";
        for ($i = 0; $i < $nChoice; $i++) {
            $startTag = "<option value=\"$i\">";
            if ($i == $val) {
                $startTag = "<option value=\"$i\" selected>";
            }
            echo $startTag.$choices[$i]."</option>";
        }
        echo "</select>";
    }

    public function MakeThreshold($mod, $ch)
    {
        $val = $this->GetVal($mod, $ch, "Threshold");
        echo "Threshold:\t";
        echo "<input type=\"number\" name=\"Threshold\" value=\"$val\" min=\"0\" max=\"650000\"> ADC channel";
    }

    public function MakeTrgHoldOff($mod, $ch)
    {
        $val = $this->GetVal($mod, $ch, "TrgHoldOff");
        echo "Trigger Hold Off:\t";
        echo "<input type=\"number\" name=\"TrgHoldOff\" value=\"$val\" min=\"0\" max=\"650000\"> ns";
    }

    public function MakeEneCoarseGain($mod, $ch)
    {
        $val = $this->GetVal($mod, $ch, "EneCoarseGain");
        echo "Energy Coarse Gain:\t";
        echo "<input type=\"number\" name=\"EneCoarseGain\" value=\"$val\" min=\"0.0\" max=\"10.0\" step=\"0.1\">";
    }

    public function MakeEneFineGain($mod, $ch)
    {
        $val = $this->GetVal($mod, $ch, "EneFineGain");
        echo "Energy Fine Gain:\t";
        echo "<input type=\"number\" name=\"EneFineGain\" value=\"$val\" min=\"0.0\" max=\"10.0\" step=\"0.1\">";
    }

    public function MakeNSPeak($mod, $ch)
    {
        $val = $this->GetVal($mod, $ch, "NSPeak");
        $nChoice = 4; // Check how to use constant
        $choices = array("1 samples", "4 samples", "16 samples", "64 samples");

        echo "NS Peak:\t";
        echo "<select name=\"NSPeak\">";
        for ($i = 0; $i < $nChoice; $i++) {
            $startTag = "<option value=\"$i\">";
            if ($i == $val) {
                $startTag = "<option value=\"$i\" selected>";
            }
            echo $startTag.$choices[$i]."</option>";
        }
        echo "</select>";
    }

    public function MakeTrapNSBaseline($mod, $ch)
    {
        $val = $this->GetVal($mod, $ch, "TrapNSBaseline");
        $nChoice = 7; // Check how to use constant
        $choices = array("0 samples", "16 samples", "64 samples", "256 samples", "1024 samples", "4096 samples", "16384 samples");

        echo "Trapezoid NS Baseline:\t";
        echo "<select name=\"TrapNSBaseline\">";
        for ($i = 0; $i < $nChoice; $i++) {
            $startTag = "<option value=\"$i\">";
            if ($i == $val) {
                $startTag = "<option value=\"$i\" selected>";
            }
            echo $startTag.$choices[$i]."</option>";
        }
        echo "</select>";
    }

    public function MakeFakeEventFlag($mod, $ch)
    {
        $val = $this->GetVal($mod, $ch, "FakeEventFlag");
        echo "Fake Event Flag:\t";
        if ($val == 1) {
            echo "<input type=\"radio\" name=\"FakeEventFlag\" value=\"0\">Disable";
            echo "<input type=\"radio\" name=\"FakeEventFlag\" value=\"1\" checked=\"checked\">Enable";
        } else {
            echo "<input type=\"radio\" name=\"FakeEventFlag\" value=\"0\" checked=\"checked\">Disable";
            echo "<input type=\"radio\" name=\"FakeEventFlag\" value=\"1\">Enable";
        }
    }

    public function MakeTrapCFDFraction($mod, $ch)
    {
        $val = $this->GetVal($mod, $ch, "TrapCFDFraction");
        $nChoice = 4; // Check how to use constant
        $choices = array("25 %", "50 %", "75 %", "100 %");

        echo "Trapezoid CFD Fraction:\t";
        echo "<select name=\"TrapCFDFraction\">";
        for ($i = 0; $i < $nChoice; $i++) {
            $startTag = "<option value=\"$i\">";
            if ($i == $val) {
                $startTag = "<option value=\"$i\" selected>";
            }
            echo $startTag.$choices[$i]."</option>";
        }
        echo "</select>";
    }

    public function MakeDynamicRange($mod, $ch)
    {
        $val = $this->GetVal($mod, $ch, "DynamicRange");
        echo "Dynamic Range:\t";
        if ($val == "1") {
            echo "<input type=\"radio\" name=\"DynamicRange\" value=\"0\">2 Vpp";
            echo "<input type=\"radio\" name=\"DynamicRange\" value=\"1\" checked=\"checked\">0.5 Vpp";
        } else {
            echo "<input type=\"radio\" name=\"DynamicRange\" value=\"0\" checked=\"checked\">2 Vpp";
            echo "<input type=\"radio\" name=\"DynamicRange\" value=\"1\">0.5 Vpp";
        }
    }
}
