<?php
$THRIFT_ROOT = './';

require_once 'Thrift/ClassLoader/ThriftClassLoader.php';

use Thrift\ClassLoader\ThriftClassLoader;

$loader = new ThriftClassLoader();
$loader->registerNamespace('Thrift', $THRIFT_ROOT);
$loader->register();

use Thrift\Protocol\TBinaryProtocol;
use Thrift\Transport\TBufferedTransport;

#Init transport thrift
require_once '_transport/Types.php';
require_once '_transport/common_service.php';
require_once '_transport/common_connection_service.php';
require_once '_transport/carrier_service.php';
require_once '_transport/physical_packer_service_proxy.php';
require_once '_transport/reverse_transport.php';
require_once '_transport/transport_service.php';

trait Thrift_Controller{
	
	private $Client;

	public function initThriftClient() {

		return $this->getSelfThriftConnection();
	}
	
	private function createThriftObject( $Address, $Port )
	{
		try
		{
			$CERT_ROOT = getenv('WEBROOT').'\apache24\conf\ssl';
				
			$PEM_PATH = $CERT_ROOT.'/server.crt';
			$KEY_PATH = $CERT_ROOT.'/server.key';
			
			#SET KEY FILE
			$SSL_CONTEXT = stream_context_create();
			stream_context_set_option($SSL_CONTEXT,'ssl','verify_peer_name',false);
    
			#SET KEY FILE
			stream_context_set_option($SSL_CONTEXT,'ssl','local_pk',$KEY_PATH);
		
			#SET CERTIFICATES FILE
			stream_context_set_option($SSL_CONTEXT,'ssl','cafile',$PEM_PATH);
			stream_context_set_option($SSL_CONTEXT,'ssl','local_cert',$PEM_PATH);
						
			$Socket = new Thrift\Transport\TSSLSocket($Address,$Port,$SSL_CONTEXT);
		
			$Socket->setSendTimeout(600 * 100); #milliseconds
			$Socket->setRecvTimeout(600 * 100); #milliseconds
		
			$OpenTransport = new TBufferedTransport($Socket,1024,1024);

			$OpenTransport->open();
		
			$Connection = new TBinaryProtocol($Socket);
	
			$ThriftObject = new saasame\transport\transport_serviceClient($Connection);
	
			$this->Client = $ThriftObject;

			return true;
		}
		catch (Throwable $e)
		{
			return false;
		}
	}
	
	private function getSelfThriftConnection(){
		
		$port = \saasame\transport\Constant::get('TRANSPORT_SERVICE_PORT');
		
		return $this->createThriftObject("127.0.0.1", $port );
	}

	//magic function
	public function __call($method, $args)
	{
		return call_user_func_array( array( $this->Client, $method ) , $args );
  	}

}
?>