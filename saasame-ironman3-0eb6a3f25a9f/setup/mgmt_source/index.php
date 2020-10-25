<?php
set_time_limit(0);
require_once '_include/_inc/_class_main.php';
require_once '_include/_inc/_class_identification.php';
require_once '_include/_inc/_class_replica.php';
require_once '_include/_inc/_class_server.php';
require_once '_include/_inc/_class_service.php';
require_once '_include/_inc/_class_aws.php';
require_once '_include/_inc/_class_openstack.php';
require_once '_include/_inc/Common_Model.php';
require_once '_include/_inc/Azure.php';
require_once '_include/_inc/Aliyun_Model.php';
require_once '_include/_inc/Aliyun.php';
require_once '_include/_inc/_class_mailer.php';
require_once '_include/_inc/_class_Tencent_Model.php';
require_once '_include/_inc/_class_Tencent.php';
require_once '_include/_inc/_class_azure_blob.php';
require_once '_include/_inc/_class_ctyun.php';
require_once '_include/_inc/_class_VMWare.php';

#DB CONNECTION
new Db_Connection();
new Misc_Class();

#RESFUL PASS VIA
$Restful = new Restful_Passvia();
$Restful -> query_restful_value($_SERVER['REQUEST_URI']);

$DBCON = null;
