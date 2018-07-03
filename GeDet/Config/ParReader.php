<?php

class ParReader
{
    private $dom;
    public function __construct($fileName)
    {
        $this->dom = new DOMDocument('1.0', 'UTF-8');
        $this->dom->load($fileName);
        $this->GenModArray();
        $this->GenChArray();

        $this->xpath = new DOMXPath($this->dom);
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
}
