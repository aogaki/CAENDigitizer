<!DOCTYPE html>
<html>

<head>
  <meta charset="utf-8">
  <title>test</title>
</head>

<?php
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

include("./ParReader.php");
$file = "/Data/DAQ/Parameters/parameters.xml";
$reader = new ParReader($file);

$parNames = array("RecordLength", "IOLevel", "Using", "DCOffset", "Polarity", "PreTrigger", "Decimation", "TrapPoleZero", "TrapFlatTop", "TrapRiseTime",
"PeakingTime", "TTFDelay", "TTFSmoothing", "Threshold", "TrgHoldOff", "EneCoarseGain", "EneFineGain", "NSPeak", "TrapNSBaseline", "FakeEventFlag",
"TrapCFDFraction", "DynamicRange");

if (isset($_POST['RecordLength'])) {
    // echo "hit!<br>";
    // echo '<p>';
    // foreach ($parNames as $key) {
    //     echo $key . "\t" .$reader->GetVal($_POST['ModuleID'], $_POST['ChannelID'], $key)."<br>";
    // }
    // echo '</p>';

    foreach ($parNames as $key) {
        $val = $_POST[$key];
        $reader->SetVal($_POST['LastModuleID'], $_POST['LastChannelID'], $key, $val);
    }
    // $reader->Save("/Data/DAQ/Parameters/test.xml");
    $reader->Save($file);
    // unset($reader);
    // $reader = new ParReader($file);
}

$modArray = $reader->GetModArray();
$chArray = $reader->GetChArray();
?>

<body>
<form action="UI.php" name="ChSelect" method="post">
<p>
  Module:
  <select name="ModuleID">
    <?php
    if (isset($_POST['ModuleID'])) {
        MakeChoices($modArray, $_POST['ModuleID']);
    } else {
        MakeChoices($modArray, "");
    }
    ?>
  </select>
<?php
if (isset($_POST['ModuleID'])) {
        echo 'Channel: ';
        echo '<select name="ChannelID">';
        if (isset($_POST['ChannelID'])) {
            MakeChoices($chArray[$_POST['ModuleID']], $_POST['ChannelID']);
        } else {
            MakeChoices($chArray[$_POST['ModuleID']], "");
        }
        echo '</select>';
    }
?>
</p>
<input type="submit" value="Choose">
<!-- </form>

<form action="UI.php" name="ParSet" method="post"> -->
<?php
if (isset($_POST['ModuleID']) && isset($_POST['ChannelID'])) {
    $mod = $_POST['ModuleID'];
    $ch = $_POST['ChannelID'];

    echo "<p>";
    echo "Module: " . $mod;
    echo "\tCh: " . $ch;
    echo "</p>";
    echo "<input type=\"hidden\" name=\"LastModuleID\" value=\"$mod\">";
    echo "<input type=\"hidden\" name=\"LastChannelID\" value=\"$ch\">";

    echo '<p>';
    $reader->MakeRecordLength($mod, $ch);
    echo '</p>';

    echo '<p>';
    $reader->MakeIOLevel($mod, $ch);
    echo '</p>';

    echo '<p>';
    $reader->MakeUsing($mod, $ch);
    echo '</p>';

    echo '<p>';
    $reader->MakeDCOffset($mod, $ch);
    echo '</p>';

    echo '<p>';
    $reader->MakePolarity($mod, $ch);
    echo '</p>';

    echo '<p>';
    $reader->MakePreTrigger($mod, $ch);
    echo '</p>';

    echo '<p>';
    $reader->MakeDecimation($mod, $ch);
    echo '</p>';

    echo '<p>';
    $reader->MakeTrapPoleZero($mod, $ch);
    echo '</p>';

    echo '<p>';
    $reader->MakeTrapFlatTop($mod, $ch);
    echo '</p>';

    echo '<p>';
    $reader->MakeTrapRiseTime($mod, $ch);
    echo '</p>';

    echo '<p>';
    $reader->MakePeakingTime($mod, $ch);
    echo '</p>';

    echo '<p>';
    $reader->MakeTTFDelay($mod, $ch);
    echo '</p>';

    echo '<p>';
    $reader->MakeTTFSmoothing($mod, $ch);
    echo '</p>';

    echo '<p>';
    $reader->MakeThreshold($mod, $ch);
    echo '</p>';

    echo '<p>';
    $reader->MakeTrgHoldOff($mod, $ch);
    echo '</p>';

    echo '<p>';
    $reader->MakeEneCoarseGain($mod, $ch);
    echo '</p>';

    echo '<p>';
    $reader->MakeEneFineGain($mod, $ch);
    echo '</p>';

    echo '<p>';
    $reader->MakeNSPeak($mod, $ch);
    echo '</p>';

    echo '<p>';
    $reader->MakeTrapNSBaseline($mod, $ch);
    echo '</p>';

    echo '<p>';
    $reader->MakeTrapCFDFraction($mod, $ch);
    echo '</p>';

    echo '<p>';
    $reader->MakeDynamicRange($mod, $ch);
    echo '</p>';

    echo '<p>';
    $reader->MakeFakeEventFlag($mod, $ch);
    echo '</p>';

    echo "<input type=\"submit\" value=\"Set\">";
}
?>
</form>


</body>

</html>
