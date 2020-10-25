<?php
require_once '_include/_inc/_class_main.php';

#DB CONNECTION
$ConnSetup = new Db_Connection();

#PAGE PASS VIA
$GetPage = new Pages_Passvia();
$NewPage = $GetPage -> query_restful_value($_SERVER['REQUEST_URI']);

$DBCON = null;
