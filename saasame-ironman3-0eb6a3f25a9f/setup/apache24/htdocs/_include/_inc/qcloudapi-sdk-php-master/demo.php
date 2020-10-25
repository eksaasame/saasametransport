<?php
error_reporting(E_ALL ^ E_NOTICE);
require_once './src/QcloudApi/QcloudApi.php';


$config = array('SecretId'       => 'AKIDlLgmdFskdIjvsarJJ4aqqa4LM7dupgBQ',
                'SecretKey'      => '6xYu0qQjnbBZhY9TWIo6SZmocBWdWBbn',
                'RequestMethod'  => 'GET',
                'DefaultRegion'  => 'bj');

$cvm = QcloudApi::load(QcloudApi::MODULE_CVM, $config);

$package = array('Version' => '2017-03-12'
				);

$a = $cvm->DescribeInstances($package);
print_r($a);
if ($a === false) {
    $error = $cvm->getError();
    echo "Error code:" . $error->getCode() . ".\n";
    echo "message:" . $error->getMessage() . ".\n";
    echo "ext:" . var_export($error->getExt(), true) . ".\n";
} else {
    var_dump( json_encode( $a , JSON_UNESCAPED_UNICODE) );
}

//echo "\nRequest :" . $cvm->getLastRequest();
//echo "\nResponse :" . $cvm->getLastResponse();
//echo "\n";
