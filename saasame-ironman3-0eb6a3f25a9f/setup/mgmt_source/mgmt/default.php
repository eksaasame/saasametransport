<?php
#set_time_limit(1200);
#error_reporting(E_ALL);
error_reporting(0);

# Thrift Root Path
$THRIFT_ROOT = '../_include/_inc';

# Init Thrift Autloader
require_once $THRIFT_ROOT . '/Thrift/ClassLoader/ThriftClassLoader.php';

use Thrift\ClassLoader\ThriftClassLoader;

$loader = new ThriftClassLoader();
$loader->registerNamespace('Thrift', $THRIFT_ROOT);
$loader->registerDefinition('Thrift', $THRIFT_ROOT . '/packages');
$loader->register();

use Thrift\Transport\TPhpStream;
use Thrift\Transport\TBufferedTransport;
use Thrift\Protocol\TBinaryProtocolAccelerated;
use Thrift\Protocol\TBinaryProtocol;

#LOAD MANAGEMENT THRIFT CLASS
require_once $THRIFT_ROOT . '/_transport/management_service.php';
require_once $THRIFT_ROOT . '/_transport/Types.php';

#LOAD MANAGEMENT CLASS
require_once $THRIFT_ROOT . '/_class_main.php';
require_once $THRIFT_ROOT . '/_class_thrift.php';

#LOAD DATABASE CONNECTION
$DBConn    = new Db_Connection();
$handler   = new management_communication();
$processor = new saasame\transport\management_serviceProcessor($handler);
$transport = new TBufferedTransport(new TPhpStream(TPhpStream::MODE_R | TPhpStream::MODE_W));
$protocol  = new TBinaryProtocol($transport, true, true);

# Begin to Exec
$transport->open();
try{
	$processor->process($protocol, $protocol);
}
catch (Exception $e){
	return $e;
}
$transport->close();