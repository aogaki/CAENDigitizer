<!DOCTYPE html>
<html>

<head>
  <meta charset="utf-8">
  <title>test</title>
</head>

<?php
include("./ParReader.php");
$reader = new ParReader("/Data/DAQ/Parameters/parameters.xml");
$modArray = $reader->GetModArray();
$chArray = $reader->GetChArray();

$parNames = array("RecordLength", "IOLevel", "Using", "DCOffset", "Polarity", "PreTrigger", "Decimation", "TrapPoleZero", "TrapFlatTop", "TrapRiseTime",
"PeakingTime", "TTFDelay", "TTFSmoothing", "Threshold", "TrgHoldOff", "EneCoarseGain", "EneFineGain", "NSPeak", "TrapNSBaseline", "FakeEventFlag",
"TrapCFDFraction", "DynamicRange");

function MakeChoices($array, $selected)
{
    foreach ($array as $val) {
        if ($val == $selected) {
            echo "<option value=\"$val\" selected>$val</option>";
        } else {
            echo "<option value=\"$val\">$val</option>";
        }
    }
}
?>

<body>
<form action="UI.php" name="ChSelect" method="post">
<p>
  Module:
  <select name="ModuleID">
    <?php
    MakeChoices($modArray, $_POST['ModuleID']);
    ?>
  </select>
</p>
<?php
if (isset($_POST['ModuleID'])) {
        echo '<p>';
        echo 'Channel:';
        echo '<select name="ChannelID">';
        MakeChoices($chArray[$_POST['ModuleID']], $_POST['ChannelID']);
        echo '</select>';
        echo '</p>';
    }

?>
<input type="submit" value="Choose">
</form>
<?php
if (isset($_POST['ChannelID'])) {
    echo '<p>';
    foreach ($parNames as $key) {
        echo $key . "\t" .$reader->GetVal($_POST['ModuleID'], $_POST['ChannelID'], $key)."<br>";
    }
    echo '</p>';
}

if (isset($_POST['ModuleID'])) { // ChannelID can have 0.  How to make if statement.
    echo $_POST['ModuleID'];
}
if (isset($_POST['ChannelID'])) {
    echo " ch:" . $_POST['ChannelID'];
}
?>
</body>

</html>
