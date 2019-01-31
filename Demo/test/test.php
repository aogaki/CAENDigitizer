<html>

<head>
  <meta charset="utf-8">
  <title>Parameters</title>
</head>

<?php
include("./GetDOM.php");
$xml = GetDOM("./test.xml");
?>

<body>

  <?php

  ?>

  <script type="text/javascript">
    let parser = new DOMParser();
    let tmp = '<?php echo $xml;?>';
    let parameters = parser.parseFromString(tmp, "text/xml");
    let modules = parameters.getElementsByTagName("module");

    for(let i = 0; i < modules.length; i++){
      console.log(i);
      let id = modules[i].getAttribute("ID");
      console.log(id);
      let sample = modules[i].getElementsByTagName("RecordLength");
      console.log(sample[0].childNodes[0].nodeValue);
      sample[0].childNodes[0].nodeValue = Number(sample[0].childNodes[0].nodeValue) + 10;

      let channels = modules[i].getElementsByTagName("channel");
      console.log(channels.length);
    }
    for(let i = 0; i < 1; i++){
      console.log(i);
      let id = modules[i].getAttribute("ID");
      console.log(id);
      let sample = modules[i].getElementsByTagName("RecordLength");
      console.log(sample[0].childNodes[0].nodeValue);

      let channels = modules[i].getElementsByTagName("channel");
      console.log(channels.length);
    }
    let hello = "Hello world!  I wanna kill all you assholes!!!!!!!!";
    document.write(hello);
  </script>
</body>

</html>
