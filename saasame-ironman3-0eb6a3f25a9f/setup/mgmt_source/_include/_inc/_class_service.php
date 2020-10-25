<?php
###################################
#
#	THRIFT SERVER MANAGEMENT
#
###################################
#Include path
$THRIFT_ROOT = '_include/_inc/';

#Init Autloader
require_once 'Thrift/ClassLoader/ThriftClassLoader.php';

use Thrift\ClassLoader\ThriftClassLoader;

$loader = new ThriftClassLoader();
$loader->registerNamespace('Thrift', $THRIFT_ROOT);
$loader->register();

use Thrift\Protocol\TBinaryProtocol;
use Thrift\Transport\THttpClient;
use Thrift\Transport\TBufferedTransport;
use Thrift\Exception\TException;

#Init transport thrift
require_once '_transport/Types.php';
require_once '_transport/common_service.php';
require_once '_transport/common_connection_service.php';
require_once '_transport/carrier_service.php';
require_once '_transport/physical_packer_service_proxy.php';
require_once '_transport/reverse_transport.php';
require_once '_transport/transport_service.php';

###################################
#
#	SERVICES MANAGEMENT
#
###################################
class Service_Class extends Db_Connection
{
	###########################
	# Construct Function
	###########################
	protected $ReplMgmt;
	protected $ServerMgmt;
	protected $MailerMgmt;
	protected $OpenStackMgmt;
	protected $AwsMgmt;
	protected $AzureMgmt;
	protected $AzureBlobMgmt;
	protected $AliMgmt;
	protected $CtyunMgmt;
	protected $TencentMgmt;
	protected $VMWareMgmt;
	
	public function __construct()
	{
		parent::__construct();
		$this -> ReplMgmt    	= new Replica_Class();	
		$this -> ServerMgmt  	= new Server_Class();
		$this -> MailerMgmt  	= new Mailer_Class();
		$this -> OpenStackMgmt	= new OpenStack_Action_Class();		
		$this -> AwsMgmt     	= new Aws_Action_Class();
		$this -> AzureMgmt   	= new Azure_Controller();
		$this -> AzureBlobMgmt 	= new Azure_Blob_Action_Class();
		$this -> AliMgmt  	 	= new Aliyun_Controller();
		$this -> CtyunMgmt 		= new Ctyun_Action_Class();
		$this -> TencentMgmt	= new Tencent_Controller();		
		$this -> VMWareMgmt		= new VM_Ware();
	}
		
	###########################
	#COMMON SERVICE CONNECTION
	###########################
	private function common_connection($Address,$Port,$USE_SECURE_SOCKET=true)
	{
		try
		{
			//$USE_SECURE_SOCKET = Misc_Class::secure_socket_config();
			if ($USE_SECURE_SOCKET == TRUE)
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
							
				$OpenSocket = new Thrift\Transport\TSSLSocket($Address,$Port,$SSL_CONTEXT);
			}
			else
			{
				$OpenSocket = new Thrift\Transport\TSocket($Address,$Port);
			}			
			$OpenSocket->setSendTimeout(300 * 100); #milliseconds
			$OpenSocket->setRecvTimeout(300 * 100); #milliseconds
		
			$OpenTransport = new Thrift\Transport\TBufferedTransport($OpenSocket,1024,1024);
			$OpenTransport->open();
		
			$OpenProtocol = new Thrift\Protocol\TBinaryProtocol($OpenSocket);
	
			return $OpenProtocol;
		}
		catch (Throwable $e)
		{
			return false;
		}
	}
	
	###########################
	#TRANSPORT SERVICE NAME TO SERVICE PORT NUMBER (NO USE)
	###########################
	private function translate_service_port($SERV_TYPE)
	{
		$DATA_LIST = array('Scheduler','Physical Packer','Virtual Packer','Carrier','Loader','Launcher');
		
		$USE_SECURE_SOCKET = Misc_Class::secure_socket_config();
		if ($USE_SECURE_SOCKET == true)
		{
			$DEF_CARRIER_PORT = 'CARRIER_SERVICE_SSL_PORT';
		}
		else
		{
			$DEF_CARRIER_PORT = 'CARRIER_SERVICE_PORT';
		}
		
		$PORT_LIST = array('SCHEDULER_SERVICE_PORT','PHYSICAL_PACKER_SERVICE_PORT','VIRTUAL_PACKER_SERVICE_PORT',$DEF_CARRIER_PORT,'LOADER_SERVICE_PORT','LAUNCHER_SERVICE_PORT');
		$PORT_NAME = str_replace($DATA_LIST,$PORT_LIST,$SERV_TYPE);
		
		return \saasame\transport\Constant::get($PORT_NAME);
	}	
	
	###########################
	#GET CONNECTION
	###########################
	public function get_connection($SERV_ADDR,$SERV_TYPE,$SERV_FILTER=null,$MGMT_ADDR=null)
	{
		$PORT_NUM = \saasame\transport\Constant::get('TRANSPORT_SERVICE_PORT');
		
		$OpenProtocol = $this -> common_connection('127.0.0.1',$PORT_NUM);
		
		$ClientCall = new saasame\transport\transport_serviceClient($OpenProtocol);	
			
		#PING THE HOST
		$PingTheHost = $ClientCall -> ping_p($SERV_ADDR);

		if (isset($PingTheHost -> version))
		{
			#GET HOST
			$RETRY = 5;
			for ($i=0; $i<$RETRY; $i++)
			{
				try{
					$HostDetail = $ClientCall -> get_host_detail_p($SERV_ADDR,$SERV_FILTER);
					if (isset($HostDetail -> client_name) AND $HostDetail -> client_name != '')
					{
						break;
					}
					else
					{
						sleep(5);
					}
				}
				catch(Throwable $e){
					Misc_Class::function_debug('_mgmt',__FUNCTION__,$e);
				}				
			}			
			
			if (!isset($HostDetail -> client_name) OR $HostDetail -> client_name == '')
			{
				Misc_Class::function_debug('_mgmt',__FUNCTION__,'Cannot get transport service information.');
				return false;
			}
			else
			{
				if (strpos($SERV_TYPE, 'Packer') == false)
				{
					#MGMT COMMUNICATION PORT
					$MGMT_COMM = Misc_Class::mgmt_comm_type('verify');
					$HTTP_PORT = $MGMT_COMM['mgmt_port'];
					$IS_SSL = $MGMT_COMM['is_ssl'];
		
					try{
						$VerifyMgmt = $ClientCall -> verify_management_p($SERV_ADDR,$MGMT_ADDR,$HTTP_PORT,$IS_SSL);
						#$VerifyMgmt = true;
						if ($VerifyMgmt == true)
						{
							$PingTheHost -> path = str_replace('\\'.$SERV_TYPE,'',$PingTheHost -> path);
							return (object)array_merge((array)$PingTheHost,(array)$HostDetail);
						}
						else
						{
							Misc_Class::function_debug('_mgmt',__FUNCTION__,'Failed to verify mgmt. service.');
							return false;
						}
					}
					catch(Throwable $e){
						Misc_Class::function_debug('_mgmt',__FUNCTION__,$e);
						return false;
					}					
				}
				else
				{
					$PingTheHost -> path = str_replace('\\'.$SERV_TYPE,'',$PingTheHost -> path);					
					return (object)array_merge((array)$PingTheHost,(array)$HostDetail);
				}
			}
		}
		else
		{
			return $PingTheHost;
		}		
	}
	
	###########################
	#GET HOST DETAIL
	###########################
	public function get_host_detail($SERV_ADDR)
	{
		$PORT_NUM = \saasame\transport\Constant::get('TRANSPORT_SERVICE_PORT');
		
		$OpenProtocol = $this -> common_connection('127.0.0.1',$PORT_NUM);
		
		$ClientCall = new saasame\transport\transport_serviceClient($OpenProtocol);
		
		try
		{
			$PingTheHost = $ClientCall -> ping_p($SERV_ADDR);
			
			if (isset($PingTheHost -> version))
			{
				$HostDetail = $ClientCall -> get_host_detail_p($SERV_ADDR,null);
			
				$PingTheHost -> path = str_replace('\\Launcher','',$PingTheHost -> path);					
				return (object)array_merge((array)$PingTheHost,(array)$HostDetail);
			}
			else
			{
				return false;
			}
		}
		catch(Throwable $e)
		{
			return false;
		}	
	}
	
	###########################
	#GET MGMT HOST INFO
	###########################
	private function get_mgmt_host_info()
	{
		$PORT_NUM = \saasame\transport\Constant::get('TRANSPORT_SERVICE_PORT');
		
		$OpenProtocol = $this -> common_connection('127.0.0.1',$PORT_NUM);
		
		$ClientCall = new saasame\transport\transport_serviceClient($OpenProtocol);
		
		$MgmtHostDetail = $ClientCall -> get_host_detail(null,null);
		
		return $MgmtHostDetail;
	}
	
	###########################
	#GET PHYSICAL HOST INFORMATION
	###########################
	public function physical_host_info($SERV_ADDR,$HOST_ADDR)
	{
		try
		{
			$PORT_NUM = \saasame\transport\Constant::get('TRANSPORT_SERVICE_PORT');
		
			$OpenProtocol = $this -> common_connection('127.0.0.1',$PORT_NUM);
		
			$ClientCall = new saasame\transport\transport_serviceClient($OpenProtocol);
		
			$HostDetail = $ClientCall -> get_physical_machine_detail_p($SERV_ADDR,$HOST_ADDR,0);
			
			Misc_Class::function_debug('_mgmt',__FUNCTION__,$HostDetail);
			
			if ($HostDetail == null)
			{
				return false;
			}
			else
			{
				return $HostDetail;
			}
		}
		catch(Throwable $e)
		{
			Misc_Class::function_debug('_mgmt',__FUNCTION__,$e);
			return false;
		}
	}
	
	###########################
	#TEST SERVICE CONNECTION
	###########################
	public function test_service_connection($ACCT_UUID,$SERV_UUID,$HOST_ADDR,$HOST_USER,$HOST_PASS,$SERV_TYPE,$MGMT_ADDR=null,$HOST_UUID=null)
	{
		if (Misc_Class::is_valid_uuid($SERV_UUID) === TRUE)
		{		
			$SERV_INFO = $this -> ServerMgmt -> query_server_info($SERV_UUID);
			if ($SERV_INFO == FALSE)
			{
				return false;
			}
			else
			{
				if ($SERV_INFO['SERV_INFO']['direct_mode'] == TRUE)
				{
					#SERVER ADDRESS ARRAY
					$SERV_ADDR = $SERV_INFO['SERV_ADDR'];
				}
				else
				{
					#SERVER ADDRESS ARRAY
					$SERV_ADDR = array($SERV_INFO['SERV_INFO']['machine_id']);	
				}
			}			
		}
		else
		{
			$SERV_ADDR = explode(',', $SERV_UUID);
		}	

		#HOST ADDRESS ARRAY
		$HOST_ADDR = explode(',', $HOST_ADDR);
		
		if (strpos($SERV_TYPE, 'Packer') == false)
		{
			try{
				$TestHostInfo = false;
				for ($s=0; $s<count($SERV_ADDR); $s++)
				{
					$TestHostInfo = $this -> get_connection($SERV_ADDR[$s],$SERV_TYPE,0,$MGMT_ADDR);
					if ($TestHostInfo != false)
					{
						$TestHostInfo -> prefer_address = $SERV_ADDR[$s];
						break;
					}
				}
				return $TestHostInfo;				
			}
			catch(Throwable $e){
				return false;
			}
		}
		else
		{
			switch ($SERV_TYPE)
			{
				case 'Physical Packer':
					
					$PackerReturn = array('Code' => false, 'Msg' => 'Failed to verify physical packer.', 'OS_Type' => '', 'Server_Addr' => '');
					
					for ($i=0; $i<count($HOST_ADDR); $i++)
					{
						for ($w=0; $w<count($SERV_ADDR); $w++)
						{
							$QueryHostInfo = $this -> physical_host_info($SERV_ADDR[$w],$HOST_ADDR[$i]);

							if ($QueryHostInfo != false)
							{
								break;
							}
						}
						
						if (isset($QueryHostInfo -> client_id))
						{
							/* Get OS Type Information */
							if (strpos($QueryHostInfo -> os_name, 'Microsoft') !== FALSE)
							{
								$OS_TYPE = 'Windows';
							}
							else
							{
								$OS_TYPE = 'Linux';
							}	

							if ($QueryHostInfo -> is_winpe == TRUE)
							{
								$PackerReturn = array('Code' => false, 'Msg' => 'Please register offline packer in offline packer page.', 'OS_Type' => $OS_TYPE, 'Server_Addr' => '');
							}
							else
							{
								$PackerReturn = array('Code' => true, 'Msg' => $HOST_UUID, 'OS_Type' => $OS_TYPE, 'Server_Addr' => $SERV_INFO['SERV_ADDR']);
							}					
							break;
						}
					}					
					return $PackerReturn;
				break;
				
				case 'Virtual Packer':
					for ($w=0; $w<count($SERV_ADDR); $w++)
					{
						$QueryHostInfo = $this -> virtual_host_info($SERV_ADDR[$w],$HOST_ADDR[0],$HOST_USER,$HOST_PASS);
						if ($QueryHostInfo != FALSE)
						{
							break;
						}
					}
					
					if ($QueryHostInfo != FALSE)
					{
						/*
						#CHECK ESX SERVER UUID
						if ($HOST_UUID == null)
						{
							#LIST REGISTER ESX
							$LIST_ESX = $this -> ServerMgmt -> list_server_with_type($ACCT_UUID,$SERV_TYPE);
							if ($LIST_ESX != FALSE)
							{
								$REGISTERED_ESX = false;
								
								$HypervisorUUID = $QueryHostInfo -> uuid;
								for ($i=0; $i<count($LIST_ESX); $i++)
								{
									$REG_UUID = $LIST_ESX[$i]['SERV_INFO'] -> hypervisor_uuid;
									
									if ($HypervisorUUID == $REG_UUID)
									{
										$REGISTERED_ESX = true;
										break;
									}				
								}

								if ($REGISTERED_ESX == TRUE)
								{
									return array('Code' => false, 'Msg' => 'The ESX has been registered.', 'OS_Type' => '');
								}
							}
						}
						*/
						
						#CHECK WITH VIRTUAL CENTER
						$VCENTER_VERSION = $QueryHostInfo -> virtual_center_version;
						
						if ($VCENTER_VERSION != '')
						{
							$CHECK_FEATURE = true;
						}
						else
						{
							#ADD CHECK LICENSE API HERE
							$ESX_LICENSE = $QueryHostInfo -> lic_features;
							reset($ESX_LICENSE);
							
							#GET LICENSE NAME
							$LICENSE_NAME = key($ESX_LICENSE);
							
							#GET LICENSE FEATURES
							$LICENSE_FEATURES = $ESX_LICENSE[$LICENSE_NAME];
							
							#LIST REQUIRED FEATURES WITH VERSIONS
							$REQUIRED_FEATURES = array('vSphere API','Storage APIs','vStorage APIs');

							#CHECK FEATURE
							$CHECK_FEATURE = array_intersect($REQUIRED_FEATURES,$LICENSE_FEATURES);
						}
						
						#CHECK VM LICENSE AND LIST
						if (count($CHECK_FEATURE) == 0)
						{
							return array('Code' => false, 'Msg' => 'The ESX license API does not meet the requirement.', 'OS_Type' => '', 'Server_Addr' => '');
						}
						else
						{
							if ($HOST_UUID != null)
							{
								#COMPARE SELECT HOST
								$HOST_VMS_UUID = $QueryHostInfo -> vms;
								if (array_key_exists($HOST_UUID, $HOST_VMS_UUID))
								{
									return array('Code' => true, 'Msg' => $HOST_UUID, 'OS_Type' => '');					
								}
								else
								{
									return array('Code' => false, 'Msg' => 'The virtual machine cannot be found on the destination ESX host.', 'OS_Type' => '', 'Server_Addr' => '');
								}
							}
							else
							{
								return array('Code' => true, 'Msg' => $HOST_UUID, 'OS_Type' => '');
							}
						}
					}
					else
					{
						return array('Code' => false, 'Msg' => 'Failed to connect to ESX server.', 'OS_Type' => '');
					}
				break;
				
				case 'Offline Packer':
					for ($i=0; $i<count($HOST_ADDR); $i++)
					{
						for ($w=0; $w<count($SERV_ADDR); $w++)
						{
							$QueryHostInfo = $this -> physical_host_info($SERV_ADDR[$w],$HOST_ADDR[$i]);
							
							if ($QueryHostInfo != false)
							{
								break;
							}
						}
						
						if (isset($QueryHostInfo -> client_id))
						{
							if ($QueryHostInfo -> is_winpe == false)
							{
								return array('Code' => false, 'Msg' => 'Please register physical packer in physical packer page.', 'OS_Type' => '', 'Server_Addr' => '');
							}
							else
							{
								return array('Code' => true, 'Msg' => '', 'OS_Type' => '', 'Server_Addr' => $SERV_INFO['SERV_ADDR']);
							}
						}
						else
						{
							return array('Code' => false, 'Msg' => 'Failed to verify offline packer.', 'OS_Type' => '', 'Server_Addr' => $SERV_INFO['SERV_ADDR']);
						}
					}
				break;		
			}
		}
	}
	
	###########################
	#TEST SERVICES CONNECTION
	###########################
	public function test_services_connection($SERV_ADDR,$SELT_SERV,$MGMT_ADDR)
	{
		$SERV_ADDR = explode(',', $SERV_ADDR);
	
		$ReturnCode = false;
		$ReturnMsg = 'Failed to verify services.';
		
		for ($i=0; $i<count($SERV_ADDR); $i++)
		{
			$TestConnection = $this -> test_service_connection(null,$SERV_ADDR[$i],null,null,null,'Launcher',$MGMT_ADDR);
			if ($TestConnection != FALSE)
			{
				$ReturnCode = true;
				$ReturnMsg = $TestConnection;
				break;
			}
		}
		
		$ConnectionMessage = array('Code' => $ReturnCode, 'Msg' => $ReturnMsg);
		return $ConnectionMessage;
				
		/*
		$SERV_TYPE = explode(',', $SELT_SERV);
		
		for ($i=0; $i<count($SERV_TYPE); $i++)
		{
			$TestConnection = $this -> test_service_connection(null,end($SERV_ADDR),null,null,null,$SERV_TYPE[$i],$MGMT_ADDR);
		
			if ($TestConnection == FALSE)
			{
				$ConnectionMessage = array('Code' => false, 'Msg' => 'Failed to verify services.');
				return $ConnectionMessage;
			}
		}
		*/
	}
	
	
	###########################
	#QUERY CONNECTION STATUS
	###########################
	public function get_connection_status($CONN_UUID,$SERV_TYPE)
	{
		$CONN_INFO = $this -> ServerMgmt -> query_connection_info($CONN_UUID);

		if ($CONN_INFO != FALSE OR $CONN_UUID != null)
		{
			switch ($SERV_TYPE)
			{
				case 'CARRIER':
					if ($CONN_INFO['CARR_DIRECT'] == TRUE)
					{
						$SERV_ADDR = $CONN_INFO['CARR_ADDR'];
					}
					else
					{
						$SERV_ADDR = array_flip($CONN_INFO['CARR_ID']);
					}					
					$SERVICE_TYPE = \saasame\transport\Constant::get('CARRIER_SERVICE');
				break;
					
				case 'LOADER':
					if ($CONN_INFO['LOAD_DIRECT'] == TRUE)
					{
						$SERV_ADDR = $CONN_INFO['LOAD_ADDR'];
					}
					else
					{
						$SERV_ADDR = array_flip($CONN_INFO['LOAD_ID']);
					}
					$SERVICE_TYPE = \saasame\transport\Constant::get('LOADER_SERVICE');
				break;
			}
				
			$PORT_NUM = \saasame\transport\Constant::get('TRANSPORT_SERVICE_PORT');					
			
			$OpenProtocol = $this -> common_connection('127.0.0.1',$PORT_NUM);
	
			$ClientCall = new saasame\transport\transport_serviceClient($OpenProtocol);
			for ($i=0; $i<count($SERV_ADDR); $i++)
			{
				try{	
					$CONN_INFO = $ClientCall -> get_connection_p($SERV_ADDR[$i],$CONN_UUID,$SERVICE_TYPE);
					if ($CONN_INFO != FALSE)
					{
						break;
					}
				}
				catch (Throwable $e){
					$CONN_INFO = false;
				}				
			}
			return $CONN_INFO;
		}
		else
		{
			return false;
		}
	}

	
	###########################
	#RE-CREATE CONNECTION
	###########################
	public function re_create_connection($CONN_UUID,$SERV_TYPE)
	{
		$CONN_INFO = $this -> ServerMgmt -> query_connection_info($CONN_UUID);

		if ($CONN_INFO != FALSE)
		{
			switch ($SERV_TYPE)
			{
				case 'CARRIER':					
					if ($CONN_INFO['CARR_DIRECT'] == TRUE)
					{
						$SERV_ADDR = $CONN_INFO['CARR_ADDR'];
					}
					else
					{
						$SERV_ADDR = array_flip($CONN_INFO['CARR_ID']);
					}
					$SERVICE_TYPE = \saasame\transport\Constant::get('CARRIER_SERVICE');
				break;
				
				case 'LOADER':
					if ($CONN_INFO['LOAD_DIRECT'] == TRUE)
					{
						$SERV_ADDR = $CONN_INFO['LOAD_ADDR'];
					}
					else
					{
						$SERV_ADDR = array_flip($CONN_INFO['LOAD_ID']);
					}					
					$SERVICE_TYPE = \saasame\transport\Constant::get('LOADER_SERVICE');
				break;				
			}
			
			$PORT_NUM = \saasame\transport\Constant::get('TRANSPORT_SERVICE_PORT');
			
			$OpenProtocol = $this -> common_connection('127.0.0.1',$PORT_NUM);

			$ClientCall = new saasame\transport\transport_serviceClient($OpenProtocol);
		
			for ($x=0; $x<count($SERV_ADDR); $x++)
			{
				try{
					$GET_TRANSPORT_INFO = $ClientCall -> ping_p($SERV_ADDR[$x]);
					$IMAGEX_PATH = str_replace('\\Launcher','',$GET_TRANSPORT_INFO -> path).'\\imagex';
					break;					
				}
				catch (Throwable $e){
					$IMAGEX_PATH = false;
				}				
			}
			
			if ($IMAGEX_PATH == false)
			{
				return false;
			}			
		
			#QUERY CONNECTION INFOMATION
			$CONN_TYPE 		= $CONN_INFO['CONN_TYPE'];
			//$IMAGEX_PATH 	= $CONN_INFO['CONN_DATA']['options']['folder'];
			$IS_COMPRESSED  = $CONN_INFO['CONN_DATA']['compressed'];			
			$IS_CHECKSUM 	= $CONN_INFO['CONN_DATA']['checksum'];
			$IS_ENCRYPTED   = $CONN_INFO['CONN_DATA']['encrypted'];
			
			if ($CONN_INFO['CONN_TYPE'] == 'LocalFolder' OR $CONN_INFO['SCHD_DIRECT'] == FALSE)
			{
				$CONN_ARRAY = array('type' 			=> 0,
									'id' 			=> $CONN_UUID,
									'options' 		=> array('folder' => $IMAGEX_PATH),
									'compressed' 	=> $IS_COMPRESSED, 
									'checksum' 		=> $IS_CHECKSUM,
									'encrypted' 	=> $IS_ENCRYPTED);
			}
			else
			{
				$WEBDAV_USER = $CONN_INFO['CONN_DATA']['detail']['remote']['username'];
				$WEBDAV_PASS = $CONN_INFO['CONN_DATA']['detail']['remote']['password'];				
				
				#GET WEBDAV CONFIGURATION
				$WEBDAV_CONFIG = Misc_Class::mgmt_comm_type('webdav');
				$WEBDAV_PORT = $WEBDAV_CONFIG['mgmt_port']; 
				$WEBDAV_SSL  = $WEBDAV_CONFIG['is_ssl'];
				
				$NETWORK_ARRAY = array('path' 		=> $CONN_INFO['CONN_DATA']['detail']['remote']['path'],
									   'username' 	=> $WEBDAV_USER,
									   'password' 	=> $WEBDAV_PASS,
									   'port'		=> $WEBDAV_PORT);
								
				$DETAIL_ARRAY = array('remote' => new saasame\transport\network_folder($NETWORK_ARRAY));		
			
				if ($WEBDAV_SSL == FALSE)
				{
					$WEBDAV_CONN_TYPE = 1;
				}
				else
				{
					$WEBDAV_CONN_TYPE = 3;
				}
				
				$CONN_ARRAY = array('type' 			=> $WEBDAV_CONN_TYPE,
									'id' 			=> $CONN_UUID,
									'options' 		=> array('folder' => $CONN_INFO['CONN_DATA']['detail']['local']['path']),
									'compressed' 	=> $IS_COMPRESSED, 
									'checksum' 		=> $IS_CHECKSUM,
									'encrypted' 	=> $IS_ENCRYPTED,
									'detail' 		=> new saasame\transport\_detail($DETAIL_ARRAY));
			}			
			$CONN_PATH = new saasame\transport\connection($CONN_ARRAY);

			for ($i=0; $i<count($SERV_ADDR); $i++)
			{
				try{
					$CONN_OUTPUT = $ClientCall -> add_connection_p($SERV_ADDR[$i],$CONN_PATH,$SERVICE_TYPE);
					break;
				}
				catch (Throwable $e){
					$CONN_OUTPUT = false;
				}
			}
			return $CONN_OUTPUT;
			
		}
		else
		{
			return false;
		}
	}
	
	
	###########################
	#GET VIRTUAL HOST INFO
	###########################
	public function virtual_host_info($SERV_ADDR,$HOST_ADDR,$HOST_USER,$HOST_PASS)
	{
		try
		{
			$PORT_NUM = \saasame\transport\Constant::get('TRANSPORT_SERVICE_PORT');

			$OpenProtocol = $this -> common_connection('127.0.0.1',$PORT_NUM);

			$ClientCall = new saasame\transport\transport_serviceClient($OpenProtocol);
			
			$HostDetail = $ClientCall -> get_virtual_host_info_p($SERV_ADDR,$HOST_ADDR,$HOST_USER,$HOST_PASS);
		
			Misc_Class::function_debug('_mgmt',__FUNCTION__,$HostDetail);
			
			if ($HostDetail == null)
			{
				return false;
			}
			else
			{
				return $HostDetail;
			}
		}
		catch(Throwable $e)
		{
			Misc_Class::function_debug('_mgmt',__FUNCTION__,$e);
			return false;
		}
	}
	
	###########################
	#GET VIRTUAL HOST INFO PART 2
	###########################
	public function virtual_vms_info($SERV_ADDR,$HOST_ADDR,$HOST_USER,$HOST_PASS,$VM_UUID)
	{
		$PORT_NUM = \saasame\transport\Constant::get('TRANSPORT_SERVICE_PORT');
			
		$OpenProtocol = $this -> common_connection('127.0.0.1',$PORT_NUM);

		$ClientCall = new saasame\transport\transport_serviceClient($OpenProtocol);
		
		try{
			$VmsInfoDetail = $ClientCall -> get_virtual_machine_detail_p($SERV_ADDR,$HOST_ADDR,$HOST_USER,$HOST_PASS,$VM_UUID);
			
			Misc_Class::function_debug('_mgmt',__FUNCTION__,$VmsInfoDetail);
			
			return $VmsInfoDetail;
		}
		catch (Throwable $e){
			Misc_Class::function_debug('_mgmt',__FUNCTION__,$e);
		}
	}
	
	###########################
	#TEST AND ADD CONNECTION
	###########################
	public function test_add_connections($SCHD_UUID,$CARR_UUID,$LOAD_UUID,$LAUN_UUID,$CONN_DEST,$INIT_TYPE,$CONN_TYPE)
	{
		#GEN CONNECTION UUID
		$CONN_UUID  = Misc_Class::guid_v4();
		
		#GET DEFAULT CONNECTION INFORMATION
		$CONN_INFO = Misc_Class::connection_parameter();
		$IS_COMPRESSED 	 = $CONN_INFO['is_compressed'];
		$IS_CHECKSUM	 = $CONN_INFO['is_checksum'];
		$IS_ENCRYPTED 	 = $CONN_INFO['is_encrypted'];
		$WEBDAV_USER 	 = $CONN_INFO['webdav_user'];
		$WEBDAV_PASS 	 = $CONN_INFO['webdav_pass'];		
		

		#LOCAL FOLDER ARRAY
		$LOCAL_ARRAY = array('path' => $CONN_DEST);
			
		#WEBDAV PATH ARRAY
		$NETWORK_ARRAY = array('path' 		=> null,
							   'username' 	=> null,
							   'password' 	=> null,
							   'port'		=> null);
			
			
		#DEFINE DETAIL ARRAY
		$DETAIL_ARRAY = array('local' => new saasame\transport\local_folder($LOCAL_ARRAY),'remote' => new saasame\transport\network_folder($NETWORK_ARRAY));		
		
		$CONN_ARRAY = array('type' 			=> 0,
							'id' 			=> $CONN_UUID,
							'options' 		=> array('folder' => $CONN_DEST),
							'compressed' 	=> $IS_COMPRESSED, 
							'checksum' 		=> $IS_CHECKSUM,
							'encrypted' 	=> $IS_ENCRYPTED,
							'detail' 		=> new saasame\transport\_detail($DETAIL_ARRAY));

		if ($CONN_TYPE == 'LocalFolder')
		{
			#BEGIN CARRIER CONNECTION
			$GET_CARR_INFO = "SELECT * FROM _SERVER WHERE _SERV_UUID = '".$CARR_UUID."' AND _STATUS = 'Y'";
			$CARR_QUERY = $this -> DBCON -> prepare($GET_CARR_INFO);
			$CARR_QUERY -> execute();
			$CARR_COUNT_ROWS = $CARR_QUERY -> rowCount();
			
			if ($CARR_COUNT_ROWS == 1)
			{
				$PORT_NUM = \saasame\transport\Constant::get('TRANSPORT_SERVICE_PORT');				
				$OpenProtocol = $this -> common_connection('127.0.0.1',$PORT_NUM);				
				$ClientCall = new saasame\transport\transport_serviceClient($OpenProtocol);					
				$SERV_TYPE = \saasame\transport\Constant::get('CARRIER_SERVICE');					
				
				#DEFINE CARRIER ADDRESS
				$CarrQueryResult = $CARR_QUERY -> fetch();				
				$CARR_SYSTEM_INFO = json_decode($CarrQueryResult['_SERV_INFO']);
				$CARR_DIRECT_MODE = $CARR_SYSTEM_INFO -> direct_mode;
				if ($CARR_DIRECT_MODE == TRUE)
				{				
					$CARR_ADDR = json_decode($CarrQueryResult['_SERV_ADDR']);
				}
				else
				{
					$CARR_ADDR = array($CARR_SYSTEM_INFO -> machine_id);
				}
				
				for ($i=0; $i<count($CARR_ADDR); $i++)
				{
					$CARR_ARRAY = array('ACCT_UUID' => $CarrQueryResult['_ACCT_UUID'], 'SERV_UUID' => $CarrQueryResult['_SERV_UUID'], 'SERV_ADDR' => $CARR_ADDR[$i]);
					$CONNECTION = new saasame\transport\connection($CONN_ARRAY);
					
					try
					{
						if ($INIT_TYPE == 'Test')
						{
							$CARR_CONN_OUTPUT = $ClientCall -> test_connection_p($CARR_ADDR[$i],$CONNECTION,$SERV_TYPE);
						}
						else
						{
							$CARR_CONN_OUTPUT = $ClientCall -> add_connection_p($CARR_ADDR[$i],$CONNECTION,$SERV_TYPE);
						}
						
						if ($CARR_CONN_OUTPUT == TRUE)
						{
							break;
						}
					}				
					catch (Throwable $e){
						Misc_Class::function_debug('_mgmt',__FUNCTION__,$e);
						//return false;
					}
				}
			}
			else
			{
				return false;
			}
		}
		
		#BEGIN LOADER CONNECTION
		$GET_LOADER_INFO = "SELECT * FROM _SERVER WHERE _SERV_UUID = '".$LOAD_UUID."' AND _STATUS = 'Y'";
		$LOADER_QUERY = $this -> DBCON -> prepare($GET_LOADER_INFO);
		$LOADER_QUERY -> execute();
		$LOADER_COUNT_ROWS = $LOADER_QUERY -> rowCount();
		
		if ($LOADER_COUNT_ROWS == 1)
		{
			$PORT_NUM = \saasame\transport\Constant::get('TRANSPORT_SERVICE_PORT');		
			$OpenProtocol = $this -> common_connection('127.0.0.1',$PORT_NUM);
			$ClientCall = new saasame\transport\transport_serviceClient($OpenProtocol);
			$SERV_TYPE = \saasame\transport\Constant::get('LOADER_SERVICE');
			
			#DEFINE LOADER ADDRESS
			$LoaderQueryResult = $LOADER_QUERY -> fetch();		
			$LOAD_SYSTEM_INFO = json_decode($LoaderQueryResult['_SERV_INFO']);
			$LOAD_DIRECT_MODE = $LOAD_SYSTEM_INFO -> direct_mode;
			if ($LOAD_DIRECT_MODE == TRUE)
			{				
				$LOAD_ADDR = json_decode($LoaderQueryResult['_SERV_ADDR']);
			}
			else
			{
				$LOAD_ADDR = array($LOAD_SYSTEM_INFO -> machine_id);
			}
				
			for ($x=0; $x<count($LOAD_ADDR); $x++)
			{
				$LOAD_ARRAY = array(
								'ACCT_UUID' => $LoaderQueryResult['_ACCT_UUID'],
								'SERV_UUID' => $LoaderQueryResult['_SERV_UUID'],
								'SERV_ADDR' => $LOAD_ADDR[$x]
							);
				$CONNECTION = new saasame\transport\connection($CONN_ARRAY);
			
				try{
					if ($INIT_TYPE == 'Test')
					{
						$LOAD_CONN_OUTPUT = $ClientCall -> test_connection_p($LOAD_ADDR[$x],$CONNECTION,$SERV_TYPE);
					}
					else
					{
						$LOAD_CONN_OUTPUT = $ClientCall -> add_connection_p($LOAD_ADDR[$x],$CONNECTION,$SERV_TYPE);
					}
				
					if ($LOAD_CONN_OUTPUT == TRUE)
					{
						break;
					}
				}
				catch (Throwable $e){
					Misc_Class::function_debug('_mgmt',__FUNCTION__,$e);
					//return $e;
				}
			}
			if ($LOAD_CONN_OUTPUT == FALSE)
			{
				return false;
			}
		}
		else
		{
			return false;
		}
		
		$SERV_ARRAY = array(
						'ACCT_UUID' => $CARR_ARRAY['ACCT_UUID'],
						'SCHD_UUID' => $SCHD_UUID,
						'CARR_UUID' => $CARR_ARRAY['SERV_UUID'],
						'LOAD_UUID' => $LOAD_ARRAY['SERV_UUID'],
						'LAUN_UUID' => $LAUN_UUID,
						'CONN_TYPE' => $CONN_TYPE
					);
		
		return array_merge($SERV_ARRAY,$CONN_ARRAY);
	}
	
	###########################
	#REMOVE CONNECTION
	###########################
	public function remove_connection($CONN_UUID,$SERV_TYPE)
	{
		$CONN_INFO = $this -> ServerMgmt -> query_connection_info($CONN_UUID);

		if ($CONN_INFO != FALSE)
		{
			switch ($SERV_TYPE)
			{
				case 'CARRIER':					
					if ($CONN_INFO['CARR_DIRECT'] == TRUE)
					{
						$SERV_ADDR = $CONN_INFO['CARR_ADDR'];
					}
					else
					{
						$SERV_ADDR = array_flip($CONN_INFO['CARR_ID']);
					}
					$SERVICE_TYPE = \saasame\transport\Constant::get('CARRIER_SERVICE');
				break;
				
				case 'LOADER':
					if ($CONN_INFO['LOAD_DIRECT'] == TRUE)
					{
						$SERV_ADDR = $CONN_INFO['LOAD_ADDR'];
					}
					else
					{
						$SERV_ADDR = array_flip($CONN_INFO['LOAD_ID']);
					}					
					$SERVICE_TYPE = \saasame\transport\Constant::get('LOADER_SERVICE');
				break;				
			}
			
			$PORT_NUM = \saasame\transport\Constant::get('TRANSPORT_SERVICE_PORT');
			
			$OpenProtocol = $this -> common_connection('127.0.0.1',$PORT_NUM);

			$ClientCall = new saasame\transport\transport_serviceClient($OpenProtocol);
		
			for ($x=0; $x<count($SERV_ADDR); $x++)
			{
				try{
					return $ClientCall -> remove_connection_p($SERV_ADDR[$x],$CONN_UUID,$SERVICE_TYPE);
					break;					
				}
				catch (Throwable $e){
					return $e;
				}				
			}			
		}
		else
		{
			return false;
		}
	}
	
	###########################
	#TEST AND ADD ALL-IN-ONE FOLDER CONNECTION
	###########################
	public function verify_folder_connection($SERV_ADDR,$CONN_PATH,$INIT_TYPE,$CONN_TYPE,$CHECKSUM=null,$SOURCE_DIRECT_MODE=true,$TARGET_DIRECT_MODE=true)
	{
		//Misc_Class::function_debug('_mgmt',__FUNCTION__,func_get_args());

		#DEFINE PATH
		$LOCAL_PATH = $CONN_PATH['LocalPath'];
		$WEBDAV_URL = $CONN_PATH['WebDavPath'];
		
		#GEN CONNECTION UUID
		$CONN_UUID  = Misc_Class::guid_v4();
	
		#GET DEFAULT CONNECTION OPTIONS
		$DEFAULT_CONN 	 = Misc_Class::connection_parameter();
		$IS_COMPRESSED 	 = $DEFAULT_CONN['is_compressed'];		
		$IS_ENCRYPTED 	 = $DEFAULT_CONN['is_encrypted'];
		$WEBDAV_USER 	 = $DEFAULT_CONN['webdav_user'];
		$WEBDAV_PASS 	 = $DEFAULT_CONN['webdav_pass'];
		
		#GET CHECKSUM FLAG
		if ($CHECKSUM == null)
		{
			$IS_CHECKSUM = $DEFAULT_CONN['is_checksum'];
		}
		else
		{
			$IS_CHECKSUM = $CHECKSUM;			
		}		
		
		#LOCAL FOLDER ARRAY
		$LOCAL_ARRAY = array('path' => $LOCAL_PATH);
		if ($CONN_TYPE == 'LocalFolder')
		{
			$CONN_CODE = 0;
			
			#WEBDAV PATH ARRAY
			$NETWORK_ARRAY = array('path' 		=> null,
								   'username' 	=> null,
								   'password' 	=> null,
								   'port'		=> null);		
		}
		else
		{
			#GET WEBDAV CONFIGURATION
			$WEBDAV_CONFIG = Misc_Class::mgmt_comm_type('webdav');
			$WEBDAV_PORT = $WEBDAV_CONFIG['mgmt_port']; 
			$WEBDAV_SSL  = $WEBDAV_CONFIG['is_ssl'];
			
			#DEFIND WEBDAV MODE
			if ($WEBDAV_SSL == FALSE)
			{
				$CONN_CODE = 1;
			}
			else
			{
				if ($SOURCE_DIRECT_MODE == TRUE)
				{
					$CONN_CODE = 3;					
				}
				else
				{
					if ($TARGET_DIRECT_MODE == TRUE)
					{
						$CONN_CODE = 4;
					}
					else
					{
						$CONN_CODE = 3;
					}
				}
			}			
			#WEBDAV PATH ARRAY
			$NETWORK_ARRAY = array('path' 		=> $WEBDAV_URL,
								   'username' 	=> $WEBDAV_USER,
								   'password' 	=> $WEBDAV_PASS,
								   'port'		=> $WEBDAV_PORT);
		}
		
		#DEFINE DETAIL ARRAY
		$DETAIL_ARRAY = array('local' => new saasame\transport\local_folder($LOCAL_ARRAY),'remote' => new saasame\transport\network_folder($NETWORK_ARRAY));	
				
		#CONSTRUCTION CONNECTION ARRAY
		$CONN_ARRAY = array('type' 			=> $CONN_CODE,
							'id' 			=> $CONN_UUID,
							'compressed' 	=> $IS_COMPRESSED, 
							'checksum' 		=> $IS_CHECKSUM,
							'encrypted' 	=> $IS_ENCRYPTED,
							'detail' 		=> new saasame\transport\_detail($DETAIL_ARRAY));
						
		#TRANSFORM TO THRIFT OBJECT
		$CONN_DETAIL = new saasame\transport\connection($CONN_ARRAY);
		
		//Misc_Class::function_debug('_mgmt',__FUNCTION__,$CONN_DETAIL);
		
		#DEFINE TRANSPORT CLIENT
		$PORT_NUM = \saasame\transport\Constant::get('TRANSPORT_SERVICE_PORT');		
		$OpenProtocol = $this -> common_connection('127.0.0.1',$PORT_NUM);
		$ClientCall = new saasame\transport\transport_serviceClient($OpenProtocol);	
				
		if ($CONN_TYPE == 'LocalFolder')
		{
			$CARRIER_SERV_TYPE = \saasame\transport\Constant::get('CARRIER_SERVICE');
			$LOADER_SERV_TYPE = \saasame\transport\Constant::get('LOADER_SERVICE');
		
			for ($i=0; $i<count($SERV_ADDR); $i++)
			{
				try{
					if ($INIT_TYPE == 'Test')
					{
						$LOCAL_CONNECTION = $ClientCall -> test_connection_p($SERV_ADDR[$i],$CONN_DETAIL,$CARRIER_SERV_TYPE);
					}
					else
					{
						$LOCAL_CONNECTION = $ClientCall -> add_connection_p($SERV_ADDR[$i],$CONN_DETAIL,$CARRIER_SERV_TYPE);
					}
				
					if ($LOCAL_CONNECTION == TRUE)
					{
						$ClientCall -> add_connection_p($SERV_ADDR[$i],$CONN_DETAIL,$LOADER_SERV_TYPE);
						$LOCAL_CONNECTION = $CONN_ARRAY;
						break;
					}
				}				
				catch (Throwable $e){
					$e -> address = $SERV_ADDR[$i];
					Misc_Class::function_debug('_mgmt',__FUNCTION__,$e);
					$LOCAL_CONNECTION = false;
				}
			}
			return $LOCAL_CONNECTION;			
		}
		else
		{			
			#CREATE WEBDAV CONNECTION
			if ($SOURCE_DIRECT_MODE == TRUE)
			{
				$SERV_TYPE = \saasame\transport\Constant::get('LOADER_SERVICE');				
			}
			else
			{
				if ($TARGET_DIRECT_MODE == TRUE)
				{
					$SERV_TYPE = \saasame\transport\Constant::get('CARRIER_SERVICE');
				}
				else
				{
					$SERV_TYPE = \saasame\transport\Constant::get('LOADER_SERVICE');
				}
			}

			for ($x=0; $x<count($SERV_ADDR); $x++)
			{
				try{
					if ($INIT_TYPE == 'Test')
					{
						$REMOTE_CONNECTION = $ClientCall -> test_connection_p($SERV_ADDR[$x],$CONN_DETAIL,$SERV_TYPE);
					}
					else
					{
						$REMOTE_CONNECTION = $ClientCall -> add_connection_p($SERV_ADDR[$x],$CONN_DETAIL,$SERV_TYPE);
					}
					
					if ($REMOTE_CONNECTION == TRUE)
					{
						$REMOTE_CONNECTION = $CONN_ARRAY;
						break;
					}
				}
				catch (Throwable $e){
					$e -> address = $SERV_ADDR[$x];
					Misc_Class::function_debug('_mgmt',__FUNCTION__,$e);
					$REMOTE_CONNECTION = false;
				} 
			}
			return $REMOTE_CONNECTION;
		}
	}
	
	###########################
	#REFLUSH VIRTUAL PACKER DISK
	###########################
	private function reflush_virtual_packer_disk($HOST_UUID,$DISK_INFO)
	{
		#ADD NEW DISK
		for ($i=0; $i<count($DISK_INFO); $i++)		
		{
			$DISK_UUID = $DISK_INFO[$i] -> id;
			$DISK_SIZE = $DISK_INFO[$i] -> size;
			$DISK_NAME = $DISK_INFO[$i] -> name;
			
			#$NEW_VIRTUAL_DISK = "SELECT * FROM _SERVER_DISK WHERE _DISK_UUID = '".$DISK_UUID."' AND _DISK_SIZE = '".$DISK_SIZE."' AND _STATUS = 'Y'";
			$NEW_VIRTUAL_DISK = "SELECT * FROM _SERVER_DISK WHERE _DISK_UUID = '".$DISK_UUID."' AND _STATUS = 'Y'";
		
			$NEW_QUERY = $this -> DBCON -> prepare($NEW_VIRTUAL_DISK);
			$NEW_QUERY -> execute();
			$NEW_DISK_COUNT = $NEW_QUERY -> rowCount();
			
			unset($NEW_DISK); #UNSET DISK ARRAY
			if ($NEW_DISK_COUNT == 0)
			{
				$NEW_DISK[] = (object)array(
										'name' 		=> $DISK_INFO[$i] -> name,
										'id' 		=> $DISK_INFO[$i] -> id,
										'size_kb' 	=> $DISK_INFO[$i] -> size_kb,
										'size' 		=> $DISK_INFO[$i] -> size);
				
				$this -> ServerMgmt -> new_initialize_disk($HOST_UUID,$NEW_DISK,'Virtual Packer');
			}
			else
			{
				$UPDATE_VIRTUAL_DISK = "UPDATE
											_SERVER_DISK
										SET
											_DISK_NAME = '".$DISK_NAME."',
											_DISK_SIZE = '".$DISK_SIZE."',
											_DISK_URI  = '".$DISK_UUID."',
											_TIMESTAMP = '".Misc_Class::current_utc_time()."'											
										WHERE
											_DISK_UUID = '".$DISK_UUID."' AND
											_STATUS    = 'Y'";
				$this -> DBCON -> prepare($UPDATE_VIRTUAL_DISK) -> execute();
			}
		}
		
		#DELETE DISK
		$DEL_VIRTUAL_DISK = "SELECT * FROM _SERVER_DISK WHERE _HOST_UUID = '".$HOST_UUID."' AND _STATUS = 'Y'";
		$DEL_QUERY = $this -> DBCON -> prepare($DEL_VIRTUAL_DISK);
		$DEL_QUERY -> execute();
		$DISK_COUNT = $DEL_QUERY -> rowCount();
		
		if ($DISK_COUNT != 0)
		{
			foreach($DEL_QUERY as $QueryResult)
			{
				$UUID_DISK[] = $QueryResult['_DISK_UUID'];
			}
			for ($x=0; $x<count($DISK_INFO); $x++)
			{
				$_UUID_DISK[] = strtoupper($DISK_INFO[$x] -> id);
			}
						
			$TO_BE_DEL = array_values(array_diff($UUID_DISK,$_UUID_DISK));
			
			$COUNT_DEL = count($TO_BE_DEL);
			for ($p=0; $p<$COUNT_DEL; $p++)
			{
				$MARK_DELETE = "UPDATE
									_SERVER_DISK
								SET
									_TIMESTAMP = '".Misc_Class::current_utc_time()."',
									_STATUS = 'X'
								WHERE
									_DISK_UUID = '".$TO_BE_DEL[$p]."'";
									
				$this -> DBCON -> prepare($MARK_DELETE) -> execute();
			}
		}
	}
	
	###########################
	#REFLUSH SELECT VIRTUAK HOST
	###########################
	private function reflush_select_virtual_host_disk($PACK_UUID,$REPL_UUID=null)
	{
		#GET HOST INFORMATION
		$HOST_INFO = $this -> ServerMgmt -> query_host_info($PACK_UUID);

		$HOST_TYPE = $HOST_INFO['HOST_TYPE'];
		if ($HOST_TYPE == 'Virtual')
		{
			if ($REPL_UUID != null)
			{
				#MESSAGE
				$MESSAGE = $this -> ReplMgmt -> job_msg('Reflush disks information for selected virtual host.');
				$this -> ReplMgmt -> update_job_msg($REPL_UUID,$MESSAGE,'Replica');
			}
			
			#GET SERVER INFORMATION
			$SERV_INFO = $HOST_INFO['HOST_SERV'];
			
			#ADDRESS ARRAY
			$ADDR_ARRAY = $SERV_INFO['SERV_ADDR'];
			
			#GET SELECT VM-GUEST INFORMATION
			for ($i=0; $i<count($ADDR_ARRAY); $i++)
			{
				try{
					$VM_INFO = $this -> virtual_vms_info($ADDR_ARRAY[$i],$SERV_INFO['SERV_MISC']['ADDR'],$SERV_INFO['SERV_MISC']['USER'],$SERV_INFO['SERV_MISC']['PASS'],$HOST_INFO['HOST_INFO']['uuid']);
				}
				catch (Exception $e) {
					
				}			
			}
			
			if (isset($VM_INFO -> disks))
			{
				$this -> reflush_virtual_packer_disk($HOST_INFO['HOST_UUID'],$VM_INFO -> disks);
				//return $VM_INFO -> disks;
			}			
		}
	}
	
	###########################
	#SORT DISK BY KEY
	###########################
	private function usort_disk_by_key($KEY_A,$KEY_B)
	{
		return strcmp($KEY_A->key, $KEY_B->key);
	}
	
	###########################
	#UNIQUE MULTIDIM ARRAY
	###########################
	private function unique_multidim_array($SET_ARRAY, $KEY)
	{
		$TEMP_ARRAY = array();
		$KEY_ARRAY = array();
		$i = 0;
		
		foreach($SET_ARRAY as $VALUE)
		{
			if (!in_array($VALUE[$KEY], $KEY_ARRAY))
			{
				$KEY_ARRAY[$i] = $VALUE[$KEY];
				$TEMP_ARRAY[$i] = $VALUE;
			}
			$i++;
		}
		return $TEMP_ARRAY;
	}
	
	###########################
	#REFLUSH VIRTUAL HOST
	###########################
	public function reflush_virtual_packer($ACCT_UUID,$HOST_UUID)
	{
		if ($HOST_UUID != '')
		{
			$GET_VIRTUAL_HOST = "SELECT * FROM _SERVER_HOST WHERE _ACCT_UUID = '".$ACCT_UUID."' AND _HOST_UUID = '".$HOST_UUID."' AND _SERV_TYPE = 'Virtual' AND _STATUS = 'Y'";
		}
		else
		{
			$GET_VIRTUAL_HOST = "SELECT * FROM _SERVER_HOST WHERE _ACCT_UUID = '".$ACCT_UUID."' AND _SERV_TYPE = 'Virtual' AND _STATUS = 'Y'";
		}

		$HOST_QUERY = $this -> DBCON -> prepare($GET_VIRTUAL_HOST);
		$HOST_QUERY -> execute();
		$HOST_ROWS = $HOST_QUERY -> rowCount();
		
		if ($HOST_ROWS != 0)
		{
			foreach($HOST_QUERY as $QueryResult)
			{
				$SERV_UUID = $QueryResult['_SERV_UUID'];
				$HOST_UUID = $QueryResult['_HOST_UUID'];
				$HOST_ADDR = $QueryResult['_HOST_ADDR'];
				$HOST_INFO = json_decode($QueryResult['_HOST_INFO']);

				$GET_VIRTUAL_PACKER = "SELECT * FROM _SERVER WHERE _ACCT_UUID = '".$ACCT_UUID."' AND _SERV_UUID = '".$SERV_UUID."' AND _SERV_TYPE = 'Virtual Packer' AND _STATUS = 'Y'";
				$SERV_QUERY = $this -> DBCON -> prepare($GET_VIRTUAL_PACKER);
				$SERV_QUERY -> execute();
				$SERV_ROWS = $SERV_QUERY -> rowCount();
		


				if ($SERV_ROWS != 0)
				{
					foreach($SERV_QUERY as $ResultQuery)
					{
						#ESX LOGIN INFORMATION
						$SERV_MISC = json_decode(Misc_Class::encrypt_decrypt('decrypt',$ResultQuery['_SERV_MISC']),FALSE);
						$TRANSPORT_ADDR = $this -> ServerMgmt -> query_server_info($ResultQuery['_HOST_UUID'])['SERV_ADDR'];
						$HYPERVISOR_ADDR = $SERV_MISC -> ADDR;
						$HYPERVISOR_USER = $SERV_MISC -> USER;
						$HYPERVISOR_PASS = $SERV_MISC -> PASS;
				
						#GET VM HOST INFORMATION
						for ($i=0; $i<count($TRANSPORT_ADDR); $i++)
						{
							$VM_INFO = $this -> virtual_vms_info($TRANSPORT_ADDR[$i],$HYPERVISOR_ADDR,$HYPERVISOR_USER,$HYPERVISOR_PASS,$HOST_UUID);
							if ($VM_INFO != FALSE)
							{
								#REMOVE ANNOTATION OBJECT
								unset($VM_INFO -> annotation);
								
								#HOSTNAME
								$HOST_NAME = $VM_INFO -> name;
								
								#GET GUEST IP ADDRESS
								if ($VM_INFO -> guest_ip != '')
								{
									$GUEST_ADDR = $VM_INFO -> guest_ip; 
								}
								else
								{
									$GUEST_ADDR = 'None';
								}

								#HOST METADATA
								$VM_INFO -> priority_addr = '127.0.0.1';
								$VM_INFO -> direct_mode = true;								
								$VM_INFO -> manufacturer = 'VMware, Inc.';
							
								#KEEP DISK PARTITION INFOS
								if (isset($HOST_INFO -> disk_partition_infos))
								{
									$VM_INFO -> disk_partition_infos = $HOST_INFO -> disk_partition_infos;
								}						
							
								#JSON ENCODE
								$HOST_INFO = addslashes(json_encode($VM_INFO,JSON_UNESCAPED_UNICODE));
								
								#UPDATE VMS INFORMATION
								$UPDATE_VMS = "UPDATE
													_SERVER_HOST
												SET
													_HOST_NAME = '".addslashes($HOST_NAME)."',
													_HOST_ADDR = '".$GUEST_ADDR."',
													_HOST_INFO = '".$HOST_INFO."',
													_TIMESTAMP = '".Misc_Class::current_utc_time()."'
												WHERE
													_HOST_UUID = '".$HOST_UUID."' AND
													_STATUS    = 'Y'";
													
								$this -> DBCON -> prepare($UPDATE_VMS) -> execute();
								
								#REFLUSH SERVER DISK INFORMATION
								$VM_DISK = $VM_INFO -> disks;
								usort($VM_DISK, array($this,'usort_disk_by_key'));	
								$this -> reflush_virtual_packer_disk($HOST_UUID,$VM_DISK);
								
								break;
							}
						}
					}
				}
			}
		}
		return true;	
	}
	
	###########################
	#REFLUSH VIRTUAL PACKER DISK
	###########################
	private function reflush_physical_packer_disk($HOST_UUID,$DISK_INFO)
	{
		#ADD NEW DISK
		for ($i=0; $i<count($DISK_INFO); $i++)		
		{
			$DISK_NAME = $DISK_INFO[$i] -> friendly_name;
			$DISK_SIZE = $DISK_INFO[$i] -> size;
			$DISK_URI  = $DISK_INFO[$i] -> uri;
			$BUS_TYPE  = $DISK_INFO[$i] -> bus_type;
			
			#$ADD_DISK  = "SELECT * FROM _SERVER_DISK WHERE _DISK_SIZE = '".$DISK_SIZE."' AND _DISK_URI = '".$DISK_URI."' AND _STATUS = 'Y'";
			$ADD_DISK  = "SELECT * FROM _SERVER_DISK WHERE _DISK_URI = '".$DISK_URI."' AND _STATUS = 'Y'";
			
			$NEW_QUERY = $this -> DBCON -> prepare($ADD_DISK);
			$NEW_QUERY -> execute();
			$NEW_DISK_COUNT = $NEW_QUERY -> rowCount();
		
			unset($INIT_DISK); #RESET DISK ARRAY
			if ($NEW_DISK_COUNT == 0)
			{
				$INIT_DISK[] = (object)array(
										'friendly_name' 	=> $DISK_NAME,
										'size' 				=> $DISK_SIZE,
										'uri' 				=> $DISK_URI,
										'bus_type'			=> $BUS_TYPE);
				
				$this -> ServerMgmt -> new_initialize_disk($HOST_UUID,$INIT_DISK,'Physical Packer');
			}
			else
			{
				$UPDATE_DISK = "UPDATE
									_SERVER_DISK
								SET
									_DISK_NAME = '".$DISK_NAME."',
									_DISK_SIZE = '".$DISK_SIZE."',
									_DISK_URI  = '".$DISK_URI."',
									_TIMESTAMP = '".Misc_Class::current_utc_time()."'											
								WHERE
									_DISK_URI = '".$DISK_URI."' AND
									_STATUS    = 'Y'";
				$this -> DBCON -> prepare($UPDATE_DISK) -> execute();
			}
		}
	
		#DELETE DISK
		$DEL_DISK = "SELECT * FROM _SERVER_DISK WHERE _HOST_UUID = '".$HOST_UUID."' AND _STATUS = 'Y'";
		$DEL_QUERY = $this -> DBCON -> prepare($DEL_DISK);
		$DEL_QUERY -> execute();
		$DISK_COUNT = $DEL_QUERY -> rowCount();
		
		if ($DISK_COUNT != 0)
		{
			foreach($DEL_QUERY as $QueryResult)
			{
				$URI_DISK[] = $QueryResult['_DISK_URI'];
			}
			for ($x=0; $x<count($DISK_INFO); $x++)
			{
				$_URI_DISK[] = $DISK_INFO[$x] -> uri;
			}
			$TO_BE_DEL = array_values(array_diff($URI_DISK,$_URI_DISK));
			
			$COUNT_DEL = count($TO_BE_DEL);
			for ($w=0; $w<$COUNT_DEL; $w++)
			{
				$MARK_DELETE = "UPDATE
									_SERVER_DISK
								SET
									_TIMESTAMP = '".Misc_Class::current_utc_time()."',
									_STATUS = 'X'
								WHERE
									_HOST_UUID = '".$HOST_UUID."' AND
									_DISK_URI  = '".$TO_BE_DEL[$w]."'";
				$this -> DBCON -> prepare($MARK_DELETE) -> execute();
			}
		}
	}
	
	###########################
	#REFLUSH PHYSICAL PACKER
	###########################
	public function reflush_physical_packer($ACCT_UUID,$HOST_UUID)
	{
		if ($HOST_UUID != '')
		{
			$GET_PHYSICAL_EXEC = "SELECT * FROM _SERVER_HOST WHERE _ACCT_UUID = '".$ACCT_UUID."' AND _HOST_UUID = '".$HOST_UUID."' AND _SERV_TYPE = 'Physical' AND _STATUS = 'Y'";
		}
		else
		{
			$GET_PHYSICAL_EXEC = "SELECT * FROM _SERVER_HOST WHERE _ACCT_UUID = '".$ACCT_UUID."' AND _SERV_TYPE = 'Physical' AND _STATUS = 'Y'";
		}
		$QUERY = $this -> DBCON -> prepare($GET_PHYSICAL_EXEC);
		$QUERY -> execute();
		$COUNT_ROWS = $QUERY -> rowCount();
	
		if ($COUNT_ROWS != 0)
		{
			foreach($QUERY as $QueryResult)
			{
				$SERV_UUID = $QueryResult['_SERV_UUID'];
				$HOST_UUID = $QueryResult['_HOST_UUID'];
				
				$HOST_INFO = json_decode($QueryResult['_HOST_INFO']);
				
				if ($HOST_INFO -> direct_mode == TRUE)
				{				
					$HOST_ADDR = $QueryResult['_HOST_ADDR'];
				}
				else
				{
					$HOST_ADDR = $HOST_INFO -> machine_id;
				}
				
				#GET PHYSICAL HOST INFORMATION
				$SERV_INFO = $this -> ServerMgmt -> query_server_info($SERV_UUID);
			
				if ($SERV_INFO['SERV_INFO']['direct_mode'] == TRUE)
				{
					$SERV_ADDR = $SERV_INFO['SERV_ADDR'];
				}
				else
				{
					$SERV_ADDR = array($SERV_INFO['SERV_INFO']['machine_id']);
				}
				
				for ($i=0; $i<count($SERV_ADDR); $i++)
				{		
					$REFLUSH_HOST_INFO = $this -> physical_host_info($SERV_ADDR[$i],$HOST_ADDR);
				}
				
				if ($REFLUSH_HOST_INFO != FALSE)
				{
					$HOST_NAME = $REFLUSH_HOST_INFO -> client_name;
					$DISK_INFO = $REFLUSH_HOST_INFO -> disk_infos;
					
					#REPLACE/APPEND QUERY HOST INFORMATION
					$REFLUSH_HOST_INFO -> manufacturer = $HOST_INFO -> manufacturer;
					$REFLUSH_HOST_INFO -> priority_addr = $HOST_INFO -> priority_addr;
					$REFLUSH_HOST_INFO -> direct_mode = $HOST_INFO -> direct_mode;
					$REFLUSH_HOST_INFO -> instance_id = $HOST_INFO -> instance_id;
					$REFLUSH_HOST_INFO -> availability_zone = $HOST_INFO -> availability_zone;
					$REFLUSH_HOST_INFO -> reg_cloud_uuid = $HOST_INFO -> reg_cloud_uuid;
					
					if (isset(json_decode($QueryResult['_HOST_INFO']) -> instance_id)) #FOR CLOUD CLIENT USE ONLY
					{
						$INSTANCE_ID 		= json_decode($QueryResult['_HOST_INFO']) -> instance_id;
						$AVAILABILITY_ZONE 	= json_decode($QueryResult['_HOST_INFO']) -> availability_zone;
						$REG_CLOUD_UUID		= json_decode($QueryResult['_HOST_INFO']) -> reg_cloud_uuid;
						$INSTANCE_INFO = array('instance_id' => $INSTANCE_ID,'availability_zone' => $AVAILABILITY_ZONE,'reg_cloud_uuid' => $REG_CLOUD_UUID);

						$REFLUSH_HOST_INFO = (object)array_merge((array)$REFLUSH_HOST_INFO,$INSTANCE_INFO);
					}
					
					if ($HOST_INFO -> direct_mode == TRUE)
					{
						#GET FROM DATABASE
						$NIC_ADDR = $QueryResult['_HOST_ADDR'];
					}
					else
					{
						$NETWORK_INFO = $REFLUSH_HOST_INFO -> network_infos;
						for($i=0; $i<count($NETWORK_INFO); $i++)
						{
							$NIC_ADDR[] = $NETWORK_INFO[$i] -> ip_addresses;
						}				
						$NIC_ADDR = call_user_func_array('array_merge', $NIC_ADDR);
						
						#FILTER OUT MS DEFAULT IP ADDRESS
						foreach ($NIC_ADDR as $Key => $Addr)
						{
							if (strpos($Addr, '169.254') !== false)
							{
								unset($NIC_ADDR[$Key]);
							}
						}
						
						$NIC_ADDR = implode(',',$NIC_ADDR);						
					}
					
					$HOST_JSON = str_replace('\\','\\\\',json_encode($REFLUSH_HOST_INFO,JSON_UNESCAPED_UNICODE));
			
					$UPDATE_HOST = "UPDATE
										_SERVER_HOST
									SET
										_HOST_ADDR = '".$NIC_ADDR."',
										_HOST_NAME = '".$HOST_NAME."',
										_HOST_INFO = '".$HOST_JSON."',
										_TIMESTAMP = '".Misc_Class::current_utc_time()."'
									WHERE
										_HOST_UUID = '".$HOST_UUID."'";

					$this -> DBCON -> prepare($UPDATE_HOST) -> execute();
					$this -> reflush_physical_packer_disk($HOST_UUID,$DISK_INFO);					
				}
			}			
		}
	}
	
	/*
	###########################
	#MODIFY CONNECTION
	###########################
	public function modify_connection($ADDRESS,$CONNECTION_ID,$USER_OPTION)
	{
		$ConnectionConfig = array(
									'type' 		=> 0,
									'id' 		=> $CONNECTION_ID,
									'checksum'	=> $USER_OPTION['is_checksum']
								);
		
		$PORT_NUM = \saasame\transport\Constant::get('TRANSPORT_SERVICE_PORT');
		
		$OpenProtocol = $this -> transport_service('127.0.0.1',$PORT_NUM);
		
		$ClientCall = new saasame\transport\transport_serviceClient($OpenProtocol);
		
		$CONNECTION = new saasame\transport\Connection($ConnectionConfig);
		
		$SERV_TYPE = \saasame\transport\Constant::get('LOADER_SERVICE');
		
		try
		{
			return $ClientCall -> modify_connection_p($ADDRESS,$CONNECTION,$SERV_TYPE);
		}
		catch (Throwable $e)
		{
			return false;
		}
	}
	*/
	
	###########################
	#	SET DISK CUSTOMIZED ID
	###########################
	public function set_disk_customized_id($SERV_ADDR,$DISK_ADDR,$SET_UUID)
	{
		$PORT_NUM = \saasame\transport\Constant::get('TRANSPORT_SERVICE_PORT');
		
		$OpenProtocol = $this -> common_connection('127.0.0.1',$PORT_NUM);
		
		$ClientCall = new saasame\transport\transport_serviceClient($OpenProtocol);	

		try{
			for ($i=0; $i<count($SERV_ADDR); $i++)
			{
				$Status = $ClientCall -> set_customized_id_p($SERV_ADDR[$i],$DISK_ADDR,$SET_UUID);
				if ($Status == TRUE)
				{
					break;
				}
			}
			return $Status;
		}
		catch (Throwable $e){
			return false;
		}
	}
	
	###########################
	#	ENUMERATE DISKS
	###########################
	public function enumerate_disks($SERV_ADDR)
	{
		$PORT_NUM = \saasame\transport\Constant::get('TRANSPORT_SERVICE_PORT');
		
		$OpenProtocol = $this -> common_connection('127.0.0.1',$PORT_NUM);
		
		$ClientCall = new saasame\transport\transport_serviceClient($OpenProtocol);	

		for ($i=0; $i<count($SERV_ADDR); $i++)
		{
			try{
				$Status = $ClientCall -> enumerate_disks_p($SERV_ADDR[$i],0);
				if ($Status != FALSE)
				{
					break;
				}
			}
			catch (Throwable $e){
				$Status = false;
			}		
		}
		return $Status;
	}
	
	###########################
	#	LIST DISK ADDR INFO
	###########################
	public function list_disk_addr_info($SERV_UUID,$SERV_ADDR,$SERV_TYPE,$OS_TYPE,$HOST_TYPE)
	{
		#FLOCK LOCK
		#Misc_Class::flock_action($SERV_UUID,'LOCK_EX');
	
		#LIST ALL DISKS
		$DISK_ARRAY = $this -> enumerate_disks($SERV_ADDR,0);

		Misc_Class::function_debug('_mgmt',__FUNCTION__,$DISK_ARRAY);
	
		#LOOP VIA DISKS
		for ($i=0; $i<count($DISK_ARRAY); $i++)
		{
			switch ($SERV_TYPE)
			{
				case "Loader":		
					switch ($OS_TYPE)
					{
						case 'MS':
							$DISK_INFO = json_decode($DISK_ARRAY[$i] -> uri);
								
							$DISK_SCSI_ADDRESS  = $DISK_INFO -> address;
							$DISK_UNIQUE_ID 	= $DISK_INFO -> unique_id;
							$DISK_SERIAL_NUMBER = $DISK_INFO -> serial_number;
							$DISK_CUSTOMIZED_ID = trim($DISK_ARRAY[$i] -> customized_id);
						
							#DISK QUERY MODE
							$QUERY_MODE = Misc_Class::enumerate_disks_action_parameter();
							if ($QUERY_MODE == TRUE)
							{
								$DISK_ADDR_INFO = $DISK_SCSI_ADDRESS;
							}
							else
							{
								if (strlen($DISK_SERIAL_NUMBER) == 20)
								{
									$DISK_ADDR_INFO = $DISK_SERIAL_NUMBER;
								}
								/*elseif (strlen($DISK_UNIQUE_ID) == 32)
								{
									$DISK_ADDR_INFO = $DISK_UNIQUE_ID;
								}*/
								elseif (strlen($DISK_CUSTOMIZED_ID) == 36 AND $DISK_CUSTOMIZED_ID != '00000000-0000-0000-0000-000000000000')
								{
									$DISK_ADDR_INFO = $DISK_CUSTOMIZED_ID;
								}
								else
								{
									$DISK_ADDR_INFO = $DISK_SCSI_ADDRESS;
								}
							}
							$DISK_ADDR[] = $DISK_ADDR_INFO;
						break;
						
						case 'RCD':
							$DISK_BUS_TYPE = $DISK_ARRAY[$i] -> bus_type;
							$REAL_DISK_SIZE = $DISK_ARRAY[$i] -> size;
								
							if ($DISK_BUS_TYPE == 1 or $DISK_BUS_TYPE == 2 or $DISK_BUS_TYPE == 3 or $DISK_BUS_TYPE == 6 or $DISK_BUS_TYPE == 8 or $DISK_BUS_TYPE == 9 or $DISK_BUS_TYPE == 10 or $DISK_BUS_TYPE == 11 or $DISK_BUS_TYPE == 16 or $DISK_BUS_TYPE == 17)
							{
								if ($HOST_TYPE == 'Virtual')
								{
									$USED_DISK_SIZE = $REAL_DISK_SIZE;
								}
								else
								{							
									$DISK_SIZE = $REAL_DISK_SIZE / 1024 / 1024 / 1024;
									if (is_float($DISK_SIZE))
									{
										$DISK_SIZE = ceil($DISK_SIZE);	
									}
									else
									{	
										$DISK_SIZE = $DISK_SIZE + 1;
									}
									
									$USED_DISK_SIZE = $DISK_SIZE * 1024 * 1024 * 1024;
								}
									
								$DISK_INFO = json_decode($DISK_ARRAY[$i] -> uri);
								$DISK_SCSI_ADDRESS  = $DISK_INFO -> address;
								$DISK_SERIAL_NUMBER = $DISK_INFO -> serial_number;
									
								$DISK_ADDR[$DISK_SCSI_ADDRESS] = $USED_DISK_SIZE;
							}							
						break;
					}
				break;
				
				case "Launcher":		
					switch ($OS_TYPE)
					{
						case 'MS':
							MS:
							$DISK_INFO = json_decode($DISK_ARRAY[$i] -> uri);
							$DISK_UNIQUE_ID 	= $DISK_INFO -> unique_id;
							$DISK_SCSI_ADDRESS  = $DISK_INFO -> address;
							$DISK_SERIAL_NUMBER = $DISK_INFO -> serial_number;
							$DISK_CUSTOMIZED_ID = trim($DISK_ARRAY[$i] -> customized_id);
								
							#DISK QUERY MODE
							$QUERY_MODE = Misc_Class::enumerate_disks_action_parameter();
							if ($QUERY_MODE == TRUE)
							{
								$DISK_ADDR_INFO = $DISK_SCSI_ADDRESS;								
							}
							else
							{
								if (strlen($DISK_SERIAL_NUMBER) == 20)
								{
									$DISK_ADDR_INFO = $DISK_SERIAL_NUMBER;
								}
								/*elseif (strlen($DISK_UNIQUE_ID) == 32)
								{
									$DISK_ADDR_INFO = $DISK_UNIQUE_ID;
								}*/
								elseif (strlen($DISK_CUSTOMIZED_ID) == 36 AND $DISK_CUSTOMIZED_ID != '00000000-0000-0000-0000-000000000000')
								{
									$DISK_ADDR_INFO = $DISK_CUSTOMIZED_ID;
								}
								else
								{
									$DISK_ADDR_INFO = $DISK_SCSI_ADDRESS;
								}
							}
							
							$DISK_ADDR[] = $DISK_ADDR_INFO;
						break;
					
						case 'LX':
							if (strpos($DISK_ARRAY[$i] -> location, 'PCIROOT') !== false || strpos($DISK_ARRAY[$i] -> location, 'PCI Slot') !== false)
							{
								goto MS; #JUMP TO LAUNCHER - MS SWITCH CASE (FOR EMBEDDED LINUX)
							}
							else if (strpos($DISK_ARRAY[$i] -> path, '\\\\?\\ide') !== false || strpos($DISK_ARRAY[$i] -> path, '\\\\?\\scsi') !== false)
							{
								goto MS; #JUMP TO LAUNCHER - MS SWITCH CASE (FOR EMBEDDED LINUX)
							}
							else
							{
								$DISK_ADDR[] = $DISK_ARRAY[$i] -> location;
							}
						break;
						
						default:
							$DISK_ADDR[] = $DISK_ARRAY[$i] -> size;
					}							
				break;	
			}
		}
		
		#FLOCK UNLOCK
		#Misc_Class::flock_action($SERV_UUID,'LOCK_UN');
		
		#FOR DEBUG
		#Misc_Class::function_debug('',__FUNCTION__,$DISK_ADDR);
		
		return $DISK_ADDR;			
	}
	
	###########################
	#	ENUMERATE CONNECTION
	###########################
	public function enumerate_connection($CARR_ADDR)
	{
		$PORT_NUM = \saasame\transport\Constant::get('TRANSPORT_SERVICE_PORT');
		
		$OpenProtocol = $this -> common_connection('127.0.0.1',$PORT_NUM);
		
		$ClientCall = new saasame\transport\transport_serviceClient($OpenProtocol);
		
		$SERVICE_TYPE = \saasame\transport\Constant::get('CARRIER_SERVICE');
		
		try{
			return $ClientCall -> enumerate_connections_p($CARR_ADDR,$SERVICE_TYPE);
		}
		catch (Throwable $e){
			return false;
		}
	}
	
	###########################
	#	UNREGISTER PACKER
	###########################
	public function unregister_packer($MACHINE_UUID)
	{
		$PORT_NUM = \saasame\transport\Constant::get('TRANSPORT_SERVICE_PORT');
	
		$OpenProtocol = $this -> common_connection('127.0.0.1',$PORT_NUM);
		
		$ClientCall = new saasame\transport\transport_serviceClient($OpenProtocol);
		
		try{
			return $ClientCall -> unregister_packer_p($MACHINE_UUID);
		}
		catch (Throwable $e){
			return false;
		}
	}
	
	###########################
	#	UNREGISTER SERVER
	###########################
	public function unregister_server($MACHINE_UUID)
	{
		$PORT_NUM = \saasame\transport\Constant::get('TRANSPORT_SERVICE_PORT');
	
		$OpenProtocol = $this -> common_connection('127.0.0.1',$PORT_NUM);
		
		$ClientCall = new saasame\transport\transport_serviceClient($OpenProtocol);
		
		try{
			return $ClientCall -> unregister_server_p($MACHINE_UUID);
		}
		catch (Throwable $e){
			return false;
		}
	}
	
	###########################
	#LIST SERVICE JOBS
	###########################
	public function list_service($ACCT_UUID = null,$SERV_UUID = null)
	{
		if ($ACCT_UUID == null)
		{
			$GET_SERV	= "SELECT * FROM _SERVICE WHERE _SERV_UUID = '".$SERV_UUID."' AND _STATUS != 'X'";
		}
		elseif ($SERV_UUID == null)
		{
			$GET_SERV 	= "SELECT * FROM _SERVICE WHERE _ACCT_UUID = '".$ACCT_UUID."' AND _STATUS != 'X'";
		}
		
		$QUERY = $this -> DBCON -> prepare($GET_SERV);
		$QUERY -> execute();
		$COUNT_ROWS = $QUERY -> rowCount();
		
		if ($COUNT_ROWS != 0)
		{
			foreach($QUERY as $QueryResult)
			{
				#GET SERVICE HISTORY MESSAGE
				$HISTORY_MSG = $this -> ReplMgmt -> list_job_history($QueryResult['_SERV_UUID'],10);
					
				$SERV_MESG 		= $HISTORY_MSG[0]['description'];					
				$SYNC_TIME 		= $HISTORY_MSG[0]['time'];
				$SERV_TIME 		= $HISTORY_MSG[0]['unsync_time'];
				$SERV_FORMAT 	= $HISTORY_MSG[0]['format'];
				$SERV_ARGUMENTS = $HISTORY_MSG[0]['arguments'];
				
				#GET HOST INFORMATION
				$HOST_INFO = $this -> ServerMgmt -> query_host_info($QueryResult['_PACK_UUID']);
				
				#JOBS JSON DECODE
				$JOBS_JSON = json_decode($QueryResult['_JOBS_JSON'],false);
				
				#LIST SERVICE JOB INFORMATION
				if ($ACCT_UUID == null)
				{
					
										
					$SERV_DATA = array(
									"ACCT_UUID" 		=> $QueryResult['_ACCT_UUID'],
									"REGN_UUID" 		=> $QueryResult['_REGN_UUID'],
									"SERV_UUID" 		=> $QueryResult['_SERV_UUID'],
									"REPL_UUID" 		=> $QueryResult['_REPL_UUID'],
									"SNAP_JSON" 		=> $QueryResult['_SNAP_JSON'],
									"PACK_UUID"			=> $QueryResult['_PACK_UUID'],
									"HOST_NAME" 		=> $HOST_INFO['HOST_NAME'],
									"HOST_ADDR" 		=> $HOST_INFO['HOST_ADDR'],
									"CONN_UUID" 		=> $QueryResult['_CONN_UUID'],
									"CLUSTER_UUID" 		=> $QueryResult['_CLUSTER_UUID'],
									"FLAVOR_ID"			=> $QueryResult['_FLAVOR_ID'],
									"NETWORK_UUID" 		=> $QueryResult['_NETWORK_UUID'],
									"SGROUP_UUID" 		=> $QueryResult['_SGROUP_UUID'],
									"NOVA_VM_UUID" 		=> $QueryResult['_NOVA_VM_UUID'],
									"ADMIN_PASS" 		=> $QueryResult['_ADMIN_PASS'],
									"JOBS_JSON"			=> $JOBS_JSON,								
									"WINPE_JOB"			=> $JOBS_JSON -> winpe_job,
									"OS_TYPE"			=> $QueryResult['_OS_TYPE'],
									"SERV_MESG"			=> $SERV_MESG,
									"SERV_FORMAT"		=> $SERV_FORMAT,
									"SERV_ARGUMENTS"	=> $SERV_ARGUMENTS,
									"SYNC_TIME"			=> $SYNC_TIME,
									"SERV_TIME"			=> $SERV_TIME,
									"TIMESTAMP"			=> $QueryResult['_TIMESTAMP'],
									"STATUS"			=> $QueryResult['_STATUS']
								);
				}
				else
				{	
					$SERV_DATA[] = array(
									"ACCT_UUID" 		=> $QueryResult['_ACCT_UUID'],
									"REGN_UUID" 		=> $QueryResult['_REGN_UUID'],
									"SERV_UUID" 		=> $QueryResult['_SERV_UUID'],
									"REPL_UUID" 		=> $QueryResult['_REPL_UUID'],
									"SNAP_JSON" 		=> $QueryResult['_SNAP_JSON'],
									"PACK_UUID"			=> $QueryResult['_PACK_UUID'],
									"HOST_NAME" 		=> $HOST_INFO['HOST_NAME'],
									"HOST_ADDR" 		=> $HOST_INFO['HOST_ADDR'],
									"CONN_UUID" 		=> $QueryResult['_CONN_UUID'],
									"CLUSTER_UUID" 		=> $QueryResult['_CLUSTER_UUID'],
									"FLAVOR_ID"			=> $QueryResult['_FLAVOR_ID'],
									"NETWORK_UUID" 		=> $QueryResult['_NETWORK_UUID'],
									"SGROUP_UUID" 		=> $QueryResult['_SGROUP_UUID'],
									"NOVA_VM_UUID" 		=> $QueryResult['_NOVA_VM_UUID'],
									"ADMIN_PASS" 		=> $QueryResult['_ADMIN_PASS'],
									"JOBS_JSON"			=> $JOBS_JSON,
									"WINPE_JOB"			=> $JOBS_JSON -> winpe_job,
									"OS_TYPE"			=> $QueryResult['_OS_TYPE'],
									"SERV_MESG"			=> $SERV_MESG,
									"SERV_FORMAT"		=> $SERV_FORMAT,
									"SERV_ARGUMENTS"	=> $SERV_ARGUMENTS,
									"SYNC_TIME"			=> $SYNC_TIME,
									"SERV_TIME"			=> $SERV_TIME,
									"TIMESTAMP"			=> $QueryResult['_TIMESTAMP'],
									"STATUS"			=> $QueryResult['_STATUS']
								);				
				}
			}
			return $SERV_DATA;
		}
		else
		{
			return false;
		}	
	}
	
	
	
	
	###########################
	#	NEW REPLICA DISK
	###########################
	private function new_replica_disk($REPL_UUID,$REPL_DISK,$USE_BOUNDARY)
	{
		for ($i=0; $i<count($REPL_DISK); $i++)
		{
			if ($USE_BOUNDARY == TRUE)
			{
				$DISK_SIZE = $REPL_DISK[$i]['DISK_BOUNDARY'];
			}
			else
			{
				$DISK_SIZE = $REPL_DISK[$i]['DISK_SIZE'];
			}
			
			$NEW_REPL_DISK = "INSERT 
									INTO _REPLICA_DISK(
											_ID,
											_DISK_UUID,
											_REPL_UUID,
											_HOST_UUID,
											_DISK_SIZE,
											
											_PACK_URI,
											_TIMESTAMP,
											_STATUS)
									VALUE(
											'',
											'".$REPL_DISK[$i]['DISK_UUID']."',
											'".$REPL_UUID."',
											'".$REPL_DISK[$i]['HOST_UUID']."',
											'".$DISK_SIZE."',
										
											'".$REPL_DISK[$i]['DISK_URI']."',
											'".Misc_Class::current_utc_time()."',
											'Y') ";
			
			$this -> DBCON -> prepare($NEW_REPL_DISK) -> execute();
		}		
	}
	
	###########################
	#	UPDATE DISK OPTIONS
	###########################
	public function update_disk_options($DISK_UUID,$SCSI_ADDR,$PURGE_OPTION)
	{
		$UPDATE_DISK_OPTION = "UPDATE _REPLICA_DISK
									SET
										_SCSI_ADDR 	= '".$SCSI_ADDR."',
										_PURGE_DATA = '".$PURGE_OPTION."'
									WHERE
										_DISK_UUID  = '".$DISK_UUID."'";
									
		$this -> DBCON -> prepare($UPDATE_DISK_OPTION) -> execute();
	}
	
	###########################
	#UPDATE SCHEDULER TRIGGER INFORMATION
	###########################
	public function update_trigger_info($JOB_UUID,$JOB_INFO,$JOB_TYPE)
	{
		if ($JOB_TYPE == 'REPLICA')
		{
			$QUERY_INFO = $this -> ReplMgmt -> query_replica($JOB_UUID);
			$JOBS_JSON = $QUERY_INFO['JOBS_JSON'];
			$LOG_LOCATION = $QUERY_INFO['LOG_LOCATION'];
		}
		else
		{
			$QUERY_INFO = $this -> query_service($JOB_UUID);
			$JOBS_JSON = json_decode($QUERY_INFO['JOBS_JSON']);
			$LOG_LOCATION = $QUERY_INFO['LOG_LOCATION'];
		}
		
		$OBJ_MERGED = (object) array_merge((array)$JOBS_JSON, (array)$JOB_INFO);
			
		//Misc_Class::function_debug($LOG_LOCATION,__FUNCTION__,debug_backtrace()[0]);
		Misc_Class::function_debug($LOG_LOCATION,__FUNCTION__,$OBJ_MERGED);
		
		#ENCODE JSON
		$JOB_JSON = json_encode($OBJ_MERGED);

		#REPLACE SLASH
		$JOB_JSON = str_replace('\\','\\\\',$JOB_JSON);
		
		if ($JOB_TYPE == 'REPLICA')
		{
			$UPDATE_TRIGGER_JSON = "UPDATE _REPLICA
										SET 
											_JOBS_JSON 	= '".$JOB_JSON."'
										WHERE
											_REPL_UUID 	= '".$JOB_UUID."'";
		}
		else
		{
			$UPDATE_TRIGGER_JSON = "UPDATE _SERVICE
										SET 
											_JOBS_JSON 	= '".$JOB_JSON."'
										WHERE
											_SERV_UUID 	= '".$JOB_UUID."'";
		}
		
		$this -> DBCON -> prepare($UPDATE_TRIGGER_JSON) -> execute();
	}
	
	###########################
	#CHECK REPLICA SUBMIT STATUS
	###########################
	private function replica_submit_status($REPL_UUID)
	{
		sleep(5);
		$REPLICA_HISTORY_TIME = "SELECT _REPL_HIST_JSON FROM _REPLICA WHERE _REPL_UUID = '".$REPL_UUID."' AND _STATUS = 'Y' LIMIT 1"; 
		$QUERY = $this -> DBCON -> prepare($REPLICA_HISTORY_TIME);
		$QUERY -> execute();
		
		$CURRENT_TIME = strtotime(Misc_Class::current_utc_time());
		
		foreach($QUERY as $QueryResult)
		{
			$REPL_TIME = strtotime(json_decode($QueryResult['_REPL_HIST_JSON'],false) -> time);
			$TIME_DIFF = $CURRENT_TIME - $REPL_TIME;
		}
		
		if ($TIME_DIFF > 10)
		{
			return false;
		}
		else
		{
			return true;
		}
	}

	###########################
	#UPDATE JOB ID INFORMATION
	###########################
	private function update_job_id_info($REPL_UUID,$JOB_UUID,$JOB_TYPE)
	{
		if ($JOB_TYPE == 'SCHEDULER')
		{
			$UPDATE_JOBID_INFO = "UPDATE _REPLICA
								SET 
									_SJOB_UUID = '".$JOB_UUID."'
								WHERE
									_REPL_UUID = '".$REPL_UUID."'";
		
		}
		elseif ($JOB_TYPE == 'LOADER')
		{
			$UPDATE_JOBID_INFO = "UPDATE _REPLICA
								SET 
									_LJOB_UUID = '".$JOB_UUID."'
								WHERE
									_REPL_UUID = '".$REPL_UUID."'";
		}
		elseif ($JOB_TYPE == 'LAUNCHER')
		{
			$UPDATE_JOBID_INFO = "UPDATE _REPLICA
								SET 
									_AJOB_UUID = '".$JOB_UUID."'
								WHERE
									_REPL_UUID = '".$REPL_UUID."'";
		}
		else
		{
			return false;
		}
		$UPDATE_QUERY = $this -> DBCON -> prepare($UPDATE_JOBID_INFO);
		$UPDATE_QUERY -> execute();		
	}
	
	###########################
	#CREATE NEW SERVICE JOB
	###########################
	private function create_service_job($ACCT_UUID,$REGN_UUID,$REPL_UUID,$CLUSTER_UUID,$SERV_UUID,$RECY_TYPE,$SNAP_JSON,$PACK_UUID,$CONN_UUID,$SERVICE_SETTINGS)
	{
		switch ($RECY_TYPE)
		{
			case "RECOVERY_PM":
				$RECOVERY_TYPE = 'PlannedMigration';
			break;
			
			case "RECOVERY_DR":
				$RECOVERY_TYPE = 'DisasterRecovery';
			break;
			
			case "RECOVERY_DT":
				$RECOVERY_TYPE = 'DevelopmentTesting';
			break;
			
			case "RECOVERY_KIT":
				$RECOVERY_TYPE = 'RecoveryKit';
			break;
			
			case "EXPORT_IMAGE":
				$RECOVERY_TYPE = 'ExportImage';
			break;
			
			default:
				$RECOVERY_TYPE = 'DevelopmentTesting';
		}
		
		#IP ADDRESS
		if (!isset($SERVICE_SETTINGS -> elastic_address_id) OR $SERVICE_SETTINGS -> elastic_address_id == ''){$SERVICE_SETTINGS -> elastic_address_id = 'DynamicAssign';}
		if (!isset($SERVICE_SETTINGS -> private_address_id) OR $SERVICE_SETTINGS -> private_address_id == ''){$SERVICE_SETTINGS -> private_address_id = 'DynamicAssign';}
		
		#SUBNET
		if(isset($SERVICE_SETTINGS -> SwitchUUID)){$SERVICE_SETTINGS -> SgroupUUID  = $SERVICE_SETTINGS -> SgroupUUID.'|'.$SERVICE_SETTINGS -> SwitchUUID;}
		if(isset($SERVICE_SETTINGS -> SubnetUUID)){$SERVICE_SETTINGS -> SgroupUUID  = $SERVICE_SETTINGS -> SgroupUUID.'|'.$SERVICE_SETTINGS -> SubnetUUID;}
		
		#DATAMODE POWER
		if (!isset($SERVICE_SETTINGS -> datamode_power) OR $SERVICE_SETTINGS -> datamode_power == ''){$SERVICE_SETTINGS -> datamode_power = 'off';}
				
		#PRE DEFINE DEFALUE SERVICE SETTINGS
		$SERVICE_SETTINGS -> recovery_type = $RECOVERY_TYPE;
		$SERVICE_SETTINGS -> task_operation = 'JOB_OP_UNKNOWN';
		$SERVICE_SETTINGS -> job_status = 'JobInitialized';
		$SERVICE_SETTINGS -> api_referer_from = $_SERVER['REMOTE_ADDR'];
		$SERVICE_SETTINGS -> delete_time = time();
		
		if( isset( $SERVICE_SETTINGS -> switch_uuid ) )
			$SERVICE_SETTINGS -> sgroup_uuid = $SERVICE_SETTINGS -> sgroup_uuid.'|'.$SERVICE_SETTINGS -> switch_uuid;
		
		if( isset( $SERVICE_SETTINGS -> subnet_uuid ) )
			$SERVICE_SETTINGS -> sgroup_uuid = $SERVICE_SETTINGS -> sgroup_uuid.'|'.$SERVICE_SETTINGS -> subnet_uuid;

		$INSERT_EXEC = "INSERT 
							INTO _SERVICE(
								_ID,
							
								_ACCT_UUID,
								_REGN_UUID,
								
								_SERV_UUID,
								_REPL_UUID,
								
								_SNAP_JSON,
								_PACK_UUID,
								_CONN_UUID,
								_CLUSTER_UUID,
								
								_FLAVOR_ID,
								_NETWORK_UUID,
								_SGROUP_UUID,
							
								_JOBS_JSON,
								_OS_TYPE,
							
								_TIMESTAMP,
								_STATUS)
							VALUE(
								'',
								
								'".$ACCT_UUID."',
								'".$REGN_UUID."',
								
								'".$SERV_UUID."',
								'".$REPL_UUID."',
								
								'".$SNAP_JSON."',								
								'".$PACK_UUID."',
								'".$CONN_UUID."',
								'".$CLUSTER_UUID."',
								
								'".$SERVICE_SETTINGS -> flavor_id."',								
								'".$SERVICE_SETTINGS -> network_uuid."',
								'".$SERVICE_SETTINGS -> sgroup_uuid."',
							
								'".json_encode($SERVICE_SETTINGS,JSON_UNESCAPED_UNICODE)."',
								'".$SERVICE_SETTINGS -> os_type."',
							
								'".Misc_Class::current_utc_time()."',
								'Y')";
	
		$QUERY = $this -> DBCON -> prepare($INSERT_EXEC) -> execute();
		
		$MESSAGE = $this -> ReplMgmt -> job_msg('Created recovery process.');
		$this -> ReplMgmt -> update_job_msg($SERV_UUID,$MESSAGE,'Service');
		
		#CHECK SAASAME LICENSE
		$CHECK_LICENSE = $this -> is_license_valid_ex($SERV_UUID);
		if ($CHECK_LICENSE == FALSE)
		{
			$MESSAGE = $this -> ReplMgmt -> job_msg('Not enough licenses to run recover workload.');
			$this -> ReplMgmt -> update_job_msg($SERV_UUID,$MESSAGE,'Service');
			exit;
		}
		else
		{			
			return $SERV_UUID;
		}
	}
	
	###########################
	#UPGRADE SERVICE INFORMATION
	###########################
	private function upgrade_service_info($JOB_JSON)
	{
		$JOB_JSON = json_decode($JOB_JSON,false);		
		
		if (!isset($JOB_JSON -> delete_time))
		{			
			$JOB_JSON -> mark_delete = false;
		}
		else
		{		
			if ((time() - $JOB_JSON -> delete_time) > 60)
			{
				$JOB_JSON -> mark_delete = false;
			}
			else
			{
				$JOB_JSON -> mark_delete = true;
			}
		}

		if (!isset($JOB_JSON -> is_azure_mgmt_disk))
		{
			$JOB_JSON -> is_azure_mgmt_disk = true;
		}		
		return json_encode($JOB_JSON);
	}	
	
	###########################
	#	QUERY SERVICE JOB
	###########################
	public function query_service($SERV_UUID)
	{
		$GET_EXEC 	= "SELECT * FROM _SERVICE WHERE _SERV_UUID = '".strtoupper($SERV_UUID)."' AND _STATUS = 'Y'";
		
		$QUERY = $this -> DBCON -> prepare($GET_EXEC);
		$QUERY -> execute();
		$COUNT_ROWS = $QUERY -> rowCount();
		
		if ($COUNT_ROWS != 0)
		{
			foreach($QUERY as $QueryResult)
			{
				$HOST_INFO = $this -> ServerMgmt -> query_host_info($QueryResult['_PACK_UUID']);
				
				$REPL_EXPLODE = explode('-',$QueryResult['_REPL_UUID']);
				
				/* 
					For upgrade use from 5xx to 7xx version 
					change from 5xx version _WINPE_JOB string(1) to _JOBS_JSON text
				*/
				if (count($this -> DBCON -> query("SHOW COLUMNS FROM _SERVICE LIKE '_WINPE_JOB'") -> fetchAll())) 
				{
					$JOBS_JSON = json_encode(array('winpe_job' => $QueryResult['_WINPE_JOB'], 'task_operation'=> 'JOB_OP_UNKNOWN'));
					$WINPE_JOB = json_decode($JOBS_JSON,false) -> winpe_job;
				}
				else
				{
					/*if (count($QueryResult['_JOBS_JSON']) == 1)
					{
						$JOBS_JSON = json_encode(array('winpe_job' => $QueryResult['_JOBS_JSON'], 'task_operation'=> 'JOB_OP_UNKNOWN'));
					}
					else
					{					
						$JOBS_JSON = $QueryResult['_JOBS_JSON'];
					}
					*/
					$JOBS_JSON = $this -> upgrade_service_info($QueryResult['_JOBS_JSON']);
					$WINPE_JOB = json_decode($QueryResult['_JOBS_JSON'],false) -> winpe_job;				
				}

				$CONN_INFO = $this -> ServerMgmt -> query_connection_info($QueryResult['_CONN_UUID']);
				
				$JOBS_JSON = json_decode($JOBS_JSON,false);
				$JOBS_JSON -> is_promote = $CONN_INFO['LAUN_PROMOTE'];
				$JOBS_JSON = json_encode($JOBS_JSON);
				
				$SERV_DATA = array(
								"ACCT_UUID" 	=> $QueryResult['_ACCT_UUID'],
								"REGN_UUID" 	=> $QueryResult['_REGN_UUID'],
								"SERV_UUID" 	=> $QueryResult['_SERV_UUID'],
								"REPL_UUID" 	=> $QueryResult['_REPL_UUID'],
															
								"SNAP_JSON" 	=> $QueryResult['_SNAP_JSON'],
								
								"PACK_UUID" 	=> $QueryResult['_PACK_UUID'],
								"CONN_UUID" 	=> $QueryResult['_CONN_UUID'],
								"HOST_NAME"		=> $HOST_INFO['HOST_NAME'],
								"LOG_LOCATION"	=> $HOST_INFO['HOST_NAME'].'-'.end($REPL_EXPLODE),
								
								"CLUSTER_UUID" 	=> $QueryResult['_CLUSTER_UUID'],
								"FLAVOR_ID" 	=> $QueryResult['_FLAVOR_ID'],
								"NETWORK_UUID" 	=> $QueryResult['_NETWORK_UUID'],
								"SGROUP_UUID" 	=> $QueryResult['_SGROUP_UUID'],
								"NOVA_VM_UUID" 	=> $QueryResult['_NOVA_VM_UUID'],
								"IMAGE_ID" 		=> $QueryResult['_IMAGE_ID'],
								"TASK_ID" 		=> $QueryResult['_TASK_ID'],
								"ADMIN_PASS"	=> $QueryResult['_ADMIN_PASS'],

								"JOBS_JSON"		=> $JOBS_JSON,
								"WINPE_JOB"		=> $WINPE_JOB,
								"OS_TYPE"		=> $QueryResult['_OS_TYPE'],
								"CLOUD_TYPE"	=> $CONN_INFO['CLOUD_TYPE'],
								"CLOUD_NAME"	=> $CONN_INFO['CLOUD_NAME'],
							
								"TIMESTAMP"		=> $QueryResult['_TIMESTAMP'],
							);	
			}			
			return $SERV_DATA;
		}
		else
		{
			return false;
		}
	}
	
	###########################
	#UPDATE SERVICE NOVA VM INFO
	###########################
	public function update_service_vm_info($SERV_UUID,$NOVA_VM_UUID,$ADMIN_PASS)
	{
		$UPDATE_SERVICE_VM_INFO = "UPDATE _SERVICE
									SET 
										_NOVA_VM_UUID = '".$NOVA_VM_UUID."',
										_ADMIN_PASS   = '".$ADMIN_PASS."'
									WHERE
										_SERV_UUID = '".$SERV_UUID."'";
										
		$this -> DBCON -> prepare($UPDATE_SERVICE_VM_INFO) -> execute();
	}
	
	###########################
	#UPDATE SERVICE IMAGE ID
	###########################
	public function update_service_image_id( $SERV_UUID, $IMAGE_ID, $TASK_ID)
	{
		/*$sql = "UPDATE _SERVICE
									SET 
										_IMAGE_ID   = :image_id
									WHERE
										_SERV_UUID = :sevice_uuid";
		Misc_Class::function_debug("",__FUNCTION__,$sql);								
		$query = $this -> DBCON -> prepare($sql);
		
		$query-> execute( array(
								"image_id" => $IMAGE_ID ,
								"sevice_uuid" => $SERV_UUID
							));*/
							
		$sql = "UPDATE _SERVICE
					SET 
						_IMAGE_ID = '".$IMAGE_ID."',
						_TASK_ID = '".$TASK_ID."'
					WHERE
						_SERV_UUID = '".$SERV_UUID."'";		

		$this -> DBCON -> prepare($sql) -> execute();
		
	}
	
	###########################
	#	NEW SERVICE DISK
	###########################
	private function new_service_disk($SERV_UUID,$PACK_UUID,$DISK_SIZE,$VOLUME_UUID,$SNAP_UUID)
	{
		$DISK_UUID  = Misc_Class::guid_v4();
		$NEW_SERV_DISK = "INSERT 
								INTO _SERVICE_DISK(
										_ID,
										
										_DISK_UUID,
										_SERV_UUID,
										_HOST_UUID,
										
										_DISK_SIZE,
						
										_OPEN_DISK,	
										_SNAP_UUID,
											
										_TIMESTAMP,
										_STATUS)
								VALUE(
										'',
										
										'".$DISK_UUID."',
										'".$SERV_UUID."',
										'".$PACK_UUID."',
										
										'".($DISK_SIZE*1024*1024)."',
										
										'".$VOLUME_UUID."',
										'".$SNAP_UUID."',
										
										'".Misc_Class::current_utc_time()."',
										'Y')";
		$this -> DBCON -> prepare($NEW_SERV_DISK) -> execute();
		
		return $DISK_UUID;
	}
	
	###########################
	#UPDATE SERVICE DISK SCSI ADDRESS
	###########################	
	private function update_disk_scsi_info($VOLUME_UUID,$SCSI_ADDR)
	{
		$UPDATE_DISK_SCSI_ADDRESS = "UPDATE _SERVICE_DISK
								     SET 
										_SCSI_ADDR = '".$SCSI_ADDR."'
									 WHERE
										_OPEN_DISK = '".$VOLUME_UUID."'";
										
		$this -> DBCON -> prepare($UPDATE_DISK_SCSI_ADDRESS) -> execute();
	}
	
	###########################
	#UPDATE SERVICE STATUS
	###########################
	public function update_service_info($SERV_UUID,$STATUS)
	{
		$UPDATE_SERVICE_STATUS = "UPDATE _SERVICE
									SET 
										_STATUS = '".$STATUS."'
									WHERE
										_SERV_UUID = '".$SERV_UUID."'";
										
		$this -> DBCON -> prepare($UPDATE_SERVICE_STATUS) -> execute();
		
		$UPDATE_SERVICE_DISK_STATUS = "UPDATE _SERVICE_DISK
										SET 
											_STATUS = '".$STATUS."'
										WHERE
											_SERV_UUID = '".$SERV_UUID."'";
										
		$this -> DBCON -> prepare($UPDATE_SERVICE_DISK_STATUS) -> execute();
	}	
	
	###########################
	#QUERY SERVICE DISK
	###########################
	public function query_service_disk($SERV_UUID)
	{
		$SERVICE_DISK_QUERY = "SELECT * FROM _SERVICE_DISK WHERE _SERV_UUID = '".$SERV_UUID."' AND _STATUS = 'Y'";

		$QUERY = $this -> DBCON -> prepare($SERVICE_DISK_QUERY);
		$QUERY -> execute();
		$COUNT_ROWS = $QUERY -> rowCount();
	
		if ($COUNT_ROWS != 0)
		{
			foreach($QUERY as $QueryResult)
			{
				$SERV_DISK_DATA[] = array(
									"DISK_UUID" 	=> $QueryResult['_DISK_UUID'],
									"SERV_UUID" 	=> $QueryResult['_SERV_UUID'],
									"HOST_UUID" 	=> $QueryResult['_HOST_UUID'],
										
									"DISK_SIZE" 	=> $QueryResult['_DISK_SIZE'],
									
									"SCSI_ADDR" 	=> $QueryResult['_SCSI_ADDR'],
									"OPEN_DISK" 	=> $QueryResult['_OPEN_DISK'],
										
									"SNAP_UUID" 	=> $QueryResult['_SNAP_UUID'],
									"TIMESTAMP"		=> $QueryResult['_TIMESTAMP']
								);
			}			
			return $SERV_DISK_DATA;
		}
		else
		{
			return false;
		}
	}
	
	###########################
	#CHECK AND CREATE CONNECTION
	###########################
	public function create_connection($ACCT_UUID,$REPL_UUID,$CARR_UUID,$LOAD_UUID,$LAUN_UUID,$CLUSTER_UUID,$CONN_INFO,$MGMT_ADDR,$CHECKSUM,$WEBDAV_PRIORITY_ADDR)
	{
		//Misc_Class::function_debug('_mgmt',__FUNCTION__,json_encode(func_get_args()));
		
		$SOURCE_DIRECT = $CONN_INFO['CARR_INFO']['SERV_INFO']['direct_mode'];
		$TARGET_DIRECT = $CONN_INFO['SERV_INFO']['SERV_INFO']['direct_mode'];
		
		$SOURCE_MACHINE_ID = $CONN_INFO['SERV_INFO']['SERV_INFO']['machine_id'];
		$TARGET_MACHINE_ID = $CONN_INFO['CARR_INFO']['SERV_INFO']['machine_id'];
		
		#DEFINE MODE ADDRESS
		if ($SOURCE_DIRECT == TRUE AND $TARGET_DIRECT == TRUE)
		{		
			$REMOTE_ADDR = $CONN_INFO['SERV_INFO']['SERV_ADDR'];
			$SCHD_UUID	 = $CONN_INFO['CONN_DATA']['SCHD_UUID'];
			
			#DEFINE DEFAULT FOLDER
			$FolderDefine = str_replace('\\Launcher','',$CONN_INFO['SERV_INFO']['SERV_INFO']['path']).'\\imagex';
			
			#PRIORITIZES WEVDAV ADDRESS
			$WEBDAV_ADDR = $CONN_INFO['CARR_INFO']['SERV_ADDR'];			
			array_unshift($WEBDAV_ADDR,$WEBDAV_PRIORITY_ADDR);					
			$WEBDAV_ADDR = array_values(array_unique($WEBDAV_ADDR));			
		}
		elseif ($SOURCE_DIRECT == TRUE AND $TARGET_DIRECT == FALSE)
		{
			$REMOTE_ADDR = array($SOURCE_MACHINE_ID);
			$SCHD_UUID	 = $CONN_INFO['CONN_DATA']['SCHD_UUID'];
			
			#DEFINE DEFAULT FOLDER
			$FolderDefine = str_replace('\\Launcher','',$CONN_INFO['SERV_INFO']['SERV_INFO']['path']).'\\imagex';			
			
			#PRIORITIZES WEVDAV ADDRESS
			$WEBDAV_ADDR = $CONN_INFO['CARR_INFO']['SERV_ADDR'];			
			array_unshift($WEBDAV_ADDR,$WEBDAV_PRIORITY_ADDR);					
			$WEBDAV_ADDR = array_values(array_unique($WEBDAV_ADDR));
		}
		elseif ($SOURCE_DIRECT == FALSE AND $TARGET_DIRECT == TRUE)
		{
			$REMOTE_ADDR = array($TARGET_MACHINE_ID);
			$SCHD_UUID = $this -> ServerMgmt -> list_match_service_id($TARGET_MACHINE_ID)['ServiceId']['Scheduler'];

			#DEFINE DEFAULT FOLDER
			$FolderDefine = str_replace('\\Launcher','',$CONN_INFO['CARR_INFO']['SERV_INFO']['path']).'\\imagex';

			#PRIORITIZES WEVDAV ADDRESS
			$WEBDAV_ADDR = $CONN_INFO['SERV_INFO']['SERV_ADDR'];			
			array_unshift($WEBDAV_ADDR,$WEBDAV_PRIORITY_ADDR);					
			$WEBDAV_ADDR = array_values(array_unique($WEBDAV_ADDR));
		}
		elseif ($SOURCE_MACHINE_ID == $TARGET_MACHINE_ID)
		{
			$REMOTE_ADDR = $TARGET_MACHINE_ID;
			$SCHD_UUID	 = $CONN_INFO['CONN_DATA']['SCHD_UUID'];
			
			#DEFINE DEFAULT FOLDER
			$FolderDefine = str_replace('\\Launcher','',$CONN_INFO['SERV_INFO']['SERV_INFO']['path']).'\\imagex';
			
			#PRIORITIZES WEVDAV ADDRESS
			$WEBDAV_ADDR = $CONN_INFO['SERV_INFO']['SERV_ADDR'];			
			array_unshift($WEBDAV_ADDR,$WEBDAV_PRIORITY_ADDR);					
			$WEBDAV_ADDR = array_values(array_unique($WEBDAV_ADDR));
		}
		else //$SOURCE_DIRECT == FALSE AND $TARGET_DIRECT == FALSE
		{			
			$MESSAGE = $this -> ReplMgmt -> job_msg('Source Transport and Target Transport cannot both in-direct mode.');
			$this -> ReplMgmt -> update_job_msg($REPL_UUID,$MESSAGE,'Replica');
			return false;				
		}
		
		#PASS TARGET CONNECTION
		$TARGET_CONN = $CONN_INFO['TARGET_CONN'];
	
		#DEFINE SERVICE UUID
		$UUID_LAUN	 = $CONN_INFO['LAUN_UUID'];
		$SERV_UUID   = array(
							'SCHD_UUID' => $SCHD_UUID,
							'CARR_UUID' => $CARR_UUID,
							'LOAD_UUID' => $LOAD_UUID,
							'LAUN_UUID' => $LAUN_UUID);

		#CHECK AND RECREATE CARRIR CONNECTION
		$SOURCE_CONN = $CONN_INFO['SOURCE_CONN'];
		$CHECK_CONN_STATUS = $this -> get_connection_status($SOURCE_CONN,'CARRIER');

		if ($CHECK_CONN_STATUS != FALSE)
		{
			if ($CHECK_CONN_STATUS -> id == '')
			{
				#RE CREATE CONNECTION
				$RE_CREATE_CONN = $this -> re_create_connection($SOURCE_CONN,'CARRIER');
				if ($RE_CREATE_CONN == TRUE)
				{
					$MESSAGE = $this -> ReplMgmt -> job_msg('Successfully rebuild Carrier connection.');
					$this -> ReplMgmt -> update_job_msg($REPL_UUID,$MESSAGE,'Replica');
				}
				else
				{
					$MESSAGE = $this -> ReplMgmt -> job_msg('Cannot rebuild Carrier connection.');
					$this -> ReplMgmt -> update_job_msg($REPL_UUID,$MESSAGE,'Replica');
					return false;
				}
			}
		}
		else
		{	
			#ERROR CHECK ONLY ON DIRECT MODE
			if ($SOURCE_DIRECT == TRUE)
			{
				$MESSAGE = $this -> ReplMgmt -> job_msg('Failed to connect to transport server. Cannot recreate the Carrier connection.');
				$this -> ReplMgmt -> update_job_msg($REPL_UUID,$MESSAGE,'Replica');
				return false;
			}
		}
		
		#CREATE WEBDAV CONNECTION
		if ($TARGET_CONN == '')
		{
			#QUERY WEBDAV CONFIGURATION
			$WEBDAV_CONFIG = Misc_Class::mgmt_comm_type('webdav');	
			$WEBDAV_SSL  = $WEBDAV_CONFIG['is_ssl'];
			$WEBDAV_PORT = $WEBDAV_CONFIG['mgmt_port'];
			
			try
			{
				$WEBDAV_ADDR = array_values(array_filter($WEBDAV_ADDR));
				
				for ($w=0; $w<count($WEBDAV_ADDR); $w++)
				{
					if ($WEBDAV_SSL != true)
					{
						$WebDevURL    = 'http://'.$WEBDAV_ADDR[$w].'/webDav';
						$LogWebDevURL = 'http://'.$WEBDAV_ADDR[$w].':'.$WEBDAV_PORT.'/webDav';
					}
					else
					{
						$WebDevURL    = 'https://'.$WEBDAV_ADDR[$w].'/webDav';
						$LogWebDevURL = 'https://'.$WEBDAV_ADDR[$w].':'.$WEBDAV_PORT.'/webDav';
					}
				
					$MESSAGE = $this -> ReplMgmt -> job_msg('Setting up WebDAV connection with URL - %1%',array($LogWebDevURL));
					$this -> ReplMgmt -> update_job_msg($REPL_UUID,$MESSAGE,'Replica');
				
					#DEFINE CONNECTION PATH
					$CONNECTION_PATH = array('LocalPath' => $FolderDefine, 'WebDavPath' => $WebDevURL);
					
					$INIT_CONN = $this -> verify_folder_connection($REMOTE_ADDR,$CONNECTION_PATH,'Add','RemoteWebDav',$CHECKSUM,$SOURCE_DIRECT,$TARGET_DIRECT);
					if($INIT_CONN != FALSE)
					{
						$ConnectionDate = array_merge(array('ACCT_UUID' => $ACCT_UUID,'CONN_TYPE' => 'Remote'),$SERV_UUID,$INIT_CONN,array('MGMT_ADDR' => $MGMT_ADDR),array('CLUSTER_UUID' => $CLUSTER_UUID));
					
						$this -> ServerMgmt -> save_connection($ConnectionDate);
						$ConnectionId = $ConnectionDate['id'];
						break;
					}
					else
					{
						$MESSAGE = $this -> ReplMgmt -> job_msg('Failed to initialize webDAV connection.');
						$this -> ReplMgmt -> update_job_msg($REPL_UUID,$MESSAGE,'Replica');
						$ConnectionId = false;
						sleep(5);
					}
				}			
				return $ConnectionId;
			}
			catch (Throwable $e)
			{
				return false;
			}
		}
		/*
		#CREATE LOCAL FOLDER WITH LINUX LAUNCHER
		elseif ($UUID_LAUN == '')
		{
			$FolderDefine = $CONN_INFO['CONN_DATA']['options']['folder'];
			$WebDevURL = null;
			
			#DEFINE CONNECTION PATH
			$CONNECTION_PATH = array('LocalPath' => $FolderDefine, 'WebDavPath' => $WebDevURL);
			
			$INIT_CONN = $this -> verify_folder_connection($REMOTE_ADDR,$CONNECTION_PATH,'Add','LocalFolder',$CHECKSUM,$SERV_UUID);
			try{	
				$ConnectionDate = array_merge(array('ACCT_UUID' => $ACCT_UUID,'CONN_TYPE' => 'LocalFolder'),$SERV_UUID,$INIT_CONN,array('MGMT_ADDR' => $MGMT_ADDR),array('CLUSTER_UUID' => $CLUSTER_UUID));
			
				$this -> ServerMgmt -> save_connection($ConnectionDate);
				return $ConnectionDate['id'];		
			}
			catch (Throwable $e){
				return false;
			}
		}
		*/
		#RETURN WITH NORMAL CONNECTION
		else
		{
			return $TARGET_CONN;
		}
	}


	###########################
	#UPDATE REPLICA DISK
	###########################
	private function update_replica_disk($REPL_UUID,$DISK_UUID,$OPEN_DISK)
	{
		$UPDATE_DISK = "UPDATE
							_REPLICA_DISK
						SET
							_OPEN_DISK			= '".$OPEN_DISK."'
						WHERE
							_DISK_UUID = '".$DISK_UUID."' AND 
							_REPL_UUID = '".$REPL_UUID."'";
							
		$this -> DBCON -> prepare($UPDATE_DISK) -> execute();
	}
			
	###########################
	#SAVE VOLUME INFO
	###########################
	private function save_volume_info($REPL_UUID,$DISK_UUID,$VOLUME_INFO,$HOST_UUID,$LOADER_ADDR,$EXTRA_GB,$FI_SCSI_ADDR)
	{
		$RETRY_COUNT = 10;
		for($i=0; $i<$RETRY_COUNT; $i++)
		{
			#GET NEW MOUNT SCSI ADDRESS
			$LA_SCSI_ADDR = $this -> list_disk_addr_info($HOST_UUID,$LOADER_ADDR,'Loader','MS','Physical');
			
			$SCSI_ADDR = array_values(array_diff($LA_SCSI_ADDR,$FI_SCSI_ADDR))[0];
			
			if ($SCSI_ADDR != '' AND $SCSI_ADDR != '65535:4294967295:65535:65535')
			{
				break;
			}
			else
			{
				sleep(15);
			}
		}	
		
		if (isset($VOLUME_INFO -> volumeMountStatus))
		{
			$SERV_UUID 		= $VOLUME_INFO -> volumeAttachment -> serverId;
			$VOLUME_ID 		= $VOLUME_INFO -> volumeAttachment -> id;
			$VOLUME_UUID 	= $VOLUME_INFO -> volumeAttachment -> volumeId;
			$DEVICE_PATH 	= $VOLUME_INFO -> volumeAttachment -> device;
		}
		else
		{
			$SERV_UUID 		= $VOLUME_INFO -> serverId;
			$VOLUME_ID		= $VOLUME_INFO -> volumeId;
			$VOLUME_UUID	= $VOLUME_INFO -> volumeId;
			$DEVICE_PATH 	= $VOLUME_INFO -> volumePath;
		}
		
		if ($SCSI_ADDR != '')
		{		
			if ($EXTRA_GB == TRUE)
			{
				$DISK_ADDR = Misc_Class::guid_v4();
					
				/*	NOTE: SET DISK ID WILL FALUE WHEN SCSI_ADDR AS SERIAL NUMBER */
				$SET_DISK_ID = $this -> set_disk_customized_id($LOADER_ADDR,$SCSI_ADDR,$DISK_ADDR);			
					
				if ($SET_DISK_ID != TRUE)
				{
					$DISK_ADDR = $SCSI_ADDR;
				}
			}
			else
			{
				$DISK_ADDR = $SCSI_ADDR;
			}
		}
		else
		{
			$DISK_ADDR = 'xx:yy:zz:yy';
		}
			
		#UPDATE REPLICA DISK SCSI INFO
		$this -> update_replica_disk_scsi_info($REPL_UUID,$DISK_UUID,$VOLUME_UUID,$DISK_ADDR);
		
		$NEW_CLOUD_VOLUME = "INSERT
								INTO _CLOUD_DISK(
									_ID,
								
									_OPEN_DISK_UUID,
									_OPEN_SERV_UUID,
									_OPEN_DISK_ID,
									_DEVICE_PATH,
									
									_REPL_UUID,
									_REPL_DISK_UUID,
							
									_TIMESTAMP,
									_STATUS)
								VALUE(
									'',
									
									'".$VOLUME_UUID."',
									'".$SERV_UUID."',
									'".$VOLUME_ID."',
									'".$DEVICE_PATH."',
									
									'".$REPL_UUID."',
									'".$DISK_UUID."',
								
									'".Misc_Class::current_utc_time()."',
									'Y')";
									
		$this -> DBCON -> prepare($NEW_CLOUD_VOLUME) -> execute();
			
		if ($DISK_ADDR == 'xx:yy:zz:yy')
		{
			$MESSAGE = $this -> ReplMgmt -> job_msg('Cannot get volume address information.');
			$this -> ReplMgmt -> update_job_msg($REPL_UUID,$MESSAGE,'Replica');
			
			exit;
			return false;
		}
		else
		{
			if (strpos($DISK_ADDR, ':') !== false)
			{
				$MESSAGE = $this -> ReplMgmt -> job_msg('The SCSI address is %1%',array($DISK_ADDR.'.'));
			}
			elseif (strlen($DISK_ADDR) == 32)
			{
				$MESSAGE = $this -> ReplMgmt -> job_msg('The disk unique id is %1%',array($DISK_ADDR.'.'));
			}
			elseif (strlen($DISK_ADDR) == 36)
			{
				$MESSAGE = $this -> ReplMgmt -> job_msg('The disk customized id is %1%',array($DISK_ADDR.'.'));
			}
			else
			{
				$MESSAGE = $this -> ReplMgmt -> job_msg('The disk serial number is %1%',array($DISK_ADDR.'.'));
			}
			$this -> ReplMgmt -> update_job_msg($REPL_UUID,$MESSAGE,'Replica');		
		}
	}
	
	###########################	
	#COMPARE ENUMERATE DISKS
	###########################	
	private function CompareEnumerateDisks($HOST_UUID,$SERV_ADDR,$SERV_TYPE,$OS_TYPE,$HOST_TYPE,$FI_SCSI_ADDR)
	{
		$try_count = 10;
		for($i=0; $i<$try_count; $i++)
		{
			#GET SCSI ADDRESS
			$LA_SCSI_ADDR = $this -> list_disk_addr_info($HOST_UUID,$SERV_ADDR,$SERV_TYPE,$OS_TYPE,$HOST_TYPE);

			$SCSI_ADDR = array_values(array_diff($LA_SCSI_ADDR,$FI_SCSI_ADDR))[0];
			
			if ($SCSI_ADDR != '' AND $SCSI_ADDR != '65535:4294967295:65535:65535')
			{
				break;
			}
			else
			{			
				sleep(15);
			}
		}		
		return $SCSI_ADDR;
	}
	
	###########################
	#UPDATE REPLICA DISK SCSI INFO
	###########################
	public function update_replica_disk_scsi_info($REPL_UUID,$DISK_UUID,$OPEN_DISK_UUID,$SCSI_ADDR)
	{
		$UPDATE_DISK = "UPDATE
							_REPLICA_DISK
						SET
							_SCSI_ADDR		= '".$SCSI_ADDR."',
							_OPEN_DISK		= '".$OPEN_DISK_UUID."'
						WHERE
							_DISK_UUID 		= '".$DISK_UUID."' AND 
							_REPL_UUID 		= '".$REPL_UUID."'";
		
		$this -> DBCON -> prepare($UPDATE_DISK) -> execute();
	}
		
	#############################
	#
	#	PRE CREATE LOADER JOB DISK
	#
	#############################
	public function pre_create_loader_job_disk($REPL_UUID,$REPL_CONF,$LOAD_INFO,$HOST_INFO)
	{
		#DEFAULT ACTION DISK TYPE
		$ACTION_DISK_TYPE = 'Cloud';
		
		#IS TARGET RECOVER KIT
		if($LOAD_INFO['SERV_INFO']['is_winpe'] == TRUE){$ACTION_DISK_TYPE = 'RCD';};

		#IS ADVANCED ON-PREMISE
		if($LOAD_INFO['VENDOR_NAME'] == "UnknownVendorType" AND $REPL_CONF['ExportPath'] == ''){$ACTION_DISK_TYPE = 'RCD';};

		#IS EXPORT JOB
		if($REPL_CONF['ExportPath'] != ''){$ACTION_DISK_TYPE = 'Export';};
		
		#IS AZURE BLOB JOB
		if($REPL_CONF['IsAzureBlobMode'] == TRUE){$ACTION_DISK_TYPE = 'AzureBlob';};		
		
		switch ($ACTION_DISK_TYPE)
		{
			case 'AzureBlob':
				$DISK_RETURN = $this -> disk_action_azure_blob($REPL_UUID,$HOST_INFO,$LOAD_INFO);
			break;
			
			case 'Cloud':
				$DISK_RETURN = $this -> disk_action_cloud($REPL_UUID,$LOAD_INFO);
			break;
		
			case 'Export':
				$DISK_RETURN = $this -> disk_action_export($REPL_UUID,$HOST_INFO);
			break;
		
			case 'RCD':
				$DISK_RETURN = $this -> disk_action_recover_kit($REPL_UUID,$HOST_INFO,$LOAD_INFO);
			break;			
		}
		return $DISK_RETURN;
	}
	
	
	#############################
	# disk action azure blob
	#############################
	public function disk_action_azure_blob($REPL_UUID,$HOST_INFO,$LOAD_INFO)
	{
		#VERIFY BLOB CONNECTION STRING		
		$BLOB_CONN_STRING = $this -> AzureBlobMgmt -> get_blob_connection_string($REPL_UUID) -> ConnectionString;
		$VERIFY_CONNECTION = $this -> verify_connection_string($BLOB_CONN_STRING);
		if ($VERIFY_CONNECTION == FALSE)
		{					
			$MESSAGE = $this -> ReplMgmt -> job_msg('Failed to verify storage connection string.');
			$this -> ReplMgmt ->update_job_msg($REPL_UUID,$MESSAGE,'Replica');
			return false;
		}

		#QUERY DEFIND DISK
		$REPL_DISK = $this -> ReplMgmt -> query_replica_disk($REPL_UUID);	
		
		$MESSAGE = $this -> ReplMgmt -> job_msg('Getting disk information of the Azure disk with Blob.');
		$this -> ReplMgmt ->update_job_msg($REPL_UUID,$MESSAGE,'Replica');	
		
		$SCSI_ADDR = $HOST_INFO['HOST_NAME'];
		
		$DISK_INFO = array("TransportServerId" => $LOAD_INFO['OPEN_HOST']);
	
		for ($i=0; $i<count($REPL_DISK); $i++)
		{
			$DISK_UUID = $REPL_DISK[$i]['DISK_UUID'];
			$OPEN_DISK_UUID =  $DISK_UUID.'-'.$REPL_DISK[$i]['ID'];
			$this -> update_replica_disk_scsi_info($REPL_UUID,$DISK_UUID,$OPEN_DISK_UUID,$SCSI_ADDR.'_'.$i);
			sleep(1);
			
			#ADD CLOUD DISK INFORMATION
			$DISK_INFO["DiskId"] = $DISK_UUID.'-'.$REPL_DISK[$i]['ID'];
			$this -> AzureBlobMgmt -> insert_blob_cloud_disk($REPL_UUID,$DISK_UUID,$DISK_INFO);
		}

		$MESSAGE = $this -> ReplMgmt -> job_msg('The disk information updated.');
		$this -> ReplMgmt -> update_job_msg($REPL_UUID,$MESSAGE,'Replica');
		
		return true;
	}	

	#############################
	# disk action export
	#############################
	private function disk_action_export($REPL_UUID,$HOST_INFO)
	{
		#QUERY DEFIND DISK
		$REPL_DISK = $this -> ReplMgmt -> query_replica_disk($REPL_UUID);
		
		$MESSAGE = $this -> ReplMgmt -> job_msg('Getting disk information of the export job.');
		$this -> ReplMgmt -> update_job_msg($REPL_UUID,$MESSAGE,'Replica');	
			
		$OPEN_DISK_UUID = '000000000-EXPORT-000-MEDIA-000000000';
		$SCSI_ADDR = $HOST_INFO['HOST_NAME'];
			
		for ($i=0; $i<count($REPL_DISK); $i++)
		{
			$DISK_UUID = $REPL_DISK[$i]['DISK_UUID'];
			$this -> update_replica_disk_scsi_info($REPL_UUID,$DISK_UUID,$OPEN_DISK_UUID,$SCSI_ADDR.'_'.$i);
			sleep(1);
		}

		$MESSAGE = $this -> ReplMgmt -> job_msg('The disk information updated.');
		$this -> ReplMgmt -> update_job_msg($REPL_UUID,$MESSAGE,'Replica');

		return true;
	}	
	
	#############################
	# disk action recovery kit
	#############################
	public function disk_action_recover_kit($REPL_UUID,$HOST_INFO,$LOAD_INFO)
	{
		#QUERY REPLICA INFORMATION
		$REPL_QUERY = $this -> ReplMgmt -> query_replica($REPL_UUID);
		$JOB_INFO = $REPL_QUERY['JOBS_JSON'];

		#USER MAPPING DISK UUID
		$CLOUD_MAP = explode(',',$JOB_INFO -> cloud_mapping_disk);

		#QUERY DEFIND DISK
		$REPL_DISK = $this -> ReplMgmt -> query_replica_disk($REPL_UUID);

		#CHECK FOR RCD COMMUNICATION MODE
		$WINPE_DIRECT_MODE = $LOAD_INFO['SERV_INFO']['direct_mode'];
	
		#LOADER ADDRESS
		if ($WINPE_DIRECT_MODE == TRUE)
		{
			$LOADER_ADDR = $LOAD_INFO['SERV_ADDR'];
		}
		else
		{
			$LOADER_ADDR = array($LOAD_INFO['SERV_INFO']['machine_id']);
		}
		
		#MESSAGE
		$MESSAGE = $this -> ReplMgmt -> job_msg(($REPL_QUERY['WINPE_JOB'] == 'Y')?"Getting disk information of the Recovery Kit.":"Reading disk information from general purpose transport server.");	
		$this -> ReplMgmt -> update_job_msg($REPL_UUID,$MESSAGE,'Replica');		

		#LOADER DISK INFORMATION
		$LOADER_DISK = $this -> query_select_transport_info($LOAD_INFO['SERV_UUID']) -> SERV_INFO -> disk_infos;
	
		$OPEN_DISK_UUID = '000000000-RECOV-0000-MEDIA-000000000';		
		
		#FOR MAP DISK
		for($i=0; $i<count($CLOUD_MAP); $i++)
		{
			if ($CLOUD_MAP[$i] != 'CreateOnDemand')
			{
				if ($JOB_INFO -> set_disk_customized_id == TRUE)
				{
					$CLOUD_DISK = $this -> use_customized_id($LOADER_ADDR,$LOADER_DISK,$CLOUD_MAP[$i]);
					$MAP_DISK[] = $CLOUD_DISK;
				}
				else
				{
					$MAP_DISK[] = $CLOUD_MAP[$i];
				}
			}
		}
		
		#UPDATE SCSI ADDRESS VALUE INTO DATABASE
		for ($x=0; $x<count($REPL_DISK); $x++)
		{
			$DISK_UUID = $REPL_DISK[$x]['DISK_UUID'];
			$this -> update_replica_disk_scsi_info($REPL_UUID,$DISK_UUID,$OPEN_DISK_UUID,$MAP_DISK[$x]);
		}

		#MESSAGE
		$MESSAGE = $this -> ReplMgmt -> job_msg(($REPL_QUERY['WINPE_JOB'] == 'Y')?'The disk matches the Recovery Kit information.':'Source disk count and size match with replication information.');
		$this -> ReplMgmt -> update_job_msg($REPL_UUID,$MESSAGE,'Replica');					
		return true;	
	}
	
	#############################
	# Use Customized id
	#############################
	private function use_customized_id($LOADER_ADDR,$LOADER_DISK,$DISK_ID)
	{
		if (substr_count($DISK_ID, ':') != 3)
		{
			for ($i=0; $i<count($LOADER_DISK); $i++)
			{
				if ($LOADER_DISK[$i] -> serial_number == $DISK_ID)
				{
					$DISK_ADDR = json_decode($LOADER_DISK[$i] -> uri) -> address;
					break;
				}
			}
		}
		else
		{
			$DISK_ADDR = $DISK_ID;
		}
		
		$SET_DISK_ID = Misc_Class::guid_v4();
		
		$this -> set_disk_customized_id($LOADER_ADDR,$DISK_ADDR,$SET_DISK_ID);
		
		return $SET_DISK_ID;
	}
	
	#############################
	# disk action standard cloud
	#############################
	public function disk_action_cloud($REPL_UUID,$LOAD_INFO)
	{
		#GET REPLICA INFO
		$REPL_QUERY = $this -> ReplMgmt -> query_replica($REPL_UUID);
		
		#GET REPLICA JOB CONFIGURATION
		$REPL_CONF = $REPL_QUERY['JOBS_JSON'];
		
		#QUERY DEFIND DISK
		$REPL_DISK = $this -> ReplMgmt -> query_replica_disk($REPL_UUID);
		
		#HOST_NAME
		$HOST_NAME = $REPL_QUERY['HOST_NAME'];
		
		#LOADER ADDRESS
		$LOADER_DIRECT_MODE = $LOAD_INFO['SERV_INFO']['direct_mode'];
		
		if ($LOADER_DIRECT_MODE == TRUE)
		{		
			$LOADER_ADDR = $LOAD_INFO['SERV_ADDR'];
		}
		else
		{
			$LOADER_ADDR = array($LOAD_INFO['SERV_INFO']['machine_id']);
		}		
		
		#HOST SERVER UUID
		$HOST_UUID = $LOAD_INFO['OPEN_HOST'];
		
		#CLOUD UUID
		$CLUSTER_UUID = $REPL_QUERY['CLUSTER_UUID'];		
		
		#CLOUD INFOMATION
		$CLOUD_INFO	= $this -> query_cloud_info($CLUSTER_UUID);
		if (strpos($LOAD_INFO['OPEN_HOST'], '|') !== false)
		{
			$INSTANCE_INFO = explode('|',$LOAD_INFO['OPEN_HOST']);
			$INSTANCE_ID   = $INSTANCE_INFO[0];
			$INSTANCE_AZ   = $INSTANCE_INFO[1];
		}
		else
		{
			$INSTANCE_ID = $LOAD_INFO['OPEN_HOST'];
			$INSTANCE_AZ = 'NA';
		}
		
		#CLOUD MAPPING DISK
		if ($REPL_CONF -> cloud_mapping_disk != false)
		{
			$CLOUD_MAPPING_DISK = explode(',',$REPL_CONF -> cloud_mapping_disk);
		}
		else
		{
			$CLOUD_MAPPING_DISK = false;
		}
		
		#EXTRA DISK CONFIGURATION
		$EXTRA_GB = $REPL_CONF -> extra_gb;

		#DEFAULT DEFINE
		$DISK_CONTROL_CODE = true;
		for ($i=0; $i<count($REPL_DISK); $i++)
		{
			$DISK_UUID = $REPL_DISK[$i]['DISK_UUID'];
			if ($REPL_CONF -> create_by_partition == TRUE)
			{
				$DISK_SIZE = floor((($REPL_DISK[$i]['DISK_SIZE'] - 1 + 1024 * 1024) / 1024 / 1024 / 1024) + 1);
			}
			else
			{
				$DISK_SIZE = $REPL_DISK[$i]['DISK_SIZE'] / 1024 / 1024 / 1024;
				
				#FOR DISK CANNOT BE DIVISIBLE
				if (is_float($DISK_SIZE))
				{
					$DISK_SIZE = ceil($DISK_SIZE);	
				}	
				
				#EXTRA GB TO CREATE DISK				
				if ($EXTRA_GB == TRUE)
				{						
					$DISK_SIZE = $DISK_SIZE + 1;
				}
			}
			
			#DEFINE BOOT FLAG
			$IS_BOOT = $REPL_DISK[$i]['IS_BOOT'];
			
			if( $CLOUD_INFO["CLOUD_TYPE"] == "Tencent" )
			{
				$DISK_SIZE = ceil( $DISK_SIZE / 10 )* 10 ;
			}
			else if($CLOUD_INFO["CLOUD_TYPE"] == "Aliyun" )
			{
				$DISK_SIZE = ( $DISK_SIZE >= 20 )? $DISK_SIZE : 20;
			}
			else if($CLOUD_INFO["CLOUD_TYPE"] == "Ctyun")
			{
				if ($IS_BOOT == TRUE)
				{
					$DISK_SIZE = ( $DISK_SIZE < 40 )? 40 : $DISK_SIZE;
				}
				else
				{
					$DISK_SIZE = ( $DISK_SIZE < 10 )? 10: $DISK_SIZE;
				}
			}
			
			if( $CLOUD_INFO["CLOUD_TYPE"] != "VMWare" )
			{
				$MESSAGE = $this -> ReplMgmt -> job_msg('Creating volume %1% with size %2%', array($i,$DISK_SIZE.'GB.'));
				$this -> ReplMgmt -> update_job_msg($REPL_UUID,$MESSAGE,'Replica');					
			}
			
			$OPEN_DISK = $REPL_DISK[$i]['OPEN_DISK'];

			if ($OPEN_DISK == '00000000-0000-0000-0000-000000000000')
			{
				#LOCK DISK
				$OPEN_DISK = '00000000-LOCK-0000-LOCK-000000000000';
				$this -> update_replica_disk($REPL_UUID,$DISK_UUID,$OPEN_DISK);
				
				#NEW DISK TAG HOSTNAME
				$TAG_NAME = $HOST_NAME.'_'.$i;
		
				#GET LIST SCSI ADDRESS
				$FI_SCSI_ADDR = $this -> list_disk_addr_info($HOST_UUID,$LOADER_ADDR,'Loader','MS','Physical');
				
				#DISK MOUNT ERROR CONTROL
				if ($FI_SCSI_ADDR == '')
				{
					#MESSAGE
					$MESSAGE = $this -> ReplMgmt -> job_msg('Cannot get volume address information.');
					$this -> ReplMgmt -> update_job_msg($REPL_UUID,$MESSAGE,'Replica');
					$DISK_CONTROL_CODE = false;
					break;
				}
				else
				{
					switch ($CLOUD_INFO["CLOUD_TYPE"])
					{
						case 'OPENSTACK':
							#CREATE VOLUME
							$VOLUME_INFO = $this -> OpenStackMgmt -> begin_volume_for_loader($CLUSTER_UUID,$HOST_UUID,$DISK_SIZE,$REPL_UUID,$HOST_NAME);
					
							if ($VOLUME_INFO -> volumeMountStatus == TRUE)
							{	
								#SAVE OPENSTACK VOLUME INFO
								$this -> save_volume_info($REPL_UUID,$DISK_UUID,$VOLUME_INFO,$HOST_UUID,$LOADER_ADDR,$EXTRA_GB,$FI_SCSI_ADDR);
							
								#MESSAGE
								$MESSAGE = $this -> ReplMgmt -> job_msg('Data volume created and attached to Transport server.');
								$this -> ReplMgmt -> update_job_msg($REPL_UUID,$MESSAGE,'Replica');
							}
							else
							{
								#MESSAGE
								$MESSAGE = $this -> ReplMgmt -> job_msg('Cannot attach volume to the Transport server.');
								$this -> ReplMgmt -> update_job_msg($REPL_UUID,$MESSAGE,'Replica');
								$DISK_CONTROL_CODE = false;
								break 2;					
							}
						break;
						
						case 'AWS':
							#CREATE VOLUME
							$VOLUME_INFO = $this -> AwsMgmt -> begin_volume_for_loader($CLUSTER_UUID,$INSTANCE_AZ,$INSTANCE_ID,$DISK_SIZE,$TAG_NAME);
							
							#SAVE AWS VOLUME INFO
							$this -> save_volume_info($REPL_UUID,$DISK_UUID,$VOLUME_INFO,$HOST_UUID,$LOADER_ADDR,$EXTRA_GB,$FI_SCSI_ADDR);
							
							#MESSAGE
							$MESSAGE = $this -> ReplMgmt -> job_msg('A volume created and attached to the Transport server.');
							$this -> ReplMgmt -> update_job_msg($REPL_UUID,$MESSAGE,'Replica');
						break;
						
						case 'Azure':
							try{
								#CREATE VOLUME
								$this->AzureMgmt->setReplicaId( $REPL_UUID );
								$this -> AzureMgmt -> getVMByServUUID($LOAD_INFO['SERV_UUID'],$CLUSTER_UUID);
								$VOLUME_INFO = $this -> AzureMgmt -> begin_azure_volume_for_loader($CLUSTER_UUID,$INSTANCE_AZ,$INSTANCE_ID,$DISK_SIZE,$TAG_NAME);

								#SAVE AWS VOLUME INFO
								$this -> save_volume_info($REPL_UUID,$DISK_UUID,$VOLUME_INFO,$HOST_UUID,$LOADER_ADDR,$EXTRA_GB,$FI_SCSI_ADDR);
								
								#MESSAGE
								$MESSAGE = $this -> ReplMgmt -> job_msg('A volume created and attached to the Transport server.');
								$this -> ReplMgmt -> update_job_msg($REPL_UUID,$MESSAGE,'Replica');
							}catch (Exception $e) {
								$MESSAGE = $this -> ReplMgmt -> job_msg('Cannot create or mount the volume to the Transport server.');
								$this -> ReplMgmt -> update_job_msg($REPL_UUID,$MESSAGE,'Replica');
								$DISK_CONTROL_CODE = false;
							}
						break;
						
						case 'Aliyun':
						case 'Tencent':								
							if( $CLOUD_INFO["CLOUD_TYPE"] == 'Aliyun')
								$VOLUME_INFO = $this ->AliMgmt -> begin_volume_for_loader($CLUSTER_UUID,$INSTANCE_AZ,$INSTANCE_ID,$DISK_SIZE,$TAG_NAME);
							else if( $CLOUD_INFO["CLOUD_TYPE"] == 'Tencent' )
								$VOLUME_INFO = $this ->TencentMgmt->begin_volume_for_loader($CLUSTER_UUID,$INSTANCE_AZ,$INSTANCE_ID,$DISK_SIZE,$TAG_NAME);
							
							if( !$VOLUME_INFO ){
								$MESSAGE = $this -> ReplMgmt -> job_msg('Cannot create or mount the volume to the Transport server.');
								$this -> ReplMgmt -> update_job_msg($REPL_UUID,$MESSAGE,'Replica');
								$DISK_CONTROL_CODE = false;
							}
							
							$this -> save_volume_info($REPL_UUID,$DISK_UUID,$VOLUME_INFO,$HOST_UUID,$LOADER_ADDR,$EXTRA_GB,$FI_SCSI_ADDR);
							
							#MESSAGE
							$MESSAGE = $this -> ReplMgmt -> job_msg('A volume created and attached to the Transport server.');
							$this -> ReplMgmt -> update_job_msg($REPL_UUID,$MESSAGE,'Replica');
						break;

						case 'Ctyun':
							if ($CLOUD_MAPPING_DISK[$i] == 'CreateOnDemand')
							{
								$VOLUME_INFO = $this -> CtyunMgmt -> begin_volume_for_loader($CLUSTER_UUID,$HOST_UUID,$DISK_SIZE,$REPL_UUID,$TAG_NAME,$IS_BOOT);
							}
							else
							{					
								$VOLUME_INFO = $this -> CtyunMgmt -> attach_subscription_volume($REPL_UUID,$CLUSTER_UUID,$HOST_UUID,$CLOUD_MAPPING_DISK[$i]);
							}
							
							if ($VOLUME_INFO == FALSE)
							{
								$DISK_CONTROL_CODE = false;
								break 2;
							}
							else
							{							
								#SAVE VOLUME INFO
								$this -> save_volume_info($REPL_UUID,$DISK_UUID,$VOLUME_INFO,$HOST_UUID,$LOADER_ADDR,$EXTRA_GB,$FI_SCSI_ADDR);
							
								#MESSAGE
								$MESSAGE = $this -> ReplMgmt -> job_msg('Data volume created and attached to Transport server.');
								$this -> ReplMgmt -> update_job_msg($REPL_UUID,$MESSAGE,'Replica');
							}
						break;
					}
				}
			}					
		}
		return $DISK_CONTROL_CODE;
	}	
	
	
	#############################
	#
	#	VIRTUAL PACKER DISK MAPPING
	#
	#############################
	private function virtual_packer_disk_mapping($REPL_UUID,$HOST_INFO)
	{
		#QUERY DEFIND DISK
		$REPL_DISK = $this -> ReplMgmt -> query_replica_disk($REPL_UUID);
		
		$HOST_DISK = $HOST_INFO['HOST_INFO']['disks'];
		
		$RETURN_OUTPUT = array();
		for ($i=0; $i<count($REPL_DISK); $i++)
		{		
			for ($x=0; $x<count($HOST_DISK); $x++)
			{
				if ($REPL_DISK[$i]['DISK_UUID'] == strtoupper($HOST_DISK[$x]['id']))
				{			
					$RETURN_OUTPUT[$HOST_DISK[$x]['id']] = $HOST_DISK[$x]['size'];
					break;
				}
			}
		}
		arsort($RETURN_OUTPUT);
		return $RETURN_OUTPUT;
	}
	
	
	#############################
	#
	#	OFFLINE PACKER DISK MAPPING
	#
	#############################
	private function offline_packer_disk_mapping($REPL_UUID,$HOST_INFO)
	{
		#QUERY DEFIND DISK
		$REPL_DISK = $this -> ReplMgmt -> query_replica_disk($REPL_UUID);
		
		$HOST_DISK = $HOST_INFO['HOST_INFO']['disk_infos'];
		
		for($i=0; $i<count($REPL_DISK); $i++)
		{	
			$REPL_DISK_URI[] = json_decode($REPL_DISK[$i]['PACK_URI'],true);
			$REPL_DISK_URI[$i]['DISK_SIZE'] = $REPL_DISK[$i]['DISK_SIZE'];
			$REPL_DISK_URI[$i]['DISK_UUID'] = $REPL_DISK[$i]['DISK_UUID'];
		}

		for ($x=0; $x<count($HOST_DISK); $x++)
		{
			$HOST_DISK_URL[] = json_decode($HOST_DISK[$x]['uri'],true);
			$HOST_DISK_URL[$x]['size'] = $HOST_DISK[$x]['size'];
			$HOST_DISK_URL[$x]['number'] = $HOST_DISK[$x]['number'];
		}
		
		$RETURN_OUTPUT = array();
		for ($w=0; $w<count($REPL_DISK_URI); $w++)
		{
			for ($w1=0; $w1<count($HOST_DISK_URL); $w1++)
			{
				if ($REPL_DISK_URI[$w]['DISK_SIZE'] == $HOST_DISK_URL[$w1]['size'])
				{
					$RETURN_OUTPUT[$REPL_DISK_URI[$w]['DISK_UUID']] = $HOST_DISK_URL[$w1]['size'];					
				}
			}		
		}
		arsort($RETURN_OUTPUT);		
		return $RETURN_OUTPUT;
	}	
	
	#############################
	#
	#	PHYSICAL PACKER DISK MAPPING
	#
	#############################
	private function physical_packer_disk_mapping($REPL_UUID,$HOST_INFO)
	{
		#QUERY REPLICA INFORMATION
		$REPL_QUERY = $this -> ReplMgmt -> query_replica($REPL_UUID);
		$JOB_INFO = $REPL_QUERY['JOBS_JSON'];
		
		#QUERY DEFIND DISK
		$REPL_DISK = $this -> ReplMgmt -> query_replica_disk($REPL_UUID);

		#FOR REPLICA DISK
		for($i=0; $i<count($REPL_DISK); $i++)
		{
			#DECODE URI JSON
			$REPL_DISK_URI = json_decode($REPL_DISK[$i]['PACK_URI'],true);
	
			#EXTRACT OUT partition_style
			$REPL_DISK[$i]['partition_style'] = $REPL_DISK_URI['partition_style'];	
			
			#PREPARE COMPARE STRING
			if ($REPL_DISK_URI['serial_number'] != '' AND $REPL_DISK_URI['serial_number'] != '2020202020202020202020202020202020202020' AND $REPL_DISK_URI['serial_number'] != '00000000-0000-0000-0000-000000000000')
			{
				$REPL_DISK[$i]['compare_string'] = $REPL_DISK_URI['serial_number'];				
			}			
			elseif (strpos($REPL_DISK_URI['friendly_name'], '/dev/') == 0)
			{
				$REPL_DISK[$i]['compare_string'] = $REPL_DISK_URI['address'];
			}
			elseif ($REPL_DISK_URI['partition_style'] == 1)
			{
				$REPL_DISK[$i]['compare_string'] = $REPL_DISK_URI['mbr_signature'];	
			}
			elseif ($REPL_DISK_URI['partition_style'] == 2)
			{
				$REPL_DISK[$i]['compare_string'] = $REPL_DISK_URI['unique_id'];	
			}
			else
			{
				return array();				
			}			
		}

		#FOR HOST DISK
		$HOST_DISK = $HOST_INFO['HOST_INFO']['disk_infos'];
		
		for ($x=0; $x<count($HOST_DISK); $x++)
		{
			#DECODE URI JSON
			$HOST_DISK_URI = json_decode($HOST_DISK[$x]['uri'],true);
			
			#EXTRACT OUT partition_style
			$HOST_DISK[$x]['partition_style'] = $HOST_DISK_URI['partition_style'];	
			
			#PREPARE COMPARE STRING
			if ($HOST_DISK_URI['serial_number'] != '' AND $HOST_DISK_URI['serial_number'] != '2020202020202020202020202020202020202020' AND $HOST_DISK_URI['serial_number'] != '00000000-0000-0000-0000-000000000000')
			{
				$HOST_DISK[$x]['compare_string'] = $HOST_DISK_URI['serial_number'];				
			}
			elseif (strpos($HOST_DISK_URI['friendly_name'], '/dev/') == 0)
			{
				$HOST_DISK[$x]['compare_string'] = $HOST_DISK_URI['address'];
			}
			elseif ($HOST_DISK_URI['partition_style'] == 1)
			{
				$HOST_DISK[$x]['compare_string'] = $HOST_DISK_URI['mbr_signature'];	
			}
			elseif ($HOST_DISK_URI['partition_style'] == 2)
			{
				$HOST_DISK[$x]['compare_string'] = $HOST_DISK_URI['unique_id'];	
			}			
			else
			{
				return array();
			}
		}

		#BEGIN TO COMPARE
		$DISK_MAPPING = false;
		for ($r=0; $r<count($REPL_DISK); $r++)
		{
			for ($h=0; $h<count($HOST_DISK); $h++)
			{
				if ($REPL_DISK[$r]['compare_string'] == $HOST_DISK[$h]['compare_string'])
				{
					$DISK_MAPPING[] = array('DISK_UUID' => $REPL_DISK[$r]['DISK_UUID'], 'number' => $HOST_DISK[$h]['number'],'partition_style' => $HOST_DISK[$h]['partition_style']);
					break;
				}
			}		
		}

		if ($DISK_MAPPING == FALSE)
		{
			$JOB_INFO -> skip_delete_conn_check = true;
			$this -> update_trigger_info($REPL_UUID,$JOB_INFO,'REPLICA');

			$MESSAGE = $this -> ReplMgmt -> job_msg('Failed to get disk mapping information.');
			$this -> ReplMgmt -> update_job_msg($REPL_UUID,$MESSAGE,'Replica');
			exit;
		}
		else
		{
			$RETURN_OUTPUT = array();

			$PARTITION_INFO = $HOST_INFO['HOST_INFO']['partition_infos'];
			for ($f=0; $f<count($DISK_MAPPING); $f++)
			{
				$DISK_UUID 		 = $DISK_MAPPING[$f]['DISK_UUID'];
				$PARTITION_STYLE = $DISK_MAPPING[$f]['partition_style'];
				$DISK_NUMBER 	 = $DISK_MAPPING[$f]['number'];

				if ($PARTITION_STYLE == 1 OR $PARTITION_STYLE == 2)
				{
					$CALC_DISK_SIZE = array();
					for ($j=0; $j<count($PARTITION_INFO); $j++)
					{
						if ($PARTITION_INFO[$j]['disk_number'] == $DISK_NUMBER)
						{
							$CALC_DISK_SIZE[] = $PARTITION_INFO[$j]['offset'] + $PARTITION_INFO[$j]['size'];
						}
					}
					$RETURN_OUTPUT[$DISK_UUID] = max($CALC_DISK_SIZE);	
				}
			}
			
			if (count($RETURN_OUTPUT) == 0)
			{
				$JOB_INFO -> skip_delete_conn_check = true;
				$this -> update_trigger_info($REPL_UUID,$JOB_INFO,'REPLICA');
			
				$MESSAGE = $this -> ReplMgmt -> job_msg('Unsupported disk partition.');
				$this -> ReplMgmt -> update_job_msg($REPL_UUID,$MESSAGE,'Replica');					
				exit;
			}
			else
			{			
				arsort($RETURN_OUTPUT);
				return $RETURN_OUTPUT;
			}
		}
	}
	
	#############################
	#
	#	TEST EXPORT FOLDER
	#
	#############################
	private function test_export_folder($LOADER_ADDR,$EXPORT_FOLDER)
	{
		#GEN CONNECTION UUID
		$CONN_UUID  = Misc_Class::guid_v4();
		
		#GET DEFAULT CONNECTION INFORMATION
		$CONN_INFO = Misc_Class::connection_parameter();
		$IS_COMPRESSED 	 = $CONN_INFO['is_compressed'];
		$IS_CHECKSUM	 = $CONN_INFO['is_checksum'];
		$IS_ENCRYPTED 	 = $CONN_INFO['is_encrypted'];
		
		$CONN_ARRAY = array('type' 			=> 0,
							'id' 			=> $CONN_UUID,
							'options' 		=> array('folder' => $EXPORT_FOLDER),
							'compressed' 	=> $IS_COMPRESSED, 
							'checksum' 		=> $IS_CHECKSUM,
							'encrypted' 	=> $IS_ENCRYPTED);
		
		$PORT_NUM = \saasame\transport\Constant::get('TRANSPORT_SERVICE_PORT');		
			
		$OpenProtocol = $this -> common_connection('127.0.0.1',$PORT_NUM);
			
		$ClientCall = new saasame\transport\transport_serviceClient($OpenProtocol);
		
		$CONNECTION = new saasame\transport\connection($CONN_ARRAY);
		
		$SERVICE_TYPE = \saasame\transport\Constant::get('LOADER_SERVICE');
			
		#DEFINE DEFAULT RESPONSE
		
		for ($i=0; $i<count($LOADER_ADDR); $i++)
		{
			try{
				$Response = $ClientCall -> test_connection_p($LOADER_ADDR[$i],$CONNECTION,$SERVICE_TYPE);
				
				if ($Response == TRUE)
				{
					break;
				}
			}
			catch (Throwable $e){			
				$Response = false;
			}
		}		
		return $Response;
	}
	
	
	#############################
	#
	#	PRE CREATE REPLICA JOB
	#
	#############################
	public function pre_create_replica_job($ACCT_UUID,$REGN_UUID,$CARR_UUID,$LOAD_UUID,$LAUN_UUID,$PACK_UUID,$REPL_CONF,$MGMT_ADDR,$REPL_JOB_UUID)
	{
		if( isset( $REPL_CONF["VMWARE_STORAGE"] ) ){
			
			$AuthInfo = $this->VMWareMgmt->getESXAuthInfo( $LAUN_UUID );

			$CommonModel = new Common_Model();
	
			$TransportInfo = $CommonModel->getTransportInfo( $LAUN_UUID );
	
			$hosts_info = (array)$this->VMWareMgmt->getVirtualHosts( $TransportInfo["ConnectAddr"], $AuthInfo["EsxIp"], $AuthInfo["Username"], $AuthInfo["Password"] );

			$REPL_CONF["CONNECTION_TYPE"] = $hosts_info[0]->connection_type;
		}

		#QUERY CONNECTION INFORMATION
		$CONN_INFO = $this -> ServerMgmt -> query_connection_by_serv_uuid($CARR_UUID,$LOAD_UUID,$LAUN_UUID);
		
		#DEFINE MACHINE ID
		$MACHINE_ID = $CONN_INFO['CARR_INFO']['SERV_INFO']['machine_id'];

		#CHECK IS EXPORT JOB
		if ($REPL_CONF['ExportPath'] == '')
		{
			$IS_EXPORT = false;
		}
		else
		{
			#SET BACK LAUNCHER AS LOADER
			#$LAUN_UUID = $this -> ServerMgmt -> list_match_service_id($MACHINE_ID)['ServiceId']['Launcher'];
			$IS_EXPORT = true;
		}
		
		#CREATE LocalFolder CONNECTIONS
		$SOURCE_CONN = $CONN_INFO['SOURCE_CONN'];
		if ($SOURCE_CONN == '')
		{ 
			$LIST_SERVICES = $this -> ServerMgmt -> list_match_service_id($MACHINE_ID)['ServiceId'];
			
			$UUID_SCHD = $LIST_SERVICES['Scheduler'];
			$UUID_CARR = $LIST_SERVICES['Carrier'];
			$UUID_LOAD = $LIST_SERVICES['Loader'];
			$UUID_LAUN = $LIST_SERVICES['Launcher'];
			$CLUSTER_UUID = $CONN_INFO['CARR_INFO']['OPEN_UUID'];
			$CONN_DEST = str_replace('\\Launcher','',$CONN_INFO['CARR_INFO']['SERV_INFO']['path']).'\imagex';
			
			$CreateConnection = $this -> test_add_connections($UUID_SCHD,$UUID_CARR,$UUID_LOAD,$UUID_LAUN,$CONN_DEST,'Add','LocalFolder');
			$CreateConnection['CLUSTER_UUID'] = $CLUSTER_UUID;
			$CreateConnection['MGMT_ADDR']  = $MGMT_ADDR;
			Misc_Class::function_debug('_mgmt',__FUNCTION__,$CreateConnection);
			
			$this -> ServerMgmt -> save_connection($CreateConnection);
			
			#RE-QUERY CONNECTION INFORMATION
			$CONN_INFO = $this -> ServerMgmt -> query_connection_by_serv_uuid($CARR_UUID,$LOAD_UUID,$LAUN_UUID);
		}
		
		$CLUSTER_UUID = $CONN_INFO['CLUSTER_UUID'];

		#QUERY CARRIER SERVER INFOMATION
		$CARRIER_INFO = $this -> ServerMgmt -> query_server_info($CARR_UUID);
		
		#DECLARED TRANSPORT TYPE
		$DIRECT_MODE = $CARRIER_INFO['SERV_INFO']['direct_mode'];
		$REPL_CONF['DirectMode'] = $DIRECT_MODE;
				
		#ADD JOB VERSION
		$SOURCE_TRANSPORT_VERSION = $CARRIER_INFO['SERV_INFO']['version'];
		$REPL_CONF['CreateByVersion'] = $SOURCE_TRANSPORT_VERSION;

		#SET REPLICA JOB START TIME
		if ($REPL_CONF['StartTime'] != '')
		{
			#CONVERT TIME TO UTC TIME
			$USER_START_TIME = $REPL_CONF['StartTime'].' '.$REPL_CONF['TimeZone'];
			$CALC_START_TIME = new DateTime($USER_START_TIME);
			$CALC_START_TIME -> setTimezone(new DateTimeZone('UTC'));
			$REPL_CONF['UTC_START_TIME'] = $CALC_START_TIME -> format('Y-m-d H:i:s');
		}
		else
		{
			$REPL_CONF['UTC_START_TIME'] = '';
		}

		#QUERY LOADER SERVER INFORMATION
		$LOADER_INFO = $this -> ServerMgmt -> query_server_info($LOAD_UUID);

		#CHECK FOR RECOVER KIT
		$IS_WINPE = $LOADER_INFO['SERV_INFO']['is_winpe'];
		
		#CHECK FOR LOADER COMMUNICATION MODE
		$LOADER_DIRECT_MODE = $LOADER_INFO['SERV_INFO']['direct_mode'];

		if ($LOADER_DIRECT_MODE == TRUE)
		{		
			$LOADER_ADDR = $LOADER_INFO['SERV_ADDR'];
		}
		else
		{
			$LOADER_ADDR = array($LOADER_INFO['SERV_INFO']['machine_id']);
		}
		
		#SET LOADER KEEP ALIVE
		$REPL_CONF['loader_keep_alive'] = $IS_WINPE;
		
		#CREATE REPLICA JOB
		$REPL_UUID = $this -> ReplMgmt -> create_replica($ACCT_UUID,$REGN_UUID,$CLUSTER_UUID,$MGMT_ADDR,$PACK_UUID,$REPL_CONF,$IS_WINPE,$REPL_JOB_UUID);

		#CHECK SAASAME LICENSE
		$CHECK_LICENSE = $this -> is_license_valid($REPL_UUID);
		if ($CHECK_LICENSE == FALSE)
		{
			$MESSAGE = $this -> ReplMgmt -> job_msg('Not enough licenses to run this process.');
			$this -> ReplMgmt -> update_job_msg($REPL_UUID,$MESSAGE,'Replica');
			exit;
		}
		
		#REFLUSH SELECT VIRTUAL HOST DISK INFORMATION
		//$this -> reflush_select_virtual_host_disk($PACK_UUID,$REPL_UUID);		
		
		#QUERY HOST INFORMATION
		$HOST_INFO = $this -> ServerMgmt -> query_host_info($PACK_UUID);
		$HOST_NAME = $HOST_INFO['HOST_NAME'];
		$HOST_TYPE = $HOST_INFO['HOST_TYPE'];
		$HOST_DISK = $HOST_INFO['HOST_DISK'];
		
		#SKIP DISK	
		$SKIP_DISK = explode(',',$REPL_CONF['SkipDisk']);
		for ($i=0; $i<count($HOST_DISK); $i++)
		{
			foreach ($SKIP_DISK as $SKIP_KEY => $SKIP_UUID)
			{
				$HOST_DISK = array_values($HOST_DISK); 	#RESET HOST DISK INDEX
				if (array_search($SKIP_UUID,$HOST_DISK[$i]) == 'DISK_UUID')
				{
					unset($HOST_DISK[$i]);
				}
			}
		}

		#GET CHECKSUM FLAG
		$CHECKSUM = $REPL_CONF['ChecksumVerify'];
		
		#SET WEBDAV PRIORITY ADDRESS
		$WEBDAV_PRIORITY_ADDR = $REPL_CONF['WebDavPriorityAddr'];
	
		#CREATE DISK BY BOUNDARY SIZE
		if ($HOST_TYPE == 'Physical')
		{
			$USE_BOUNDARY = $REPL_CONF['CreateByPartition'];
		}
		else
		{
			$USE_BOUNDARY = false;
		}
	
		#QUERY AND CREATE TARGET CONNECTION
		$TARGET_CONN = $this -> create_connection($ACCT_UUID,$REPL_UUID,$CARR_UUID,$LOAD_UUID,$LAUN_UUID,$CLUSTER_UUID,$CONN_INFO,$MGMT_ADDR,$CHECKSUM,$WEBDAV_PRIORITY_ADDR);

		#SET REPLICA JOB ARRAY
		$JOB_INFO = array();
		
		if ($TARGET_CONN != FALSE)
		{		
			#CREATE REPLICA DISK INFORMATION
			$this -> new_replica_disk($REPL_UUID,$HOST_DISK,$USE_BOUNDARY);
		
			#EXPORT JOB MESSAGE
			if ($IS_EXPORT == TRUE)
			{
				$TEST_FOLDER = $this -> test_export_folder($LOADER_ADDR,$REPL_CONF['ExportPath']);
				if ($TEST_FOLDER == FALSE)
				{
					#MESSAGE
					$MESSAGE = $this -> ReplMgmt -> job_msg('Image export folder %1% does not exist.',array($REPL_CONF['ExportPath']));
					$this -> ReplMgmt -> update_job_msg($REPL_UUID,$MESSAGE,'Replica');
					return false;
				}
				else
				{
					#MESSAGE
					$MESSAGE = $this -> ReplMgmt -> job_msg('The export image will save to %1%',array($REPL_CONF['ExportPath']));
					$this -> ReplMgmt -> update_job_msg($REPL_UUID,$MESSAGE,'Replica');
				}
			}			
			
			#PRE CREATE LOADER JOB DISK
			$OS_TYPE = 'WINDOWS'; #DEFINE FOR MUTEX ACTION

			#MUTEX MSG
			$MUTEX_MSG = $REPL_UUID.'-'.$MGMT_ADDR.'-'.__FUNCTION__;
		
			#LOCK WITH MUTEX CONTROL
			$this -> disk_mutex_action($REPL_UUID,'Replica',$LOADER_ADDR,$MUTEX_MSG,'LOCK_EX',$OS_TYPE);
			
			#CREATE LOADER DISK			
			$CREATE_DISK = $this -> pre_create_loader_job_disk($REPL_UUID,$REPL_CONF,$LOADER_INFO,$HOST_INFO);
			
			#UNLOCK WITH MUTEX CONTROL
			$this -> disk_mutex_action($REPL_UUID,'Replica',$LOADER_ADDR,$MUTEX_MSG,'LOCK_UN',$OS_TYPE);

			#CREATE REPLICA JOB
			if ($CREATE_DISK == FALSE)
			{
				#MARK ERROR
				$JOB_INFO["is_error"] = true;
				$this -> update_trigger_info($REPL_UUID,$JOB_INFO,'REPLICA');
				return false;
			}
			else
			{	
				$this -> create_replica_job($REPL_UUID,$CONN_INFO,$TARGET_CONN,$PACK_UUID,$HOST_TYPE,$REPL_CONF,$MGMT_ADDR,$DIRECT_MODE);
			}
		}
		else
		{
			#MESSAGE
			$MESSAGE = $this -> ReplMgmt -> job_msg('Cannot create webDAV connection.');
			$this -> ReplMgmt -> update_job_msg($REPL_UUID,$MESSAGE,'Replica');
			
			#MARK ERROR
			$JOB_INFO["is_error"] = true;
			$this -> update_trigger_info($REPL_UUID,$JOB_INFO,'REPLICA');
			return false;
		}
	}
	
	###########################
	#DISK FLOCK ACTION
	###########################
	private function create_mutex($SERVER_ADDR,$MUTEX_MSG,$TIMEOUT)
	{
		$PORT_NUM = \saasame\transport\Constant::get('TRANSPORT_SERVICE_PORT');
		
		$OpenProtocol = $this -> common_connection('127.0.0.1',$PORT_NUM);
		
		$ClientCall = new saasame\transport\transport_serviceClient($OpenProtocol);

		for ($i=0; $i<count($SERVER_ADDR); $i++)
		{
			try{	
				$CreateMutex = $ClientCall -> create_mutex_p($SERVER_ADDR[$i],$MUTEX_MSG,$TIMEOUT);
				
				if ($CreateMutex == TRUE)
				{
					break;
				}
			}
			catch (Throwable $e){
				$CreateMutex = true;
				break;
			}
		}
		return $CreateMutex;
	}
		
	###########################
	#DISK FLOCK ACTION
	###########################
	private function delete_mutex($SERVER_ADDR,$MUTEX_MSG)
	{
		$PORT_NUM = \saasame\transport\Constant::get('TRANSPORT_SERVICE_PORT');
		
		$OpenProtocol = $this -> common_connection('127.0.0.1',$PORT_NUM);
		
		$ClientCall = new saasame\transport\transport_serviceClient($OpenProtocol);

		for ($i=0; $i<count($SERVER_ADDR); $i++)
		{
			try{
				$DeleteMutex = $ClientCall -> delete_mutex_p($SERVER_ADDR[$i],$MUTEX_MSG);
				if ($DeleteMutex == TRUE)
				{
					break;
				}
			}
			catch (Throwable $e){
				$DeleteMutex = true;
				break;
			}
		}
		return $DeleteMutex;
	}
	
	###########################
	#DISK MUTEX ACTION
	###########################
	public function disk_mutex_action($JOB_UUID,$JOB_TYPE,$SERVER_ADDR,$MUTEX_MSG,$ACTION,$TYPE)
	{
		if ($TYPE == 'WINDOWS')
		{
			switch ($ACTION)
			{
				case "LOCK_EX":
					$TIMEOUT = 25*15;
					
					if ($this -> create_mutex($SERVER_ADDR,$MUTEX_MSG,$TIMEOUT) == FALSE)
					{
						for($i=1; $i<=20; $i++)
						{
							if ($this -> create_mutex($SERVER_ADDR,$MUTEX_MSG,$TIMEOUT) == FALSE)
							{
								$MESSAGE = $this -> ReplMgmt -> job_msg('The other disk action is processing please wait.<sup>%1%</sup>',array($i));
								$this -> ReplMgmt -> update_job_msg($JOB_UUID,$MESSAGE,$JOB_TYPE);
								sleep(15);
							}
							else
							{
								break;
							}
						}
					}
				break;
					
				case "LOCK_UN":
					return $this -> delete_mutex($SERVER_ADDR,$MUTEX_MSG);				
				break;			
			}
		}
		else
		{
			#DEFINE LOCK FILE
			$LOCK_FILE = __DIR__ .'\\_flock\\'.$JOB_UUID.'.lock';
			
			switch ($ACTION)
			{
				case "LOCK_EX":
					if (file_exists($LOCK_FILE) == TRUE)
					{
						for($i=1; $i<=20; $i++)
						{
							$CHECK_CODE = false;
							if (file_exists($LOCK_FILE) == TRUE)
							{
								$MESSAGE = $this -> ReplMgmt -> job_msg('The other disk action is processing please wait.<sup>%1%</sup>',array($i));
								$this -> ReplMgmt -> update_job_msg($JOB_UUID,$MESSAGE,$JOB_TYPE);
								sleep(15);					
							}
							else
							{
								$CHECK_CODE = true;
								break;
							}
						}
						
						if ($CHECK_CODE == TRUE)
						{
							fopen($LOCK_FILE,'wb');
						}
						else
						{
							$MESSAGE = $this -> ReplMgmt -> job_msg('Fail to proceed disk create action.');
							$this -> ReplMgmt -> update_job_msg($JOB_UUID,$MESSAGE,$JOB_TYPE);
							exit;
						}
					}
					else
					{
						fopen($LOCK_FILE,'wb');
					}
				break;
				
				case "LOCK_UN":
					unlink($LOCK_FILE);
				break;			
			}			
		}		
	}
	
	###########################
	#QUERY AND SUBMIT MULTIPLE MGMT ADDRESS
	###########################
	public function query_multiple_mgmt_address($ACCT_UUID,$REPL_UUID,$MGMT_ADDR=null)
	{
		#MGMT ADDRESS FROM SELECT REPLICA
		$QUERY_REPL_MGMT = "SELECT * FROM _REPLICA WHERE _REPL_UUID = '".$REPL_UUID."' AND _STATUS = 'Y'";
		$REPL_QUERY = $this -> DBCON -> query($QUERY_REPL_MGMT);
		$REPL_QUERY -> setFetchMode(PDO::FETCH_ASSOC);		
		$ReplQueryResult = $REPL_QUERY -> fetchAll();
		
		$ReplCount = count($ReplQueryResult);
		if ($ReplCount != 0)
		{
			for ($i=0; $i<$ReplCount; $i++)
			{
				$REPL_MGMT_ADDR_ARRAY[$i] = $ReplQueryResult[$i]['_MGMT_ADDR'];
			}
		}
		else
		{
			return false;
		}
		
		#MGMT ADDRESS FROM ACCOUNT CONNECTION
		$QUERY_CONN_MGMT = "SELECT DISTINCT _MGMT_ADDR FROM _SERVER_CONN WHERE _ACCT_UUID = '".$ACCT_UUID."' AND _STATUS = 'Y'";		
		$CONN_QUERY = $this -> DBCON -> query($QUERY_CONN_MGMT);
		$CONN_QUERY -> setFetchMode(PDO::FETCH_ASSOC);		
		$ConnQueryResult = $CONN_QUERY -> fetchAll();
		
		#DATABASE MGMT ADDRESS
		$ServCount = count($ConnQueryResult);
		if ($ServCount != 0)
		{
			for ($x=0; $x<$ServCount; $x++)
			{
				$CONN_MGMT_ADDR_ARRAY[$x] = $ConnQueryResult[$x]['_MGMT_ADDR'];
			}
		}
		else
		{
			return false;
		}
		
		#MGMT ADDRESS FROM LOCAL HOST
		$MGMT_LAN_IP = array();
		$MGMT_NETWORK_INFO = $this -> get_mgmt_host_info() -> network_infos;
		foreach ($MGMT_NETWORK_INFO as $Network_Key => $Network_Value)
		{
			if (isset($Network_Value -> ip_addresses))
			{
				foreach ($Network_Value -> ip_addresses as $Addr_Key => $Addr_Value)
				{
					if (filter_var($Addr_Value, FILTER_VALIDATE_IP,FILTER_FLAG_IPV4))
					{
						$MGMT_LAN_IP[] = $Addr_Value; 
					}
				}
			}
		}
		
		#MGMT ADDRESS ARRAY RE-INDEX, UNIQUE AND MERGE
		$MGMT_ADDR_ARRAY = array_values(array_unique(array_merge($REPL_MGMT_ADDR_ARRAY,$CONN_MGMT_ADDR_ARRAY,$MGMT_LAN_IP)));
	
		#SHIFT SUBMIT ADDRESS TO THE FRONT
		if ($MGMT_ADDR != null)
		{
			if (!in_array($MGMT_ADDR,$MGMT_ADDR_ARRAY))
			{
				array_unshift($MGMT_ADDR_ARRAY,$MGMT_ADDR);
			}
		}
		
		#FLIP MGMT VALUE AND KEY
		$MGMT_ADDR_ARRAY = array_flip($MGMT_ADDR_ARRAY);

		return $MGMT_ADDR_ARRAY;	
	}
	
	
	###########################
	#CREATE REPLICA JOB AND SUBMIT TO TRANSPORT SERVER
	###########################
	private function create_replica_job($REPL_UUID,$CONN_INFO,$TARGET_CONN,$PACK_UUID,$HOST_TYPE,$REPL_CONF,$MGMT_ADDR,$DIRECT_MODE)
	{
		$ACCT_UUID = $CONN_INFO['ACCT_UUID'];

		#DIRECT/IN-DIRECT MODE
		if ($DIRECT_MODE == TRUE)
		{		
			#QUERY SOURCE CONNECTION INFORMATION		
			$SOURCE_CONN = $CONN_INFO['SOURCE_CONN'];		
			$SOURCE_INFO = $this -> ServerMgmt -> query_connection_info($SOURCE_CONN);			
			$SCHD_ADDR   = $SOURCE_INFO['SCHD_ADDR'];
		}
		else
		{
			#REVERSE SOUCE AND TARGET CONNECTION
			$SOURCE_CONN = $TARGET_CONN;
			#$LOADER_UUID = $CONN_INFO['SERV_INFO']['SERV_UUID'];
			#$TARGET_CONN = $this -> ServerMgmt -> query_server_connection($LOADER_UUID,'LOAD_UUID','LocalFolder');
			
			#USE ADDRESS AS MACHINE UUID			
			$SCHD_ADDR = array($CONN_INFO['CARR_INFO']['SERV_INFO']['machine_id']);
		}

		#GET AZURE MGMT DISK
		$AZURE_MGMT_DISK = $this -> ServerMgmt -> query_connection_info($TARGET_CONN)['AZURE_MGMT_DISK'];		
		
		#GET MGMT ADDRESS
		$MGMT_ADDR_ARRAY = $this -> query_multiple_mgmt_address($ACCT_UUID,$REPL_UUID,$MGMT_ADDR);
		if ($MGMT_ADDR_ARRAY == FALSE)
		{
			$MESSAGE = $this -> ReplMgmt -> job_msg('Cannot get the management server address.');
			$this -> ReplMgmt -> update_job_msg($REPL_UUID,$MESSAGE,'Replica');
			return false;
		}

		#UPDATE CONNECTION JSON INFORMATION
		$CONN_JSON = json_encode(array('SOURCE' => $SOURCE_CONN,'TARGET' => $TARGET_CONN));
		$this -> ReplMgmt -> update_replica_connection($REPL_UUID,$CONN_JSON);
		$MESSAGE = $this -> ReplMgmt -> job_msg('Connection information updated.');
		$this -> ReplMgmt -> update_job_msg($REPL_UUID,$MESSAGE,'Replica');

		#QUERY REPLICA INFORMATION
		$REPL_QUERY = $this -> ReplMgmt -> query_replica($REPL_UUID);
		$JOB_INFO = $REPL_QUERY['JOBS_JSON'];
		$LOG_LOCATION = $REPL_QUERY['LOG_LOCATION'];

		#INITIAL THRIFT INFORMATION
		if ($HOST_TYPE == 'Physical')
		{
			$JOB_TYPE = \saasame\transport\job_type::physical_transport_type;
		}
		elseif ($HOST_TYPE == 'Virtual')
		{
			$JOB_TYPE = \saasame\transport\job_type::virtual_transport_type;
		}
		elseif ($HOST_TYPE == 'Offline')
		{
			$JOB_TYPE = \saasame\transport\job_type::winpe_transport_job_type;
		}
		else
		{
			return false;
		}
		
		#FOR SET START TIME TO DISPLAY MESSAGE
		if ($REPL_CONF['StartTime'] != '')
		{
			$USER_START_TIME = $REPL_CONF['StartTime'].' '.$REPL_CONF['TimeZone'];

			$MESSAGE = $this -> ReplMgmt -> job_msg('Replication workload will begin at %1%',array($USER_START_TIME));
			$this -> ReplMgmt -> update_job_msg($REPL_UUID,$MESSAGE,'Replica');
		}
		
		#MGMT COMMUNICATION PORT
		$MGMT_COMM = Misc_Class::mgmt_comm_type('scheduler');

		#ADD TIGTTER TYPE
		if ($REPL_CONF['IntervalMinutes'] == 0)
		{
			$SET_INTERVAL = 2147483647;
		}
		else
		{
			$SET_INTERVAL = $REPL_CONF['IntervalMinutes'];
		}
		
		#CONSTRACT JOB TIGTTER INFORMATION
		$TRIGGER_INFO = array(
								'type' 					=> $JOB_TYPE,
								'triggers' 				=> array(new saasame\transport\job_trigger(array('type'		=> 1, 
																										 'start' 	=> $REPL_CONF['UTC_START_TIME'],
																										 'finish' 	=> '',
																										 'interval' => $SET_INTERVAL))),
								'management_id' 		 	 	 => $REPL_UUID,
								'mgmt_addr'				 	 	 => $MGMT_ADDR_ARRAY,
								'mgmt_port'				 	 	 => $MGMT_COMM['mgmt_port'],
								'is_ssl'				 	 	 => $MGMT_COMM['is_ssl'],
								'buffer_size'				 	 => $REPL_CONF['BufferSize'],
								'sync_control'				 	 => false,
								'last_sync'						 => time(),
								'initialization_email'			 => false,
								'snapshot_rotation' 	 	 	 => $REPL_CONF['SnapshotsNumber'],								
								'worker_thread_number'   	 	 => $REPL_CONF['WorkerThreadNumber'],								
								'loader_thread_number'   	 	 => $REPL_CONF['LoaderThreadNumber'],
								'loader_trigger_percentage'  	 => $REPL_CONF['LoaderTriggerPercentage'],								
								'checksum_verify'			 	 => $REPL_CONF['ChecksumVerify'],
								'block_mode_enable'  	 	 	 => $REPL_CONF['UseBlockMode'],
								'schedule_pause'			 	 => $REPL_CONF['SchedulePause'],
								'is_full_replica'			 	 => true,
								'is_compressed'			 	 	 => $REPL_CONF['DataCompressed'],
								'is_checksum'				 	 => $REPL_CONF['DataChecksum'],	
								'file_system_filter'		 	 => $REPL_CONF['FileSystemFilter'],
								'create_by_partition'		 	 => $REPL_CONF['CreateByPartition'],
								'extra_gb'					 	 => $REPL_CONF['ExtraGB'],
								'job_version'			 	 	 => $REPL_CONF['CreateByVersion'],
								'export_path' 				 	 => $REPL_CONF['ExportPath'],
								'export_type'				 	 => $REPL_CONF['ExportType'],								
								'edit_lock'					 	 => false,
								'is_executing'				 	 => false,
								'init_carrier'				 	 => false,
								'init_loader'				 	 => false,
								'loader_keep_alive'			 	 => $REPL_CONF['loader_keep_alive'],
								'is_resume'					 	 => true,
								'is_continuous_data_replication' => $REPL_CONF['EnableCDR'],
								'skip_disk'						 => $REPL_CONF['SkipDisk'],
								'cloud_mapping_disk'			 => $REPL_CONF['CloudMappingDisk'],	
								'pre_snapshot_script'			 => $REPL_CONF['PreSnapshotScript'],
								'post_snapshot_script'			 => $REPL_CONF['PostSnapshotScript'],
								'priority_addr'		 		 	 => $REPL_CONF['PriorityAddr'],								
								'webdav_priority_addr' 		 	 => $REPL_CONF['WebDavPriorityAddr'],
								'timezone'					 	 => $REPL_CONF['TimeZone'],
								'cloud_type'					 => $REPL_CONF['CloudType'],
								'task_operation'			 	 => 'JOB_OP_UNKNOWN',
								'direct_mode'				 	 => $DIRECT_MODE,
								'is_azure_blob_mode'			 => $REPL_CONF['IsAzureBlobMode'],
								'is_azure_mgmt_disk'			 => $AZURE_MGMT_DISK,
								'is_packer_data_compressed'		 => $REPL_CONF['IsPackerDataCompressed'],
								'always_retry'		 		 	 => $REPL_CONF['ReplicationRetry'],
								'is_encrypted'					 => $REPL_CONF['PackerEncryption'],
								'post_loader_script'			 => $REPL_CONF['PostLoaderScript'],
								'source_transport_uuid'			 => $CONN_INFO['CARR_INFO']['SERV_INFO']['machine_id'],
								'target_transport_uuid'			 => $CONN_INFO['SERV_INFO']['SERV_INFO']['machine_id'],
								'repair_sync_mode'				 => 'full'
							);
							
		if( isset( $REPL_CONF['VMWARE_STORAGE'] ) ){
			$TRIGGER_INFO["VMWARE_STORAGE"] = $REPL_CONF['VMWARE_STORAGE'];
			$TRIGGER_INFO["VMWARE_ESX"] = $REPL_CONF['VMWARE_ESX'];
			$TRIGGER_INFO["VMWARE_FOLDER"] = $REPL_CONF['VMWARE_FOLDER'];
			$TRIGGER_INFO["VMWARE_THIN_PROVISIONED"] = $REPL_CONF['VMWARE_THIN_PROVISIONED'];
		}
		
		#QUERY CASCADE JOB
		$CASCADE_INFO = $this -> ReplMgmt -> cascading_connection($PACK_UUID);
		$CASCADE_COUNT = $CASCADE_INFO['count'];
		
		#UPDATE TRIGGER INFORMATION		
		$TRIGGER_INFO["job_order"] = $CASCADE_COUNT;
		$this -> update_trigger_info($REPL_UUID,$TRIGGER_INFO,'REPLICA');
		
		#SETUP CASCADE JOB
		if ($CASCADE_COUNT > 1)
		{				
			/* CASCADE REPLICA PART */
			$CASCADE_RPLICA = $CASCADE_INFO['replica'];
			$JOBS_INFO = $this -> ReplMgmt -> query_replica($CASCADE_RPLICA)['JOBS_JSON'];
			$JOBS_INFO -> schedule_pause = $REPL_CONF['SchedulePause'];
			$JOBS_INFO -> sync_control = false;
			$JOBS_INFO -> last_sync = time();
			$JOBS_INFO -> is_full_replica = true;
			$this -> update_trigger_info($CASCADE_RPLICA,$JOBS_INFO,'REPLICA');
			$this -> resume_service_job($CASCADE_RPLICA,'SCHEDULER');
			
			/* RUNNING REPLICA PART */
			$TRIGGER_INFO["init_carrier"] = true;
			$this -> update_trigger_info($REPL_UUID,$TRIGGER_INFO,'REPLICA');
			
			$JOB_TYPE = ucfirst($JOB_INFO -> multi_layer_protection);
			$MESSAGE = $this -> ReplMgmt -> job_msg('%1% job submitted to the recovery process.',array($JOB_TYPE));
			$this -> ReplMgmt -> update_job_msg($REPL_UUID,$MESSAGE,'Replica');
			
			return true;
		}
		
		#FOR DEBUG
		Misc_Class::function_debug($LOG_LOCATION,__FUNCTION__,$TRIGGER_INFO);
				
		#INIT THRIFT CLIENT
		$PORT_NUM = \saasame\transport\Constant::get('TRANSPORT_SERVICE_PORT');
		$OpenProtocol = $this -> common_connection('127.0.0.1',$PORT_NUM);
		$ClientCall = new saasame\transport\transport_serviceClient($OpenProtocol);
		
		$SERVICE_TYPE = \saasame\transport\Constant::get('SCHEDULER_SERVICE');
		$TRIGGER_DETAIL = new saasame\transport\create_job_detail($TRIGGER_INFO);
		
		#BEGIN TO CREATE REPLICA JOB
		for ($x=0; $x<5; $x++)
		{
			for ($i=0; $i<count($SCHD_ADDR); $i++)
			{
				try{
					$GetJobInfo = $ClientCall -> create_job_ex_p($SCHD_ADDR[$i],$REPL_UUID,$TRIGGER_DETAIL,$SERVICE_TYPE);
					if ($GetJobInfo != FALSE)
					{
						sleep(5);
						#CHECK REPLICA SUBMIT STATUS
						$CHECK_SUBMIT = $this -> replica_submit_status($REPL_UUID);
						if ($CHECK_SUBMIT == false)
						{
							//$MESSAGE = $this -> ReplMgmt -> job_msg('Cannot submit the recovery process.');
							//$this -> ReplMgmt -> update_job_msg($REPL_UUID,$MESSAGE,'Replica');
						}
						else
						{
							#UPDATE TO REPORTING SERVER
							#$REPORT_DATA = $this -> workload_report('create',$REPL_UUID,'Success');
							#Misc_Class::transport_report($REPORT_DATA);
						}
						break 2;
					}
				}
				catch (Throwable $e){
					#CHECK SAASAME LICENSE
					$CHECK_LICENSE = $this -> is_license_valid($REPL_UUID);
					if ($CHECK_LICENSE == FALSE)
					{
						$MESSAGE = $this -> ReplMgmt -> job_msg('Not enough licenses to run this process.');
						$this -> ReplMgmt -> update_job_msg($SERVICE_UUID,$MESSAGE,'Replica');
						exit;
					}					
					Misc_Class::function_debug($LOG_LOCATION,__FUNCTION__,$e);
					//return false;
				}
			}		
			sleep(5);			
		}
				
		return true;		
	}
	
	###########################	
	# REPAIR REPLICA JOB
	###########################
	public function repair_replica_job($REPL_UUID,$TARGET_TRANSPORT,$REPAIR_SYNC_MODE)
	{
		$REPL_INFO = $this -> ReplMgmt -> query_replica($REPL_UUID); 
		$TRANSPORT_INFO = $this -> ServerMgmt -> query_server_info($TARGET_TRANSPORT);
	
		#DEFAULT INFORMATION
		$ACCT_UUID 	  = $REPL_INFO['ACCT_UUID'];
		$LOG_LOCATION = $REPL_INFO['LOG_LOCATION'];
		$HOST_TYPE 	  = $REPL_INFO['HOST_TYPE'];
		
		#GET REPLICA CONNECTION INFORMATION
		$CONN_INFO = json_decode($REPL_INFO['CONN_UUID']);
		$SOURCE_CONN = $CONN_INFO -> SOURCE;
		$TARGET_CONN = $CONN_INFO -> TARGET;

		//return $this -> ServerMgmt -> query_connection_info($SOURCE_CONN);

		#DIRECT/IN-DIRECT MODE
		$DIRECT_MODE = $TRANSPORT_INFO['SERV_INFO']['direct_mode'];
		if ($DIRECT_MODE == TRUE)
		{		
			$SCHD_ADDR = $TRANSPORT_INFO['SERV_ADDR'];
		}
		else
		{
			#USE ADDRESS AS MACHINE UUID			
			$SCHD_ADDR = array($TRANSPORT_INFO['SERV_INFO']['machine_id']);
		}
	
		#GET AZURE MGMT DISK
		$AZURE_MGMT_DISK = $this -> ServerMgmt -> query_connection_info($TARGET_CONN)['AZURE_MGMT_DISK'];		
		
		#GET MGMT ADDRESS
		$MGMT_ADDR_ARRAY = $this -> query_multiple_mgmt_address($ACCT_UUID,$REPL_UUID);
		if ($MGMT_ADDR_ARRAY == FALSE)
		{
			$MESSAGE = $this -> ReplMgmt -> job_msg('Cannot get the management server address.');
			$this -> ReplMgmt -> update_job_msg($REPL_UUID,$MESSAGE,'Replica');
			return false;
		}
		
		#HOST TYPE
		if ($HOST_TYPE == 'Physical')
		{
			$JOB_TYPE = \saasame\transport\job_type::physical_transport_type;
		}
		elseif ($HOST_TYPE == 'Virtual')
		{
			$JOB_TYPE = \saasame\transport\job_type::virtual_transport_type;
		}
		elseif ($HOST_TYPE == 'Offline')
		{
			$JOB_TYPE = \saasame\transport\job_type::winpe_transport_job_type;
		}
		else
		{
			return false;
		}
					
		#CONSTRACT JOB TIGTTER INFORMATION
		//$TRIGGER_INFO = $REPL_INFO['JOBS_JSON'];
		//$TRIGGER_INFO -> task_operation = 'JOB_OP_REPAIR';
		//$TRIGGER_INFO -> is_repaired = true;
		
		#MGMT COMMUNICATION PORT
		$MGMT_COMM = Misc_Class::mgmt_comm_type('scheduler');

		#SET_INTERVAL
		$SET_INTERVAL = $REPL_INFO['JOBS_JSON'] -> triggers[0] -> interval == 0 ? 2147483647 : $REPL_INFO['JOBS_JSON'] -> triggers[0] -> interval;
		
		$TRIGGER_INFO = array(
								'type' 					=> $JOB_TYPE,
								'triggers' 				=> array(new saasame\transport\job_trigger(array('type'		=> 1, 
																										 'start' 	=> $REPL_INFO['JOBS_JSON'] -> triggers[0] -> start,
																										 'finish' 	=> '',
																										 'interval' => $SET_INTERVAL))),
								'management_id' 		 	 	 => $REPL_UUID,
								'mgmt_addr'				 	 	 => $MGMT_ADDR_ARRAY,
								'mgmt_port'				 	 	 => $REPL_INFO['JOBS_JSON'] -> mgmt_port,
								'is_ssl'				 	 	 => $MGMT_COMM['is_ssl'],
								'buffer_size'				 	 => $REPL_INFO['JOBS_JSON'] -> buffer_size,
								'sync_control'				 	 => false,
								'last_sync'						 => time(),
								'initialization_email'			 => false,
								'snapshot_rotation' 	 	 	 => $REPL_INFO['JOBS_JSON'] -> snapshot_rotation,								
								'worker_thread_number'   	 	 => $REPL_INFO['JOBS_JSON'] -> worker_thread_number,								
								'loader_thread_number'   	 	 => $REPL_INFO['JOBS_JSON'] -> loader_thread_number,
								'loader_trigger_percentage'  	 => $REPL_INFO['JOBS_JSON'] -> loader_trigger_percentage,								
								'checksum_verify'			 	 => $REPL_INFO['JOBS_JSON'] -> checksum_verify,
								'block_mode_enable'  	 	 	 => $REPL_INFO['JOBS_JSON'] -> block_mode_enable,
								'schedule_pause'			 	 => $REPL_INFO['JOBS_JSON'] -> schedule_pause,
								'is_full_replica'			 	 => false,
								'is_compressed'			 	 	 => $REPL_INFO['JOBS_JSON'] -> is_compressed,
								'is_checksum'				 	 => $REPL_INFO['JOBS_JSON'] -> is_checksum,	
								'file_system_filter'		 	 => $REPL_INFO['JOBS_JSON'] -> file_system_filter,
								'create_by_partition'		 	 => $REPL_INFO['JOBS_JSON'] -> create_by_partition,
								'extra_gb'					 	 => $REPL_INFO['JOBS_JSON'] -> extra_gb,
								'job_version'			 	 	 => $REPL_INFO['JOBS_JSON'] -> job_version,
								'export_path' 				 	 => $REPL_INFO['JOBS_JSON'] -> export_path,
								'export_type'				 	 => $REPL_INFO['JOBS_JSON'] -> export_type,								
								'edit_lock'					 	 => false,
								'is_executing'				 	 => false,
								'init_carrier'				 	 => false,
								'init_loader'				 	 => true, //DIFFERENT
								'loader_keep_alive'			 	 => $REPL_INFO['JOBS_JSON'] -> loader_keep_alive,
								'is_resume'					 	 => true,
								'is_continuous_data_replication' => $REPL_INFO['JOBS_JSON'] -> is_continuous_data_replication,
								'skip_disk'						 => $REPL_INFO['JOBS_JSON'] -> skip_disk,
								'cloud_mapping_disk'			 => $REPL_INFO['JOBS_JSON'] -> cloud_mapping_disk,	
								'pre_snapshot_script'			 => $REPL_INFO['JOBS_JSON'] -> pre_snapshot_script,
								'post_snapshot_script'			 => $REPL_INFO['JOBS_JSON'] -> post_snapshot_script,
								'priority_addr'		 		 	 => $REPL_INFO['JOBS_JSON'] -> priority_addr,								
								'webdav_priority_addr' 		 	 => $REPL_INFO['JOBS_JSON'] -> webdav_priority_addr,
								'timezone'					 	 => $REPL_INFO['JOBS_JSON'] -> timezone,
								'cloud_type'					 => $REPL_INFO['JOBS_JSON'] -> cloud_type,
								'task_operation'			 	 => 'JOB_OP_REPAIR',
								'direct_mode'				 	 => $DIRECT_MODE,
								'is_azure_blob_mode'			 => $REPL_INFO['JOBS_JSON'] -> is_azure_blob_mode,
								'is_azure_mgmt_disk'			 => $AZURE_MGMT_DISK,
								'is_packer_data_compressed'		 => $REPL_INFO['JOBS_JSON'] -> is_packer_data_compressed,
								'always_retry'		 		 	 => $REPL_INFO['JOBS_JSON'] -> always_retry,
								'is_encrypted'					 => $REPL_INFO['JOBS_JSON'] -> is_encrypted,
								'repair_sync_mode'				 => $REPAIR_SYNC_MODE
							);
							
		if( isset( $REPL_INFO['VMWARE_STORAGE'] ) ){
			$TRIGGER_INFO["VMWARE_STORAGE"] = $REPL_INFO['VMWARE_STORAGE'];
			$TRIGGER_INFO["VMWARE_ESX"] = $REPL_INFO['VMWARE_ESX'];
			$TRIGGER_INFO["VMWARE_FOLDER"] = $REPL_INFO['VMWARE_FOLDER'];
			$TRIGGER_INFO["VMWARE_THIN_PROVISIONED"] = $REPL_INFO['VMWARE_THIN_PROVISIONED'];
		}		
			
		#BEGIN TO RUN JOB
		$PORT_NUM = \saasame\transport\Constant::get('TRANSPORT_SERVICE_PORT');
		$OpenProtocol = $this -> common_connection('127.0.0.1',$PORT_NUM);
		$ClientCall = new saasame\transport\transport_serviceClient($OpenProtocol);
		
		$SERVICE_TYPE = \saasame\transport\Constant::get('SCHEDULER_SERVICE');
		$TRIGGER_DETAIL = new saasame\transport\create_job_detail($TRIGGER_INFO);

		#FOR DEBUG
		Misc_Class::function_debug($LOG_LOCATION,__FUNCTION__,$TRIGGER_INFO);
		
		for ($x=0; $x<5; $x++)
		{
			for ($i=0; $i<count($SCHD_ADDR); $i++)
			{
				try{
					$GetJobInfo = $ClientCall -> create_job_ex_p($SCHD_ADDR[$i],$REPL_UUID,$TRIGGER_DETAIL,$SERVICE_TYPE);
					if ($GetJobInfo != FALSE)
					{
						#UPDATE TRIGGER INFORMATION
						$this -> update_trigger_info($REPL_UUID,$TRIGGER_INFO,'REPLICA');
						break 2;
					}
				}
				catch (Throwable $e){
					#CHECK SAASAME LICENSE
					$CHECK_LICENSE = $this -> is_license_valid($REPL_UUID);
					if ($CHECK_LICENSE == FALSE)
					{
						$MESSAGE = $this -> ReplMgmt -> job_msg('Not enough licenses to run this process.');
						$this -> ReplMgmt -> update_job_msg($SERVICE_UUID,$MESSAGE,'Replica');
						exit;
					}					
					Misc_Class::function_debug($LOG_LOCATION,__FUNCTION__,$e);
				}
			}		
			sleep(5);			
		}
		return true;	
	}
	
	
	###########################	
	#GET JOB STATUS
	###########################
	public function get_service_job_status($JOB_UUID,$TYPE)
	{
		if ($TYPE == 'LAUNCHER')
		{
			$SERV_INFO = $this -> query_service($JOB_UUID);
			$REPL_UUID = $SERV_INFO['REPL_UUID'];
		}
		else
		{
			$REPL_UUID = $JOB_UUID;
		}
		
		$REPL_QUERY = $this -> ReplMgmt -> query_replica($REPL_UUID);
		if ($REPL_QUERY['JOBS_JSON'] -> direct_mode == TRUE)
		{
			$CONN_UUID = json_decode($REPL_QUERY['CONN_UUID'],false) -> TARGET; 
		}
		else
		{
			$CONN_UUID = json_decode($REPL_QUERY['CONN_UUID'],false) -> SOURCE; 
		}
		
		#GET SERVER INFORMATION
		$CONN_INFO = $this -> ServerMgmt -> query_connection_info($CONN_UUID);
		
		switch ($TYPE)
		{
			case 'SCHEDULER':
				#GET SCHEDULER ADDRESS	
				if ($CONN_INFO['SCHD_DIRECT'] == TRUE)
				{
					$SERVICE_ADDR = $CONN_INFO['SCHD_ADDR'];
				}
				else
				{
					$SERVICE_ADDR = array_flip($CONN_INFO['SCHD_ID']);
				}
				$SERVICE_TYPE = \saasame\transport\Constant::get('SCHEDULER_SERVICE');
			break;
			
			case 'LOADER':
				#GET LOADER ADDRESS	
				if ($CONN_INFO['LOAD_DIRECT'] == TRUE)
				{				
					$SERVICE_ADDR = $CONN_INFO['LOAD_ADDR'];
				}
				else
				{
					$SERVICE_ADDR = array_flip($CONN_INFO['LOAD_ID']);
				}				
				$SERVICE_TYPE = \saasame\transport\Constant::get('LOADER_SERVICE');
			break;
			
			case 'LAUNCHER':
				#GET LAUNCHER ADDRESS
				if ($CONN_INFO['LAUN_DIRECT'] == TRUE)
				{				
					$SERVICE_ADDR = $CONN_INFO['LAUN_ADDR'];
				}
				else
				{
					$SERVICE_ADDR = array_flip($CONN_INFO['LAUN_ID']);
				}				
				$SERVICE_TYPE = \saasame\transport\Constant::get('LAUNCHER_SERVICE');
			break;
		}
		
		$PORT_NUM = \saasame\transport\Constant::get('TRANSPORT_SERVICE_PORT');
		
		$OpenProtocol = $this -> common_connection('127.0.0.1',$PORT_NUM);
		
		$ClientCall = new saasame\transport\transport_serviceClient($OpenProtocol);

		for ($i=0; $i<count($SERVICE_ADDR); $i++)
		{
			try{				
				$JOB_INFO = $ClientCall -> get_job_p($SERVICE_ADDR[$i],$JOB_UUID,$SERVICE_TYPE);
				if ($JOB_INFO != FALSE){
					$JOB_INFO = true;
					break;
				}			
			}
			catch (Throwable $e){
				$JOB_INFO = $e -> what_op;
				break;
			}			
		}
		sleep(5);
		return $JOB_INFO;
	}	
	
	###########################	
	#INTERRUPT SERVICE JOB
	###########################
	private function interrupt_service_job($JOB_UUID,$TYPE)
	{
		if ($TYPE == 'LAUNCHER')
		{
			$SERV_INFO = $this -> query_service($JOB_UUID);
			$REPL_UUID = $SERV_INFO['REPL_UUID'];
		}
		else
		{
			$REPL_UUID = $JOB_UUID;
		}
		
		$REPL_QUERY = $this -> ReplMgmt -> query_replica($REPL_UUID);
		if ($REPL_QUERY['JOBS_JSON'] -> direct_mode == TRUE)
		{
			$CONN_UUID = json_decode($REPL_QUERY['CONN_UUID'],false) -> TARGET; 
		}
		else
		{
			$CONN_UUID = json_decode($REPL_QUERY['CONN_UUID'],false) -> SOURCE; 
		}
		
		$CONN_INFO = $this -> ServerMgmt -> query_connection_info($CONN_UUID);
		
		switch ($TYPE)
		{
			case 'SCHEDULER':
				#GET SCHEDULER ADDRESS	
				if ($CONN_INFO['SCHD_DIRECT'] == TRUE)
				{
					$SERVICE_ADDR = $CONN_INFO['SCHD_ADDR'];
				}
				else
				{
					$SERVICE_ADDR = array_flip($CONN_INFO['SCHD_ID']);
				}
				$SERVICE_TYPE = \saasame\transport\Constant::get('SCHEDULER_SERVICE');
			break;
			
			case 'LOADER':
				#GET LOADER ADDRESS
				if ($CONN_INFO['LOAD_DIRECT'] == TRUE)
				{				
					$SERVICE_ADDR = $CONN_INFO['LOAD_ADDR'];
				}
				else
				{
					$SERVICE_ADDR = array_flip($CONN_INFO['LOAD_ID']);
				}					
				$SERVICE_TYPE = \saasame\transport\Constant::get('LOADER_SERVICE');
			break;
			
			case 'LAUNCHER':
				#GET LAUNCHER ADDRESS
				if ($CONN_INFO['LAUN_DIRECT'] == TRUE)
				{				
					$SERVICE_ADDR = $CONN_INFO['LAUN_ADDR'];
				}
				else
				{
					$SERVICE_ADDR = array_flip($CONN_INFO['LAUN_ID']);
				}
				$SERVICE_TYPE = \saasame\transport\Constant::get('LAUNCHER_SERVICE');
			break;
		}
		
		$PORT_NUM = \saasame\transport\Constant::get('TRANSPORT_SERVICE_PORT');
		
		$OpenProtocol = $this -> common_connection('127.0.0.1',$PORT_NUM);
		
		$ClientCall = new saasame\transport\transport_serviceClient($OpenProtocol);
		
		
		for ($i=0; $i<count($SERVICE_ADDR); $i++)
		{
			try{
				$JOB_INFO = $ClientCall -> interrupt_job_p($SERVICE_ADDR[$i],$JOB_UUID,$SERVICE_TYPE);
				if ($JOB_INFO == TRUE)
				{
					break;
				}
			}
			catch (Throwable $e){
				$JOB_INFO = false;
			}				
		}
		return $JOB_INFO;
	}
		
	###########################	
	#REMOVE SERVICE JOB
	###########################
	public function remove_service_job($JOB_UUID,$TYPE)
	{
		if ($TYPE == 'LAUNCHER')
		{
			$SERV_INFO = $this -> query_service($JOB_UUID);
			$REPL_UUID = $SERV_INFO['REPL_UUID'];
		}
		else
		{
			$REPL_UUID = $JOB_UUID;
		}
		
		#GET CONNECTION INFORMATION
		$REPL_QUERY = $this -> ReplMgmt -> query_replica($REPL_UUID);		
		if ($REPL_QUERY['JOBS_JSON'] -> direct_mode == TRUE)
		{
			$CONN_UUID = json_decode($REPL_QUERY['CONN_UUID'],false) -> TARGET; 
		}
		else
		{
			$CONN_UUID = json_decode($REPL_QUERY['CONN_UUID'],false) -> SOURCE; 
		}
		
		#GET SERVER INFORMATION
		$CONN_INFO = $this -> ServerMgmt -> query_connection_info($CONN_UUID);
	
		switch ($TYPE)
		{
			case 'SCHEDULER':
				#GET SCHEDULER ADDRESS
				if ($CONN_INFO['SCHD_DIRECT'] == TRUE)
				{
					$SERVICE_ADDR = $CONN_INFO['SCHD_ADDR'];
				}
				else
				{
					$SERVICE_ADDR = array_flip($CONN_INFO['SCHD_ID']);
				}
				$SERVICE_TYPE = \saasame\transport\Constant::get('SCHEDULER_SERVICE');
			break;
			
			case 'LOADER':
				#GET LOADER ADDRESS	
				if ($CONN_INFO['LOAD_DIRECT'] == TRUE)
				{				
					$SERVICE_ADDR = $CONN_INFO['LOAD_ADDR'];
				}
				else
				{
					$SERVICE_ADDR = array_flip($CONN_INFO['LOAD_ID']);
				}	
				$SERVICE_TYPE = \saasame\transport\Constant::get('LOADER_SERVICE');
			break;
			
			case 'LAUNCHER':
				#GET LAUNCHER ADDRESS
				if ($CONN_INFO['LAUN_DIRECT'] == TRUE)
				{				
					$SERVICE_ADDR = $CONN_INFO['LAUN_ADDR'];
				}
				else
				{
					$SERVICE_ADDR = array_flip($CONN_INFO['LAUN_ID']);
				}
				$SERVICE_TYPE = \saasame\transport\Constant::get('LAUNCHER_SERVICE');
			break;
		}
	
		$PORT_NUM = \saasame\transport\Constant::get('TRANSPORT_SERVICE_PORT');
		
		$OpenProtocol = $this -> common_connection('127.0.0.1',$PORT_NUM);
		
		$ClientCall = new saasame\transport\transport_serviceClient($OpenProtocol);
		
		for ($i=0; $i<count($SERVICE_ADDR); $i++)
		{
			//Misc_Class::function_debug('_mgmt',__FUNCTION__,array($TYPE => $SERVICE_ADDR[$i]));
			try{
				$JOB_INFO = $ClientCall -> remove_job_p($SERVICE_ADDR[$i],$JOB_UUID,$SERVICE_TYPE);
				if ($JOB_INFO == TRUE)
				{
					break;
				}
			}
			catch (Throwable $e){
				//Misc_Class::function_debug('_mgmt',__FUNCTION__,$e);
				$JOB_INFO = false;
			}
		}
		return $JOB_INFO;
	}
	
	###########################	
	#RESUME SERVICE JOB
	###########################
	public function resume_service_job($JOB_UUID,$TYPE)
	{
		if ($TYPE == 'LAUNCHER')
		{
			$SERV_INFO = $this -> query_service($JOB_UUID);
			$REPL_UUID = $SERV_INFO['REPL_UUID'];
			$REPL_QUERY = $this -> ReplMgmt -> query_replica($REPL_UUID);
		}
		else
		{
			$REPL_UUID = $JOB_UUID;
			
			#SET JOB ON EXECUTING
			$REPL_QUERY = $this -> ReplMgmt -> query_replica($REPL_UUID);
			$JOB_INFO = $REPL_QUERY['JOBS_JSON'];
			$JOB_INFO -> last_sync = time();
			//$JOB_INFO -> is_executing = true;
			$this -> update_trigger_info($REPL_UUID,$JOB_INFO,'REPLICA');			
		}
		
		if ($REPL_QUERY['JOBS_JSON'] -> direct_mode == TRUE)
		{
			$CONN_UUID = json_decode($REPL_QUERY['CONN_UUID'],false) -> TARGET; 
		}
		else
		{
			$CONN_UUID = json_decode($REPL_QUERY['CONN_UUID'],false) -> SOURCE; 
		}
			
		#GET SERVER INFORMATION
		$CONN_INFO = $this -> ServerMgmt -> query_connection_info($CONN_UUID);
		
		switch ($TYPE)
		{
			case 'SCHEDULER':
				#GET SCHEDULER ADDRESS	
				if ($CONN_INFO['SCHD_DIRECT'] == TRUE)
				{
					$SERVICE_ADDR = $CONN_INFO['SCHD_ADDR'];
				}
				else
				{
					$SERVICE_ADDR = array_flip($CONN_INFO['SCHD_ID']);
				}				
				$SERVICE_TYPE = \saasame\transport\Constant::get('SCHEDULER_SERVICE');
			break;
			
			case 'LOADER':
				#GET LOADER ADDRESS	
				if ($CONN_INFO['LOAD_DIRECT'] == TRUE)
				{				
					$SERVICE_ADDR = $CONN_INFO['LOAD_ADDR'];
				}
				else
				{
					$SERVICE_ADDR = array_flip($CONN_INFO['LOAD_ID']);
				}
				$SERVICE_TYPE = \saasame\transport\Constant::get('LOADER_SERVICE');
			break;
			
			case 'LAUNCHER':
				#GET LAUNCHER ADDRESS
				if ($CONN_INFO['LAUN_DIRECT'] == TRUE)
				{				
					$SERVICE_ADDR = $CONN_INFO['LAUN_ADDR'];
				}
				else
				{
					$SERVICE_ADDR = array_flip($CONN_INFO['LAUN_ID']);
				}
				$SERVICE_TYPE = \saasame\transport\Constant::get('LAUNCHER_SERVICE');
			break;
		}
		
		$PORT_NUM = \saasame\transport\Constant::get('TRANSPORT_SERVICE_PORT');		

		$OpenProtocol = $this -> common_connection('127.0.0.1',$PORT_NUM);

		$ClientCall = new saasame\transport\transport_serviceClient($OpenProtocol);
		
		for ($i=0; $i<count($SERVICE_ADDR); $i++)
		{
			try{
				$JOB_INFO = $ClientCall -> resume_job_p($SERVICE_ADDR[$i],$JOB_UUID,$SERVICE_TYPE);
				break;
			}
			catch (Throwable $e){
				$JOB_INFO = false;
			}
		}
		return $JOB_INFO;
	}
	
	###########################
	#CHECK JOB RUNNING STATUS
	###########################
	public function check_running_job($JOB_UUID,$TYPE)
	{
		if ($TYPE == 'LAUNCHER')
		{
			$SERV_INFO = $this -> query_service($JOB_UUID);
			$REPL_UUID = $SERV_INFO['REPL_UUID'];
		}
		else
		{
			$REPL_UUID = $JOB_UUID;
		}
		
		#GET CONNECTION INFORMATION
		$REPL_QUERY = $this -> ReplMgmt -> query_replica($REPL_UUID);
		if ($REPL_QUERY['JOBS_JSON'] -> direct_mode == TRUE)
		{
			$CONN_UUID = json_decode($REPL_QUERY['CONN_UUID'],false) -> TARGET; 
		}
		else
		{
			$CONN_UUID = json_decode($REPL_QUERY['CONN_UUID'],false) -> SOURCE; 
		}
		
		#GET SERVER INFORMATION
		$CONN_INFO = $this -> ServerMgmt -> query_connection_info($CONN_UUID);
		
		switch ($TYPE)
		{
			case 'SCHEDULER':
				#GET SCHEDULER ADDRESS
				if ($CONN_INFO['SCHD_DIRECT'] == TRUE)
				{
					$SERVICE_ADDR = $CONN_INFO['SCHD_ADDR'];
				}
				else
				{
					$SERVICE_ADDR = array_flip($CONN_INFO['SCHD_ID']);
				}				
				$SERVICE_TYPE = \saasame\transport\Constant::get('SCHEDULER_SERVICE');
			break;
			
			case 'LOADER':
				#GET LOADER ADDRESS	
				if ($CONN_INFO['LOAD_DIRECT'] == TRUE)
				{				
					$SERVICE_ADDR = $CONN_INFO['LOAD_ADDR'];
				}
				else
				{
					$SERVICE_ADDR = array_flip($CONN_INFO['LOAD_ID']);
				}
				$SERVICE_TYPE = \saasame\transport\Constant::get('LOADER_SERVICE');
			break;
			
			case 'LAUNCHER':
				#GET LAUNCHER ADDRESS
				if ($CONN_INFO['LAUN_DIRECT'] == TRUE)
				{				
					$SERVICE_ADDR = $CONN_INFO['LAUN_ADDR'];
				}
				else
				{
					$SERVICE_ADDR = array_flip($CONN_INFO['LAUN_ID']);
				}
				$SERVICE_TYPE = \saasame\transport\Constant::get('LAUNCHER_SERVICE');
			break;
		}
		
		$PORT_NUM = \saasame\transport\Constant::get('TRANSPORT_SERVICE_PORT');
		
		$OpenProtocol = $this -> common_connection('127.0.0.1',$PORT_NUM);
		
		$ClientCall = new saasame\transport\transport_serviceClient($OpenProtocol);
		
		for ($i=0; $i<count($SERVICE_ADDR); $i++)
		{
			try{
				$JOB_INFO = $ClientCall -> running_job_p($SERVICE_ADDR[$i],$JOB_UUID,$SERVICE_TYPE);
				if ($JOB_INFO == TRUE)
				{
					break;
				}
			}
			catch (Throwable $e){
				$JOB_INFO = false;
			}
		}
		return $JOB_INFO;
	}
	
	###########################	
	#REMOVE SNAPSHOT IMAGE
	###########################
	public function remove_snapshot_images($REPL_UUID,$TYPE)
	{
		#QUERY SOURCE CONNECTION INFORMATION
		$REPL_QUERY = $this -> ReplMgmt -> query_replica($REPL_UUID);
		$CONN_UUID = json_decode($REPL_QUERY['CONN_UUID'],false) -> SOURCE;
		
		#QUERY REPL DISK INFORMATION
		$QUERY_REPL_DISK = $this -> ReplMgmt -> get_replica_disk_info($REPL_UUID);
		
		if ($QUERY_REPL_DISK != false)
		{
			#LOOP IMAGE MAP INFO
			for ($i=0; $i<count($QUERY_REPL_DISK); $i++)
			{
				#REPL DISK UUID
				$DISK_ID   = $QUERY_REPL_DISK[$i]['ID'];
				$DISK_UUID = $QUERY_REPL_DISK[$i]['DISK_UUID'];
				
				#QUERY REPL SNAPSHOT INFORMATION
				$SNAP_INFO = $this -> ReplMgmt -> query_replica_snapshot($REPL_UUID,$DISK_UUID);
				
				#BASE IMAGE
				$BASE_DISK_UUID = $DISK_UUID.'-'.$DISK_ID;							
			
				#IMAGE MAP INFO ARRAY
				if ($SNAP_INFO != false)
				{			
					for ($x=0; $x<count($SNAP_INFO); $x++)
					{
						$SNAPSHOT_ARRAY = array(
												'image' => $SNAP_INFO[$x]['SNAP_NAME'],
												'base_image' => $BASE_DISK_UUID,
												'connection_ids' => array($CONN_UUID => 0));
											
						$IMAGE_MAP_INFO = new \saasame\transport\image_map_info($SNAPSHOT_ARRAY);
						
						$REMOVE_IMAGE_INFO[$SNAP_INFO[$x]['SNAP_NAME']] = $IMAGE_MAP_INFO;
					}
				}
				else
				{
					return true;
				}		
			}

			if ($CONN_UUID != '')
			{
				#GET ADDRESS INFO
				if ($TYPE == 'Carrier')
				{
					$CONN_INFO = $this -> ServerMgmt -> query_connection_info(json_decode($REPL_QUERY['CONN_UUID'],false) -> SOURCE);
					$TRANSPORT_ADDR = ($CONN_INFO['CARR_DIRECT'] == TRUE)? $CONN_INFO['CARR_ADDR']:array_flip($CONN_INFO['CARR_ID']);
				}
				else
				{
					$CONN_INFO = $this -> ServerMgmt -> query_connection_info(json_decode($REPL_QUERY['CONN_UUID'],false) -> TARGET);
					$TRANSPORT_ADDR = ($CONN_INFO['LOAD_DIRECT'] == TRUE)? $CONN_INFO['LOAD_ADDR']:array_flip($CONN_INFO['LOAD_ID']);
				}
				
				$SERVICE_TYPE = \saasame\transport\Constant::get('CARRIER_SERVICE');
				
				$PORT_NUM = \saasame\transport\Constant::get('TRANSPORT_SERVICE_PORT');		
				
				$OpenProtocol = $this -> common_connection('127.0.0.1',$PORT_NUM);
				
				$ClientCall = new saasame\transport\transport_serviceClient($OpenProtocol);
								
				for ($w=0; $w<count($TRANSPORT_ADDR); $w++)
				{
					try{
						$JOB_INFO = $ClientCall -> remove_snapshot_image_p($TRANSPORT_ADDR[$w],$REMOVE_IMAGE_INFO,$SERVICE_TYPE);
						if ($JOB_INFO == TRUE)
						{
							break;
						}
					}
					catch (Throwable $e){
						$JOB_INFO = false;
					}
				}
				return $JOB_INFO;
			}
		}
		else
		{
			return false;
		}
	}

	
	###########################
	#	CREATE LOADER JOB
	###########################
	public function create_loader_job($REPL_UUID)
	{
		$REPL_QUERY = $this -> ReplMgmt -> query_replica($REPL_UUID);
		$CONN_UUID = json_decode($REPL_QUERY['CONN_UUID'],false) -> TARGET;
	
		#LOG LOCATION
		$LOG_LOCATION = $REPL_QUERY['LOG_LOCATION'];
		
		#GET REPLICA JOB INFORMATION
		$JOB_INFO = $REPL_QUERY['JOBS_JSON'];
		
		#GET SERVER INFORMATION
		$CONN_INFO = $this -> ServerMgmt -> query_connection_info($CONN_UUID);
		$ACCT_UUID = $CONN_INFO['ACCT_UUID'];
		$LOAD_UUID = $CONN_INFO['LOAD_UUID'];
	
		#DIRECT/IN-DIRECT ADDRESS
		if ($CONN_INFO['LOAD_DIRECT'] == TRUE)
		{
			$LOAD_ADDR = $CONN_INFO['LOAD_ADDR'];
		}
		else
		{
			$LOAD_ADDR = array_flip($CONN_INFO['LOAD_ID']);
		}

		$MGMT_ADDR = $CONN_INFO['MGMT_ADDR'];
		
		#MGMT ADDRESS ARRAY
		$MGMT_ADDR_ARRAY = $this -> query_multiple_mgmt_address($ACCT_UUID,$REPL_UUID,$MGMT_ADDR);
		if ($MGMT_ADDR_ARRAY == FALSE)
		{				
			$MESSAGE = $this -> ReplMgmt -> job_msg('Cannot get the management server address.');
			$this -> ReplMgmt -> update_job_msg($REPL_UUID,$MESSAGE,'Replica');
			return false;
		}		

		#GET JOB TYPE
		$JOB_TYPE = \saasame\transport\job_type::loader_job_type;

		#MGMT COMMUNICATION PORT
		$MGMT_COMM = Misc_Class::mgmt_comm_type('loader');
		
		#CONSTRACT JOB TIGTTER INFORMATION
		$TRIGGER_INFO = array(
								'type' 			=> $JOB_TYPE,
								'triggers' 		=> array(new saasame\transport\job_trigger(array('type'		=> 1, 
																								 'start' 	=> '',
																								 'finish' 	=> '',
																								 'interval' => 1))),
								'management_id' => $REPL_UUID,
								'mgmt_addr'		=> $MGMT_ADDR_ARRAY,
								'mgmt_port'		=> $MGMT_COMM['mgmt_port'],
								'is_ssl'		=> $MGMT_COMM['is_ssl']
							);
							
					
		#BEGIN TO RUN JOB
		$PORT_NUM = \saasame\transport\Constant::get('TRANSPORT_SERVICE_PORT');		
		
		$OpenProtocol = $this -> common_connection('127.0.0.1',$PORT_NUM);
		
		$ClientCall = new saasame\transport\transport_serviceClient($OpenProtocol);
		
		$SERVICE_TYPE = \saasame\transport\Constant::get('LOADER_SERVICE');
		
		$TRIGGER_DETAIL = new saasame\transport\create_job_detail($TRIGGER_INFO);
		
		Misc_Class::function_debug($LOG_LOCATION,__FUNCTION__,$TRIGGER_INFO);
		
		for ($i=0; $i<count($LOAD_ADDR); $i++)
		{
			try{
				$Status = $ClientCall -> get_job_p($LOAD_ADDR[$i],$REPL_UUID,$SERVICE_TYPE);
				if ($Status != FALSE)
				{
					break;
				}
			}
			catch (Throwable $e){
				for ($x=0; $x<5; $x++)
				{
					try{
						$Status = $ClientCall -> create_job_ex_p($LOAD_ADDR[$i],$REPL_UUID,$TRIGGER_DETAIL,$SERVICE_TYPE);
						if ($Status != FALSE)
						{
							break 2;
						}						
					}
					catch (Throwable $e){
						Misc_Class::function_debug($LOG_LOCATION,__FUNCTION__,$e);
						$Status = false;
					}
					sleep(5);
				}
			}
		}
		
		#SEND NOTIFICATION EMAIL
		if ($JOB_INFO -> initialization_email == false)
		{
			$JOB_INFO -> init_loader = true;
			$JOB_INFO -> initialization_email = true;
			$this -> update_trigger_info($REPL_UUID,$JOB_INFO,'REPLICA');
			$this -> MailerMgmt -> gen_replication_notification($ACCT_UUID,$REPL_UUID);
		}	
				
		return $Status;
	}
	
	
	###########################
	#TRIGGER RESYNC AND STOP SCHEDULER AND LOADER
	###########################
	private function trigger_resync($REPL_UUID,$SERVICE_UUID,$TRIGGER)
	{
		if ($TRIGGER == true)
		{
			#TRIGGER MESSAGE
			$MESSAGE = $this -> ReplMgmt -> job_msg('Trigger job synchronization.');
			$this -> ReplMgmt -> update_job_msg($REPL_UUID,$MESSAGE,'Replica');	
			
			sleep(5);
			
			#TRIGGER RE-SYNC AND CHECK
			$MESSAGE = $this -> ReplMgmt -> job_msg('The Scheduler sync started.');
			$this -> ReplMgmt -> update_job_msg($SERVICE_UUID,$MESSAGE,'Service');
			$this -> resume_service_job($REPL_UUID,'SCHEDULER');
			
			sleep(15);
			
			$MESSAGE = $this -> ReplMgmt -> job_msg('Checking the Scheduler job status.');
			$this -> ReplMgmt -> update_job_msg($SERVICE_UUID,$MESSAGE,'Service');
			$CHECK_SCHD_RUN = $this -> check_running_job($REPL_UUID,'SCHEDULER');
			
			if ($CHECK_SCHD_RUN == TRUE)
			{
				#RETRY CHECK ON SCHEDULER
				$SCHD_RETRY_COUNT = 5;
				for ($i=0; $i<count($SCHD_RETRY_COUNT); $i++)
				{
					sleep(30);
					$SCHD_RUN_CHECK = $this -> check_running_job($REPL_UUID,'SCHEDULER');
					if ($SCHD_RUN_CHECK == FALSE)
					{
						break;
					}
					else
					{
						$MESSAGE = $this -> ReplMgmt -> job_msg('Checking the Scheduler job status.<sup>%1%</sup>',array(($i+1)));
						$this -> ReplMgmt -> update_job_msg($SERVICE_UUID,$MESSAGE,'Service');
					}					
				}
			}
			
			#$MESSAGE = $this -> ReplMgmt -> job_msg('The Loader sync started.');
			$this -> ReplMgmt -> update_job_msg($SERVICE_UUID,$MESSAGE,'Service');
			$this -> resume_service_job($REPL_UUID,'LOADER');
			sleep(10);
			
			$MESSAGE = $this -> ReplMgmt -> job_msg('Checking the Loader job status.');
			$this -> ReplMgmt -> update_job_msg($SERVICE_UUID,$MESSAGE,'Service');
			$CHECK_LOAD_RUN = $this -> check_running_job($REPL_UUID,'LOADER');
			#sleep(35);
			if ($CHECK_LOAD_RUN == TRUE)
			{
				#RETRY CHECK ON LOADER
				$LOAD_RETRY_COUNT = 5;
				for ($x=0; $x<count($LOAD_RETRY_COUNT); $x++)
				{
					sleep(30);
					$LOAD_RUN_CHECK = $this -> check_running_job($REPL_UUID,'LOADER');
					if ($LOAD_RUN_CHECK == FALSE)
					{
						break;
					}
					else
					{
						$MESSAGE = $this -> ReplMgmt -> job_msg('Checking the Loader job status.<sup>%1%</sup>',array(($x+1)));
						$this -> ReplMgmt -> update_job_msg($SERVICE_UUID,$MESSAGE,'Service');
					}
				}		
			}		
		}
		
		#INTERRUPT SCHEDULER JOB
		$MESSAGE = $this -> ReplMgmt -> job_msg('Stop the Scheduler job.');
		$this -> ReplMgmt -> update_job_msg($SERVICE_UUID,$MESSAGE,'Service');
		$this -> interrupt_service_job($REPL_UUID,'SCHEDULER');
	
		#REMOVE SCHEDULER JOB
		#$this -> remove_service_job($REPL_UUID,'SCHEDULER');
			
		#INTERRUPT LOADER JOB
		$MESSAGE = $this -> ReplMgmt -> job_msg('Stop the Loader job.');
		$this -> ReplMgmt -> update_job_msg($SERVICE_UUID,$MESSAGE,'Service');
		$this -> interrupt_service_job($REPL_UUID,'LOADER');
		
		#REMOVE LOADER JOB
		#$this -> remove_service_job($REPL_UUID,'LOADER');

		#SHOW JOB CANCELLED MSG ON REPLICA
		$MESSAGE = $this -> ReplMgmt -> job_msg('Job canceled.');
		$this -> ReplMgmt -> update_job_msg($REPL_UUID,$MESSAGE,'Replica');
	}
	
	###########################
	#DELAY LAUNCHER JOB SUBMIT
	###########################
	private function delay_launcher_job_submit($SERVICE_UUID, $CONN_UUID)
	{
		$CHECK_RUNNING_JOB = "SELECT * FROM _SERVICE WHERE _CONN_UUID = '".$CONN_UUID."' AND _ADMIN_PASS = '' AND _STATUS = 'Y'";
		$QUERY = $this -> DBCON -> prepare($CHECK_RUNNING_JOB);
		$QUERY -> execute();
		$COUNT_ROWS = $QUERY -> rowCount();
		if ($COUNT_ROWS > 1)
		{
			#$MESSAGE = $this -> ReplMgmt -> job_msg('Wait %1% seconds for previous job finish.',array(($COUNT_ROWS*5)));
			#$this -> ReplMgmt -> update_job_msg($SERVICE_UUID,$MESSAGE,'Service');			
			sleep($COUNT_ROWS * 5);
		}
	}
	
	###########################
	# GET SERVICE LIST
	###########################
	private function get_service_list($LAUN_ADDR)
	{
		try{
			$PORT_NUM = \saasame\transport\Constant::get('TRANSPORT_SERVICE_PORT');
		
			$OpenProtocol = $this -> common_connection('127.0.0.1',$PORT_NUM);
			
			$ClientCall = new saasame\transport\transport_serviceClient($OpenProtocol);

			$GetServiceList = $ClientCall -> get_service_list_p($LAUN_ADDR);
			
			return $GetServiceList;
		}
		catch (Throwable $e) {
			return null;
		}
	}
	
	###########################
	# CHECK SERVICE
	###########################
	public function check_service($SERVICE_NAME,$SERVICE_ADDR)
	{
		$SERVICE_UUID = \saasame\transport\Constant::get($SERVICE_NAME);
		
		$CheckCode = false;
		for ($i=0; $i<count($SERVICE_ADDR); $i++)
		{
			$SERVICE_LIST = $this -> get_service_list($SERVICE_ADDR[$i]);
			
			if ($SERVICE_LIST == NULL)
			{
				$SERVICE_LIST = $this -> get_service_list($SERVICE_ADDR[$i]);
				sleep(3);
			}
			
			foreach ($SERVICE_LIST as $SERVICE_ARRAY)
			{
				if ($SERVICE_ARRAY -> id == $SERVICE_UUID)
				{
					$CheckCode = true;
					break;
				}
			}
		}
		return $CheckCode;
	}
	
	###########################
	# CHECK MEMORY REQUIREMENT
	###########################
	private function check_memory_requirement($SERVICE_ADDR)
	{
		$CheckCode = false;
		for ($i=0; $i<count($SERVICE_ADDR); $i++)
		{
			$HOST_INFO = $this -> get_host_detail($SERVICE_ADDR[$i]);
				
			if ($HOST_INFO -> physical_memory >= 2000)
			{
				$CheckCode = true;
				break; 
			}
		}		
		return $CheckCode;
	}
	
	###########################
	# QUERY RECOVERY PLAN INFORMATION
	###########################
	public function recover_plan($PLAN_UUID)
	{
		$RECOVER_PLAN = json_decode($this -> ServerMgmt -> query_recover_plan($PLAN_UUID)['_PLAN_JSON']);
		$CLUSTER_UUID	 = $RECOVER_PLAN -> ClusterUUID;
		$SERVER_REGION	 = $RECOVER_PLAN -> ServiceRegin;
		$PLAN_CLOUD_TYPE = $RECOVER_PLAN -> CloudType;
		$PLAN_CLOUD_DISK = $RECOVER_PLAN -> CloudDisk;		
	
		switch ($PLAN_CLOUD_TYPE)
		{
			case 'OPENSTACK':
				for ($x=0; $x<count($PLAN_CLOUD_DISK); $x++)
				{
					$SELECT_LAST_SNAPSHOT[] = $this -> OpenStackMgmt -> list_available_snapshot($CLUSTER_UUID,$PLAN_CLOUD_DISK[$x])[0] -> id;
				}
			break;

			case 'AWS':
				for ($x=0; $x<count($PLAN_CLOUD_DISK); $x++)
				{
					$GET_SNAPSHOT_LIST = $this -> AwsMgmt -> describe_snapshots($CLUSTER_UUID,$SERVER_REGION,$PLAN_CLOUD_DISK[$x]);
					$SELECT_LAST_SNAPSHOT[] = array_pop($GET_SNAPSHOT_LIST)['SnapshotId'];
				}
			break;

			case 'Azure':
				for ($x=0; $x<count($PLAN_CLOUD_DISK); $x++)
				{
					if ($RECOVER_PLAN -> IsAzureBlobMode == TRUE)
					{
						$GET_SNAPSHOT_LIST = $this -> AzureBlobMgmt -> list_snapshot($RECOVER_PLAN -> ReplUUID,$PLAN_CLOUD_DISK[$x]);
						$SELECT_LAST_SNAPSHOT[] = array_pop($GET_SNAPSHOT_LIST) -> id;
					}
					else
					{
						$GET_SNAPSHOT_LIST = $this -> AzureMgmt -> describe_snapshots($CLUSTER_UUID,$SERVER_REGION,$PLAN_CLOUD_DISK[$x]);
						$SELECT_LAST_SNAPSHOT[] = array_pop($GET_SNAPSHOT_LIST)['name'];
					}
				}
			break;

			case 'Aliyun':
				for ($x=0; $x<count($PLAN_CLOUD_DISK); $x++)
				{
					$GET_SNAPSHOT_LIST = $this -> AliMgmt -> describe_snapshots($CLUSTER_UUID,$SERVER_REGION,$PLAN_CLOUD_DISK[$x]);

					$SELECT_LAST_SNAPSHOT[] = array_pop($GET_SNAPSHOT_LIST)['id'];
				}
				
				$RECOVER_PLAN->SGroupUUID = $RECOVER_PLAN->SGroupUUID.'|'.$RECOVER_PLAN->SwitchId;				
			break;
		
			case 'Tencent':
				for ($x=0; $x<count($PLAN_CLOUD_DISK); $x++)
				{
					$GET_SNAPSHOT_LIST = $this -> TencentMgmt -> describe_snapshots($CLUSTER_UUID,$SERVER_REGION,$PLAN_CLOUD_DISK[$x]);

					$SELECT_LAST_SNAPSHOT[] = array_pop($GET_SNAPSHOT_LIST)['id'];
				}
				
				$RECOVER_PLAN->SGroupUUID = $RECOVER_PLAN->SGroupUUID.'|'.$RECOVER_PLAN->SubnetUUID;		
			break;
			
			case 'Ctyun':
				for ($x=0; $x<count($PLAN_CLOUD_DISK); $x++)
				{
					$GET_SNAPSHOT_LIST = $this -> CtyunMgmt -> list_available_snapshot($CLUSTER_UUID,$PLAN_CLOUD_DISK[$x]);
					$SELECT_LAST_SNAPSHOT[] = array_pop($GET_SNAPSHOT_LIST) -> id;
				}
			break;
		}
		
		$SNAP_INFO 		= implode(',',$SELECT_LAST_SNAPSHOT);
		$RECOVER_PLAN -> snap_uuid = $SNAP_INFO;
		
		if (!isset($RECOVER_PLAN -> IsAzureBlobMode))
		{
			$RECOVER_PLAN -> is_azure_blob_mode = false;
		}
		return $RECOVER_PLAN;	
	}
	
	###########################
	#PRE CREATE LAUNCHER JOB
	###########################
	public function pre_create_launcher_job($ACCT_UUID,$REGN_UUID,$REPL_UUID,$SERV_UUID,$PLAN_UUID,$RECY_TYPE,$SERVICE_SETTINGS)
	{
		#FOR RECOVERY PLAN
		if ($PLAN_UUID != FALSE)
		{
			$SERVICE_SETTINGS = $this -> recover_plan($PLAN_UUID);
			$RECY_TYPE 		  = $SERVICE_SETTINGS -> RecoverType;
			$REPL_UUID		  = $SERVICE_SETTINGS -> ReplUUID;
		}
		
		#GET SNAPSHOT INFORMATION
		$SNAP_INFO = $SERVICE_SETTINGS -> snap_uuid;	
			
		#QUERY REPLICA INFORMATION
		$REPL_QUERY = $this -> ReplMgmt -> list_replica(null,$REPL_UUID);
		$PACK_UUID = $REPL_QUERY['PACK_UUID'];
		$CONN_UUID = json_decode($REPL_QUERY['CONN_UUID'],false) -> TARGET;
		$HOST_NAME = $REPL_QUERY['HOST_NAME'];
		$JOBS_INFO = $REPL_QUERY['JOBS_JSON'];

		#GET OS TYPE FOR MICROSOFT OR LINUX
		$PACKER_INFO = $this -> ServerMgmt -> query_host_info($PACK_UUID);
		if (isset($PACKER_INFO['HOST_INFO']['os_name']))
		{
			$OS_NAME = $PACKER_INFO['HOST_INFO']['os_name'];
		}
		else
		{
			$OS_NAME = $PACKER_INFO['HOST_INFO']['guest_os_name'];
		}

		#GETTING DIFFERENT TYPE OF ADDRESS
		if (strpos($OS_NAME, 'Microsoft') !== false)
		{ 
			$OS_TYPE = 'MS';
			$TAG_NAME = 'volume';
		}
		else
		{
			$OS_TYPE = 'LX';
			$TAG_NAME = 'mount point';
		}
		
		#MAP SNAPSHOT INTO JSON
		if ($RECY_TYPE == 'RECOVERY_PM')
		{
			$SNAP_ARRAY = array_values(array_diff(explode(',',$SNAP_INFO),array('false')));
			$SNAP_JSON = $SNAP_INFO;
		}
		else
		{
			$SNAP_JSON_INFO = json_decode($SNAP_INFO);
			if (json_last_error() === JSON_ERROR_NONE)
			{
				$SNAP_INFO = implode(',',$SNAP_JSON_INFO);
			}
			$SNAP_ARRAY = array_values(array_diff(explode(',',$SNAP_INFO),array('false')));
			$SNAP_JSON = json_encode($SNAP_ARRAY);
		}
		
		#GET CONNECTION INFORMATION
		$CONN_INFO 					= $this -> ServerMgmt -> query_connection_info($CONN_UUID);
		$HOST_UUID 					= $CONN_INFO['HOST_UUID'];
		$LOADER_OPEN_SERV_INFO   	= $CONN_INFO['LOAD_OPEN'];
		$LAUNCHER_OPEN_SERV_INFO   	= $CONN_INFO['LAUN_OPEN'];
		$LAUNCHER_UUID 				= $CONN_INFO['LAUN_UUID'];
		$LAUNCHER_SYST				= $CONN_INFO['LAUN_SYST'];
		$MGMT_ADDR					= $CONN_INFO['MGMT_ADDR'];
		$CLUSTER_UUID				= $CONN_INFO['CLUSTER_UUID'];
		$IS_PROMOTE					= $CONN_INFO['LAUN_PROMOTE'];
		
		#TRANSFORM CLOUD TYPE FROM AZURE TO AZURE BLOB TYPE
		if ($JOBS_INFO -> is_azure_blob_mode == TRUE)
		{
			$CONN_INFO['CLOUD_TYPE'] = 'AzureBlob';
		}

		#GET AZURE MGMT DISK TYPE
		$AZURE_MGMT_DISK = false;
		if (isset($JOBS_INFO -> is_azure_mgmt_disk))
		{
			$AZURE_MGMT_DISK = $JOBS_INFO -> is_azure_mgmt_disk;
		}

		#DETECTION LOADER INFORMATION
		$LOAD_SERVER_INFO = explode('|',$LOADER_OPEN_SERV_INFO);
		if (isset($LOAD_SERVER_INFO[1]))
		{
			$LOAD_SERVER_UUID = $LOAD_SERVER_INFO[0];
			$LOAD_SERVER_ZONE = $LOAD_SERVER_INFO[1];
		}
		else
		{
			$LOAD_SERVER_UUID = $LOADER_OPEN_SERV_INFO;
			$LOAD_SERVER_ZONE = 'Neptune';
		}
		
		#DETECTION LAUNCHER INFORMATION
		$LAUN_SERVER_INFO = explode('|',$LAUNCHER_OPEN_SERV_INFO);
		if (isset($LAUN_SERVER_INFO[1]))
		{
			$LAUN_SERVER_UUID = $LAUN_SERVER_INFO[0];
			$LAUN_SERVER_ZONE = $LAUN_SERVER_INFO[1];
		}
		else
		{
			$LAUN_SERVER_UUID = $LAUNCHER_OPEN_SERV_INFO;
			$LAUN_SERVER_ZONE = 'Neptune';
		}
		
		#PRE_DEFAULT RECOVERY SETTINGS
		$SERVICE_SETTINGS -> os_type = $OS_TYPE;
		$SERVICE_SETTINGS -> winpe_job = $REPL_QUERY['WINPE_JOB'];
		$SERVICE_SETTINGS -> auto_reboot = false;
		$SERVICE_SETTINGS -> is_azure_mgmt_disk = $AZURE_MGMT_DISK;
		$SERVICE_SETTINGS -> convert_type = -1;
		$SERVICE_SETTINGS -> serverId = $LAUNCHER_UUID ;

		#DATAMODE
		if (!isset($SERVICE_SETTINGS -> datamode_instance) OR $SERVICE_SETTINGS -> datamode_instance == ''){$SERVICE_SETTINGS -> datamode_instance = 'NoAssociatedDataModeInstance';}
		if (!isset($SERVICE_SETTINGS -> is_datamode_boot) OR $SERVICE_SETTINGS -> is_datamode_boot == ''){$SERVICE_SETTINGS -> is_datamode_boot = false;}
		if (!isset($SERVICE_SETTINGS -> datamode_power) OR $SERVICE_SETTINGS -> datamode_power == ''){$SERVICE_SETTINGS -> datamode_power = 'off';}

		#SAVE NEW SERVICE JOB
		$SERVICE_UUID = $this -> create_service_job($ACCT_UUID,$REGN_UUID,$REPL_UUID,$CLUSTER_UUID,$SERV_UUID,$RECY_TYPE,$SNAP_JSON,$PACK_UUID,$CONN_UUID,$SERVICE_SETTINGS);

		#FOR NON-BOOTABLE DATAMODE RECOVER
		if ($SERVICE_SETTINGS -> datamode_instance != 'NoAssociatedDataModeInstance' AND $SERVICE_SETTINGS -> is_datamode_boot == FALSE)
		{
			$this -> datamode_recovery($REPL_UUID,$SERVICE_UUID,$SERVICE_SETTINGS,$RECY_TYPE);
			return true;
		}

		#CHECK CLOUD LIMITATIONS
		switch ($CONN_INFO["CLOUD_TYPE"])
		{
			case 'OPENSTACK':
				#NOT AVAILABLE
			break;

			case 'AWS':
				#NOT AVAILABLE
			break;

			case 'Azure':
			case 'AzureBlob':
				if($SERVICE_SETTINGS -> private_address_id == 'DynamicAssign' || $SERVICE_SETTINGS -> private_address_id == '' )
					break;
				
				$ret = explode( '|', $SERVICE_SETTINGS -> network_uuid );
				
				$virtualNetwork = $ret[0];
				
				$this -> AzureMgmt -> getVMByServUUID($LAUNCHER_UUID,$CLUSTER_UUID);
				
				$result = $this -> AzureMgmt -> CheckPrivateIp( $CLUSTER_UUID, $virtualNetwork, $SERVICE_SETTINGS -> private_address_id );
				
				if( json_decode( $result, true )["available"] == false ){
					
					$MESSAGE = $this -> ReplMgmt -> job_msg('cannot use private ip %1%',array($SERVICE_SETTINGS -> private_address_id));
					
					$this -> ReplMgmt -> update_job_msg($SERVICE_UUID,$MESSAGE,'Service');
					
					return false;
				}
			break;

			case 'Aliyun':
				$LAUN_INFO = explode('|',$CONN_INFO['LAUN_OPEN']);			
				$SERVER_REGION = $LAUN_INFO[1];
				
				$AliModel = new Aliyun_Model();				
				$cloud_info = $AliModel->query_cloud_connection_information( $CONN_INFO["CLUSTER_UUID"] );

				$bucketName = "saasame-".$SERVER_REGION.'-'.md5( $cloud_info['ACCESS_KEY'] );
				$RAMret = $this->AliMgmt->CheckOssPermission( $CONN_INFO["CLUSTER_UUID"] );
				$OSSret = $this->AliMgmt->VerifyOss( $CONN_INFO["CLUSTER_UUID"], $SERVER_REGION, $bucketName, false );
				if( $OSSret !== true or $RAMret === false )
				{
					$MESSAGE = $this -> ReplMgmt -> job_msg('Please activate OSS and RAM services from Alibaba console.');
					$this -> ReplMgmt -> update_job_msg($SERVICE_UUID,$MESSAGE,'Service');
					return false;				
				}
			break;

			case 'Tencent':
				$LAUN_INFO = explode('|',$CONN_INFO['LAUN_OPEN']);
				$SERVER_REGION = $LAUN_INFO[1];
			
				$AliModel = new Aliyun_Model();
				$cloud_info = $AliModel->query_cloud_connection_information( $CONN_INFO["CLUSTER_UUID"] );

				$bucketName = "saasame-".$SERVER_REGION;
				$OSSret = $this->TencentMgmt->VerifyOss( $CONN_INFO["CLUSTER_UUID"], $SERVER_REGION, $bucketName );
			
				if( $OSSret !== true )
				{
					$MESSAGE = $this -> ReplMgmt -> job_msg('Bucket create fail.');
					$this -> ReplMgmt -> update_job_msg($SERVICE_UUID,$MESSAGE,'Service');
					return false;				
				}
			break;
		}		
		
		#DIRECT/IN-DIRECT ADDRESS
		if ($CONN_INFO['LAUN_DIRECT'] == TRUE)
		{		
			$LAUNCHER_ADDR = $CONN_INFO['LAUN_ADDR'];
		}
		else
		{
			$LAUNCHER_ADDR = array_flip($CONN_INFO['LAUN_ID']);
		}
		
		#CHECK LINUX SERVICES
		if ($OS_TYPE == 'LX')
		{
			$CHECK_LX_SERVICE = $this -> check_service('LINUX_LAUNCHER_SERVICE',$LAUNCHER_ADDR);
			if ($CHECK_LX_SERVICE == FALSE)
			{
				$MESSAGE = $this -> ReplMgmt -> job_msg('Linux Launcher service cannot be found.');
				$this -> ReplMgmt -> update_job_msg($SERVICE_UUID,$MESSAGE,'Service');
				return false;
			}
			
			$MEMORY_REQUIREMENT = $this -> check_memory_requirement($LAUNCHER_ADDR);
			if ($MEMORY_REQUIREMENT == FALSE)
			{
				$MESSAGE = $this -> ReplMgmt -> job_msg('Linux Launcher service required at least 2GB of memory.');
				$this -> ReplMgmt -> update_job_msg($SERVICE_UUID,$MESSAGE,'Service');
				return false;
			}			
		}
		
		#DELAY LAUNCHER JOB SUBMIT
		$this -> delay_launcher_job_submit($SERVICE_UUID,$CONN_UUID);
		
		#GET REPLICA BOOT DISK ID
		$REPL_DISK_ARRAY = $this -> ReplMgmt -> query_replica_disk($REPL_UUID);
		
		#FILTER OUT RECOVERY DISK
		foreach ($SNAP_ARRAY as $PM_FILTER_UUID){
			for ($f=0; $f<count($REPL_DISK_ARRAY); $f++)
			{
				if ($REPL_DISK_ARRAY[$f]['OPEN_DISK'] == $PM_FILTER_UUID)
				{
					unset($REPL_DISK_ARRAY[$f]);
				}
			}
			$REPL_DISK_ARRAY = array_values($REPL_DISK_ARRAY); #RE-INDEX
		}

		#UPDATE BOOT DISK ID
		$SERV_INFO = $this -> query_service($SERVICE_UUID);
		$SERV_JOB_INFO = json_decode($SERV_INFO['JOBS_JSON'],false);
		
		#DEFAULT BOOT DISK ID SET TO 0 FOR MOST OF CASE
		$BOOT_DISK_ID = 0;
		for ($i=0; $i<count($REPL_DISK_ARRAY); $i++)
		{
			if ($REPL_DISK_ARRAY[$i]['IS_BOOT'] == true)
			{
				$BOOT_DISK_ID = $i;
				break;
			}
		}		
		$SERV_JOB_INFO -> boot_disk_id = $BOOT_DISK_ID;
		
		#DEFAULT SYSTEM DISK ID
		$SYSTEM_DISK_ID = array();
		for ($x=0; $x<count($REPL_DISK_ARRAY); $x++)
		{
			if ($REPL_DISK_ARRAY[$x]['SYSTEM_DISK'] == true)
			{
				$SYSTEM_DISK_ID[] = $x;
			}
		}
		#GIVE DEFAULT EMPTY INDEX ID
		if (empty($SYSTEM_DISK_ID)){$SYSTEM_DISK_ID = array(0);};
		
		$SERV_JOB_INFO -> system_disk_id = json_encode($SYSTEM_DISK_ID);

		#UPDATE TRUGGER INFORMATION
		$this -> update_trigger_info($SERVICE_UUID,$SERV_JOB_INFO,'SERVICE');
		
		#QUERY DEFINE TYPE
		$DISK_TYPE = $SERVICE_SETTINGS -> disk_type;
		
		#MUTEX MSG
		$MUTEX_MSG = $SERVICE_UUID.'-'.$MGMT_ADDR.'-'.__FUNCTION__;
		
		#LOCK WITH MUTEX CONTROL
		$this -> disk_mutex_action($SERVICE_UUID,'Service',$LAUNCHER_ADDR,$MUTEX_MSG,'LOCK_EX',$LAUNCHER_SYST);
		
		#SWITCH BETWEEN PLANNED MIGRATION, DISASTER RECOVERY AND DEVELOPMENT TESTING		
		###################
		# Planned_Migration
		###################	
		switch ($RECY_TYPE)
		{	
			case "RECOVERY_PM":			
				#OVER WRITE TRIGGER SYNC FLAG
				if (isset($JOBS_INFO -> migration_executed) AND $JOBS_INFO -> migration_executed == TRUE)
				{
					$TRIGGER_SYNC = false;
				}
				
				#DISABLE SYNC BUTTON
				$JOBS_INFO -> migration_executed = true;
				$this -> update_trigger_info($REPL_UUID,$JOBS_INFO,'REPLICA');
							
				#TRIGGER RESYNC
				$this -> trigger_resync($REPL_UUID,$SERVICE_UUID,$TRIGGER_SYNC);
				
				#QUERY REPLICA DISK INFORMATION
				$MESSAGE = $this -> ReplMgmt -> job_msg('Getting replica disk information.');
				$this -> ReplMgmt -> update_job_msg($SERVICE_UUID,$MESSAGE,'Service');
				
				for ($REPL_DISK = 0; $REPL_DISK<count($REPL_DISK_ARRAY); $REPL_DISK++)
				{
					$VOLUME_UUID = $REPL_DISK_ARRAY[$REPL_DISK]['OPEN_DISK'];
					$DISK_SIZE   = $REPL_DISK_ARRAY[$REPL_DISK]['DISK_SIZE'] / 1024 / 1024 / 1024;
					if (is_float($DISK_SIZE))
					{
						$DISK_SIZE = ceil($DISK_SIZE);	
					}
					else
					{	
						$DISK_SIZE = $DISK_SIZE + 1;
					}
					
					#NEW SERVICE DISK
					$SNAP_UUID = 'Planned_Migration';				
					$this -> new_service_disk($SERVICE_UUID,$PACK_UUID,$DISK_SIZE,$VOLUME_UUID,$SNAP_UUID);
					
					#BEGIN TO PREPARE PLANNED MIGRATION DISK				
					if ($LAUNCHER_SYST == 'WINDOWS')
					{
						#GET DISK ADDRESS
						$SCSI_ADDR = $REPL_DISK_ARRAY[$REPL_DISK]['SCSI_ADDR'];
						
						#UPDATE VOLUME INFORMATION
						$this -> update_disk_scsi_info($VOLUME_UUID,$SCSI_ADDR);
					}
					else
					{
						#VOLUME DETACH FROM WINDOWS TRANSPORT SERVER
						switch ($CONN_INFO["CLOUD_TYPE"])
						{
							case 'OPENSTACK':
								$this -> OpenStackMgmt -> detach_volume_from_server($CLUSTER_UUID,$LOAD_SERVER_UUID,$VOLUME_UUID);
							break;
									
							case 'AWS':
								$this -> AwsMgmt -> detach_volume($CLUSTER_UUID,$LOAD_SERVER_ZONE,$VOLUME_UUID);
							break;
									
							case 'Azure':
								#array_push($detach_disk_array,$VOLUME_UUID);	#TO BE CHECK
							break;
						
							case 'AzureBlob':
								#NOT AVAILABLE
							break;
							
							case 'Aliyun':
								#$this -> AliMgmt -> detach_volume($CLUSTER_UUID,$LOAD_SERVER_ZONE,$LOAD_SERVER_UUID,$VOLUME_UUID); #TO BE CHECK
							break;									
						}

						#SLEEP 5
						sleep(5);
				
						#GET FIRST SCSI ADDRESS LIST
						$MESSAGE = $this -> ReplMgmt -> job_msg('Getting system %1% information.',array($TAG_NAME));
						$this -> ReplMgmt -> update_job_msg($SERVICE_UUID,$MESSAGE,'Service');
							
						sleep(5);
						$FI_SCSI_ADDR = $this -> list_disk_addr_info($HOST_UUID,$LAUNCHER_ADDR,'Launcher',$OS_TYPE,'Physical');					
						
						#ATTACH VOLUME TO SERVER
						switch ($CONN_INFO["CLOUD_TYPE"])
						{
							case 'OPENSTACK':
								$ATTACH_VOLUME = $this -> OpenStackMgmt -> attach_volume($CLUSTER_UUID,$LAUN_SERVER_UUID,$SERVICE_UUID,$VOLUME_UUID);
							break;
								
							case 'AWS':
								$ATTACH_VOLUME = $this -> AwsMgmt -> attach_volume($CLUSTER_UUID,$LAUN_SERVER_ZONE,$LAUN_SERVER_UUID,$VOLUME_UUID);
							break;
								
							case 'Azure':
								try{
									$this -> AzureMgmt -> getVMByServUUID($LAUNCHER_UUID,$CLUSTER_UUID);
									$ATTACH_VOLUME = $this -> AzureMgmt -> attach_volume($CLUSTER_UUID,$LAUN_SERVER_ZONE,$LAUN_SERVER_UUID,$VOLUME_UUID);
								}
								catch (Exception $e) {
									$ATTACH_VOLUME = FALSE;
								}
							break;
							
							case 'AzureBlob':
								#NOT AVAILABLE
							break;
								
							case 'Aliyun':
								$ATTACH_VOLUME = $this -> AliMgmt -> attach_volume($CLUSTER_UUID,$LAUN_SERVER_ZONE,$LAUN_SERVER_UUID,$VOLUME_UUID);
							break;								
						}
			
						if ($ATTACH_VOLUME == FALSE)
						{
							$MESSAGE = $this -> ReplMgmt -> job_msg('Cannot attach the volume to the Transport server.');
							$this -> ReplMgmt -> update_job_msg($SERVICE_UUID,$MESSAGE,'Service');
							
							#UNLOCK WITH MUTEX CONTROL
							$this -> disk_mutex_action($SERVICE_UUID,'Service',$LAUNCHER_ADDR,$MUTEX_MSG,'LOCK_UN',$LAUNCHER_SYST);
							
							return false;
						}
						else
						{
							#GET SECOND SCSI ADDRESS LIST
							$MESSAGE = $this -> ReplMgmt -> job_msg('Getting new %1% information.',array($TAG_NAME));
							$this -> ReplMgmt -> update_job_msg($SERVICE_UUID,$MESSAGE,'Service');
									
							sleep(5);
									
							$SCSI_ADDR = $this -> CompareEnumerateDisks($HOST_UUID,$LAUNCHER_ADDR,'Launcher',$OS_TYPE,'Physical',$FI_SCSI_ADDR);
								
							if ($SCSI_ADDR == '')
							{
								$MESSAGE = $this -> ReplMgmt -> job_msg('Cannot get volume address information.');
								$this -> ReplMgmt -> update_job_msg($SERVICE_UUID,$MESSAGE,'Service');
								
								#UNLOCK WITH MUTEX CONTROL
								$this -> disk_mutex_action($SERVICE_UUID,'Service',$LAUNCHER_ADDR,$MUTEX_MSG,'LOCK_UN',$LAUNCHER_SYST);
								
								return false;
							}
							else
							{
								#UPDATE DISPLAY MESSAGE
								if (strlen($SCSI_ADDR) == 32)
								{
									$MESSAGE = $this -> ReplMgmt -> job_msg('New %1% unique id %2%',array($TAG_NAME,$SCSI_ADDR));
								}
								elseif (strlen($SCSI_ADDR) == 36)
								{
									$MESSAGE = $this -> ReplMgmt -> job_msg('New %1% customized id %2%',array($TAG_NAME,$SCSI_ADDR));
								}
								elseif (strlen($SCSI_ADDR) > 16)
								{
									$MESSAGE = $this -> ReplMgmt -> job_msg('New %1% serial number is %2%',array($TAG_NAME,$SCSI_ADDR));
								}
								else
								{
									$MESSAGE = $this -> ReplMgmt -> job_msg('New %1% address is %2%',array($TAG_NAME,$SCSI_ADDR));
								}
								$this -> ReplMgmt -> update_job_msg($SERVICE_UUID,$MESSAGE,'Service');
								
								$this -> update_disk_scsi_info($VOLUME_UUID,$SCSI_ADDR);
							}
						}					
					}				
				}

				#LAST PREPARE VOLUME ACTION MESSAGE
				$MESSAGE = $this -> ReplMgmt -> job_msg('The volume is ready for the Launcher.');
				$this -> ReplMgmt -> update_job_msg($SERVICE_UUID,$MESSAGE,'Service');
			break;
			
			case 'RECOVERY_DR':
			case 'RECOVERY_DT':
				if ($RECY_TYPE == 'RECOVERY_DR')
				{
					#DISABLE SYNC BUTTON
					$JOBS_INFO -> migration_executed = true;
					$this -> update_trigger_info($REPL_UUID,$JOBS_INFO,'REPLICA');
				
					#TRIGGER RESYNC (DISABLE PREPARE WORKLOAD)
					$this -> trigger_resync($REPL_UUID,$SERVICE_UUID,false);
				}
				
				#INTERRUPT_SERVICE_JOB FOR CTYUN
				if ($RECY_TYPE == 'RECOVERY_DT' AND $CONN_INFO["CLOUD_TYPE"] == 'Ctyun')
				{
					$MESSAGE = $this -> ReplMgmt -> job_msg('Job paused.');
					$this -> ReplMgmt -> update_job_msg($REPL_UUID,$MESSAGE,'Replica');
					$this -> interrupt_service_job($REPL_UUID,'LOADER');
					$this -> check_running_job($REPL_UUID,'LOADER');
					
					#UPDATE PAUSE STATUS
					$JOBS_INFO -> is_paused = true;
					$this -> update_trigger_info($REPL_UUID,$JOBS_INFO,'REPLICA');
				}					
			
				for ($DISK=0; $DISK<count($SNAP_ARRAY); $DISK++)
				{
					#GET SNAPSHOT UUID
					$SNAP_UUID = $SNAP_ARRAY[$DISK];

					#GET SNAPSHOT INFORMATION
					$MESSAGE = $this -> ReplMgmt -> job_msg('Getting snapshot information.');
					$this -> ReplMgmt -> update_job_msg($SERVICE_UUID,$MESSAGE,'Service');
					
					#GET DISK SIZE
					switch ($CONN_INFO["CLOUD_TYPE"])
					{
						case 'OPENSTACK':
							$DISK_SIZE = $this -> OpenStackMgmt -> get_snapshot_detail_list($CLUSTER_UUID,$SNAP_UUID) -> snapshot -> size;
						break;
						
						case 'AWS':
							$DISK_SIZE = $this -> AwsMgmt -> describe_snapshots($CLUSTER_UUID,$LAUN_SERVER_ZONE,null,$SNAP_UUID)[0]['VolumeSize'];
						break;
						
						case 'Azure':
							try{
								$this -> AzureMgmt -> getVMByServUUID($LAUNCHER_UUID,$CLUSTER_UUID);
								$SNAPSHOT_DETAIL = $this -> AzureMgmt -> describe_snapshot_detail($CLUSTER_UUID,$LAUN_SERVER_ZONE,$SNAP_UUID);
								$DISK_SIZE = $SNAPSHOT_DETAIL['properties']['diskSizeGB'];
							}
							catch (Exception $e) {
								$DISK_SIZE = NULL;
							}
						break;
						
						case 'AzureBlob':
							$SNAPSHOT_DETAIL = $this -> AzureBlobMgmt -> get_snapshot_info($REPL_UUID,$SNAP_UUID);
							$DISK_SIZE = $SNAPSHOT_DETAIL[0]['properties']['diskSizeGB'];
							$DISK_NAME = $SNAPSHOT_DETAIL[0]['properties']['diskName'];
						break;
						
						case 'Aliyun':
							$SNAPSHOT_DETAIL = $this -> AliMgmt -> describe_snapshot_detail($CLUSTER_UUID,$LAUN_SERVER_ZONE,$SNAP_UUID);
							$DISK_SIZE = $SNAPSHOT_DETAIL[0]['size'];
						break;
						
						case 'Tencent':
							$SNAPSHOT_DETAIL = $this -> TencentMgmt -> describe_snapshot_detail($CLUSTER_UUID,$LAUN_SERVER_ZONE,$SNAP_UUID);
							$DISK_SIZE = $SNAPSHOT_DETAIL[0]['size'];
						break;
						
						case 'Ctyun':
							$DISK_SIZE = $this -> CtyunMgmt -> describe_snapshot($CLUSTER_UUID,$SNAP_UUID) -> backup -> size;
						break;
					}
					
					$MESSAGE = $this -> ReplMgmt -> job_msg('The snapshot size is %1%',array($DISK_SIZE.'GB'));
					$this -> ReplMgmt -> update_job_msg($SERVICE_UUID,$MESSAGE,'Service');
					
					#############
					# VOLUME:SYSTEM_DISK
					#############
					if (in_array($DISK, $SYSTEM_DISK_ID))
					{
						$MESSAGE = $this -> ReplMgmt -> job_msg('Creating a new system volume.');
						$this -> ReplMgmt -> update_job_msg($SERVICE_UUID,$MESSAGE,'Service');
							
						###################
						# CLOUD_ZONE
						###################
						switch ($CONN_INFO["CLOUD_TYPE"])
						{
							case 'OPENSTACK':
								$VOLUME_UUID = $this -> OpenStackMgmt -> create_volume_by_snapshot($CLUSTER_UUID,$SERVICE_UUID,$DISK_SIZE,$HOST_NAME,$SNAP_UUID);
							break;
								
							case 'AWS':
								$VOLUME_UUID = $this -> AwsMgmt -> create_volume_from_snapshot($CLUSTER_UUID,$LAUN_SERVER_ZONE,$SNAP_UUID,$HOST_NAME);
							break;
									
							case 'Azure':
								try{
									$this->AzureMgmt->setReplicaId( $REPL_UUID );
									$this -> AzureMgmt -> getVMByServUUID($LAUNCHER_UUID,$CLUSTER_UUID);
									$VOLUME_UUID = $this -> AzureMgmt -> create_volume_from_snapshot($CLUSTER_UUID,$LAUN_SERVER_ZONE,$SNAP_UUID,$HOST_NAME.'_'.$DISK);
								}
								catch (Exception $e) {
									$VOLUME_UUID = false;
								}
							break;
								
							case 'AzureBlob':
								$VOLUME_UUID = $this -> AzureBlobMgmt -> create_volume_from_snapshot($CLUSTER_UUID,$LAUNCHER_UUID,$LAUN_SERVER_ZONE,$REPL_UUID,$AZURE_MGMT_DISK,$DISK,$DISK_NAME,$DISK_SIZE,$SNAP_UUID,$IS_PROMOTE);
							break;

							case 'Aliyun':
								$VOLUME_UUID = $this -> AliMgmt -> create_volume_from_snapshot($CLUSTER_UUID,$LAUN_SERVER_ZONE,$SNAP_UUID,$HOST_NAME.'_'.$DISK,$DISK_TYPE);
							break;
								
							case 'Tencent':
								$VOLUME_UUID = $this -> TencentMgmt -> create_volume_from_snapshot($CLUSTER_UUID,$LAUN_SERVER_ZONE,$SNAP_UUID,$HOST_NAME,"HDD",$DISK_SIZE);
							break;
								
							case 'Ctyun':
								$VOLUME_UUID = $this -> CtyunMgmt -> create_volume_by_snapshot($CLUSTER_UUID,$SERVICE_UUID,$SNAP_UUID);
							break;
						}
							
						#NEW SERVICE DISK
						$this -> new_service_disk($SERVICE_UUID,$PACK_UUID,$DISK_SIZE,$VOLUME_UUID,$SNAP_UUID);
								
						if ($VOLUME_UUID == false)
						{
							$MESSAGE = $this -> ReplMgmt -> job_msg('Cannot create volume.');
							$this -> ReplMgmt -> update_job_msg($SERVICE_UUID,$MESSAGE,'Service');
								
							#UNLOCK WITH MUTEX CONTROL
							$this -> disk_mutex_action($SERVICE_UUID,'Service',$LAUNCHER_ADDR,$MUTEX_MSG,'LOCK_UN',$LAUNCHER_SYST);
							
							return false;
						}
							
						#SLEEP 5
						sleep(5);
				
						#GET FIRST SCSI ADDRESS LIST
						$MESSAGE = $this -> ReplMgmt -> job_msg('Getting system %1% information.',array($TAG_NAME));
						$this -> ReplMgmt -> update_job_msg($SERVICE_UUID,$MESSAGE,'Service');
							
						$FI_SCSI_ADDR = $this -> list_disk_addr_info($HOST_UUID,$LAUNCHER_ADDR,'Launcher',$OS_TYPE,'Physical');
						sleep(5);
						
						#ATTACH VOLUME TO SERVER
						switch ($CONN_INFO["CLOUD_TYPE"])
						{
							case 'OPENSTACK':
								$ATTACH_VOLUME = $this -> OpenStackMgmt -> attach_volume($CLUSTER_UUID,$LAUN_SERVER_UUID,$SERVICE_UUID,$VOLUME_UUID);					
							break;
							
							case 'AWS':
								$ATTACH_VOLUME = $this -> AwsMgmt -> attach_volume($CLUSTER_UUID,$LAUN_SERVER_ZONE,$LAUN_SERVER_UUID,$VOLUME_UUID);
							break;
								
							case 'Azure':
							case 'AzureBlob':
								if ($IS_PROMOTE == TRUE)
								{
									$ATTACH_VOLUME = $IS_PROMOTE;
								}
								elseif($AZURE_MGMT_DISK == FALSE)
								{
									$ATTACH_VOLUME = $this -> AzureBlobMgmt -> attach_volume($CLUSTER_UUID,$LAUNCHER_UUID,$LAUN_SERVER_UUID,$LAUN_SERVER_ZONE,$REPL_UUID,$VOLUME_UUID,$DISK_SIZE);
								}
								else
								{								
									try{
										$this -> AzureMgmt -> getVMByServUUID($LAUNCHER_UUID,$CLUSTER_UUID);
										$ATTACH_VOLUME = $this -> AzureMgmt -> attach_volume($CLUSTER_UUID,$LAUN_SERVER_ZONE,$LAUN_SERVER_UUID,$VOLUME_UUID);
									}
									catch (Exception $e) {
										$ATTACH_VOLUME = false;
									}
								}
							break;
								
							case 'Aliyun':
								$ATTACH_VOLUME = $this -> AliMgmt -> attach_volume($CLUSTER_UUID,$LAUN_SERVER_ZONE,$LAUN_SERVER_UUID,$VOLUME_UUID);
							break;

							case 'Tencent':
								$ATTACH_VOLUME = $this -> TencentMgmt -> attach_volume($CLUSTER_UUID,$LAUN_SERVER_ZONE,$LAUN_SERVER_UUID,$VOLUME_UUID);
							break;
							
							case 'Ctyun':
								$ATTACH_VOLUME = $this -> CtyunMgmt -> attach_volume($SERVICE_UUID,$CLUSTER_UUID,$LAUN_SERVER_UUID,$VOLUME_UUID,false);
							break;
						}	
	
						if ($ATTACH_VOLUME == FALSE)
						{
							$MESSAGE = $this -> ReplMgmt -> job_msg('Cannot attach the volume to the Transport server.');
							$this -> ReplMgmt -> update_job_msg($SERVICE_UUID,$MESSAGE,'Service');
								
							#UNLOCK WITH MUTEX CONTROL
							$this -> disk_mutex_action($SERVICE_UUID,'Service',$LAUNCHER_ADDR,$MUTEX_MSG,'LOCK_UN',$LAUNCHER_SYST);
								
							return false;
						}
						elseif (!isset($IS_PROMOTE) OR $IS_PROMOTE == FALSE)
						{
							#GET SECOND SCSI ADDRESS LIST
							$MESSAGE = $this -> ReplMgmt -> job_msg('Getting new %1% information.',array($TAG_NAME));
							$this -> ReplMgmt -> update_job_msg($SERVICE_UUID,$MESSAGE,'Service');
									
							sleep(5);
									
							$SCSI_ADDR = $this -> CompareEnumerateDisks($HOST_UUID,$LAUNCHER_ADDR,'Launcher',$OS_TYPE,'Physical',$FI_SCSI_ADDR);
							
							if ($SCSI_ADDR == '')
							{
								$MESSAGE = $this -> ReplMgmt -> job_msg('Cannot get volume address information.');
								$this -> ReplMgmt -> update_job_msg($SERVICE_UUID,$MESSAGE,'Service');
									
								#UNLOCK WITH MUTEX CONTROL
								$this -> disk_mutex_action($SERVICE_UUID,'Service',$LAUNCHER_ADDR,$MUTEX_MSG,'LOCK_UN',$LAUNCHER_SYST);
									
								return false;
							}
							else
							{
								$DISK_ADDR = Misc_Class::guid_v4();
						
								#NOTE: SET DISK ID WILL FALSE WHEN SCSI_ADDR AS SERIAL NUMBER
								$SET_DISK_ID = $this -> set_disk_customized_id($LAUNCHER_ADDR,$SCSI_ADDR,$DISK_ADDR);
									
								if ($SET_DISK_ID != TRUE)
								{
									$DISK_ADDR = $SCSI_ADDR;
								}
								
								$MESSAGE = $this -> ReplMgmt -> job_msg('A volume created and attached to the Transport server.');
								$this -> ReplMgmt -> update_job_msg($SERVICE_UUID,$MESSAGE,'Service');
							}
						}
						else
						{
							#FOR DIRECT AZURE BLOB
							$DISK_ADDR = $VOLUME_UUID;
						}
						
						#UPDATE DISK DISPLAY MESSAGE
						if (strlen($DISK_ADDR) == 32)
						{
							$MESSAGE = $this -> ReplMgmt -> job_msg('New %1% unique id %2%',array($TAG_NAME,$DISK_ADDR));
						}
						elseif (strlen($DISK_ADDR) == 36)
						{
							$MESSAGE = $this -> ReplMgmt -> job_msg('New %1% customized id %2%',array($TAG_NAME,$DISK_ADDR));
						}
						elseif (strlen($DISK_ADDR) > 16)
						{
							$MESSAGE = $this -> ReplMgmt -> job_msg('New %1% serial number is %2%',array($TAG_NAME,$DISK_ADDR));
						}
						else
						{
							$MESSAGE = $this -> ReplMgmt -> job_msg('New %1% address is %2%',array($TAG_NAME,$DISK_ADDR));
						}
						
						$this -> ReplMgmt -> update_job_msg($SERVICE_UUID,$MESSAGE,'Service');
					}
					#############
					# VOLUME:DATA
					#############
					else
					{
						$MESSAGE = $this -> ReplMgmt -> job_msg('Creating a new data volume.');
						$this -> ReplMgmt -> update_job_msg($SERVICE_UUID,$MESSAGE,'Service');
							
						#SCSI TEMP ADDRESS
						$DISK_ADDR = 'L:O:C:K';
					
						###################
						# CLOUD_ZONE
						###################
						switch ($CONN_INFO["CLOUD_TYPE"])
						{
							case 'OPENSTACK':
								$VOLUME_UUID = $this -> OpenStackMgmt -> create_volume_by_snapshot($CLUSTER_UUID,$SERVICE_UUID,$DISK_SIZE,$HOST_NAME,$SNAP_UUID);								
							break;
								
							case 'AWS':
								$VOLUME_UUID = $this -> AwsMgmt -> create_volume_from_snapshot($CLUSTER_UUID,$LAUN_SERVER_ZONE,$SNAP_UUID,$HOST_NAME);
							break;
									
							case 'Azure':
								try{
									$this->AzureMgmt->setReplicaId( $REPL_UUID );
									$this -> AzureMgmt -> getVMByServUUID($LAUNCHER_UUID,$CLUSTER_UUID);
									$VOLUME_UUID = $this -> AzureMgmt -> create_volume_from_snapshot($CLUSTER_UUID,$LAUN_SERVER_ZONE,$SNAP_UUID,$HOST_NAME.'_'.$DISK);
								}
								catch (Exception $e) {
									$VOLUME_UUID = FALSE;
								}
							break;
								
							case 'AzureBlob':
								$VOLUME_UUID = $this -> AzureBlobMgmt -> create_volume_from_snapshot($CLUSTER_UUID,$LAUNCHER_UUID,$LAUN_SERVER_ZONE,$REPL_UUID,$AZURE_MGMT_DISK,$DISK,$DISK_NAME,$DISK_SIZE,$SNAP_UUID,$IS_PROMOTE);
							break;
									
							case 'Aliyun':
								$VOLUME_UUID = $this -> AliMgmt -> create_volume_from_snapshot($CLUSTER_UUID,$LAUN_SERVER_ZONE,$SNAP_UUID,$HOST_NAME,$DISK_TYPE);	
							break;

							case 'Tencent':					
								$VOLUME_UUID = $this -> TencentMgmt -> create_volume_from_snapshot($CLUSTER_UUID,$LAUN_SERVER_ZONE,$SNAP_UUID,$HOST_NAME,$DISK_TYPE,$DISK_SIZE);	
							break;
							
							case 'Ctyun':
								$VOLUME_UUID = $this -> CtyunMgmt -> create_volume_by_snapshot($CLUSTER_UUID,$SERVICE_UUID,$SNAP_UUID);
							break;
						}
							
						#NEW SERVICE DISK
						$this -> new_service_disk($SERVICE_UUID,$PACK_UUID,$DISK_SIZE,$VOLUME_UUID,$SNAP_UUID);
						
						if ($VOLUME_UUID == FALSE)
						{
							$MESSAGE = $this -> ReplMgmt -> job_msg('Cannot create volume.');
							$this -> ReplMgmt -> update_job_msg($SERVICE_UUID,$MESSAGE,'Service');
							
							#UNLOCK WITH MUTEX CONTROL
							$this -> disk_mutex_action($SERVICE_UUID,'Service',$LAUNCHER_ADDR,$MUTEX_MSG,'LOCK_UN',$LAUNCHER_SYST);
							
							return false;
						}
					}

					#UPDATE DISK SCSI INFO
					$this -> update_disk_scsi_info($VOLUME_UUID,$DISK_ADDR);
			
					#LAST VOLUME ACTION MESSAGE
					$MESSAGE = $this -> ReplMgmt -> job_msg('The volume is ready for the Launcher.');
					$this -> ReplMgmt -> update_job_msg($SERVICE_UUID,$MESSAGE,'Service');
				}
			break;			
		}		
	
		#UNLOCK WITH MUTEX CONTROL
		$this -> disk_mutex_action($SERVICE_UUID,'Service',$LAUNCHER_ADDR,$MUTEX_MSG,'LOCK_UN',$LAUNCHER_SYST);

		#UPDATE JOB STATUS
		$SERV_JOB_INFO -> job_status = 'CloudDiskCreated';
		$this -> update_trigger_info($SERVICE_UUID,$SERV_JOB_INFO,'SERVICE');
		
		$MESSAGE = $this -> ReplMgmt -> job_msg('Submitted recovery process.');
		$this -> ReplMgmt -> update_job_msg($SERVICE_UUID,$MESSAGE,'Service');
		$this -> create_launcher_job($REPL_UUID,$CONN_UUID,$SERVICE_UUID);
	}
	
	###########################
	#PRE CREATE RECOVERY KIT LAUNCHER JOB
	###########################
	public function pre_create_recover_kit_launcher_job($ACCT_UUID,$REGN_UUID,$REPL_UUID,$CONN_UUID,$TRIGGER_SYNC,$AUTO_REBOOT,$CONVER_TYPE,$SERV_UUID)
	{
		#QUERY REPLICA INFORMATION
		$REPL_QUERY   = $this -> ReplMgmt -> list_replica(null,$REPL_UUID);
		$PACK_UUID    = $REPL_QUERY['PACK_UUID'];
		$HOST_NAME    = $REPL_QUERY['HOST_NAME'];
		$CLUSTER_UUID = $REPL_QUERY['CLUSTER_UUID'];
		$WINPE_JOB    = $REPL_QUERY['WINPE_JOB'];
		
		#DEFINE SNAPSHOT JSON
		$SNAP_JSON    = '00000000-RECOVERY-KIT-JOB-0000000000';
		
		#GET OS TYPE FOR MICROSOFT OR LINUX
		$PACKER_INFO = $this -> ServerMgmt -> query_host_info($PACK_UUID);
		if (isset($PACKER_INFO['HOST_INFO']['os_name']))
		{
			$OS_NAME = $PACKER_INFO['HOST_INFO']['os_name'];
		}
		else
		{
			$OS_NAME = $PACKER_INFO['HOST_INFO']['guest_os_name'];
		}		
		
		#GETTING DIFFERENT TYPE OF ADDRESS
		if (strpos($OS_NAME, 'Microsoft') !== false)
		{ 
			$OS_TYPE = 'MS';
		}
		else
		{
			$OS_TYPE = 'LX';
		}
		
		#GET CONNECTION INFORMATION
		$CONN_INFO = $this -> ServerMgmt -> query_connection_info($CONN_UUID);
		$LAUNCHER_UUID = $CONN_INFO['LAUN_UUID'];
				
		#DIRECT/IN-DIRECT ADDRESS
		if ($CONN_INFO['LAUN_DIRECT'] == TRUE)
		{		
			$LAUNCHER_ADDR = $CONN_INFO['LAUN_ADDR'];
		}
		else
		{
			$LAUNCHER_ADDR = array_flip($CONN_INFO['LAUN_ID']);
		}
		
		#RECOVERY TYPE
		$RECY_TYPE = 'RECOVERY_KIT';
		
		#SERVICE SETTINGS
		$SERVICE_SETTINGS = json_decode(
								json_encode(
									array(
										'auto_reboot'		 => $AUTO_REBOOT,
										'winpe_job'			 => $WINPE_JOB,
										'flavor_id' 		 => '00000000-RECOVERY-000-KIT-0000000000',
										'network_uuid' 		 => '00000000-RECOVERY-000-KIT-0000000000',
										'sgroup_uuid' 		 => '00000000-RECOVERY-000-KIT-0000000000',
										'os_type'			 => $OS_TYPE,
										'is_azure_mgmt_disk' => false,
										'convert_type'  	 => $CONVER_TYPE
							)));
		
		#CREATE SERVICE JOB
		$SERVICE_UUID = $this -> create_service_job($ACCT_UUID,$REGN_UUID,$REPL_UUID,$CLUSTER_UUID,$SERV_UUID,$RECY_TYPE,$SNAP_JSON,$PACK_UUID,$CONN_UUID,$SERVICE_SETTINGS);

		#CHECK LINUX SERVICES
		if ($OS_TYPE == 'LX')
		{
			$CHECK_LX_SERVICE = $this -> check_service('LINUX_LAUNCHER_SERVICE',$LAUNCHER_ADDR);
			if ($CHECK_LX_SERVICE == FALSE)
			{
				$MESSAGE = $this -> ReplMgmt -> job_msg('Linux Launcher service cannot be found.');
				$this -> ReplMgmt -> update_job_msg($SERVICE_UUID,$MESSAGE,'Service');
				return false;
			}
			
			$MEMORY_REQUIREMENT = $this -> check_memory_requirement($LAUNCHER_ADDR);
			if ($MEMORY_REQUIREMENT == FALSE)
			{
				$MESSAGE = $this -> ReplMgmt -> job_msg('Linux Launcher service required at least 2GB of memory.');
				$this -> ReplMgmt -> update_job_msg($SERVICE_UUID,$MESSAGE,'Service');
				return false;
			}
		}

		#TRIGGER RESYNC
		$this -> trigger_resync($REPL_UUID,$SERVICE_UUID,$TRIGGER_SYNC);
			
		#QUERY REPLICA DISK AND SNAPSHOT INFORMATION
		$MESSAGE = $this -> ReplMgmt -> job_msg('Getting replica disk information.');
		$this -> ReplMgmt -> update_job_msg($SERVICE_UUID,$MESSAGE,'Service');
		
		#NEW SERVICE DISK
		$REPL_DISK = $this -> ReplMgmt -> query_replica_disk($REPL_UUID);
		for ($i=0; $i<count($REPL_DISK); $i++)
		{			
			$SCSI_ADDR = $REPL_DISK[$i]['SCSI_ADDR'];
			$DISK_SIZE = $REPL_DISK[$i]['DISK_SIZE'] / 1024 / 1024 / 1024;
			$DISK_UUID = Misc_Class::guid_v4();
			$SNAP_UUID = $REPL_DISK[$i]['SNAPSHOT_MAPPING'];
			
			$this -> new_service_disk($SERVICE_UUID,$PACK_UUID,$DISK_SIZE,$DISK_UUID,$SNAP_UUID);
			$this -> update_disk_scsi_info($DISK_UUID,$SCSI_ADDR);
		}
		
		#QUERY SERVICE JOB INFO
		$SERV_INFO = $this -> query_service($SERVICE_UUID);
		$SERV_JOB_INFO = json_decode($SERV_INFO['JOBS_JSON'],false);
		
		#DEFAULT BOOT DISK ID SET TO 0 FOR MOST OF CASE
		$BOOT_DISK_ID = 0;
		for ($x=0; $x<count($REPL_DISK); $x++)
		{
			if ($REPL_DISK[$x]['IS_BOOT'] == true)
			{
				$BOOT_DISK_ID = $x;
				break;
			}
		}
		$SERV_JOB_INFO -> boot_disk_id = $BOOT_DISK_ID;

		#DEFAULT SYSTEM DISK ID
		$SYSTEM_DISK_ID = array();
		for ($x=0; $x<count($REPL_DISK); $x++)
		{
			if ($REPL_DISK[$x]['SYSTEM_DISK'] == true)
			{
				$SYSTEM_DISK_ID[] = $x;
			}
		}
		#GIVE DEFAULT EMPTY INDEX ID
		if (empty($SYSTEM_DISK_ID)){$SYSTEM_DISK_ID = array(0);};
		
		$SERV_JOB_INFO -> system_disk_id = json_encode($SYSTEM_DISK_ID);
		
		#UPDATE SERVUCE JOB INFO
		$this -> update_trigger_info($SERVICE_UUID,$SERV_JOB_INFO,'SERVICE');

		#LAST VOLUME ACTION MESSAGE
		$MESSAGE = $this -> ReplMgmt -> job_msg('The volume is ready for the Launcher.');
		$this -> ReplMgmt -> update_job_msg($SERVICE_UUID,$MESSAGE,'Service');
		
		$MESSAGE = $this -> ReplMgmt -> job_msg('Submitted recovery process.');
		$this -> ReplMgmt -> update_job_msg($SERVICE_UUID,$MESSAGE,'Service');
		
		$this -> create_launcher_job($REPL_UUID,$CONN_UUID,$SERVICE_UUID);
	}
	
	###########################
	# VMWARE : checkLauncher
	###########################
	public function checkLauncher( $ipaddress, $serviceId ){

		$CHECK_LX_SERVICE = $this -> check_service('LINUX_LAUNCHER_SERVICE',$ipaddress);
		if ($CHECK_LX_SERVICE == FALSE)
		{
			$MESSAGE = $this -> ReplMgmt -> job_msg('Linux Launcher service cannot be found.');
			$this -> ReplMgmt -> update_job_msg($serviceId,$MESSAGE,'Service');
			return false;
		}

		$MEMORY_REQUIREMENT = $this -> check_memory_requirement($ipaddress);
		if ($MEMORY_REQUIREMENT == FALSE)
		{
			$MESSAGE = $this -> ReplMgmt -> job_msg('Linux Launcher service required at least 2GB of memory.');
			$this -> ReplMgmt -> update_job_msg($serviceId,$MESSAGE,'Service');
			return false;
		}
		
		return true;
	}
	
	###########################
	#PRE CREATE VMWARE LAUNCHER JOB
	###########################
	public function pre_create_vmware_launcher_job($ACCT_UUID,$REGN_UUID,$REPL_UUID,$SERV_UUID,$PLAN_UUID,$RECY_TYPE,$SERVICE_SETTINGS){

		$sqlClient = new Common_Model();
		
		$replicaInfo = $sqlClient->getReplicatInfo( $REPL_UUID );
		
		$TransportInfo = $sqlClient->getTransportInfo( $replicaInfo[0]["ServerId"] );

		$snapshots = $SERVICE_SETTINGS->snap_uuid;
		
		if ($RECY_TYPE != 'RECOVERY_PM')
			$snapshots = json_encode( explode(',',$SERVICE_SETTINGS->snap_uuid) );

		$SERVICE_SETTINGS->flavor_id = "none";
		$SERVICE_SETTINGS->network_uuid = addslashes( json_encode( $SERVICE_SETTINGS->NETWORK_UUID ) );
		$SERVICE_SETTINGS->sgroup_uuid = "none";
		$SERVICE_SETTINGS->winpe_job = "N";
		$SERVICE_SETTINGS->auto_reboot = "N";
		
		$SERVICE_SETTINGS->os_type  = (strpos( $replicaInfo[0]["os_name"], 'Microsoft') !== false || strpos( $replicaInfo[0]["guest_os_name"], 'Microsoft') !== false )?"MS":"LX";

		$SERVICE_UUID = $this -> create_service_job( $ACCT_UUID, $REGN_UUID, $REPL_UUID, $replicaInfo[0]["CloudId"], $SERV_UUID,
			$RECY_TYPE, $snapshots, $replicaInfo[0]["PackId"], $replicaInfo[0]["TargetConnectionId"], $SERVICE_SETTINGS);

		if( $SERVICE_SETTINGS->os_type == "LX" && !$this->checkLauncher( $TransportInfo["ConnectAddr"], $SERVICE_UUID ) )
			return false;

		if ($RECY_TYPE == 'RECOVERY_PM' || $RECY_TYPE == 'RECOVERY_DR'){
			$REPL_QUERY = $this -> ReplMgmt -> list_replica(null,$REPL_UUID);
			$JOBS_INFO = $REPL_QUERY['JOBS_JSON'];
			$JOBS_INFO -> migration_executed = true;
			$this -> update_trigger_info($REPL_UUID,$JOBS_INFO,'REPLICA');

			$this->trigger_resync($REPL_UUID,$SERVICE_UUID,false);
		}

		$this->InsertBootDiskToServiceDisk( $REPL_UUID, $SERVICE_UUID );
		
		$MESSAGE = $this->ReplMgmt->job_msg('Submitted recovery process.');
		$this->ReplMgmt->update_job_msg($SERVICE_UUID,$MESSAGE,'Service');
		
		$this->create_launcher_job( $REPL_UUID, $replicaInfo[0]["TargetConnectionId"], $SERVICE_UUID );

		return;
	}
	
	###########################
	# VMWARE : InsertBootDiskToServiceDisk
	###########################
	private function InsertBootDiskToServiceDisk( $REPL_UUID, $SERVICE_UUID ){
		
		$REPL_DISK = $this -> ReplMgmt -> query_replica_disk($REPL_UUID);
		
		for ($i=0; $i<count($REPL_DISK); $i++)
		{			
			if($REPL_DISK[$i]['SYSTEM_DISK'] == TRUE)
			{
				$SCSI_ADDR = strtoupper($REPL_DISK[$i]['DISK_UUID']);
			}
			else
			{
				$SCSI_ADDR = "L:O:C:K";
			}
			
			$DISK_SIZE = $REPL_DISK[$i]['DISK_SIZE'] / 1024 / 1024 / 1024;
			$DISK_UUID = Misc_Class::guid_v4();
			$SNAP_UUID = $REPL_DISK[$i]['SNAPSHOT_MAPPING'];
			
			$this -> new_service_disk($SERVICE_UUID,$REPL_DISK[$i]["HOST_UUID"],$DISK_SIZE,$DISK_UUID,$SNAP_UUID);
			$this -> update_disk_scsi_info($DISK_UUID,$SCSI_ADDR);
		}
		
		/*$sqlClient = new Common_Model();
		
		if( isset($replicaInfo[0]["disks"]) )
			$disks = json_decode( $replicaInfo[0]["disks"], true );
		
		$replicaDisksId = explode( ',', $replicaInfo[0]["replicaDisksId"] );
		
		foreach( $replicaDisksId as $diskId ){
			$key = array_search( $diskId, array_column($disks, 'id'));
			
			if($key == false)
				return false;
			
			if( $disks[$key]["is_boot"] != true )
				$scsiAddr = "L:O:C:K";
			else
				$scsiAddr = $disk["id"];
			
			$SNAP_MAPS = $this->ReplMgmt->get_snapshot_mapping( $REPL_UUID, $disk["id"] );
		
			$sqlClient->insertServiceDisk( $disk["id"], $SERVICE_UUID, $replicaInfo[0]["PackId"], $disk["size"]/ 1024,
				$scsiAddr, $disk["id"], $SNAP_MAPS );
		}*/

	}
	
	###########################
	#PRE CREATE IMAGE EXPORT LAUNCHER JOB
	###########################
	public function pre_create_image_export_launcher_job($ACCT_UUID,$REGN_UUID,$REPL_UUID,$CONN_UUID,$TRIGGER_SYNC,$CONVERT_TYPE,$SERV_UUID)
	{
		#QUERY REPLICA INFORMATION
		$REPL_QUERY   = $this -> ReplMgmt -> list_replica(null,$REPL_UUID);
		$PACK_UUID    = $REPL_QUERY['PACK_UUID'];
		$CLUSTER_UUID = $REPL_QUERY['CLUSTER_UUID'];
		$WINPE_JOB    = $REPL_QUERY['WINPE_JOB'];
		
		#SAVE NEW SERVICE JOB
		$SNAP_JSON    = '00000000-IMAGE-00-EXPORT-00000000000';
		
		#GET OS TYPE FOR MICROSOFT OR LINUX
		$PACKER_INFO = $this -> ServerMgmt -> query_host_info($PACK_UUID);
		if (isset($PACKER_INFO['HOST_INFO']['os_name']))
		{
			$OS_NAME = $PACKER_INFO['HOST_INFO']['os_name'];
		}
		else
		{
			$OS_NAME = $PACKER_INFO['HOST_INFO']['guest_os_name'];
		}		
		
		#GETTING DIFFERENT TYPE OF ADDRESS
		if (strpos($OS_NAME, 'Microsoft') !== false)
		{ 
			$OS_TYPE = 'MS';
		}
		else
		{
			$OS_TYPE = 'LX';
		}
				
		#GET CONNECTION INFORMATION
		$CONN_INFO = $this -> ServerMgmt -> query_connection_info($CONN_UUID);
		$LAUNCHER_UUID = $CONN_INFO['LAUN_UUID'];
		
		#DIRECT/IN-DIRECT ADDRESS
		if ($CONN_INFO['LAUN_DIRECT'] == TRUE)
		{		
			$LAUNCHER_ADDR = $CONN_INFO['LAUN_ADDR'];
		}
		else
		{
			$LAUNCHER_ADDR = array_flip($CONN_INFO['LAUN_ID']);
		}

		#RECOVERY TYPE
		$RECY_TYPE = 'EXPORT_IMAGE';
		
		#SERVICE SETTINGS
		$SERVICE_SETTINGS = json_decode(
								json_encode(
									array(
										'winpe_job'			 => $WINPE_JOB,
										'flavor_id' 		 => '00000000-IMAGE-00-EXPORT-00000000000',
										'network_uuid' 		 => '00000000-IMAGE-00-EXPORT-00000000000',
										'sgroup_uuid' 		 => '00000000-IMAGE-00-EXPORT-00000000000',
										'os_type'			 => $OS_TYPE,
										'is_azure_mgmt_disk' => false,
										'convert_type'  	 => $CONVERT_TYPE
							)));
		
		#CREATE SERVICE JOB
		$SERVICE_UUID = $this -> create_service_job($ACCT_UUID,$REGN_UUID,$REPL_UUID,$CLUSTER_UUID,$SERV_UUID,$RECY_TYPE,$SNAP_JSON,$PACK_UUID,$CONN_UUID,$SERVICE_SETTINGS);
	
		#CHECK LINUX SERVICES
		if ($OS_TYPE == 'LX')
		{
			$CHECK_LX_SERVICE = $this -> check_service('LINUX_LAUNCHER_SERVICE',$LAUNCHER_ADDR);
			if ($CHECK_LX_SERVICE == FALSE)
			{
				$MESSAGE = $this -> ReplMgmt -> job_msg('Linux Launcher service cannot be found.');
				$this -> ReplMgmt -> update_job_msg($SERVICE_UUID,$MESSAGE,'Service');
				return false;
			}
			
			$MEMORY_REQUIREMENT = $this -> check_memory_requirement($LAUNCHER_ADDR);
			if ($MEMORY_REQUIREMENT == FALSE)
			{
				$MESSAGE = $this -> ReplMgmt -> job_msg('Linux Launcher service required at least 2GB of memory.');
				$this -> ReplMgmt -> update_job_msg($SERVICE_UUID,$MESSAGE,'Service');
				return false;
			}
		}	
	
		#TRIGGER RESYNC
		$this -> trigger_resync($REPL_UUID,$SERVICE_UUID,$TRIGGER_SYNC);
		
		#QUERY REPLICA DISK AND SNAPSHOT INFORMATION
		$MESSAGE = $this -> ReplMgmt -> job_msg('Getting replica disk information.');
		$this -> ReplMgmt -> update_job_msg($SERVICE_UUID,$MESSAGE,'Service');
		
		#NEW SERVICE DISK
		$REPL_DISK = $this -> ReplMgmt -> query_replica_disk($REPL_UUID);
		for ($i=0; $i<count($REPL_DISK); $i++)
		{			
			$SCSI_ADDR = $REPL_DISK[$i]['SCSI_ADDR'];
			$DISK_SIZE = $REPL_DISK[$i]['DISK_SIZE'] / 1024 / 1024 / 1024;
			$DISK_UUID = Misc_Class::guid_v4();
			$SNAP_UUID = $REPL_DISK[$i]['SNAPSHOT_MAPPING'];
			
			$this -> new_service_disk($SERVICE_UUID,$PACK_UUID,$DISK_SIZE,$DISK_UUID,$SNAP_UUID);
			$this -> update_disk_scsi_info($DISK_UUID,$SCSI_ADDR);
		}
		
		#QUERY SERVICE JOB INFORMATION
		$SERV_INFO = $this -> query_service($SERVICE_UUID);
		$SERV_JOB_INFO = json_decode($SERV_INFO['JOBS_JSON'],false);
		
		#DEFAULT BOOT DISK ID SET TO 0 FOR MOST OF CASE
		$BOOT_DISK_ID = 0;
		for ($x=0; $x<count($REPL_DISK); $x++)
		{
			if ($REPL_DISK[$x]['IS_BOOT'] == true)
			{
				$BOOT_DISK_ID = $x;
				break;
			}
		}		
		$SERV_JOB_INFO -> boot_disk_id = $BOOT_DISK_ID;
		
		#DEFAULT SYSTEM DISK ID
		$SYSTEM_DISK_ID = array(0);
		for ($x=0; $x<count($REPL_DISK); $x++)
		{
			if ($REPL_DISK[$x]['SYSTEM_DISK'] == true)
			{
				$SYSTEM_DISK_ID[] = $x;
			}
		}
		#GIVE DEFAULT EMPTY INDEX ID
		if (empty($SYSTEM_DISK_ID)){$SYSTEM_DISK_ID = array(0);};
		
		$SERV_JOB_INFO -> system_disk_id = json_encode($SYSTEM_DISK_ID);

		#UPDATE SERVICE JOB INFORMATION		
		$this -> update_trigger_info($SERVICE_UUID,$SERV_JOB_INFO,'SERVICE');		
		
		#LAST VOLUME ACTION MESSAGE
		$MESSAGE = $this -> ReplMgmt -> job_msg('The volume is ready for the Launcher.');
		$this -> ReplMgmt -> update_job_msg($SERVICE_UUID,$MESSAGE,'Service');
		
		$MESSAGE = $this -> ReplMgmt -> job_msg('Submitted recovery process.');
		$this -> ReplMgmt -> update_job_msg($SERVICE_UUID,$MESSAGE,'Service');
		
		sleep(30);			
		$this -> create_launcher_job($REPL_UUID,$CONN_UUID,$SERVICE_UUID);		
	}
	
	
	###########################
	#RUN NONE BOOTABLE DATAMODE RECOVERY
	###########################
	public function datamode_recovery($REPL_UUID,$SERVICE_UUID,$SERVICE_SETTINGS,$RECY_TYPE)
	{
		#QUERY REPLICA INFORMATION
		$REPL_QUERY = $this -> ReplMgmt -> list_replica(null,$REPL_UUID);
		$PACK_UUID = $REPL_QUERY['PACK_UUID'];
		$CONN_UUID = json_decode($REPL_QUERY['CONN_UUID'],false) -> TARGET;
		$HOST_NAME = $REPL_QUERY['HOST_NAME'];
		$JOBS_INFO = $REPL_QUERY['JOBS_JSON'];
		
		$IS_AZURE_MGMT_DISK = $JOBS_INFO -> is_azure_mgmt_disk;
		
		$CONN_INFO 					= $this -> ServerMgmt -> query_connection_info($CONN_UUID);
		$TARGET_TRANSPORT_SERV_INFO = $CONN_INFO['LAUN_OPEN'];
		$CLUSTER_UUID				= $CONN_INFO['CLUSTER_UUID'];
		
		#QUERY SERVICE INFORMATION
		$SERV_QUERY  = $this -> query_service($SERVICE_UUID);
		$SERV_JOB_INFO = json_decode($SERV_QUERY['JOBS_JSON'],false);

		#DETECTION TARGET TRANSPORT INFORMATION
		$TARGET_TRANSPORT_SERV_INFO = explode('|',$TARGET_TRANSPORT_SERV_INFO);
		if (isset($TARGET_TRANSPORT_SERV_INFO[1]))
		{
			$TARGET_SERVER_UUID = $TARGET_TRANSPORT_SERV_INFO[0];
			$TARGET_SERVER_ZONE = $TARGET_TRANSPORT_SERV_INFO[1];
		}
		else
		{
			$TARGET_SERVER_UUID = $TARGET_TRANSPORT_SERV_INFO[0];
			$TARGET_SERVER_ZONE = 'Neptune';
		}
		
		#DATAMODE INSTANCE
		$DATAMODE_INSTANCE = $SERVICE_SETTINGS -> datamode_instance;
		
		if ($JOBS_INFO -> is_azure_blob_mode == TRUE)
		{
			$CONN_INFO['CLOUD_TYPE'] = 'AzureBlob';
		}
		
		switch ($RECY_TYPE)
		{
			case "RECOVERY_PM":
				#OVER WRITE TRIGGER SYNC FLAG
				if (isset($JOBS_INFO -> migration_executed) AND $JOBS_INFO -> migration_executed == TRUE)
				{
					$TRIGGER_SYNC = false;
				}
				
				#DISABLE SYNC BUTTON
				$JOBS_INFO -> migration_executed = true;
				$this -> update_trigger_info($REPL_UUID,$JOBS_INFO,'REPLICA');
		
				#TRIGGER RESYNC
				$this -> trigger_resync($REPL_UUID,$SERVICE_UUID,$TRIGGER_SYNC);
				
				#QUERY REPLICA DISK INFORMATION
				$MESSAGE = $this -> ReplMgmt -> job_msg('Getting replica disk information.');
				$this -> ReplMgmt -> update_job_msg($SERVICE_UUID,$MESSAGE,'Service');
				
				#GET RECOVER DISK INFORMATION
				$SELECT_DISK = array_values(array_diff(explode(',',$SERVICE_SETTINGS -> snap_uuid),array('false')));
				$RECOVER_DISK = $this -> ReplMgmt -> query_replica_disk($REPL_UUID);
		
				#FILTER OUT RECOVERY DISK
				foreach ($SELECT_DISK as $PM_FILTER_UUID)
				{
					for ($f=0; $f<count($RECOVER_DISK); $f++)
					{
						if ($RECOVER_DISK[$f]['OPEN_DISK'] == $PM_FILTER_UUID)
						{
							unset($RECOVER_DISK[$f]);			
						}
						
					}
					$RECOVER_DISK = array_values($RECOVER_DISK); #RE-INDEX
				}
				
				$DETACH_DISK_ARRAY = array();
				
				for ($DISK = 0; $DISK<count($RECOVER_DISK); $DISK++)
				{
					$VOLUME_UUID = $RECOVER_DISK[$DISK]['OPEN_DISK'];
					$DISK_SIZE   = $RECOVER_DISK[$DISK]['DISK_SIZE'] / 1024 / 1024 / 1024;
					if (is_float($DISK_SIZE))
					{
						$DISK_SIZE = ceil($DISK_SIZE);	
					}
					else
					{	
						$DISK_SIZE = $DISK_SIZE + 1;
					}
					
					#NEW SERVICE DISK
					$SNAP_UUID = 'Planned_Migration';				
					$this -> new_service_disk($SERVICE_UUID,$PACK_UUID,$DISK_SIZE,$VOLUME_UUID,$SNAP_UUID);

					#UPDATE SCSI ADDRESS INFORMATION
					$this -> update_disk_scsi_info($VOLUME_UUID,'DataMode:Recovery');
					
					#############################					
					# DETACH VOLUME FROM SERVER
					#############################
					$MESSAGE = $this -> ReplMgmt -> job_msg('Detaching volume %1% from transport server.',array($DISK));
					$this -> ReplMgmt -> update_job_msg($SERVICE_UUID,$MESSAGE,'Service');
						
					switch ($CONN_INFO["CLOUD_TYPE"])
					{
						case 'OPENSTACK':
							$this -> OpenStackMgmt -> detach_volume_from_server($CLUSTER_UUID,$TARGET_SERVER_UUID,$VOLUME_UUID);
						break;

						case 'AWS':
							$this -> AwsMgmt -> detach_volume($CLUSTER_UUID,$TARGET_SERVER_ZONE,$VOLUME_UUID);
						break;

						case 'Azure':
							array_push($DETACH_DISK_ARRAY,$VOLUME_UUID);
							#TO BE CHECK
						break;
							
						case 'AzureBlob':
							#NOT AVAILABLE
						break;

						case 'Aliyun':
							$this -> AliMgmt -> detach_volume($CLUSTER_UUID,$TARGET_SERVER_ZONE,$TARGET_SERVER_UUID,$VOLUME_UUID);
							#TO BE CHECK
						break;
						
						case 'Tencent':
							#TO BE CHECK
						break;
							
						case 'Ctyun':
							$this -> CtyunMgmt -> detach_volume($CLUSTER_UUID,$VOLUME_UUID);
						break;
					}	
					
					if ($CONN_INFO["CLOUD_TYPE"] == 'Azure') 
					{
						$this -> AzureMgmt -> getVMByServUUID($SERVICE_SETTINGS -> serverId,$CLUSTER_UUID);
						if( $IS_AZURE_MGMT_DISK )			
							$this -> AzureMgmt -> detach_volume($CLUSTER_UUID,$TARGET_SERVER_ZONE,$TARGET_SERVER_UUID,$DETACH_DISK_ARRAY);
					}
					
					#############################
					#ATTACH VOLUME TO SERVER
					#############################
					$MESSAGE = $this -> ReplMgmt -> job_msg('Attaching volume %1% to selected instance.',array($DISK));
					$this -> ReplMgmt -> update_job_msg($SERVICE_UUID,$MESSAGE,'Service');
					
					switch ($CONN_INFO["CLOUD_TYPE"])
					{
						case 'OPENSTACK':
							$ATTACH_VOLUME = $this -> OpenStackMgmt -> attach_volume($CLUSTER_UUID,$DATAMODE_INSTANCE,$SERVICE_UUID,$VOLUME_UUID);
						break;
							
						case 'AWS':
							$ATTACH_VOLUME = $this -> AwsMgmt -> attach_volume($CLUSTER_UUID,$TARGET_SERVER_ZONE,$DATAMODE_INSTANCE,$VOLUME_UUID);
						break;
							
						case 'Azure':
							#TO BE DONE
							try{
								$this -> AzureMgmt -> getVMByServUUID($SERVICE_SETTINGS -> serverId,$CLUSTER_UUID);
								$ATTACH_VOLUME = $this -> AzureMgmt -> attach_volume($CLUSTER_UUID,$TARGET_SERVER_ZONE,$DATAMODE_INSTANCE,$VOLUME_UUID);
								$this->AzureMgmt->PowerControlVM($DATAMODE_INSTANCE, true);
							}
							catch (Exception $e) {
								$ATTACH_VOLUME = FALSE;
							}
							
						break;
						
						case 'AzureBlob':
							#NOT AVAILABLE
						break;
							
						case 'Aliyun':
							$ATTACH_VOLUME = $this -> AliMgmt -> attach_volume($CLUSTER_UUID,$TARGET_SERVER_ZONE,$DATAMODE_INSTANCE,$VOLUME_UUID);
							sleep(20);
							$r = $this->AliMgmt->StartInstance( array("instanceId" => $DATAMODE_INSTANCE) );

						break;
						
						case 'Tencent':
							#NOT AVAILABLE
						break;
							
						case 'Ctyun':
							$ATTACH_VOLUME = $this -> CtyunMgmt -> attach_volume($SERVICE_UUID,$CLUSTER_UUID,$DATAMODE_INSTANCE,$VOLUME_UUID,false);
						break;						
					}
					
					if ($ATTACH_VOLUME == FALSE)
					{
						$MESSAGE = $this -> ReplMgmt -> job_msg('Cannot attach the volume %1% to selected instance.',array($DISK));
						$this -> ReplMgmt -> update_job_msg($SERVICE_UUID,$MESSAGE,'Service');
						exit;
					}
				}
				#LAST VOLUME ACTION MESSAGE
				$MESSAGE = $this -> ReplMgmt -> job_msg('Success attached the volume(s) to selected instance.');
				$this -> ReplMgmt -> update_job_msg($SERVICE_UUID,$MESSAGE,'Service');
			break;
			
			case "RECOVERY_DR":
			case "RECOVERY_DT":
				if ($RECY_TYPE == 'RECOVERY_DR')
				{
					#DISABLE SYNC BUTTON
					$JOBS_INFO -> migration_executed = true;
					$this -> update_trigger_info($REPL_UUID,$JOBS_INFO,'REPLICA');
				
					#TRIGGER RESYNC (DISABLE PREPARE WORKLOAD)
					$this -> trigger_resync($REPL_UUID,$SERVICE_UUID,false);
				}

				#GET SNAPSHOT INFORMATION
				$SNAP_INFO = $SERVICE_SETTINGS -> snap_uuid;

				$SNAP_JSON_INFO = json_decode($SNAP_INFO);
				if (json_last_error() === JSON_ERROR_NONE)
				{
					$SNAP_INFO = implode(',',$SNAP_JSON_INFO);
				}
				$SNAP_ARRAY = array_values(array_diff(explode(',',$SNAP_INFO),array('false')));
				#$SNAP_ARRAY = explode(',',$SNAP_INFO);
				
				#GET SNAPSHOT INFORMATION
				$MESSAGE = $this -> ReplMgmt -> job_msg('Getting snapshot information.');
				$this -> ReplMgmt -> update_job_msg($SERVICE_UUID,$MESSAGE,'Service');
					
				for ($DISK=0; $DISK<count($SNAP_ARRAY); $DISK++)
				{
					#GET SNAPSHOT UUID
					$SNAP_UUID = $SNAP_ARRAY[$DISK];

					#############################
					#GET SNAPSHOT SIZE INFORMATION
					#############################
					switch ($CONN_INFO["CLOUD_TYPE"])
					{
						case 'OPENSTACK':
							$DISK_SIZE = $this -> OpenStackMgmt -> get_snapshot_detail_list($CLUSTER_UUID,$SNAP_UUID) -> snapshot -> size;;
						break;
						
						case 'AWS':
							$DISK_SIZE = $this -> AwsMgmt -> describe_snapshots($CLUSTER_UUID,$TARGET_SERVER_ZONE,null,$SNAP_UUID)[0]['VolumeSize'];
						break;
						
						case 'Azure':
							#TODO
							$this -> AzureMgmt -> getVMByServUUID($SERVICE_SETTINGS -> serverId,$CLUSTER_UUID);

							$SNAPSHOT_DETAIL = $this -> AzureMgmt -> describe_snapshot_detail($CLUSTER_UUID,$TARGET_SERVER_ZONE,$SNAP_UUID);

							$DISK_SIZE = $SNAPSHOT_DETAIL['properties']['diskSizeGB'];
							
						break;
						
						case 'AzureBlob':
							$SNAPSHOT_DETAIL = $this -> AzureBlobMgmt -> get_snapshot_info($REPL_UUID,$SNAP_UUID);
							$DISK_SIZE = $SNAPSHOT_DETAIL[0]['properties']['diskSizeGB'];
							$DISK_NAME = $SNAPSHOT_DETAIL[0]['properties']['diskName'];
							
						break;
						
						case 'Aliyun':
							#TODO
							$SNAPSHOT_DETAIL = $this -> AliMgmt -> describe_snapshot_detail($CLUSTER_UUID,$TARGET_SERVER_ZONE,$SNAP_UUID);
							$DISK_SIZE = $SNAPSHOT_DETAIL[0]['size'];
						break;
						
						case 'Tencent':
							#TODO
							/*
							$SNAPSHOT_DETAIL = $this -> TencentMgmt -> describe_snapshot_detail($CLUSTER_UUID,$TARGET_SERVER_ZONE,$SNAP_UUID);
							$DISK_SIZE = $SNAPSHOT_DETAIL[0]['size'];
							*/
						break;
						
						case 'Ctyun':
							$DISK_SIZE = $this -> CtyunMgmt -> describe_snapshot($CLUSTER_UUID,$SNAP_UUID) -> backup -> size;
						break;
					}
					
					$MESSAGE = $this -> ReplMgmt -> job_msg('The snapshot size is %1%',array($DISK_SIZE.'GB'));
					$this -> ReplMgmt -> update_job_msg($SERVICE_UUID,$MESSAGE,'Service');
					
					#############################
					# CREATE DISK FROM SNAPSHOT
					#############################
					$MESSAGE = $this -> ReplMgmt -> job_msg('Create volume %1% from snapshot.',array($DISK));
					$this -> ReplMgmt -> update_job_msg($SERVICE_UUID,$MESSAGE,'Service');
						
					switch ($CONN_INFO["CLOUD_TYPE"])
					{
						case 'OPENSTACK':
							$VOLUME_UUID = $this -> OpenStackMgmt -> create_volume_by_snapshot($CLUSTER_UUID,$SERVICE_UUID,$DISK_SIZE,$HOST_NAME,$SNAP_UUID);							
						break;
						
						case 'AWS':
							$VOLUME_UUID = $this -> AwsMgmt -> create_volume_from_snapshot($CLUSTER_UUID,$TARGET_SERVER_ZONE,$SNAP_UUID,$HOST_NAME);							
						break;
						
						case 'Azure':
							try{
								$this->AzureMgmt->setReplicaId( $REPL_UUID );
								$this -> AzureMgmt -> getVMByServUUID($SERVICE_SETTINGS -> serverId,$CLUSTER_UUID);
								$VOLUME_UUID = $this -> AzureMgmt -> create_volume_from_snapshot($CLUSTER_UUID,$TARGET_SERVER_ZONE,$SNAP_UUID,$HOST_NAME.'_'.$DISK);
							}
							catch (Exception $e) {
								$VOLUME_UUID = false;
							}
							
						break;
						
						case 'AzureBlob':
							$BLOB_DIRECT = false;
							$VOLUME_UUID = $this -> AzureBlobMgmt -> create_volume_from_snapshot($CLUSTER_UUID,$SERVICE_SETTINGS -> serverId,$TARGET_SERVER_ZONE,$REPL_UUID,$IS_AZURE_MGMT_DISK,$DISK,$DISK_NAME,$DISK_SIZE,$SNAP_UUID,$BLOB_DIRECT);
						break;
						
						case 'Aliyun':
							$VOLUME_UUID = $this -> AliMgmt -> create_volume_from_snapshot($CLUSTER_UUID,$TARGET_SERVER_ZONE,$SNAP_UUID,$HOST_NAME.'_'.$DISK);
						break;
						
						case 'Tencent':
							#TODO
							/*
							$VOLUME_UUID = $this -> TencentMgmt -> create_volume_from_snapshot($CLUSTER_UUID,$TARGET_SERVER_ZONE,$SNAP_UUID,$HOST_NAME,"HDD",$DISK_SIZE);
							*/
						break;
						
						case 'Ctyun':
							$VOLUME_UUID = $this -> CtyunMgmt -> create_volume_by_snapshot($CLUSTER_UUID,$SERVICE_UUID,$SNAP_UUID);
						break;
						
					}
					
					#NEW SERVICE DISK
					$this -> new_service_disk($SERVICE_UUID,$PACK_UUID,$DISK_SIZE,$VOLUME_UUID,$SNAP_UUID);
					
					#UPDATE SCSI ADDRESS INFORMATION
					$this -> update_disk_scsi_info($VOLUME_UUID,'DataMode:Recovery');
						
					if ($VOLUME_UUID == FALSE)
					{
						$MESSAGE = $this -> ReplMgmt -> job_msg('Cannot create volume.');
						$this -> ReplMgmt -> update_job_msg($SERVICE_UUID,$MESSAGE,'Service');
						exit;
					}
					
					#############################
					#ATTACH DISK TO DATAMODE INSTANCE
					#############################
					$MESSAGE = $this -> ReplMgmt -> job_msg('Attach volume %1% to instance.',array($DISK));
					$this -> ReplMgmt -> update_job_msg($SERVICE_UUID,$MESSAGE,'Service');	
					
					switch ($CONN_INFO["CLOUD_TYPE"])
					{
						case 'OPENSTACK':
							$ATTACH_VOLUME = $this -> OpenStackMgmt -> attach_volume($CLUSTER_UUID,$DATAMODE_INSTANCE,$SERVICE_UUID,$VOLUME_UUID);
						break;
						
						case 'AWS':
							$ATTACH_VOLUME = $this -> AwsMgmt -> attach_volume($CLUSTER_UUID,$TARGET_SERVER_ZONE,$DATAMODE_INSTANCE,$VOLUME_UUID);
						break;
						
						case 'Azure':
						case 'AzureBlob':
							if($IS_AZURE_MGMT_DISK == FALSE)
							{
								$ATTACH_VOLUME = $this -> AzureBlobMgmt -> attach_volume($CLUSTER_UUID,$TARGET_SERVER_UUID,$TARGET_SERVER_ZONE,$REPL_UUID,$VOLUME_UUID,$DISK_SIZE);
							}
							
							try{

								$this -> AzureMgmt -> getVMByServUUID($SERVICE_SETTINGS -> serverId,$CLUSTER_UUID);
								$ATTACH_VOLUME = $this -> AzureMgmt -> attach_volume($CLUSTER_UUID,$TARGET_SERVER_ZONE,$DATAMODE_INSTANCE,$VOLUME_UUID);
								$this->AzureMgmt->PowerControlVM($DATAMODE_INSTANCE, true);
							}
							catch (Exception $e) {
								$ATTACH_VOLUME = FALSE;
							}							
						break;
						
						case 'Aliyun':
							$ATTACH_VOLUME = $this -> AliMgmt -> attach_volume($CLUSTER_UUID,$TARGET_SERVER_ZONE,$DATAMODE_INSTANCE,$VOLUME_UUID);
							sleep(20);
							$r = $this->AliMgmt->StartInstance( array("instanceId" => $DATAMODE_INSTANCE) );
						break;
						
						case 'Tencent':
							/*
							$ATTACH_VOLUME = $this -> TencentMgmt -> attach_volume($CLUSTER_UUID,$TARGET_SERVER_ZONE,$DATAMODE_INSTANCE,$VOLUME_UUID);
							*/
						break;
						
						case 'Ctyun':
							$ATTACH_VOLUME = $this -> CtyunMgmt -> attach_volume($SERVICE_UUID,$CLUSTER_UUID,$DATAMODE_INSTANCE,$VOLUME_UUID,false);
						break;						
					}
				
					if ($ATTACH_VOLUME == FALSE)
					{
						$MESSAGE = $this -> ReplMgmt -> job_msg('Cannot attach the volume to selected instance.');
						$this -> ReplMgmt -> update_job_msg($SERVICE_UUID,$MESSAGE,'Service');
						exit;
					}
					
					#LAST VOLUME ACTION MESSAGE
					$MESSAGE = $this -> ReplMgmt -> job_msg('The volume %1% is ready for the instance.',array($DISK));
					$this -> ReplMgmt -> update_job_msg($SERVICE_UUID,$MESSAGE,'Service');	
				}
			break;
		}
		
		#UPDATE SERVICE JOB TO TEMP LOCK
		$this -> update_service_vm_info($SERVICE_UUID,'TEMP_LOCK','LOCK_TEMP');
		
		#############################
		#POWER ON DATAMODE INSTANCE
		#############################
		if ($SERVICE_SETTINGS -> datamode_power == 'on')
		{
			switch ($CONN_INFO["CLOUD_TYPE"])
			{
				case 'OPENSTACK':
					$START_VM = $this -> OpenStackMgmt -> start_vm($CLUSTER_UUID,$DATAMODE_INSTANCE);
				break;
				
				case 'AWS':
					$START_VM = $this -> AwsMgmt -> start_vm($CLUSTER_UUID,$TARGET_SERVER_ZONE,$DATAMODE_INSTANCE);
				break;
				
				case 'Azure':
					$START_VM = $this->AzureMgmt->PowerControlVM($DATAMODE_INSTANCE, true);
				break;
				
				case 'AzureBlob':
					$START_VM = $this->AzureMgmt->PowerControlVM($DATAMODE_INSTANCE, true);
				break;
				
				case 'Aliyun':
					sleep(20);
					$START_VM = $this->AliMgmt->StartInstance( array("instanceId" => $DATAMODE_INSTANCE) );
				break;
				
				case 'Tencent':
					#NOT AVAILABLE
				break;
				
				case 'Ctyun':
					#TO BE DONE
				break;						
			}
			
			if ($START_VM == TRUE)
			{
				$MESSAGE = $this -> ReplMgmt -> job_msg('Success power on date mode instance.');
				$this -> ReplMgmt -> update_job_msg($SERVICE_UUID,$MESSAGE,'Service');
			}
		}
		
		$SERV_JOB_INFO -> job_status = 'InstanceCreated';
		$this -> update_trigger_info($SERVICE_UUID,$SERV_JOB_INFO,'SERVICE');
		
		#QUERY REPLICA DISK INFORMATION
		$MESSAGE = $this -> ReplMgmt -> job_msg('Instance is ready.');
		$this -> ReplMgmt -> update_job_msg($SERVICE_UUID,$MESSAGE,'Service');
	}
	
	
	###########################
	#CREATE LAUNCHER JOB
	###########################
	public function create_launcher_job($REPL_UUID,$CONN_UUID,$SERV_UUID)
	{		
		#QUERY SERVICE INFORMATION
		$SERVICE_INFO = $this -> query_service($SERV_UUID);
		
		#LOG LOCATION 
		$LOG_LOCATION = $SERVICE_INFO['LOG_LOCATION'];
			
		#GET SERVER INFORMATION
		$CONN_INFO = $this -> ServerMgmt -> query_connection_info($CONN_UUID);
		$ACCT_UUID = $CONN_INFO['ACCT_UUID'];
		
		#DIRECT/IN-DIRECT ADDRESS
		if ($CONN_INFO['LAUN_DIRECT'] == TRUE)
		{		
			$LAUN_ADDR = $CONN_INFO['LAUN_ADDR'];
		}
		else
		{
			$LAUN_ADDR = array_flip($CONN_INFO['LAUN_ID']);
		}
		
		$MGMT_ADDR = $CONN_INFO['MGMT_ADDR'];		

		#MGMT ADDRESS ARRAY
		$MGMT_ADDR_ARRAY = $this -> query_multiple_mgmt_address($ACCT_UUID,$REPL_UUID,$MGMT_ADDR);
		if ($MGMT_ADDR_ARRAY == FALSE)
		{
			$MESSAGE = $this -> ReplMgmt -> job_msg('Cannot get the management server address.');
			$this -> ReplMgmt -> update_job_msg($REPL_UUID,$MESSAGE,'Service');
			return false;
		}		
	
		#GET JOB TYPE
		$JOB_TYPE = \saasame\transport\job_type::launcher_job_type;

		#MGMT COMMUNICATION PORT
		$MGMT_COMM = Misc_Class::mgmt_comm_type('launcher');
		
		#CONSTRACT JOB TIGTTER INFORMATION
		$TRIGGER_INFO = array(
								'type' 			=> $JOB_TYPE,
								'triggers' 		=> array(new saasame\transport\job_trigger(array('type'		=> 0, 
																								 'start' 	=> '',
																								 'finish' 	=> '',
																								 'interval' => 0))),
								'management_id' => $REPL_UUID,
								'mgmt_addr'		=> $MGMT_ADDR_ARRAY,
								'mgmt_port'		=> $MGMT_COMM['mgmt_port'],
								'is_ssl'		=> $MGMT_COMM['is_ssl']
							);
							
		#BEGIN TO RUN JOB
	
		$PORT_NUM = \saasame\transport\Constant::get('TRANSPORT_SERVICE_PORT');		
		
		$OpenProtocol = $this -> common_connection('127.0.0.1',$PORT_NUM);

		$ClientCall = new saasame\transport\transport_serviceClient($OpenProtocol);
				
		$TRIGGER_DETAIL = new saasame\transport\create_job_detail($TRIGGER_INFO);
		
		$SERVICE_TYPE = \saasame\transport\Constant::get('LAUNCHER_SERVICE');
	
		#FOR DEBUG
		Misc_Class::function_debug($LOG_LOCATION,__FUNCTION__,$TRIGGER_DETAIL);
	
		#UPDATE JOB STATUS
		$JOB_INFO = json_decode($SERVICE_INFO['JOBS_JSON'],false);
		$JOB_INFO -> job_status = 'JobSubmitToTransport';
		$this -> update_trigger_info($SERV_UUID,$JOB_INFO,'SERVICE');
	
		for ($i=0; $i<count($LAUN_ADDR); $i++)
		{
			try{
				$ClientCall -> create_job_ex_p($LAUN_ADDR[$i],$SERV_UUID,$TRIGGER_DETAIL,$SERVICE_TYPE);
				break;
			}
			catch (Throwable $e){
				Misc_Class::function_debug($LOG_LOCATION,__FUNCTION__,$e);
			}
			sleep(10);
		}
		return true;
	}
	
	
	###########################
	#CHECK RUNNING SERVICE JOB
	###########################
	public function check_running_service($REPL_UUID,$PLANNED_MIGRATION)
	{
		if ($PLANNED_MIGRATION == TRUE)
		{
			$RUNNING_SERVICE_CHECK = "SELECT * FROM _SERVICE WHERE _REPL_UUID = '".$REPL_UUID."' AND _SNAP_JSON = 'Planned_Migration' AND _STATUS = 'Y'";
		}
		else
		{
			$RUNNING_SERVICE_CHECK = "SELECT * FROM _SERVICE WHERE _REPL_UUID = '".$REPL_UUID."' AND _STATUS = 'Y'";
		}
		
		$QUERY = $this -> DBCON -> prepare($RUNNING_SERVICE_CHECK);
		$QUERY -> execute();
		$COUNT_ROWS = $QUERY -> rowCount();
		
		if ($COUNT_ROWS == 0)
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	
	
	###########################
	#BEGIN TO DELETE REPLICA JOB
	###########################
	public function delete_replica_job($REPL_UUID,$IS_RECOVERY)
	{
		#MESSAGE
		$MESSAGE = $this -> ReplMgmt -> job_msg('Deleting replication job.');
		$this -> ReplMgmt -> update_job_msg($REPL_UUID,$MESSAGE,'Replica');
		
		$REPL_QUERY = $this -> ReplMgmt -> query_replica($REPL_UUID);
		$CLOUD_UUID   = $REPL_QUERY['CLUSTER_UUID'];
		$JOB_INFO	  = $REPL_QUERY['JOBS_JSON'];
		$WINPE_JOB    = $REPL_QUERY['WINPE_JOB'];
		$LOG_LOCATION = $REPL_QUERY['LOG_LOCATION'];
		$AZURE_BLOB   = $REPL_QUERY['JOBS_JSON'] -> is_azure_blob_mode;
		$CONN_UUID	  = $REPL_QUERY['CONN_UUID'];

		if (empty($JOB_INFO -> skip_delete_conn_check))
		{
			if ($CONN_UUID == false)
			{
				for ($i=0; $i<5; $i++)
				{
					$REPL_QUERY = $this -> ReplMgmt -> query_replica($REPL_UUID);
					if ($REPL_QUERY['CONN_UUID'] != false)
					{
						$CONN_UUID = $REPL_QUERY['CONN_UUID'];
						break;
					}
				}
			}
		
			#CHECK RUNNING SERVICE
			$IS_RUNNING = $this -> check_running_service($REPL_UUID,false);
			if ($IS_RUNNING == FALSE)
			{
				$MESSAGE = $this -> ReplMgmt -> job_msg('Cannot remove the replication process due to the active recovery process.');
				$this -> ReplMgmt -> update_job_msg($REPL_UUID,$MESSAGE,'Replica');
				return false;
			}

			#MARK DELETE
			$JOB_INFO -> delete_time = time();
			$JOB_INFO -> mark_delete = true;
			$this -> update_trigger_info($REPL_UUID,$JOB_INFO,'REPLICA');
			
			
			if ($CONN_UUID != FALSE)
			{
				if($JOB_INFO -> init_carrier == TRUE)
				{			
					#GET JOB STATUS
					$MESSAGE = $this -> ReplMgmt -> job_msg('Get the Scheduler job status.');
					$this -> ReplMgmt -> update_job_msg($REPL_UUID,$MESSAGE,'Replica');
					$JOB_STATUS = $this -> get_service_job_status($REPL_UUID,'SCHEDULER');
					
					if ($JOB_STATUS == TRUE)
					{
						#STOP SCHEDULER JOB
						$MESSAGE = $this -> ReplMgmt -> job_msg('Stop the Scheduler job.');
						$this -> ReplMgmt -> update_job_msg($REPL_UUID,$MESSAGE,'Replica');
						$this -> interrupt_service_job($REPL_UUID,'SCHEDULER');
								
						#REMOVE SCHEDULER JOB
						$MESSAGE = $this -> ReplMgmt -> job_msg('Remove the Scheduler job.');
						$this -> ReplMgmt -> update_job_msg($REPL_UUID,$MESSAGE,'Replica');
						$this -> remove_service_job($REPL_UUID,'SCHEDULER');
						
						#REMOVE SNAPSHOT IMAGE
						$MESSAGE = $this -> ReplMgmt -> job_msg('Remove the snapshot images.');
						$this -> ReplMgmt -> update_job_msg($REPL_UUID,$MESSAGE,'Replica');
						$remove_image = $this -> remove_snapshot_images($REPL_UUID,'Carrier');
						if ($remove_image == FALSE)
						{
							$MESSAGE = $this -> ReplMgmt -> job_msg('Cannot remove the snapshot images.');
							$this -> ReplMgmt -> update_job_msg($REPL_UUID,$MESSAGE,'Replica');
						}
					}
				}

				#REMOVE CARRIER CONNECTION
				if ($CONN_UUID != FALSE)
				{
					#$SOURCE_CONN = json_decode($CONN_UUID) -> SOURCE;
					#$this -> remove_connection($SOURCE_CONN,'CARRIER');
				}
				
				if ($WINPE_JOB == 'N')
				{
					if($JOB_INFO -> init_loader == TRUE)
					{			
						#GET JOB STATUS
						$MESSAGE = $this -> ReplMgmt -> job_msg('Get the Loader job status.');
						$this -> ReplMgmt -> update_job_msg($REPL_UUID,$MESSAGE,'Replica');
						$LOADER_JOB_STATUS = $this -> get_service_job_status($REPL_UUID,'LOADER');
				
						if ($LOADER_JOB_STATUS == TRUE)
						{
							#STOP LOADER JOB
							$MESSAGE = $this -> ReplMgmt -> job_msg('Stop the Loader job.');
							$this -> ReplMgmt -> update_job_msg($REPL_UUID,$MESSAGE,'Replica');
							$this -> interrupt_service_job($REPL_UUID,'LOADER');	
						
							#REMOVE LOADER JOB
							$MESSAGE = $this -> ReplMgmt -> job_msg('Remove the Loader job.');
							$this -> ReplMgmt -> update_job_msg($REPL_UUID,$MESSAGE,'Replica');
							$this -> remove_service_job($REPL_UUID,'LOADER');
							
							#REMOVE SNAPSHOT IMAGE
							$this -> remove_snapshot_images($REPL_UUID,'Loader');
						}
					}
					
					#REMOVE LOADER CONNECTION
					if ($CONN_UUID != FALSE)
					{
						$TARGET_CONN = json_decode($CONN_UUID) -> TARGET;
						$this -> remove_connection($TARGET_CONN,'LOADER');
					}
				
					#BEGIN TO DELETE CLOUD DISK AND SNPASHOT
					if ($IS_RECOVERY == 'N' AND $AZURE_BLOB == FALSE)
					{	
						$CONN_INFO = $this -> ServerMgmt -> query_connection_info(json_decode($CONN_UUID) -> TARGET);
						
						$SERVER_UUID = $CONN_INFO["LAUN_UUID"];
						$LOAD_INFO = explode('|',$CONN_INFO['LOAD_OPEN']);
						if (isset($LOAD_INFO[1]))
						{
							$INSTANCE_UUID = $LOAD_INFO[0];
							$INSTANCE_REGN = $LOAD_INFO[1];
						}
						else
						{
							$INSTANCE_UUID = $LOAD_INFO[0];
							$INSTANCE_REGN = 'Mars';
						}
						
						#DELETE REPLICA DISK AND RECORD
						$this -> delete_replica_disk($REPL_UUID,$CLOUD_UUID,$SERVER_UUID,$INSTANCE_UUID,$INSTANCE_REGN);
					}
				}
			}	
		}
		
		#UPDATE TO REPORTING SERVER
		#$REPORT_DATA = $this -> workload_report('delete',$REPL_UUID,'Success');
		#$REPORT_RESPONSE = Misc_Class::transport_report($REPORT_DATA);

		#UPDATE DELETE RECOVER PLAN
		$this -> ServerMgmt -> delete_recover_plan_with_replica($REPL_UUID);

		#UPDATE DELETE REPLICA
		$this -> ReplMgmt -> delete_replica($REPL_UUID);
	
		#ARCHIVE REPLICA JOB
		Misc_Class::archive_log_folder($LOG_LOCATION);
		
		exit;
	}
	
	###########################
	#BEGIN TO DELETE REPLICA DISK
	###########################
	public function delete_replica_disk($REPL_UUID,$CLOUD_UUID,$SERVER_UUID,$INSTANCE_UUID,$INSTANCE_REGN)
	{	
		#GET REPLICA INFORMATION
		$REPL_INFO = $this -> ReplMgmt -> query_replica($REPL_UUID);
		
		#GET SERVER INFORMATION
		$SERVER_INFO = $this -> ServerMgmt -> query_server_info($SERVER_UUID);
		
		#GET LOADER ADDRESS
		if ($SERVER_INFO['SERV_INFO']['direct_mode'] == TRUE)
		{
			$LOADER_ADDR = $SERVER_INFO['SERV_ADDR'];
		}
		else
		{
			$LOADER_ADDR = array($SERVER_INFO['SERV_INFO']['machine_id']);
		}		
			
		#MUTEX MSG
		$MUTEX_MSG = $REPL_UUID.'-'.$REPL_INFO['MGMT_ADDR'].'-'.__FUNCTION__;
		
		#RESET MUTEX
		$this -> disk_mutex_action($REPL_UUID,'Replica',$LOADER_ADDR,$MUTEX_MSG,'LOCK_UN','WINDOWS');

		if($REPL_INFO['JOBS_JSON'] -> mark_delete == TRUE)
		{
			#LOCK MUTEX
			$this -> disk_mutex_action($REPL_UUID,'Replica',$LOADER_ADDR,$MUTEX_MSG,'LOCK_EX','WINDOWS');
	
			#GET DISK INFORMATION
			$DISK_UUID = $this -> query_cloud_disk($REPL_UUID);			
		
			#GET CLOUD INFORMATION
			$CLOUD_INFO	= $this -> query_cloud_info($CLOUD_UUID);
		
			switch ($CLOUD_INFO["CLOUD_TYPE"])
			{
				case 'OPENSTACK':
					$this -> OpenStackMgmt -> delete_replica_job($CLOUD_UUID,$REPL_UUID,$INSTANCE_UUID,$DISK_UUID);
				break;
				
				case 'AWS':
					$this -> AwsMgmt -> delete_replica_job($CLOUD_UUID,$REPL_UUID,$INSTANCE_REGN,$DISK_UUID);
				break;
				
				case 'Azure':
					$MESSAGE = $this -> ReplMgmt -> job_msg('Removing the snapshots.');
					$this -> ReplMgmt -> update_job_msg($REPL_UUID,$MESSAGE,'Replica');
					
					$MESSAGE = $this -> ReplMgmt -> job_msg('The volume removed.');
					$this -> ReplMgmt -> update_job_msg($REPL_UUID,$MESSAGE,'Replica');
					
					$this->AzureMgmt->getVMByServUUID( $SERVER_UUID, $CLOUD_UUID );						
					$this->AzureMgmt->setReplicaId( $REPL_UUID );						
					$this -> AzureMgmt ->delete_replica_job($CLOUD_UUID,$INSTANCE_REGN,$INSTANCE_UUID,$DISK_UUID);
				break;
				
				case 'Aliyun':
					$MESSAGE = $this -> ReplMgmt -> job_msg('Removing the snapshots.');
					$this -> ReplMgmt -> update_job_msg($REPL_UUID,$MESSAGE,'Replica');
					
					$MESSAGE = $this -> ReplMgmt -> job_msg('The volume removed.');
					$this -> ReplMgmt -> update_job_msg($REPL_UUID,$MESSAGE,'Replica');
					
					$this -> AliMgmt -> delete_replica_job($CLOUD_UUID,$INSTANCE_REGN,$INSTANCE_UUID,$DISK_UUID);
				break;
				
				case 'Tencent':
					$MESSAGE = $this -> ReplMgmt -> job_msg('Removing the snapshots.');
					$this -> ReplMgmt -> update_job_msg($REPL_UUID,$MESSAGE,'Replica');
					
					$MESSAGE = $this -> ReplMgmt -> job_msg('The volume removed.');
					$this -> ReplMgmt -> update_job_msg($REPL_UUID,$MESSAGE,'Replica');
					
					$this -> TencentMgmt -> delete_replica_job($CLOUD_UUID,$INSTANCE_REGN,$INSTANCE_UUID,$DISK_UUID);
				break;
				
				case 'Ctyun':
					$MESSAGE = $this -> ReplMgmt -> job_msg('Removing the snapshots.');
					$this -> ReplMgmt -> update_job_msg($REPL_UUID,$MESSAGE,'Replica');
					
					$MESSAGE = $this -> ReplMgmt -> job_msg('The volume removed.');
					$this -> ReplMgmt -> update_job_msg($REPL_UUID,$MESSAGE,'Replica');
					
					$this -> CtyunMgmt -> delete_replica_job($CLOUD_UUID,$INSTANCE_UUID,$DISK_UUID);
				break;
				
				case 'VMWare':
					$this->VMWareMgmt->deleteReplicaMechine( $CLOUD_UUID, $SERVER_UUID, $REPL_UUID );					
				break;
			}
			
			#MUTEX UNLOCK
			$this -> disk_mutex_action($REPL_UUID,'Replica',$LOADER_ADDR,$MUTEX_MSG,'LOCK_UN','WINDOWS');
		}
		else
		{
			$this -> disk_mutex_action($REPL_UUID,'Replica',$LOADER_ADDR,$MUTEX_MSG,'LOCK_UN','WINDOWS');
		}
	}
	
	###########################
	#BEGIN TO DELETE SERVICE JOB
	###########################
	public function delete_service_job($SERV_UUID,$INSTANCE_ACTION,$DELETE_SNAPSHOT)
	{
		#MESSAGE
		$MESSAGE = $this -> ReplMgmt -> job_msg('Removing the recovery process.');
		$this -> ReplMgmt -> update_job_msg($SERV_UUID,$MESSAGE,'Service');
		
		#GET SERVICE INFOMATION
		$SERV_INFO 		= $this -> query_service($SERV_UUID);
		$REPL_UUID 		= $SERV_INFO['REPL_UUID'];
		$JOB_INFO		= json_decode($SERV_INFO['JOBS_JSON']);
		$RECOVERY_TYPE	= $JOB_INFO -> recovery_type;
		
		#MARK AS DELETE
		$JOB_INFO -> delete_time = time();
		$this -> update_trigger_info($SERV_UUID,$JOB_INFO,'SERVICE');
		
		#PUSH SERVER REPORT INFORMATION
		$REPORT_DATA = $this -> recovery_report('delete',$SERV_UUID,'Success');
		
		#UPDATE TO REPORTING SERVER
		$REPORT_RESPONSE = Misc_Class::transport_report($REPORT_DATA);
		
		#REMOVE LAUNCHER SERVICE JOB
		$this -> remove_service_job($SERV_UUID,'LAUNCHER');
		
		$CommonModel = new Common_Model();

		$ReplicaInfo = $CommonModel->getReplicatInfo( $REPL_UUID );

		########################
		# BEGIN SWITCH INSTANCE ACTION
		########################		
		switch ($INSTANCE_ACTION)
		{
			case 'DeleteInstance':
				switch ($RECOVERY_TYPE)
				{
					case 'DevelopmentTesting':
					case 'DisasterRecovery':
						#DELETE RECOVER INSTANCE
						$this -> delete_recover_instance($SERV_INFO);
					break;
			
					case 'PlannedMigration':
						#DELETE RECOVER INSTANCE AND MOUNT BACK CONVERT DISK
						$this -> delete_recover_instance($SERV_INFO);
					break;
				
					case 'RecoveryKit':
						#REMOVE RECOVERY JOB
					break;
				
					case 'ExportImage':
						#REMOVE RECOVERY JOB				
					break;					
				}				
				#REMOVE RECOVERY JOB MESSAGE
				$MESSAGE = $this -> ReplMgmt -> job_msg('The Recovery process removed.');
				$this -> ReplMgmt -> update_job_msg($SERV_UUID,$MESSAGE,'Service');
				$this -> update_service_info($SERV_UUID,'X');
			break;
			
			case 'KeepInstance':
				switch ($RECOVERY_TYPE)
				{
					case 'DevelopmentTesting':
					case 'DisasterRecovery':
						#REMOVE RECOVERY JOB
						$MESSAGE = $this -> ReplMgmt -> job_msg('The Recovery process removed.');
						$this -> ReplMgmt -> update_job_msg($SERV_UUID,$MESSAGE,'Service');
						$this -> update_service_info($SERV_UUID,'X');

						if( $ReplicaInfo[0]["connectionType"] == \saasame\transport\hv_connection_type::HV_CONNECTION_TYPE_HOST && $RECOVERY_TYPE == "DisasterRecovery"){
							$this -> delete_replica_job($REPL_UUID,'Y');
						}

					break;
			
					case 'PlannedMigration':
						$this -> keep_recover_instance($SERV_INFO,$DELETE_SNAPSHOT);
						
						#UPDATE REPLICA TASK STATUS
						$REPL_QUERY = $this -> ReplMgmt -> query_replica($REPL_UUID);
						$JOB_INFO 	= $REPL_QUERY['JOBS_JSON'];
						$JOB_INFO -> task_operation = 'RECOVER_MIGRATION_EXEC';
						$this -> update_trigger_info($REPL_UUID,$JOB_INFO,'REPLICA');	

						#REMOVE RECOVERY JOB
						$MESSAGE = $this -> ReplMgmt -> job_msg('The Recovery process removed.');
						$this -> ReplMgmt -> update_job_msg($SERV_UUID,$MESSAGE,'Service');
						$this -> update_service_info($SERV_UUID,'X');
						
						#UPDATE DELETE REPLICA
						$this -> delete_replica_job($REPL_UUID,'Y');
					break;
				
					case 'RecoveryKit':
						#REMOVE RECOVERY JOB
						$MESSAGE = $this -> ReplMgmt -> job_msg('The Recovery process removed.');
						$this -> ReplMgmt -> update_job_msg($SERV_UUID,$MESSAGE,'Service');
						$this -> update_service_info($SERV_UUID,'X');
						
						#GET PAIR LAUNCHER UUID
						$SERVER_UUID = $this -> get_replica_pair_launcher_uuid($REPL_UUID);						

						#REMVOE PREPARE JOB
						$this -> delete_replica_job($REPL_UUID,'Y');
						
						#DELETE RECOVERY KIT SERVER
						$this -> ServerMgmt -> delete_server($SERVER_UUID,FALSE);
					break;
				
					case 'ExportImage':
						#REMOVE RECOVERY JOB
						$MESSAGE = $this -> ReplMgmt -> job_msg('The Recovery process removed.');
						$this -> ReplMgmt -> update_job_msg($SERV_UUID,$MESSAGE,'Service');
						$this -> update_service_info($SERV_UUID,'X');
						
						#REMVOE PREPARE JOB
						$this -> delete_replica_job($REPL_UUID,'Y');
					break;
				}		
			break;
			
			default:
				#REMOVE RECOVERY JOB MESSAGE
				$MESSAGE = $this -> ReplMgmt -> job_msg('The Recovery process removed.');
				$this -> ReplMgmt -> update_job_msg($SERV_UUID,$MESSAGE,'Service');
				$this -> update_service_info($SERV_UUID,'X');
		}
	}
	
	
	###########################
	# DELETE RECOVER INSTANCE
	###########################
	private function delete_recover_instance($SERV_INFO)
	{
		#QUERY SERVICE INFORMATION
		$SERV_UUID		 	= $SERV_INFO['SERV_UUID'];
		$REPL_UUID		 	= $SERV_INFO['REPL_UUID'];
		$CONN_UUID  	 	= $SERV_INFO['CONN_UUID'];
		$CLOUD_UUID 	 	= $SERV_INFO['CLUSTER_UUID'];
		$VM_UUID    	 	= $SERV_INFO['NOVA_VM_UUID'];
		$ADMIN_PASS 	 	= $SERV_INFO['ADMIN_PASS'];
		$WINPE_JOB  	 	= $SERV_INFO['WINPE_JOB'];
		$SNAP_INFO  	 	= $SERV_INFO['SNAP_JSON'];
		$RECOVERY_TYPE	 	= json_decode($SERV_INFO['JOBS_JSON']) -> recovery_type;
		$RECOVERY_STATUS 	= json_decode($SERV_INFO['JOBS_JSON']) -> job_status;
		$IS_PROMOTE 		= json_decode($SERV_INFO['JOBS_JSON']) -> is_promote;
		$IS_AZURE_MGMT_DISK = json_decode($SERV_INFO['JOBS_JSON']) -> is_azure_mgmt_disk;
		
		$DATAMODE_INSTANCE  = json_decode($SERV_INFO['JOBS_JSON']) -> datamode_instance;
		$DATAMODE_BOOTABLE 	= json_decode($SERV_INFO['JOBS_JSON']) -> is_datamode_boot;
		
		#QUERY SERVICE DISK INFO
		$SERV_DISK = $this -> query_service_disk($SERV_UUID);
		
		#GET CLOUD INFORMATION
		$CONN_INFO = $this -> ServerMgmt -> query_connection_info($CONN_UUID);
		$LAUN_INFO = explode('|',$CONN_INFO['LAUN_OPEN']);
		if (isset($LAUN_INFO[1]))
		{
			$SERVER_UUID   = $LAUN_INFO[0];
			$SERVER_REGION = $LAUN_INFO[1];
		}
		else
		{
			$SERVER_UUID   = $LAUN_INFO[0];
			$SERVER_REGION = 'Venus';
		}

		########################
		# BEGIN TO DELETE INSTANCE
		########################
		if ($VM_UUID != '' AND $VM_UUID != 'TEMP_LOCK')
		{
			$MESSAGE = $this -> ReplMgmt -> job_msg('Start to terminate instance.');
			$this -> ReplMgmt -> update_job_msg($SERV_UUID,$MESSAGE,'Service');

			try{
				switch ($CONN_INFO["CLOUD_TYPE"])
				{
					case 'OPENSTACK':
						$this -> OpenStackMgmt -> delete_instance($CLOUD_UUID,$VM_UUID);
					break;
							
					case 'AWS':
						$this -> AwsMgmt -> terminate_instances($CLOUD_UUID,$SERVER_REGION,$VM_UUID,$DATAMODE_BOOTABLE);
					break;
							
					case 'Azure':
						$this -> AzureMgmt -> getVMByServUUID( $CONN_INFO["LAUN_UUID"], $CLOUD_UUID );
						$this -> AzureMgmt -> terminate_instances($CLOUD_UUID,$SERVER_REGION,$VM_UUID);
					break;
							
					case 'Aliyun':
						$this -> AliMgmt -> terminate_instances($CLOUD_UUID,$SERVER_REGION,$VM_UUID);
					break;

					case 'Tencent':
						$this -> TencentMgmt -> terminate_instances($CLOUD_UUID,$SERVER_REGION,$VM_UUID);
					break;
					
					case 'Ctyun':
						$this -> CtyunMgmt -> terminate_instances($CLOUD_UUID,$VM_UUID,$SERV_UUID,$DATAMODE_BOOTABLE);
					break;
					
					case 'VMWare':
						$this->VMWareMgmt->terminate_instances($CLOUD_UUID,$SERV_UUID, $RECOVERY_TYPE );
					break;
				}
				
				$MESSAGE = $this -> ReplMgmt -> job_msg('The instance terminated.');
				$this -> ReplMgmt -> update_job_msg($SERV_UUID,$MESSAGE,'Service');
			}
			catch (Throwable $e)
			{
				$MESSAGE = $this -> ReplMgmt -> job_msg('Failed to terminate instance.');
				$this -> ReplMgmt -> update_job_msg($SERV_UUID,$MESSAGE,'Service');
				return false;
			}
		}

		switch ($RECOVERY_TYPE)
		{		
			case 'DevelopmentTesting':
			case 'DisasterRecovery':		
				########################
				# BEGIN TO DELETE DISK
				########################
				
				if ($SERV_DISK != false)
				{
					for ($x=0; $x<count($SERV_DISK); $x++)
					{
						if( $CONN_INFO["CLOUD_TYPE"] != "VMWare" ){
							$MESSAGE = $this -> ReplMgmt -> job_msg('Delete volume %1% from instance.',array($x));
							$this -> ReplMgmt -> update_job_msg($SERV_UUID,$MESSAGE,'Service');
						}
						
						$VOLUME_UUID = $SERV_DISK[$x]['OPEN_DISK'];
						sleep(5);
						
						switch ($CONN_INFO["CLOUD_TYPE"])
						{
							case 'OPENSTACK':
								$this -> OpenStackMgmt -> delete_volume($SERV_UUID,$CLOUD_UUID,$VOLUME_UUID);
							break;
							
							case 'AWS':
								$this -> AwsMgmt -> delete_volume($CLOUD_UUID,$SERVER_REGION,$VOLUME_UUID);
							break;
							
							case 'Azure':
								//$this -> AzureMgmt -> getVMByServUUID($LAUNCHER_UUID,$CLOUD_UUID);
							
								/*if ($this -> AzureMgmt -> IsAzureStack($CLOUD_UUID) == TRUE)
								{
									$this -> AzureBlobMgmt -> delete_temp_vhd_disk_in_container($REPL_UUID,$VOLUME_UUID);									
								}
								else*/
								{
									if ($IS_PROMOTE == FALSE AND $IS_AZURE_MGMT_DISK == FALSE)
									{
										$this -> AzureBlobMgmt -> delete_temp_vhd_disk_in_container($REPL_UUID,$VOLUME_UUID);
									}
									else
									{									
										$this -> AzureMgmt -> delete_volume($CLOUD_UUID,$SERVER_REGION,$VOLUME_UUID);									
									}
								}									
							break;
							
							case 'Aliyun':
								$this -> AliMgmt -> delete_volume($CLOUD_UUID,$SERVER_REGION,$VOLUME_UUID);
							break;

							case 'Tencent':
								if( $x != 0 )
									$this -> TencentMgmt -> delete_volume($CLOUD_UUID,$SERVER_REGION,$VOLUME_UUID);
							break;
							
							case 'Ctyun':
								$this -> CtyunMgmt -> rollback_backup_to_volume($CLOUD_UUID,$SERV_UUID,$SERVER_UUID,$VOLUME_UUID);
							break;					
						}							

						$MESSAGE = $this -> ReplMgmt -> job_msg('The volume %1% removed.',array($x));
						$this -> ReplMgmt -> update_job_msg($SERV_UUID,$MESSAGE,'Service');
					}
				}
				
				if ($RECOVERY_TYPE == 'DevelopmentTesting' AND $CONN_INFO["CLOUD_TYPE"] == 'Ctyun')
				{
					$this -> resume_service_job($REPL_UUID,'LOADER');
					
					$JOB_INFO = new stdClass();
					$JOB_INFO -> is_paused = false;
					$this -> update_trigger_info($REPL_UUID,$JOB_INFO,'REPLICA');
					
					$MESSAGE = $this -> ReplMgmt -> job_msg('Job resumed.');
					$this -> ReplMgmt -> update_job_msg($REPL_UUID,$MESSAGE,'Replica');
				}			
			break;
			
			case 'PlannedMigration':
				if ($SERV_DISK != false)
				{
					for ($y=0; $y<count($SERV_DISK); $y++)
					{
						if( $CONN_INFO["CLOUD_TYPE"] != "VMWare" ){
							$MESSAGE = $this -> ReplMgmt -> job_msg('Reattach volume %1% to transport server.',array($y));
							$this -> ReplMgmt -> update_job_msg($SERV_UUID,$MESSAGE,'Service');
						}
						
						$VOLUME_UUID = $SERV_DISK[$y]['OPEN_DISK'];
						sleep(5);
						
						switch ($CONN_INFO["CLOUD_TYPE"])
						{
							case 'OPENSTACK':
								$this -> OpenStackMgmt -> attach_volume($CLOUD_UUID,$SERVER_UUID,$SERV_UUID,$VOLUME_UUID);
							break;
								
							case 'AWS':
								$this -> AwsMgmt -> attach_volume($CLOUD_UUID,$SERVER_REGION,$SERVER_UUID,$VOLUME_UUID);
							break;
								
							case 'Azure':
								try{
									$this -> AzureMgmt -> getVMByServUUID($SERVER_UUID,$CLOUD_UUID);
									
									if( $DATAMODE_INSTANCE != 'NoAssociatedDataModeInstance' )
										$this -> AzureMgmt -> detach_volume($CLOUD_UUID,$SERVER_REGION,$DATAMODE_INSTANCE,array($VOLUME_UUID));
									
									$this -> AzureMgmt -> attach_volume($CLOUD_UUID,$SERVER_REGION,$SERVER_UUID,$VOLUME_UUID);
								}
								catch(Exception $e){
									
								}
							break;
								
							case 'Aliyun':
									if( $DATAMODE_INSTANCE != 'NoAssociatedDataModeInstance' )
										$this -> AliMgmt -> detach_volume($CLOUD_UUID,$SERVER_REGION,$DATAMODE_INSTANCE,$VOLUME_UUID);
									
									sleep(10);
									$this -> AliMgmt -> attach_volume($CLOUD_UUID,$SERVER_REGION,$SERVER_UUID,$VOLUME_UUID);
								//$this -> AliMgmt -> attach_volume($CLOUD_UUID,$SERVER_REGION,$SERVER_UUID,$VOLUME_UUID);
							break;

							case 'Tencent':
								//$this -> TencentMgmt -> attach_volume($CLOUD_UUID,$SERVER_REGION,$SERVER_UUID,$VOLUME_UUID);
							break;
							
							case 'Ctyun':
								$this -> CtyunMgmt -> attach_volume($SERV_UUID,$CLOUD_UUID,$SERVER_UUID,$VOLUME_UUID,false);
							break;
						}
					}
				}
			break;
		}
	}	
	
	###########################
	# KEEP RECOVER INSTANCE
	###########################
	private function keep_recover_instance($SERV_INFO,$DELETE_SNAPSHOT)
	{
		#QUERY SERVICE INFORMATION
		$SERV_UUID		 = $SERV_INFO['SERV_UUID'];
		$REPL_UUID		 = $SERV_INFO['REPL_UUID'];
		$CONN_UUID  	 = $SERV_INFO['CONN_UUID'];
		$CLOUD_UUID 	 = $SERV_INFO['CLUSTER_UUID'];
		#$VM_UUID    	 = $SERV_INFO['NOVA_VM_UUID'];
		#$ADMIN_PASS 	 = $SERV_INFO['ADMIN_PASS'];
		#$WINPE_JOB  	 = $SERV_INFO['WINPE_JOB'];
		#$SNAP_INFO  	 = $SERV_INFO['SNAP_JSON'];
		#$RECOVERY_TYPE	 = json_decode($SERV_INFO['JOBS_JSON']) -> recovery_type;
		#$RECOVERY_STATUS = json_decode($SERV_INFO['JOBS_JSON']) -> job_status;
		$IS_AZURE_MGMT_DISK = json_decode($SERV_INFO['JOBS_JSON']) -> is_azure_mgmt_disk;
		
		#QUERY SERVICE DISK INFO
		$SERV_DISK = $this -> query_service_disk($SERV_UUID);
				
		#PROCESSING KEEP INSTANCE DETAIL
		if ($SERV_DISK != FALSE)
		{
			#GET CLOUD INFORMATION
			$CONN_INFO = $this -> ServerMgmt -> query_connection_info($CONN_UUID);
			$LAUN_INFO = explode('|',$CONN_INFO['LAUN_OPEN']);
			if (isset($LAUN_INFO[1]))
			{
				$SERVER_UUID   = $LAUN_INFO[0];
				$SERVER_REGION = $LAUN_INFO[1];
			}
			else
			{
				$SERVER_UUID   = $LAUN_INFO[0];
				$SERVER_REGION = 'Venus';
				}
			
			if( $CONN_INFO["CLOUD_TYPE"] == "VMWare" && $DELETE_SNAPSHOT == 'on')
				$this->VMWareMgmt->snapshot_control( $REPL_UUID, 0 );
		
			#DELETE/KEEP SNASPHOT
			for ($x=0; $x<count($SERV_DISK); $x++)
			{
				$VOLUME_UUID = $SERV_DISK[$x]['OPEN_DISK'];
				sleep(5);
					
				switch ($CONN_INFO["CLOUD_TYPE"])
				{
					case 'OPENSTACK':									
						#TODO
					break;
							
					case 'AWS':
						#TODO
					break;
							
					case 'Azure':
						#TODO
					break;
			
					case 'Aliyun':
						$this -> AliMgmt -> delete_volume($CLOUD_UUID,$SERVER_REGION,$VOLUME_UUID);
					break;

					case 'Tencent':
						$this -> TencentMgmt -> delete_volume($CLOUD_UUID,$SERVER_REGION,$VOLUME_UUID);
					break;
					
					case 'Ctyun':
						#TODO
					break;
				}
				
				if ($DELETE_SNAPSHOT == 'on')
				{
					#LIST AVAILABLE SNAPSHOTS
					switch ($CONN_INFO["CLOUD_TYPE"])
					{
						case 'OPENSTACK':									
							$LIST_SNAPSHOT = $this -> OpenStackMgmt -> list_available_snapshot($CLOUD_UUID,$VOLUME_UUID);
						break;
							
						case 'AWS':
							$LIST_SNAPSHOT = $this -> AwsMgmt -> describe_snapshots($CLOUD_UUID,$SERVER_REGION,$VOLUME_UUID);
						break;
							
						case 'Azure':
							$this -> AzureMgmt -> getVMByServUUID( $CONN_INFO["LAUN_UUID"], $CLOUD_UUID );
							$LIST_SNAPSHOT = $this -> AzureMgmt -> describe_snapshots($CLOUD_UUID,$SERVER_REGION,$VOLUME_UUID);
						break;
			
						case 'Aliyun':
							$LIST_SNAPSHOT = $this -> AliMgmt -> describe_snapshots($CLOUD_UUID,$SERVER_REGION,$VOLUME_UUID);
						break;

						case 'Tencent':
							$LIST_SNAPSHOT = $this -> TencentMgmt -> describe_snapshots($CLOUD_UUID,$SERVER_REGION,$VOLUME_UUID);
						break;
						
						case 'Ctyun':
							$LIST_SNAPSHOT = $this -> CtyunMgmt -> list_available_snapshot($CLOUD_UUID,$VOLUME_UUID);
						break;
					}
					$SNPSHOT_COUNT = count($LIST_SNAPSHOT);
			
					if ($SNPSHOT_COUNT != 0)
					{
						$MESSAGE = $this -> ReplMgmt -> job_msg('Removing the snapshots.');
						$this -> ReplMgmt -> update_job_msg($SERV_UUID,$MESSAGE,'Service');
						
						#BEGIN TO DELETE AVAILABLE SNAPSHOTS
						for ($i=0; $i<$SNPSHOT_COUNT; $i++)
						{
							switch ($CONN_INFO["CLOUD_TYPE"])
							{
								case 'OPENSTACK':
									$SNAPSHOT_ID = $LIST_SNAPSHOT[$i] -> id;												
									$this -> OpenStackMgmt -> delete_snapshot($CLOUD_UUID,$SNAPSHOT_ID);																	
								break;
									
								case 'AWS':
									$SNAPSHOT_ID = $LIST_SNAPSHOT[$i]['SnapshotId'];
									$this -> AwsMgmt -> delete_snapshot($CLOUD_UUID,$SERVER_REGION,$SNAPSHOT_ID);
								break;
									
								case 'Azure':
									$SNAPSHOT_ID = $LIST_SNAPSHOT[$i]['name'];
									
									if( $IS_AZURE_MGMT_DISK ){
										$this -> AzureMgmt -> getVMByServUUID( $CONN_INFO["LAUN_UUID"], $CLOUD_UUID );
										$this -> AzureMgmt -> delete_snapshot($CLOUD_UUID,$SERVER_REGION,$SNAPSHOT_ID);
									}
									else{
										$BLOB_CONN_STRING = $this -> AzureBlobMgmt -> get_blob_connection_string($REPL_UUID) -> ConnectionString;
										
										$this->delete_vhd_disk_snapshot($BLOB_CONN_STRING,$REPL_UUID,$VOLUME_UUID,$SNAPSHOT_ID);
									}
								break;
					
								case 'Aliyun':
									$SNAPSHOT_ID = $LIST_SNAPSHOT[$i]['id'];
									$this -> AliMgmt -> delete_snapshot($CLOUD_UUID,$SERVER_REGION,$SNAPSHOT_ID);
								break;
								
								case 'Tencent':
									$SNAPSHOT_ID = $LIST_SNAPSHOT[$i]['id'];
									$this -> TencentMgmt -> delete_snapshot($CLOUD_UUID,$SERVER_REGION,$SNAPSHOT_ID);
								break;
								
								case 'Ctyun':
									$SNAPSHOT_ID = $LIST_SNAPSHOT[$i];
									$this -> CtyunMgmt -> delete_snapshot($CLOUD_UUID,$SNAPSHOT_ID);
								break;
							}
							
							$MESSAGE = $this -> ReplMgmt -> job_msg('Snapshot %1% removed.',array($SNAPSHOT_ID));
							$this -> ReplMgmt -> update_job_msg($SERV_UUID,$MESSAGE,'Service');
								
							sleep(3);
						}
					}
				}					
			}	
		}
	}
	
	###########################
	# GET REPLICA PAIR LUNCHER UUID
	###########################
	private function get_replica_pair_launcher_uuid($REPL_UUID)
	{
		#GET REPLICA INFORMATION
		$REPL_INFO = $this -> ReplMgmt -> query_replica($REPL_UUID);
		
		#GET REPLICA CONNECTION TARGET UUID
		$TARGET_UUID = json_decode($REPL_INFO['CONN_UUID'],true)['TARGET'];
		
		#GET CONNECTION INFORMATION
		$CONN_INFO = $this -> ServerMgmt -> query_connection_info($TARGET_UUID);

		#GET LAUNCHER UUID
		$SERVER_UUID = $CONN_INFO['LAUN_UUID'];
		
		return $SERVER_UUID;
	}
	
	###########################
	# Report Global Object
	###########################
	private function global_object($REPL_UUID)
	{
		#GET REPLICA QUERY
		$REPL_QUERY = $this -> ReplMgmt -> query_replica($REPL_UUID);
		
		#CONNECTION INFORMATION
		$CONN_SOURCE = $this -> ServerMgmt -> query_connection_info(json_decode($REPL_QUERY['CONN_UUID']) -> SOURCE); 
		$CONN_TARGET = $this -> ServerMgmt -> query_connection_info(json_decode($REPL_QUERY['CONN_UUID']) -> TARGET);
		
		#CLOUD QUERY
		$CLOUD_QUERY  = $this -> query_cloud_info($REPL_QUERY['CLUSTER_UUID']);
		$CLUSTER_ADDR = $CLOUD_QUERY['CLUSTER_ADDR'];
		
		switch ($CLOUD_QUERY["CLOUD_TYPE"])
		{
			case 'OPENSTACK':
				$CLOUD_TYPE 	= 'OpenStack';
				$PROJECT_NAME 	= $CLOUD_QUERY['PROJECT_NAME'];
			break;	
			
			case 'AWS':
				$CLOUD_TYPE 	= 'AWS';
				$PROJECT_NAME 	= 'NA';
			break;
			
			case 'Azure':
				$CLOUD_TYPE 	= 'Azure';
				$PROJECT_NAME 	= 'NA';
			break;
			
			case 'Aliyun':
				$CLOUD_TYPE 	= 'Aliyun';
				$PROJECT_NAME 	= 'NA';
			break;
			
			case 'Tencent':
				$CLOUD_TYPE 	= 'Tencent';
				$PROJECT_NAME 	= 'NA';
			break;
			
			default:
				$CLOUD_TYPE 	= 'UNKNOWN';
				$PROJECT_NAME 	= 'NA';
		}
		
		#HOST INFORMATION
		$HOST_QUERY = $this -> ServerMgmt -> query_host_info($REPL_QUERY['PACK_UUID']);

		#DETERMENT HOST TYPE
		$HOST_TYPE 		= $HOST_QUERY['HOST_TYPE'];
		$MANUFACTURER 	= $HOST_QUERY['HOST_INFO']['manufacturer'];
		if ($HOST_TYPE == 'Physical')
		{
			if ($MANUFACTURER == 'Xen')
			{
				$HOST_TYPE = 'AWS';
			}
			elseif ($MANUFACTURER == 'OpenStack Foundation')
			{
				$HOST_TYPE = 'OpenStack';
			}
			elseif ($MANUFACTURER == 'Microsoft Corporation')
			{
				$HOST_TYPE = 'Azure';
			}
			elseif ($MANUFACTURER == 'Alibaba Cloud')
			{
				$HOST_TYPE = 'Aliyun';
			}
			else
			{
				$HOST_TYPE = 'Physical';
			}
			
			#MACHINE UUID
			$MACHINE_UUID = $HOST_QUERY['HOST_INFO']['client_id'];
			
			#MAC ADDRESS
			$MAC_ADDRESS = $HOST_QUERY['HOST_INFO']['network_infos'][0]['mac_address'];
		}
		elseif ($HOST_TYPE == 'Virtual')
		{
			$HOST_TYPE = 'Virtual';
			
			#MACHINE UUID
			$MACHINE_UUID = $HOST_QUERY['HOST_INFO']['uuid'];
			
			#MAC ADDRESS
			$MAC_ADDRESS = $HOST_QUERY['HOST_INFO']['network_adapters'][0]['mac_address'];
		}
		elseif ($HOST_TYPE == 'Offline')
		{
			$HOST_TYPE = 'Offline';
			
			#MACHINE UUID
			$MACHINE_UUID = $HOST_QUERY['HOST_INFO']['client_id'];
			
			#MAC ADDRESS
			$MAC_ADDRESS = $HOST_QUERY['HOST_INFO']['network_infos'][0]['mac_address'];
		}
		else
		{
			$HOST_TYPE = 'unknown host type';
			
			#MACHINE UUID
			$MACHINE_UUID = '00000UNK-NOWN-TYPE-NWON-KNU0000000000';
			
			#MAC ADDRESS
			$MAC_ADDRESS = 'SS:AA:SS:AA:MM:EE';
		}	
		
		#DETERMENT HOST OS
		if (isset($HOST_QUERY['HOST_INFO']['os_name']))
		{
			$OS_NAME = $HOST_QUERY['HOST_INFO']['os_name'];
		}
		else
		{
			$OS_NAME = $HOST_QUERY['HOST_INFO']['guest_os_name'];
		}		

		#GETTING DIFFERENT TYPE OF ADDRESS
		if (strpos($OS_NAME, 'Microsoft') !== false)
		{ 
			$OS_TYPE = 'Windows';
		}
		else
		{
			$OS_TYPE = 'Linux';
		}
			
		$REPORT_GLOBAL_INFO = array(
								'mgmt_id' 							=> $REPL_QUERY['REGN_UUID'],
								'user_id' 							=> $REPL_QUERY['ACCT_UUID'],
								'reported_from_mgmt'				=> gmdate('Y-m-d G:i:s', time()),
								'Home_Transport_UUID' 				=> $CONN_SOURCE['SCHD_UUID'],
								'Home_Transport_Name' 				=> $CONN_SOURCE['SCHD_HOST'],
								'Home_Transport_IP'   				=> $CONN_SOURCE['SCHD_ADDR'],
								'Destination_Transport_UUID'		=> $CONN_TARGET['LOAD_OPEN'],
								'Destination_Transport_Name'		=> $CONN_TARGET['LOAD_HOST'],
								'Destination_Transport_IP'   		=> $CONN_TARGET['LOAD_ADDR'],
								'Destination_Virtualization_Type' 	=> $CLOUD_TYPE,
								'Destination_Virtualization_UUID' 	=> $CLOUD_QUERY['CLUSTER_UUID'],
								'Destination_Virtualization_Name' 	=> $PROJECT_NAME,
								'Destination_Virtualization_Addr'	=> $CLUSTER_ADDR,
								'job_configuration'					=> json_encode($REPL_QUERY['JOBS_JSON']),
								'Host_Name'							=> $HOST_QUERY['HOST_NAME'],
								'Host_IP'							=> $HOST_QUERY['HOST_ADDR'],
								'Host_Type'							=> $HOST_TYPE,
								'Host_OS'							=> $OS_TYPE,
								'machine_id'						=> $MACHINE_UUID,
								'mac_address'						=> $MAC_ADDRESS,
								'log_location'						=> $REPL_QUERY['LOG_LOCATION']
							);
		
		return $REPORT_GLOBAL_INFO;
	}
	
	
	###########################
	# Report Workload Action (To Be Remove)
	###########################
	public function workload_report($ACTION,$REPL_UUID,$STATUS)
	{		
		#QUERY GLOBAL OBJECT
		$GLOBAL_OBJECT = $this -> global_object($REPL_UUID);
	
		#REPLICA VOLUME INFORMATION
		$VOLUME_INFO = $this -> ReplMgmt -> query_replica_disk($REPL_UUID);
		for ($i=0; $i<count($VOLUME_INFO); $i++)
		{
			if ($ACTION == 'create')
			{
				$VolumeActionTime = $VOLUME_INFO[$i]['TIMESTAMP'];
			}
			elseif ($ACTION == 'interval')
			{
				$VolumeActionTime = $VOLUME_INFO[$i]['TIMESTAMP'];
			}
			else
			{
				$VolumeActionTime = Misc_Class::current_utc_time();
			}			
			
			
			if ($GLOBAL_OBJECT['Host_Type'] == 'Physical')
			{
				$VOLUME_NAME = json_decode($VOLUME_INFO[$i]['PACK_URI'],false) -> friendly_name;
			}
			else
			{
				$VOLUME_NAME = $VOLUME_INFO[$i]['PACK_URI'];
			}
			
			
			$VOLUME_LIST[] = array(
									'Volume_UUID'		=> $VOLUME_INFO[$i]['OPEN_DISK'],
									'Volume_Name'		=> $VOLUME_NAME,
									'Volume_Size' 		=> $VOLUME_INFO[$i]['DISK_SIZE'],	#SIZE IN BYTE
									'Volume_Time'		=> $VolumeActionTime,
									'Volume_Action'		=> $ACTION								
								);
		}
		
		#REPORT SUMMARY
		$REPORT_INFO = array(
							'mgmt_id' 	 						=> $GLOBAL_OBJECT['mgmt_id'],
							'user_id' 	 						=> $GLOBAL_OBJECT['user_id'],
							'reported_from_mgmt'				=> $GLOBAL_OBJECT['reported_from_mgmt'],
							'collection'						=> '_REPLICA',
							'job_configuration'					=> $GLOBAL_OBJECT['job_configuration'],
							'Usage_Type'						=> $ACTION,
							'Usage_Result'						=> $STATUS,
							'Workload_UUID' 			 		=> $REPL_UUID,
							'Home_Transport_UUID' 		 		=> $GLOBAL_OBJECT['Home_Transport_UUID'],
							'Home_Transport_Name' 		 		=> $GLOBAL_OBJECT['Home_Transport_Name'],
							'Home_Transport_IP'   		 		=> $GLOBAL_OBJECT['Home_Transport_IP'],
							'Destination_Transport_UUID' 		=> $GLOBAL_OBJECT['Destination_Transport_UUID'],
							'Destination_Transport_Name' 		=> $GLOBAL_OBJECT['Destination_Transport_Name'],
							'Destination_Transport_IP'   		=> $GLOBAL_OBJECT['Destination_Transport_IP'],
							'Destination_Virtualization_Type' 	=> $GLOBAL_OBJECT['Destination_Virtualization_Type'],
							'Destination_Virtualization_UUID' 	=> $GLOBAL_OBJECT['Destination_Virtualization_UUID'],
							'Destination_Virtualization_Name' 	=> $GLOBAL_OBJECT['Destination_Virtualization_Name'],
							'Host_Name'							=> $GLOBAL_OBJECT['Host_Name'],
							'Host_IP'							=> $GLOBAL_OBJECT['Host_IP'],
							'Host_Type'							=> $GLOBAL_OBJECT['Host_Type'],
							'Host_OS'							=> $GLOBAL_OBJECT['Host_OS'],							
							'Volume_List'						=> $VOLUME_LIST,
							'machine_id'						=> $GLOBAL_OBJECT['machine_id'],
							'mac_address'						=> $GLOBAL_OBJECT['mac_address'],
							'LogLocation'						=> $GLOBAL_OBJECT['log_location']
						);
		
		return $REPORT_INFO;
	}
	
	
	###########################
	# Replica Data Transfer
	###########################
	public function replica_data_transfer($ACTION,$REPL_UUID,$BACKUP_SIZE)
	{
		#QUERY GLOBAL OBJECT
		$GLOBAL_OBJECT = $this -> global_object($REPL_UUID);
		
		$REPORT_INFO = array(
							'mgmt_id' 	 						=> $GLOBAL_OBJECT['mgmt_id'],
							'user_id' 	 						=> $GLOBAL_OBJECT['user_id'],
							'reported_from_mgmt'				=> $GLOBAL_OBJECT['reported_from_mgmt'],
							'collection'						=> '_REPLICA_SNAP',
							'Usage_Type'						=> $ACTION,
							'Workload_UUID' 			 		=> $REPL_UUID,
							'DataTransferSize'					=> $BACKUP_SIZE,
							'LogLocation'						=> $GLOBAL_OBJECT['log_location']
						);
		
		return $REPORT_INFO;
	}
	
		
	###########################
	# Report Recovery Action
	###########################
	public function recovery_report($ACTION,$SERV_UUID,$STATUS)
	{
		#QUERY SERVICE INFORMATION
		$QUERY_SERVICE = $this -> query_service($SERV_UUID);

		#GET REPL UUID
		$REPL_UUID = $QUERY_SERVICE['REPL_UUID'];
		
		#QUERY GLOBAL OBJECT

		$GLOBAL_OBJECT = $this -> global_object($REPL_UUID);

		#DEFINE SERVICE DISK
		$VOLUME_INFO = $this -> query_service_disk($SERV_UUID);
		
		for ($i=0; $i<count($VOLUME_INFO); $i++)
		{
			$VOLUME_LIST[] = array(
								'Volume_UUID'		=> $VOLUME_INFO[$i]['OPEN_DISK'],
								'Volume_Name'		=> null,
								'Volume_Size' 		=> $VOLUME_INFO[$i]['DISK_SIZE'],	#SIZE IN K-BYTE
								'Volume_Time'		=> $VOLUME_INFO[$i]['TIMESTAMP'],
								'Volume_Action'		=> $ACTION);
		}
		
		#QUERY INSTANCE INFORMATION
		$CLUSTER_UUID   = $QUERY_SERVICE['CLUSTER_UUID'];
		$CONN_UUID		= $QUERY_SERVICE['CONN_UUID'];
		$INSTANCE_UUID  = $QUERY_SERVICE['NOVA_VM_UUID'];
		
		#GET INSTANCE SERVICE REGION
		$CONN_INFO = $this -> ServerMgmt -> query_connection_info($CONN_UUID);
		$SERV_INFO = explode("|",$CONN_INFO['LAUN_OPEN']);		
		if (isset($SERV_INFO[1]))
		{
			$SERVER_UUID = $SERV_INFO[0];
			$SERVER_REGION = $SERV_INFO[1];
		}
		else
		{
			$SERVER_UUID = $CONN_INFO['LAUN_OPEN'];
			$SERVER_REGION = 'Mercury';
		}
		
		switch ($GLOBAL_OBJECT['Destination_Virtualization_Type'])
		{
			case 'OPENSTACK':
				$INSTANCE_DETAIL = $this -> OpenStackMgmt -> get_instance_detail($CLUSTER_UUID,$INSTANCE_UUID);
				
				if (isset($INSTANCE_DETAIL -> server -> flavor -> id))
				{			
					$FLVOR_UUID = $INSTANCE_DETAIL -> server -> flavor -> id;
					$FLVOR_INFO = $this -> OpenStackMgmt -> get_flavor_detail_info($CLUSTER_UUID,$FLVOR_UUID);
					
					$INSTANCE_NAME = $INSTANCE_DETAIL -> server -> name;
					$INSTANCE_TYPE = $FLVOR_INFO -> flavor -> name;
					$INSTANCE_VCPU = $FLVOR_INFO -> flavor -> vcpus;
					$INSTANCE_RAM  = $FLVOR_INFO -> flavor -> ram;
				}
				else 
				{
					if ($INSTANCE_UUID == '')
					{
						$INSTANCE_UUID = 'None';
					}
					$INSTANCE_NAME = 'None';
					$INSTANCE_TYPE = 'None';
					$INSTANCE_VCPU = 'None';
					$INSTANCE_RAM  = 'None';
				}
			break;
			
			case 'AWS':
				$EC2_INFO = $this -> AwsMgmt -> describe_instance($CLUSTER_UUID,$SERVER_REGION,$INSTANCE_UUID)[0]['Instances'][0];
			
				if (isset($EC2_INFO))
				{
					$INSTANCE_NAME = $EC2_INFO['Tags'][0]['Value'];
					$INSTANCE_TYPE = $EC2_INFO['InstanceType'];
					
					$INSTANCE_DESC = $this -> AwsMgmt -> describe_instance_types($EC2_INFO['InstanceType']);
					$INSTANCE_VCPU = $INSTANCE_DESC['vCPU'];
					$INSTANCE_RAM  = $INSTANCE_DESC['Memory'];
				}
				else
				{
					if ($INSTANCE_UUID == '')
					{
						$INSTANCE_UUID = 'None';
					}
					$INSTANCE_NAME = 'None';
					$INSTANCE_TYPE = 'None';
					$INSTANCE_VCPU = 'None';
					$INSTANCE_RAM  = 'None';
				}
			break;
			
			case 'Azure':
				$this -> AzureMgmt -> getVMByServUUID( $CONN_INFO["LAUN_UUID"], $CLUSTER_UUID );
				
				if( $INSTANCE_UUID != "TEMP_LOCK" )
					$vm_info = $this -> AzureMgmt -> describe_instance($CLUSTER_UUID,$SERVER_REGION,$INSTANCE_UUID)[0]['Instances'][0];
				
				if (isset($vm_info))
				{
					$INSTANCE_NAME = $INSTANCE_UUID;
					$INSTANCE_TYPE = $vm_info['type'];
					
					$INSTANCE_DESC = json_decode( $this -> AzureMgmt -> describe_instance_types( $CLUSTER_UUID, $SERVER_REGION, $vm_info['type'] ), true );
					$INSTANCE_VCPU = $INSTANCE_DESC['numberOfCores'];
					$INSTANCE_RAM  = $INSTANCE_DESC['memoryInMB']/1024;
				}
				else
				{
					if ($INSTANCE_UUID == '')
					{
						$INSTANCE_UUID = 'None';
					}
					$INSTANCE_NAME = 'None';
					$INSTANCE_TYPE = 'None';
					$INSTANCE_VCPU = 'None';
					$INSTANCE_RAM  = 'None';
				}
			break;
			
			case 'Aliyun':
				if( $INSTANCE_UUID != "TEMP_LOCK" )
					$instance = $this -> AliMgmt -> describe_instance($CLUSTER_UUID, $this->AliMgmt->getCurrectRegion( substr($SERVER_REGION, 0, -1) ),$INSTANCE_UUID);	
								
				if (isset($instance))
				{
					$INSTANCE_NAME = $instance["InstanceName"];
					$INSTANCE_TYPE = $instance["InstanceType"];
					
					$INSTANCE_VCPU = $instance["CPU"];
					$INSTANCE_RAM  = $instance["Memory"];
				}
				else
				{
					if ($INSTANCE_UUID == '')
					{
						$INSTANCE_UUID = 'None';
					}
					$INSTANCE_NAME = 'None';
					$INSTANCE_TYPE = 'None';
					$INSTANCE_VCPU = 'None';
					$INSTANCE_RAM  = 'None';
				}
			break;
			
			case 'Tencent':
				if( $INSTANCE_UUID != "TEMP_LOCK" && $INSTANCE_UUID != '')
					$instance = $this -> TencentMgmt -> describe_instance($CLUSTER_UUID, $this->TencentMgmt->getCurrectRegion( $SERVER_REGION ),$INSTANCE_UUID);	
								
				if (isset($instance))
				{
					$INSTANCE_NAME = $instance["InstanceName"];
					$INSTANCE_TYPE = $instance["InstanceType"];
					
					$INSTANCE_VCPU = $instance["CPU"];
					$INSTANCE_RAM  = $instance["Memory"];
				}
				else
				{
					if ($INSTANCE_UUID == '')
					{
						$INSTANCE_UUID = 'None';
					}
					$INSTANCE_NAME = 'None';
					$INSTANCE_TYPE = 'None';
					$INSTANCE_VCPU = 'None';
					$INSTANCE_RAM  = 'None';
				}
			break;
		}
				
		#DEFINE INSTANCE
		$INSTANCE_INFO = array(
								'Instance_UUID' => $INSTANCE_UUID,
								'Instance_Name' => $INSTANCE_NAME,
								'Instance_Type' => $INSTANCE_TYPE,
								'Instance_vCPU' => $INSTANCE_VCPU,
								'Instance_RAM'  => $INSTANCE_RAM	
							);
		
		#DEFINE RECOVERY TYPE
		$RECOVERY_TYPE = json_decode($QUERY_SERVICE['JOBS_JSON']) -> recovery_type;
		
		#REPORT SUMMARY
		$REPORT_INFO = array(
							'mgmt_id' 	 						=> $GLOBAL_OBJECT['mgmt_id'],
							'user_id' 	 						=> $GLOBAL_OBJECT['user_id'],
							'reported_from_mgmt'				=> $GLOBAL_OBJECT['reported_from_mgmt'],
							'collection'						=> '_SERVICE',
							'Usage_Type'						=> $ACTION,
							'Usage_Result'						=> $STATUS,
							'Workload_UUID' 			 		=> $REPL_UUID,
							'Service_UUID'						=> $SERV_UUID,
							'Home_Transport_UUID' 		 		=> $GLOBAL_OBJECT['Home_Transport_UUID'],
							'Home_Transport_Name' 		 		=> $GLOBAL_OBJECT['Home_Transport_Name'],
							'Home_Transport_IP'   		 		=> $GLOBAL_OBJECT['Home_Transport_IP'],
							'Destination_Transport_UUID' 		=> $GLOBAL_OBJECT['Destination_Transport_UUID'],
							'Destination_Transport_Name' 		=> $GLOBAL_OBJECT['Destination_Transport_Name'],
							'Destination_Transport_IP'   		=> $GLOBAL_OBJECT['Destination_Transport_IP'],
							'Destination_Virtualization_Type' 	=> $GLOBAL_OBJECT['Destination_Virtualization_Type'],
							'Destination_Virtualization_UUID' 	=> $GLOBAL_OBJECT['Destination_Virtualization_UUID'],
							'Destination_Virtualization_Name' 	=> $GLOBAL_OBJECT['Destination_Virtualization_Name'],
							'Host_Name'							=> $GLOBAL_OBJECT['Host_Name'],
							'Host_IP'							=> $GLOBAL_OBJECT['Host_IP'],
							'Host_Type'							=> $GLOBAL_OBJECT['Host_Type'],
							'Host_OS'							=> $GLOBAL_OBJECT['Host_OS'],
							'Recovery_Type'						=> $RECOVERY_TYPE,
							'Recovery_Volume_List'				=> $VOLUME_LIST,
							'Recovery_Instance'					=> $INSTANCE_INFO,
							'machine_id'						=> $GLOBAL_OBJECT['machine_id'],
							'mac_address'						=> $GLOBAL_OBJECT['mac_address'],
							'LogLocation'						=> $GLOBAL_OBJECT['log_location']
						);
		
		return $REPORT_INFO;
	}
	
	###########################
	# UPDATE REPLICA JOB CONFIGURATION
	###########################
	public function update_replica_job_configuration($REPL_UUID,$JOB_JSON)
	{
		#QUERY REPLICA INFO
		$REPL_INFO = $this -> ReplMgmt -> query_replica($REPL_UUID);
		
		#LOG LOCATION
		$LOG_LOCATION = $REPL_INFO['LOG_LOCATION'];
		
		#QUERY REPLICA CONNECTION INFORMATION
		$ACCT_UUID = $REPL_INFO['ACCT_UUID'];
		if ($REPL_INFO['JOBS_JSON'] -> direct_mode == TRUE)
		{
			$CONN_UUID = json_decode($REPL_INFO['CONN_UUID'],false) -> TARGET; 
		}
		else
		{
			$CONN_UUID = json_decode($REPL_INFO['CONN_UUID'],false) -> SOURCE; 
		}
		
		$CONN_INFO = $this -> ServerMgmt -> query_connection_info($CONN_UUID);
		
		if ($CONN_INFO['SCHD_DIRECT'] == TRUE)
		{
			$SCHD_ADDR = $CONN_INFO['SCHD_ADDR'];
		}
		else
		{
			$SCHD_ADDR = array_flip($CONN_INFO['SCHD_ID']);
		}
		
		#REPLICA JOB INFO
		$REPL_JOBS_INFO	= $REPL_INFO['JOBS_JSON'];

		#CONVERT TIME TO UTC TIME
		if ($JOB_JSON['StartTime'] != '')
		{
			$USER_START_TIME = $JOB_JSON['StartTime'].' '.$JOB_JSON['TimeZone'];
			$CALC_START_TIME = new DateTime($USER_START_TIME);
			$CALC_START_TIME -> setTimezone(new DateTimeZone('UTC'));
			$SET_START_TIME = $CALC_START_TIME -> format('Y-m-d H:i:s');
		}
		else
		{
			$SET_START_TIME	= '';
		}	
		
		/*
		#READ TRIIGER TYPE
		if (isset($REPL_JOBS_INFO -> triggers -> type))
		{
			$QUERY_TRIGGER_TYPE = $REPL_JOBS_INFO -> triggers -> type;
		}
		*/		
		
		#READ FROM REPLICA JOB SETTING
		$QUERY_JOB_TYPE		= $REPL_JOBS_INFO -> type;
		$QUERY_MGMT_UUID	= $REPL_JOBS_INFO -> management_id;
		$QUERY_MGMT_ADDR	= $REPL_JOBS_INFO -> mgmt_addr;
		$QUERY_MGMT_PORT	= $REPL_JOBS_INFO -> mgmt_port;
		$QUERY_IS_SSL		= $REPL_JOBS_INFO -> is_ssl;
	
		#READ FROM USER UI SETTING
		$SET_INTERVAL_MIN	 	 	= $JOB_JSON['IntervalMinutes'];
		$SET_WORKER_THREAD	 	 	= $JOB_JSON['WorkerThreadNumber'];
		$SET_LOADER_THREAD	 	 	= $JOB_JSON['LoaderThreadNumber'];
		$SET_LOADER_TRIGGER  	 	= $JOB_JSON['LoaderTriggerPercentage'];
		$SET_EXOIRT_PATH     	 	= $JOB_JSON['ExportPath'];
		$SET_EXOIRT_TYPE     	 	= $JOB_JSON['ExportType'];
		$SET_BLOCK_MODE		 	 	= $JOB_JSON['UseBlockMode'];
		$SET_SNAP_ROTATION	 	 	= $JOB_JSON['SnapshotsNumber'];
		$SET_BUFFER_SIZE	 	 	= $JOB_JSON['BufferSize'];
		$SET_CHECKSUM_VERIFY 	 	= $JOB_JSON['ChecksumVerify'];
		$SET_SCHEDULE_PAUSE  	 	= $JOB_JSON['SchedulePause'];
		$SET_IS_COMPRESSED   	 	= $JOB_JSON['DataCompressed'];
		$SET_IS_CHECKSUM   	 	 	= $JOB_JSON['DataChecksum'];
		$SET_FILE_SYSTEM_FILTER  	= $JOB_JSON['FileSystemFilter'];
		$SET_CREATE_BY_PARTITION 	= $JOB_JSON['CreateByPartition'];
		$SET_ENABLE_CDR 		 	= $JOB_JSON['EnableCDR'];
		$SET_PRE_SNAPSHOT_SCRIPT	= $JOB_JSON['PreSnapshotScript'];
		$SET_POST_SNAPSHOT_SCRIPT	= $JOB_JSON['PostSnapshotScript'];
		$SET_EXTRA_GB		 	 	= $JOB_JSON['ExtraGB'];
		$SET_USER_TIMEZONE	 	 	= $JOB_JSON['TimeZone'];
		$SET_PACKER_COMPRESSION 	= $JOB_JSON['IsPackerDataCompressed'];
		$SET_REPLICATION_RETRY		= $JOB_JSON['ReplicationRetry'];
		$SET_PACKER_ENCRYPTION		= isset( $JOB_JSON['PackerEncryption'] )?$JOB_JSON['PackerEncryption']:null;
		$SET_EXCLUDED_PATHS			= $JOB_JSON['ExcludedPaths'];
		$SET_POST_LOADER_SCRIPT		= $JOB_JSON['PostLoaderScript'];
			
		#SET TRIGGER TYPE
		if ($SET_INTERVAL_MIN == 0)
		{
			$SET_INTERVAL = 2147483647;
		}
		else
		{
			$SET_INTERVAL = $SET_INTERVAL_MIN;
		}		
	
		#CONSTRACT JOB TIGTTER INFORMATION
		$TRIGGER_INFO = array(
								'type' 						=> $QUERY_JOB_TYPE,
								'triggers' 					=> array(new saasame\transport\job_trigger(array('type'		=> 1, 
																											 'start' 	=> $SET_START_TIME,
																											 'finish' 	=> '',
																											 'interval' => $SET_INTERVAL))),
								'management_id' 			=> $QUERY_MGMT_UUID,
								'mgmt_addr'					=> (array)$QUERY_MGMT_ADDR,
								'mgmt_port'					=> $QUERY_MGMT_PORT,
								'is_ssl'					=> $QUERY_IS_SSL
							);
							
		#BEGIN TO RUN JOB
		$PORT_NUM = \saasame\transport\Constant::get('TRANSPORT_SERVICE_PORT');		
		$OpenProtocol = $this -> common_connection('127.0.0.1',$PORT_NUM);
		$ClientCall = new saasame\transport\transport_serviceClient($OpenProtocol);

		#DECODE REPLICA TRIGGER DETAIL AND EXECUTE
		$TRIGGER_DETAIL = new saasame\transport\create_job_detail($TRIGGER_INFO);
		$SERVICE_TYPE = \saasame\transport\Constant::get('SCHEDULER_SERVICE');

		for ($i=0; $i<count($SCHD_ADDR); $i++)
		{
			try{
				$UpdateJobInfo = $ClientCall -> update_job_p($SCHD_ADDR[$i],$REPL_UUID,$TRIGGER_DETAIL,$SERVICE_TYPE);
			
				#UPDATE TRIGGER INFORMATION
				$REPL_JOBS_INFO -> triggers[0] -> type		 		= 1;
				$REPL_JOBS_INFO -> triggers[0] -> start		 		= $SET_START_TIME;
				$REPL_JOBS_INFO -> triggers[0] -> interval	 		= $SET_INTERVAL;
					
				$REPL_JOBS_INFO -> worker_thread_number 	 		= $SET_WORKER_THREAD;
				$REPL_JOBS_INFO -> block_mode_enable 		 		= $SET_BLOCK_MODE;
				$REPL_JOBS_INFO -> snapshot_rotation 		 		= $SET_SNAP_ROTATION;
				$REPL_JOBS_INFO -> loader_thread_number  	 		= $SET_LOADER_THREAD;
				$REPL_JOBS_INFO -> loader_trigger_percentage 		= $SET_LOADER_TRIGGER;
				#$REPL_JOBS_INFO -> export_path				 		= $SET_EXOIRT_PATH;
				#$REPL_JOBS_INFO -> export_type				 		= $SET_EXOIRT_TYPE;
				$REPL_JOBS_INFO -> buffer_size 				 		= $SET_BUFFER_SIZE;
				$REPL_JOBS_INFO -> checksum_verify 			 		= $SET_CHECKSUM_VERIFY;
				$REPL_JOBS_INFO -> schedule_pause			 		= $SET_SCHEDULE_PAUSE;
				$REPL_JOBS_INFO -> is_compressed			 		= $SET_IS_COMPRESSED;
				$REPL_JOBS_INFO -> is_checksum			 	 		= $SET_IS_CHECKSUM;
				$REPL_JOBS_INFO -> create_by_partition		 		= $SET_CREATE_BY_PARTITION;
				$REPL_JOBS_INFO -> pre_snapshot_script				= $SET_PRE_SNAPSHOT_SCRIPT;
				$REPL_JOBS_INFO -> post_snapshot_script				= $SET_POST_SNAPSHOT_SCRIPT;
				$REPL_JOBS_INFO -> extra_gb			 	 	 		= $SET_EXTRA_GB;
				$REPL_JOBS_INFO -> is_continuous_data_replication	= $SET_ENABLE_CDR;
				$REPL_JOBS_INFO -> is_packer_data_compressed		= $SET_PACKER_COMPRESSION;
				$REPL_JOBS_INFO -> always_retry						= $SET_REPLICATION_RETRY;
				$REPL_JOBS_INFO -> timezone							= $SET_USER_TIMEZONE;
				$REPL_JOBS_INFO -> excluded_paths					= str_replace("\\\\","\\",$SET_EXCLUDED_PATHS);
				$REPL_JOBS_INFO -> post_loader_script				= $SET_POST_LOADER_SCRIPT;
				
				Misc_Class::function_debug($LOG_LOCATION,__FUNCTION__,$REPL_JOBS_INFO);
					
				$this -> update_trigger_info($REPL_UUID,$REPL_JOBS_INFO,'REPLICA');

				$MESSAGE = $this -> ReplMgmt -> job_msg('The replication configuration updated.');
				$this -> ReplMgmt -> update_job_msg($REPL_UUID,$MESSAGE,'Replica');
				break;			
			}
			catch (Throwable $e){
				$UpdateJobInfo = false;
			}
		}
		
		if ($UpdateJobInfo == FALSE)
		{	
			$MESSAGE = $this -> ReplMgmt -> job_msg('Cannot update the replication configuration.');
			$this -> ReplMgmt -> update_job_msg($REPL_UUID,$MESSAGE,'Replica');
		}
		return $UpdateJobInfo;
	}
	
	###########################
	# TAKE XRAY
	###########################
	public function take_xray($ACCT_UUID,$XRAY_PATH,$XRAY_TYPE,$SERV_UUID)
	{
		//Misc_Class::function_debug('_mgmt',__FUNCTION__,func_get_args());
		
		#SET DEBUG FOLDER AND XRAY DOWNLOAD PATH
		$DEBUG_FOLDER = $_SERVER['DOCUMENT_ROOT'].'/_include/_debug/';
		
		#GET SAASAME INSTALL LOCATION
		$WEB_ROOT = getenv('WEBROOT');
		
		#CHECK AND CREATE PATH EXISTS
		if(!file_exists($XRAY_PATH))
		{
			mkdir($XRAY_PATH);
		}
		
		#DEFINE_COMPATIBLE_VERSION
		$COMPATIBLE_VERSION_WIN = Misc_Class::compatibility_version() -> xray_win_version;
		$COMPATIBLE_VERSION_LX = Misc_Class::compatibility_version() -> xray_lx_version;
	
		#DEFINE XARY FILE NAME
		$XRAY_FILE_NAME = 'transport_xray_'.date('Ymd_Hi_s').'.zip';
		
		#CONVERT TO ARRAY
		$XRAY_TYPE_ARRAY = explode(',',$XRAY_TYPE);
		
		#NEW PHP ZIP ARCHIVE CLASS
		$zip = new ZipArchive();
			
		#CREATE PHP ZIP ARCHIVE FILE INTO MEMORY
		$zip -> open($XRAY_PATH.'/'.$XRAY_FILE_NAME, ZipArchive::CREATE);
	
		#BEGIN XRAY FOR WINDOWS TRANSPORT SERVER
		if (in_array('server', $XRAY_TYPE_ARRAY))
		{
			if ($SERV_UUID == null)
			{			
				$LAUNCHER_LIST = $this -> ServerMgmt -> list_server_with_type($ACCT_UUID,'Launcher');
			}
			else
			{
				$LAUNCHER_INFO = $this -> ServerMgmt -> query_server_info($SERV_UUID);
				$LAUNCHER_INFO['SERV_INFO'] = (object)$LAUNCHER_INFO['SERV_INFO'];
				$LAUNCHER_LIST[] = $LAUNCHER_INFO;
			}
		
			if ($LAUNCHER_LIST != FALSE)
			{			
				#DEFINE SERVER XRAY FILE NAME
				$SERV_TIME_NAME = time();
				
				for ($i=0; $i<count($LAUNCHER_LIST); $i++)
				{
					if ($LAUNCHER_LIST[$i]['SERV_INFO'] -> direct_mode == TRUE)
					{					
						$LAUNCHER_ADDRESS = $LAUNCHER_LIST[$i]['SERV_ADDR'];
					}
					else
					{
						$LAUNCHER_ADDRESS = array($LAUNCHER_LIST[$i]['SERV_INFO'] -> machine_id);
					}
					
					$PORT_NUM = \saasame\transport\Constant::get('TRANSPORT_SERVICE_PORT');
					
					$OpenProtocol = $this -> common_connection('127.0.0.1',$PORT_NUM);
					
					$ClientCall = new saasame\transport\transport_serviceClient($OpenProtocol);
					
					for ($w=0; $w<count($LAUNCHER_ADDRESS); $w++)
					{
						try{
							$PingTheHost = $ClientCall -> ping_p($LAUNCHER_ADDRESS[$w]);
							
							if (isset($PingTheHost -> version))
							{
								$SERVER_VERSION = $PingTheHost -> version;
								if (($LAUNCHER_LIST[$i]['SYST_TYPE'] == 'WINDOWS' AND version_compare($SERVER_VERSION,$COMPATIBLE_VERSION_WIN) >= 0) OR ($LAUNCHER_LIST[$i]['SYST_TYPE'] == 'LINUX' AND version_compare($SERVER_VERSION,$COMPATIBLE_VERSION_LX) >= 0))
								{
									try{
										#GET TRANSPORT SERVER HOST INFO
										$SERV_INFO = $this -> get_host_detail($LAUNCHER_ADDRESS[$w]);
										file_put_contents($XRAY_PATH.$LAUNCHER_LIST[$i]['HOST_NAME'].'-HostInfo-'.$SERV_TIME_NAME.'.txt',print_r($SERV_INFO,TRUE));
										$SERV_ZIP_NAME[] = $LAUNCHER_LIST[$i]['HOST_NAME'].'-HostInfo-'.$SERV_TIME_NAME.'.txt';

										#BEGIN TO TAKE XRAY FROM TRANSPORT SERVICE
										$XRAY_BINARY = $ClientCall -> take_xrays_p($LAUNCHER_ADDRESS[$w]);
									
										file_put_contents($XRAY_PATH.'/'.$LAUNCHER_LIST[$i]['HOST_NAME'].'-Logs-'.$SERV_TIME_NAME.'.zip', $XRAY_BINARY);
									
										$SERV_ZIP_NAME[] = $LAUNCHER_LIST[$i]['HOST_NAME'].'-Logs-'.$SERV_TIME_NAME.'.zip';
									}
									catch (Throwable $e){
										#return $e;
									}
									#sleep(5);
									break;
								}
							}
						}						
						catch (Throwable $e){
							file_put_contents($XRAY_PATH.'/FailToConnectServer-'.$LAUNCHER_LIST[$i]['HOST_NAME'].'.txt',$LAUNCHER_ADDRESS[$w]);
							$SERV_ZIP_NAME[] = 'FailToConnectServer-'.$LAUNCHER_LIST[$i]['HOST_NAME'].'.txt';
						}
					}			
				}
				
				#ADD TRANSPORT SERVER ZIP INTO XRAY ARCHIVE
				foreach ($SERV_ZIP_NAME as $XRAY_KEY => $FILE_NAME)
				{
					$zip -> addFile($XRAY_PATH.'/'.$FILE_NAME,'_server/'.$FILE_NAME);
				}	
			}
		}

		#BEGIN TO XRAY FOR PHYSICAL PACKER
		if (in_array('host', $XRAY_TYPE_ARRAY))
		{	
			if ($SERV_UUID == null)
			{
				$PHY_HOST_LIST = $this -> ServerMgmt -> list_host_with_type($ACCT_UUID,'Physical Packer');
			}
			else
			{
				$PHY_HOST_LIST[] = $this -> ServerMgmt -> query_host_info($SERV_UUID);
				$PHY_HOST_LIST[0]['PACK_ADDR'] = $PHY_HOST_LIST[0]['HOST_ADDR'];
				$PHY_HOST_LIST[0]['SERV_MISC'] = $PHY_HOST_LIST[0]['HOST_INFO'];
				$PHY_HOST_LIST[0]['SERV_UUID'] = $PHY_HOST_LIST[0]['HOST_SERV']['SERV_UUID'];
				unset($PHY_HOST_LIST[0]['HOST_ADDR']);
				unset($PHY_HOST_LIST[0]['HOST_INFO']);
			}
			
			if ($PHY_HOST_LIST != FALSE)
			{
				#DEFINE HOST LOG FILE TIME
				$HOST_FILE_TIME = time();
			
				for ($x=0; $x<count($PHY_HOST_LIST); $x++)
				{
					if ($PHY_HOST_LIST[$x]['IS_DIRECT'] == TRUE)
					{
						$PACKER_ADDRESS = $PHY_HOST_LIST[$x]['PACK_ADDR'];
					}
					else
					{
						$PACKER_ADDRESS = $PHY_HOST_LIST[$x]['SERV_MISC']['machine_id'];
					}				
					
					$SCHEDULER_UUID = $PHY_HOST_LIST[$x]['SERV_UUID'];
					$GET_SCHEDULER_INFO = $this -> ServerMgmt -> query_server_info($SCHEDULER_UUID);
					
					if ($GET_SCHEDULER_INFO != FALSE)
					{
						if ($GET_SCHEDULER_INFO['SERV_INFO']['direct_mode'] == TRUE)
						{
							$SERV_ADDR = $GET_SCHEDULER_INFO['SERV_ADDR'];
						}
						else
						{
							$SERV_ADDR = array($GET_SCHEDULER_INFO['SERV_INFO']['machine_id']);
						}
						
						$PORT_NUM = \saasame\transport\Constant::get('TRANSPORT_SERVICE_PORT');		
						$OpenProtocol = $this -> common_connection('127.0.0.1',$PORT_NUM);
						$ClientCall = new saasame\transport\transport_serviceClient($OpenProtocol);
											
						#GET SERVER INFOMATION
						try{
							for ($h=0; $h<count($SERV_ADDR); $h++)
							{
								$PingTheHost = $ClientCall -> ping_p($SERV_ADDR[$h]);
								if (isset($PingTheHost -> version))
								{
									$SERVER_VERSION = $PingTheHost -> version;
									if (version_compare($SERVER_VERSION,$COMPATIBLE_VERSION_WIN) >= 0)
									{
										try{
											#GET HOST SOFTWARE VERSION
											$HOST_VERSION = $ClientCall -> get_packer_service_info_p($SERV_ADDR[$h],$PACKER_ADDRESS) -> version;
											$HOST_INFO = $ClientCall -> get_physical_machine_detail_p($SERV_ADDR[$h],$PACKER_ADDRESS,0);
										
											#BEGIN TO TAKE XRAY FROM TRANSPORT SERVICE
											if (version_compare($HOST_VERSION,$COMPATIBLE_VERSION_WIN) >= 0)
											{
												file_put_contents($XRAY_PATH.$PHY_HOST_LIST[$x]['HOST_NAME'].'-HostInfo-'.$HOST_FILE_TIME.'.txt',print_r($HOST_INFO,TRUE));
												$HOST_ZIP_NAME[] = $PHY_HOST_LIST[$x]['HOST_NAME'].'-HostInfo-'.$HOST_FILE_TIME.'.txt';
												
												$HOST_XRAY_BINARY = $ClientCall -> take_packer_xray_p($SERV_ADDR[$h],$PACKER_ADDRESS);
												
												if ($HOST_XRAY_BINARY != null)
												{
													file_put_contents($XRAY_PATH.'/'.$PHY_HOST_LIST[$x]['HOST_NAME'].'-Logs-'.$HOST_FILE_TIME.'.zip', $HOST_XRAY_BINARY);
													$HOST_ZIP_NAME[] = $PHY_HOST_LIST[$x]['HOST_NAME'].'-Logs-'.$HOST_FILE_TIME.'.zip';
												}
											}
										}
										catch (Throwable $e){
											file_put_contents($XRAY_PATH.'/FailToConnectHost-'.$PHY_HOST_LIST[$x]['HOST_NAME'].'.txt','WolfJumpingOverSheep');
											$HOST_ZIP_NAME[] = 'FailToConnectHost-'.$PHY_HOST_LIST[$x]['HOST_NAME'].'.txt';
											#return $e;
										}
									}
									break;
								}
							}
						}
						catch (Throwable $e){
							#return $e;
						}						
					}
				}
		
				#ADD PHYSICAL PACKER ZIP INTO XRAY ARCHIVE
				if (isset($HOST_ZIP_NAME))
				{
					foreach ($HOST_ZIP_NAME as $HOST_XRAY_KEY => $HOST_FILE_NAME)
					{
						$zip -> addFile($XRAY_PATH.'/'.$HOST_FILE_NAME,'_host/'.$HOST_FILE_NAME);
					}
				}
			}
		}			
			
		#BEGIN TO XRAY FOR MGMT DEBUG
		if (in_array('mgmt', $XRAY_TYPE_ARRAY))
		{
			#BEGIN XAY FOR LICENSE INFORMATION
			$QUERY_LICENSE = $this -> query_license();
			file_put_contents($XRAY_PATH.'/license_info.txt',print_r(json_decode(json_encode($QUERY_LICENSE),JSON_UNESCAPED_UNICODE),TRUE));
			$zip -> addFile($XRAY_PATH.'/license_info.txt','_mgmt/license_info.txt');
			
			#BEGIN XAY FOR IRM_TRANSPORT LOG
			$TRANSPORT_LOG = getenv('WEBROOT').'logs\\';	#GET TRANSPORT LOG FOLDER
			$LogZip = new ZipArchive();
			$TEMP_LOG_ZIP = $XRAY_PATH.'/'.'irm_transport.zip';
			$LogZip -> open($TEMP_LOG_ZIP, ZipArchive::CREATE);
			foreach (scandir($TRANSPORT_LOG) as $LOG_KEY => $LOG_FILE)
			{
				if (!in_array($LOG_FILE,array(".","..")) AND strpos($LOG_FILE,'irm_transporter') !== false)
				{
					$LogZip -> addFile($TRANSPORT_LOG.$LOG_FILE,$LOG_FILE);
				}
			}
			$LogZip -> close();
			$zip -> addFile($TEMP_LOG_ZIP,'_mgmt/irm_transport.zip');

			#BACKUP DATABSE
			Misc_Class::backup_database('XRAY');
			
			#GET MGMT DEBUG FILE INFORMATION
			$DEBUG_FILES = new RecursiveIteratorIterator(new RecursiveDirectoryIterator($DEBUG_FOLDER), RecursiveIteratorIterator::LEAVES_ONLY);
		
			#LOOP VIA MGMT DEBUG FILES
			foreach ($DEBUG_FILES as $DEBUG_NAME => $DEBUG_FILE)
			{
				#SKIP DIRECTORIES
				if (!$DEBUG_FILE -> isDir())
				{
					#GET REAL AND RELATIVE PATH FOR CURRENT FILE
					$DEBUG_FILE_PATH = $DEBUG_FILE -> getRealPath();
					$DEBUG_RELATIVE_PATH = substr($DEBUG_FILE_PATH, strlen($DEBUG_FOLDER));
			 
					#ADD CURRENT FILE TO XRAY ARCHIVE
					$zip -> addFile($DEBUG_FILE_PATH,$DEBUG_RELATIVE_PATH);
				}
			}
		}

		#CLOSE PHP ZIP ARCHIVE CLASS
		$zip -> close();

		
		#DELETE TEMP LICENSE_INFO.TXT
		if (file_exists($XRAY_PATH.'license_info.txt'))
		{
			unlink($XRAY_PATH.'license_info.txt');
		}
		
		#DELETE TEMP IRM_TRANSPORT ZIP
		if (file_exists($XRAY_PATH.'irm_transport.zip'))
		{
			unlink($XRAY_PATH.'irm_transport.zip');
		}
		
		#DELETE TRANSPORT SERVER ZIP FILES
		if (in_array('server', $XRAY_TYPE_ARRAY))
		{	
			if (isset($SERV_ZIP_NAME))
			{
				foreach ($SERV_ZIP_NAME as $UNLINK_KEY => $UNLINK_FILE_NAME)
				{
					unlink($XRAY_PATH.'/'.$UNLINK_FILE_NAME);
				}
			}
		}		

		#DELETE TEMP HOST ZIP FILES
		if (in_array('host', $XRAY_TYPE_ARRAY))
		{	
			if (isset($HOST_ZIP_NAME))
			{
				foreach ($HOST_ZIP_NAME as $UNLINK_HOST_KEY => $UNLINK_HOST_FILE_NAME)
				{
					unlink($XRAY_PATH.'/'.$UNLINK_HOST_FILE_NAME);
				}
			}
		}
		
		#DELETE TEMP MGMT SQL BACKUP FILE
		if (in_array('mgmt', $XRAY_TYPE_ARRAY))
		{	
			foreach (glob($DEBUG_FOLDER.'*.sql') as $XRAY_SQL)
			{
				unlink($XRAY_SQL);
			}
		}
		
		#RETURN XRAY ARCHIVE FILE NAME
		if (file_exists($XRAY_PATH.'/'.$XRAY_FILE_NAME) == TRUE)
		{
			return $XRAY_FILE_NAME;
		}
		else
		{
			return false;
		}	
	}
	
	###########################
	# QUERY SERVICE BY REPLICA
	###########################
	public function query_service_by_replica($REPL_UUID)
	{
		$GET_EXEC 	= "SELECT * FROM _SERVICE WHERE _REPL_UUID = '".strtoupper($REPL_UUID)."' AND _STATUS = 'Y'";
		
		$QUERY = $this -> DBCON -> prepare($GET_EXEC);
		$QUERY -> execute();
		$COUNT_ROWS = $QUERY -> rowCount();
		
		if ($COUNT_ROWS != 0)
		{
			foreach($QUERY as $QueryResult)
			{
				$HOST_INFO = $this -> ServerMgmt -> query_host_info($QueryResult['_PACK_UUID']);
				
				$REPL_EXPLODE = explode('-',$QueryResult['_REPL_UUID']);
				
				$SERV_DATA = array(
								"ACCT_UUID" 	=> $QueryResult['_ACCT_UUID'],
								"REGN_UUID" 	=> $QueryResult['_REGN_UUID'],
								"SERV_UUID" 	=> $QueryResult['_SERV_UUID'],
								"REPL_UUID" 	=> $QueryResult['_REPL_UUID'],
															
								"SNAP_JSON" 	=> $QueryResult['_SNAP_JSON'],
								
								"PACK_UUID" 	=> $QueryResult['_PACK_UUID'],
								"CONN_UUID" 	=> $QueryResult['_CONN_UUID'],
								"HOST_NAME"		=> $HOST_INFO['HOST_NAME'],
								"LOG_LOCATION"	=> $HOST_INFO['HOST_NAME'].'-'.end($REPL_EXPLODE),
								
								"CLUSTER_UUID" 	=> $QueryResult['_CLUSTER_UUID'],
								"FLAVOR_ID" 	=> $QueryResult['_FLAVOR_ID'],
								"NETWORK_UUID" 	=> $QueryResult['_NETWORK_UUID'],
								"SGROUP_UUID" 	=> $QueryResult['_SGROUP_UUID'],
								"NOVA_VM_UUID" 	=> $QueryResult['_NOVA_VM_UUID'],
								"ADMIN_PASS"	=> $QueryResult['_ADMIN_PASS'],

								"JOBS_JSON"		=> $QueryResult['_JOBS_JSON'],
								"WINPE_JOB"		=> json_decode($QueryResult['_JOBS_JSON'],false) -> winpe_job,
								"OS_TYPE"		=> $QueryResult['_OS_TYPE'],
								
								"TIMESTAMP"		=> $QueryResult['_TIMESTAMP'],
							);	
			}			
			return $SERV_DATA;
		}
		else
		{
			return false;
		}
	}
	
	###########################
	# CHECK TYPE X
	###########################
	public function recover_kit_type_check($SERVER_UUID,$TYPE)
	{
		$SERVER_INFO = $this -> ServerMgmt -> query_server_info($SERVER_UUID);					
		if(isset($SERVER_INFO['SERV_INFO']['recover_kit_type']) AND ($SERVER_INFO['SERV_INFO']['recover_kit_type'] == $TYPE))
		{
			return $SERVER_INFO;
		}
		else
		{
			return false;
		}
	}
	
	###########################
	# IS LICENSE VALID
	###########################
	public function is_license_valid($REPL_UUID)
	{
		$PORT_NUM = \saasame\transport\Constant::get('TRANSPORT_SERVICE_PORT');
		
		$OpenProtocol = $this -> common_connection('127.0.0.1',$PORT_NUM);
		
		$ClientCall = new saasame\transport\transport_serviceClient($OpenProtocol);

		$CheckLicense = $ClientCall -> is_license_valid($REPL_UUID);

		return $CheckLicense;		
	}	
	
	###########################
	# IS LICENSE VALID
	###########################
	public function is_license_valid_ex($SERVICE_UUID)
	{
		$PORT_NUM = \saasame\transport\Constant::get('TRANSPORT_SERVICE_PORT');
		
		$OpenProtocol = $this -> common_connection('127.0.0.1',$PORT_NUM);
		
		$ClientCall = new saasame\transport\transport_serviceClient($OpenProtocol);

		$CheckLicense = $ClientCall -> is_license_valid_ex($SERVICE_UUID,true);

		return $CheckLicense;		
	}	
	
	###########################
	# QUERY LICENSE
	###########################
	public function query_license()
	{
		$PORT_NUM = \saasame\transport\Constant::get('TRANSPORT_SERVICE_PORT');
		
		$OpenProtocol = $this -> common_connection('127.0.0.1',$PORT_NUM);
		
		$ClientCall = new saasame\transport\transport_serviceClient($OpenProtocol);

		$QueryLicense = $ClientCall -> get_licenses();

		return $QueryLicense;
	}
	
	###########################
	# GET PACKAGE INFO
	###########################
	public function get_package_info($LICENSE_NAME,$LICENSE_EMAIL,$LICENSE_KEY)
	{
		$PORT_NUM = \saasame\transport\Constant::get('TRANSPORT_SERVICE_PORT');
		
		$OpenProtocol = $this -> common_connection('127.0.0.1',$PORT_NUM);
		
		$ClientCall = new saasame\transport\transport_serviceClient($OpenProtocol);

		$PackageInfo = $ClientCall -> get_package_info($LICENSE_EMAIL,$LICENSE_NAME,$LICENSE_KEY);
		
		return $PackageInfo;
	}
	
	###########################
	# GET PACKAGE INFO
	###########################
	public function query_package_info($LICENSE_KEY)
	{
		$PORT_NUM = \saasame\transport\Constant::get('TRANSPORT_SERVICE_PORT');
		
		$OpenProtocol = $this -> common_connection('127.0.0.1',$PORT_NUM);
		
		$ClientCall = new saasame\transport\transport_serviceClient($OpenProtocol);

		$PackageInfo = $ClientCall -> query_package_info($LICENSE_KEY);
		
		return $PackageInfo;		
	}
	
	###########################
	# REMOVE LICENSE
	###########################
	public function remove_license($LICENSE_KEY)
	{
		$PORT_NUM = \saasame\transport\Constant::get('TRANSPORT_SERVICE_PORT');
		
		$OpenProtocol = $this -> common_connection('127.0.0.1',$PORT_NUM);
		
		$ClientCall = new saasame\transport\transport_serviceClient($OpenProtocol);

		$RemoveInfo = $ClientCall -> remove_license($LICENSE_KEY);
		
		return $RemoveInfo;			
	}
	
	###########################
	# ONLINE ACTIVE LICENSE
	###########################
	public function online_active_license($LICENSE_NAME,$LICENSE_EMAIL,$LICENSE_KEY)
	{
		$PORT_NUM = \saasame\transport\Constant::get('TRANSPORT_SERVICE_PORT');
		
		$OpenProtocol = $this -> common_connection('127.0.0.1',$PORT_NUM);
		
		$ClientCall = new saasame\transport\transport_serviceClient($OpenProtocol);

		try{
			$LicenseKey = $ClientCall -> active_license($LICENSE_EMAIL,$LICENSE_NAME,$LICENSE_KEY);
			
			return array('status' => $LicenseKey, 'why' => null);			
		}
		catch (Throwable $e)
		{
			return array('status' => false, 'why' => $e -> why);
		}
	}
	
	###########################
	# ONFFLINE ACTIVE LICENSE
	###########################
	public function offline_active_license($LICENSE_KEY,$LICENSE_TEXT)
	{
		$PORT_NUM = \saasame\transport\Constant::get('TRANSPORT_SERVICE_PORT');
		
		$OpenProtocol = $this -> common_connection('127.0.0.1',$PORT_NUM);
		
		$ClientCall = new saasame\transport\transport_serviceClient($OpenProtocol);

		try{
			$LicenseText = $ClientCall -> add_license_with_key($LICENSE_KEY,$LICENSE_TEXT);
			
			return array('status' => $LicenseText, 'why' => null);
		}
		catch (Throwable $e)
		{
			return array('status' => false, 'why' => $e -> why);
		}
	}
	
	###########################
	# CREATE CALLBACK TASK
	###########################
	public function create_task($PARAMETERS)
	{
		$TASK_PARAMETERS = array(
								'id' 		=> $PARAMETERS['id'],
								'triggers' 	=> array(new saasame\transport\job_trigger(array( 
																							'type'	   => 1,
																							'start'    => $PARAMETERS['utc_start_time'],
																							'finish'   => '',
																							'interval' => $PARAMETERS['task_interval'])
																					)),
								'mgmt_addr'	=> '127.0.0.1',
								'mgmt_port'	=> 80,
								'is_ssl'	=> false,
								'parameters'=> $PARAMETERS['parameters']
							);
		
		$TASK_PARAMETERS = new saasame\transport\running_task($TASK_PARAMETERS);		

		$PORT_NUM = \saasame\transport\Constant::get('TRANSPORT_SERVICE_PORT');
		
		$OpenProtocol = $this -> common_connection('127.0.0.1',$PORT_NUM);
		
		$ClientCall = new saasame\transport\transport_serviceClient($OpenProtocol);

		$CreateTask = $ClientCall -> create_task($TASK_PARAMETERS);

		return $CreateTask;
	}
	
	###########################
	# REMOVE CALLBACK TASK
	###########################
	public function remove_task($TASK_ID)
	{
		$PORT_NUM = \saasame\transport\Constant::get('TRANSPORT_SERVICE_PORT');
		
		$OpenProtocol = $this -> common_connection('127.0.0.1',$PORT_NUM);
		
		$ClientCall = new saasame\transport\transport_serviceClient($OpenProtocol);

		$CreateTask = $ClientCall -> remove_task($TASK_ID);

		return $CreateTask;
	}
	
	###########################
	# GET CLOUD TYPE BY SERVICEID
	###########################
	public function getCloudTypeByServiceId( $SERVICE_UUID )
	{
		$sql = "select Ta.*, Tc._CLOUD_TYPE as CLOUD, Tb.*, Td._HOST_NAME
				from _SERVICE as Ta
				join _CLOUD_MGMT as Tb on Ta._CLUSTER_UUID = Tb._CLUSTER_UUID
				join _SYS_CLOUD_TYPE as Tc on Tb._CLOUD_TYPE = Tc._ID
				join _SERVER_HOST as Td on Ta._PACK_UUID = Td._HOST_UUID
				where 
					Ta._SERV_UUID = :SERVICE_UUID AND 
					Ta._STATUS = 'Y'";
		
		$QUERY = $this -> DBCON -> prepare($sql);

		$QUERY -> execute( array( "SERVICE_UUID" => $SERVICE_UUID ) );

		$data = $QUERY->fetchAll();

		return $data;
	}
	
	###########################
	# QUERY SELECT HOST INFO
	###########################
	public function query_select_host_info($HOST_UUID)
	{
		$SelectHostInfo = (object)$this -> ServerMgmt -> query_host_info($HOST_UUID);
		if (isset($SelectHostInfo -> HOST_SERV['SERV_ADDR']))
		{
			if ($SelectHostInfo -> HOST_SERV['SERV_INFO']['direct_mode'] == TRUE)
			{ 			
				$TransportAddress = $SelectHostInfo -> HOST_SERV['SERV_ADDR'];
			}
			else
			{
				$TransportAddress = array($SelectHostInfo -> HOST_SERV['SERV_INFO']['machine_id']);
			}
		
			foreach ($TransportAddress as $Index => $Address)
			{
				$TransportInfo = $this -> get_host_detail($Address);
				if ($TransportInfo != FALSE)
				{
					$SelectHostInfo -> HOST_SERV['SERV_INFO'] = $TransportInfo;
					break;
				}
			}
		}		
		return $SelectHostInfo;
	}
	
	###########################
	# QUERY SELECT TRANSPORT INFO
	###########################
	public function query_select_transport_info($SERV_UUID)
	{
		$SelectTransportInfo = (object)$this -> ServerMgmt -> query_server_info($SERV_UUID);
	
		$LIST_REPLICA = $this -> ServerMgmt -> query_listing_replica_by_server_id($SelectTransportInfo -> SERV_UUID,'Launcher');

		if ($LIST_REPLICA != false)
		{
			foreach ($LIST_REPLICA as $REPL_UUID => $PACK_UUID)
			{
				$SelectTransportInfo -> LIST_REPLICA[$REPL_UUID] = $this -> ReplMgmt -> query_replica($REPL_UUID)['HOST_NAME'];
			}
		}	

		if ($SelectTransportInfo -> SERV_INFO['direct_mode'] == TRUE)
		{		
			$TransportAddress = $SelectTransportInfo -> SERV_ADDR;
		}
		else
		{
			$TransportAddress = array($SelectTransportInfo -> SERV_INFO['machine_id']);
		}

		foreach ($TransportAddress as $Index => $Address)
		{
			$TransportInfo = $this -> get_host_detail($Address);			
			if ($TransportInfo != FALSE)
			{
				$SelectTransportInfo -> SERV_INFO = $TransportInfo;				
				break;
			}
		}
		
		#CHECK DISK IN USE FOR REPLICA
		$DISK_INFO = $SelectTransportInfo -> SERV_INFO -> disk_infos;
		for($i=0; $i<count($DISK_INFO); $i++)
		{
			$DISK_ADDR = ($DISK_INFO[$i] -> serial_number != "")?$DISK_INFO[$i] -> serial_number:json_decode($DISK_INFO[$i] -> uri) -> address;
			$IS_IN_USE = $this -> ReplMgmt -> is_replica_disk_in_use($DISK_ADDR);
			$SelectTransportInfo -> SERV_INFO -> disk_infos[$i] -> in_use = $IS_IN_USE;
		}
		return $SelectTransportInfo;
	}
	
	###########################
	# QUERY PREPARE WORKLOAD INFO
	###########################
	public function query_prepare_workload($REPL_UUID)
	{
		$HOST_INFO = (object)array(
								'JobUUID' => $REPL_UUID,
								'PackerUUID' => 'processing..',
								'Hostname' => 'processing..',
								'HostType' => 'processing..',
								'manufacturer' => 'processing..',
								'OSName' => 'processing..',							
								'Address' => 'processing..',
								'Disk' => 'processing..',
								'JobConfig' => 'processing..',
								'PairTransport' => 'processing..',
								'CloudVolume' => 'processing..',
								'LogLocation' => 'processing..',
								'TransferData' => 'processing..'
						);
	
		#QUERY REPLICA INFO
		$QUERY_REPLICA = $this -> ReplMgmt -> query_replica($REPL_UUID);

		#QUERY CLOUD INFO
		$CLUSTER_UUID = $QUERY_REPLICA['CLUSTER_UUID'];
		$CLOUD_INFO	= $this -> query_cloud_info($CLUSTER_UUID);

		#QUERY PACKER INFO
		$PACKER_INFO = $this -> ServerMgmt -> query_host_info($QUERY_REPLICA['PACK_UUID']);	
		
		#GET DB HOST DISK
		$DB_HOST_DISK = $PACKER_INFO['HOST_DISK'];
			
		#GET SET SKIP DISK
		$DB_SKIP_DISK = explode(',',$QUERY_REPLICA['JOBS_JSON'] -> skip_disk);

		#GET SKIP DISK URI
		$SKIP_URI = array();
		for ($i=0; $i<count($DB_HOST_DISK); $i++)
		{
			for ($ii=0; $ii<count($DB_SKIP_DISK); $ii++)
			{
				if ($DB_HOST_DISK[$i]['DISK_UUID'] == $DB_SKIP_DISK[$ii])
				{
					$SKIP_URI[] = $DB_HOST_DISK[$i]['DISK_URI'];
					break;
				}
			}		
		}
		
		#TRANSPORT DATA
		$TRANSFER_DATA = $this -> ReplMgmt -> snapshot_transport_data($REPL_UUID);
		$HOST_INFO -> TransferData = $TRANSFER_DATA;
		
		#PACKER UUID
		$HOST_INFO -> PackerUUID = $QUERY_REPLICA['PACK_UUID'];
	
		#LOG LOCATION
		$HOST_INFO -> LogLocation = $QUERY_REPLICA['LOG_LOCATION'];
		
		#CHANGE HOSTNAME
		$HOST_INFO -> Hostname = $PACKER_INFO['HOST_NAME'];
			
		#CHANGE ADDRESS
		$HOST_INFO -> Address = $PACKER_INFO['HOST_ADDR'];
			
		#CHANGE HOST_TYPE
		$HOST_INFO -> HostType = $PACKER_INFO['HOST_TYPE'];
			
		#CHANGE OS_NAME AND OS_TYPE
		if ($PACKER_INFO['HOST_TYPE'] == 'Physical' OR $PACKER_INFO['HOST_TYPE'] == 'Offline')		
		{
			$HOST_INFO -> OSName = $PACKER_INFO['HOST_INFO']['os_name'];
			if (isset($PACKER_INFO['HOST_INFO']['manufacturer']))
			{
				$SET_MANUFACTURER = $PACKER_INFO['HOST_INFO']['manufacturer'];
			}
			else
			{
				$SET_MANUFACTURER = 'Generic Physical';
			}
			$HOST_INFO -> manufacturer = $SET_MANUFACTURER;
		}
		else
		{
			$HOST_INFO -> OSName = $PACKER_INFO['HOST_INFO']['guest_os_name'];
			if (isset($PACKER_INFO['HOST_INFO']['manufacturer']))
			{
				$SET_MANUFACTURER = $PACKER_INFO['HOST_INFO']['manufacturer'];
			}
			else
			{
				$SET_MANUFACTURER = 'VMware, Inc.';
			}			
			$HOST_INFO -> manufacturer = $SET_MANUFACTURER;
		}	
	
		#CHANGE HOST_DISK
		if ($PACKER_INFO['HOST_TYPE'] == 'Physical' OR $PACKER_INFO['HOST_TYPE'] == 'Offline')
		{
			$QUERY_HOST_DISK = $PACKER_INFO['HOST_INFO']['disk_infos'];
			
		}
		else
		{
			$QUERY_HOST_DISK = $PACKER_INFO['HOST_INFO']['disks'];
		}
		
		for ($x=0; $x<count($QUERY_HOST_DISK); $x++)
		{
			$IS_BOOT = false;
			$IS_SKIP = false;
			if (isset( $QUERY_HOST_DISK[$x]['boot_from_disk'] ) && $QUERY_HOST_DISK[$x]['boot_from_disk'] == true)
			{		
				$IS_BOOT = true;
			}
			
			if ($PACKER_INFO['HOST_TYPE'] == 'Physical')
			{
				$DISK_NAME = $QUERY_HOST_DISK[$x]['friendly_name'];
				for ($xx=0; $xx<count($SKIP_URI); $xx++)
				{
					if ($QUERY_HOST_DISK[$x]['uri'] == $SKIP_URI[$xx])
					{
						$IS_SKIP = true;
						break;
					}
				}
			}
			else
			{
				$DISK_NAME = $QUERY_HOST_DISK[$x]['name'];
				for ($xx=0; $xx<count($SKIP_URI); $xx++)
				{
					if (strtoupper($QUERY_HOST_DISK[$x]['id']) == strtoupper($SKIP_URI[$xx]))
					{
						$IS_SKIP = true;
						break;
					}
				}
			}			
			$HOST_DISK[] = array('size' => $QUERY_HOST_DISK[$x]['size'], 'disk_name' => $DISK_NAME, 'is_boot' => $IS_BOOT, 'is_skip' => $IS_SKIP);
		}
		$HOST_INFO -> Disk = $HOST_DISK;

		#CHANGE REPLICA JOB CONFIGURATION
		$HOST_INFO -> JobConfig = $QUERY_REPLICA['JOBS_JSON'];
	
		if ($QUERY_REPLICA['CONN_UUID'] != '')
		{
			#PAIR TRANSPORT INFORMATION		
			$SOURCE_CONN = (object)$this -> ServerMgmt -> query_connection_info(json_decode($QUERY_REPLICA['CONN_UUID'],false) -> SOURCE);
			$TARGET_CONN = (object)$this -> ServerMgmt -> query_connection_info(json_decode($QUERY_REPLICA['CONN_UUID'],false) -> TARGET);

			#SOURCE TRANSPORT TYPE
			if ($SOURCE_CONN -> CLUSTER_UUID == 'ONPREMISE-00000-LOCK-00000-PREMISEON' OR $SOURCE_CONN -> SCHD_DIRECT == FALSE)
			{
				$SOURCE_TYPE = 'General Purpose';
			}
			else
			{
				$SOURCE_CLOUD_TYPE  = $SOURCE_CONN -> VENDOR_NAME;				
				$SOURCE_CLOUD_TYPE = ($SOURCE_CLOUD_TYPE == 'AzureBlob' AND $SOURCE_CONN -> SCHD_PROMOTE == TRUE)? 'Azure On-Premises': $SOURCE_CLOUD_TYPE;
				$DEFINE_NAME = ["OPENSTACK", "Aliyun", "Ctyun", "VMWare"];
				$CHANGE_NAME = ["OpenStack", "Alibaba Cloud", "天翼云", "VMware"];

				$SOURCE_TYPE = str_replace($DEFINE_NAME, $CHANGE_NAME, $SOURCE_CLOUD_TYPE);
			}
			
			#TARGET TRANSPORT TYPE
			if ($QUERY_REPLICA['JOBS_JSON'] -> export_path != '')
			{
				$TARGET_TYPE = 'ExportType';
			}
			elseif ($QUERY_REPLICA['WINPE_JOB'] == 'Y')
			{
				$TARGET_TYPE = 'Recover Kit';
			}
			elseif ($QUERY_REPLICA['WINPE_JOB'] == 'N' AND $CLUSTER_UUID == 'ONPREMISE-00000-LOCK-00000-PREMISEON')
			{
				$TARGET_TYPE = 'Disk To Disk';
			}
			elseif ($QUERY_REPLICA['JOBS_JSON'] -> is_azure_blob_mode == TRUE)
			{
				$TARGET_TYPE = ($TARGET_CONN -> LAUN_PROMOTE == TRUE)? 'Azure On-Premises': 'Azure Blob';
				$CLOUD_INFO['CLOUD_TYPE'] = 'AzureBlob';
			}
			else
			{
				$TARGET_CLOUD_TYPE  = $CLOUD_INFO['VENDOR_NAME'];
				$DEFINE_NAME = ["OPENSTACK", "Aliyun", "Ctyun", "VMWare"];
				$CHANGE_NAME = ["OpenStack", "Alibaba Cloud", "天翼云", "VMware"];
				$TARGET_TYPE = str_replace($DEFINE_NAME, $CHANGE_NAME, $TARGET_CLOUD_TYPE);						
			}		
			$PAIR_TRANSPORT = array(
								'source_hostname' => $SOURCE_CONN -> CARR_HOST,
								'source_address' => $SOURCE_CONN -> CARR_ADDR,
								'source_type' => $SOURCE_TYPE,
								'target_hostname' => $TARGET_CONN -> LOAD_HOST,
								'target_address' => $TARGET_CONN -> LOAD_ADDR,
								'target_type' => $TARGET_TYPE
								);
			
			$HOST_INFO -> PairTransport = $PAIR_TRANSPORT;
		}
		
		if ($QUERY_REPLICA['JOBS_JSON'] -> init_loader == true)
		{
			#REPLICA DISK
			$REPL_DISK = $this -> ReplMgmt -> query_replica_disk($REPL_UUID);
		
			#GET ZONE INFORMATION
			$SERV_INFO = explode("|",$TARGET_CONN -> LAUN_OPEN);
			if (isset($SERV_INFO[1]))
			{
				$SERVER_UUID = $SERV_INFO[0];
				$SERVER_ZONE = $SERV_INFO[1];
			}
			else
			{
				$SERVER_UUID = $TARGET_CONN -> LAUN_OPEN;
				$SERVER_ZONE = 'Mercury';
			}		
			
			#BEGIN TO CLOUD DISK INFORMATION
			switch($CLOUD_INFO['CLOUD_TYPE'])
			{
				case 'OPENSTACK':
					for ($r=0; $r<count($REPL_DISK); $r++)
					{
						$CLOUD_DISK_UUID = $REPL_DISK[$r]['OPEN_DISK'];
						
						$VOLUME_NAME = $this -> OpenStackMgmt -> get_volume_detail_info($CLUSTER_UUID,$CLOUD_DISK_UUID) -> volume_name;
						$VOLUME_SIZE = $this -> OpenStackMgmt -> get_volume_detail_info($CLUSTER_UUID,$CLOUD_DISK_UUID) -> volume_size; #IN GB
						
						$VOLUME_INFO[$VOLUME_NAME] = $VOLUME_SIZE;
					}
				break;
				
				case 'AWS':
					for ($r=0; $r<count($REPL_DISK); $r++)
					{
						$CLOUD_DISK_UUID = $REPL_DISK[$r]['OPEN_DISK'];
						
						#QUERY VOLUME INFORMATION
						$VOLUME_STATUS = $this -> AwsMgmt -> describe_volumes($CLUSTER_UUID,$SERVER_ZONE,$CLOUD_DISK_UUID);
						
						$VOLUME_NAME = $VOLUME_STATUS[0]['Tags'][1]['Value'];
						$VOLUME_SIZE = $VOLUME_STATUS[0]['Size'];	#IN GB
						
						$VOLUME_INFO[$VOLUME_NAME] = $VOLUME_SIZE;
					}
				break;
				
				case 'Azure':
				case 'Aliyun':			
					$CloudProvider = $this->getCloudController( $CLOUD_INFO['CLOUD_TYPE'] );

					if( $CLOUD_INFO['CLOUD_TYPE'] == "Azure" )
						$CloudProvider ->getVMByServUUID( $TARGET_CONN->LAUN_UUID , $CLUSTER_UUID);

					for ($r=0; $r<count($REPL_DISK); $r++)
					{
						$CLOUD_DISK_UUID = $REPL_DISK[$r]['OPEN_DISK'];
				
						#QUERY VOLUME INFORMATION
						$VOLUME_STATUS = $CloudProvider->describe_volume($CLUSTER_UUID,$SERVER_ZONE,$CLOUD_DISK_UUID);
				
						$VOLUME_NAME = $VOLUME_STATUS[0]['diskName'];
						$VOLUME_SIZE = $VOLUME_STATUS[0]['size'];	#IN GB
						
						$VOLUME_INFO[$VOLUME_NAME] = $VOLUME_SIZE;
					}
				break;
				
				case 'AzureBlob':
					for ($r=0; $r<count($REPL_DISK); $r++)
					{
						$VOLUME_STATUS = explode('_', $REPL_DISK[$r]['SNAPSHOT_MAPPING']);
						$VOLUME_NAME = $VOLUME_STATUS[0].'.vhd';
						$VOLUME_SIZE = ceil($REPL_DISK[$r]['DISK_SIZE'] / 1024 / 1024 / 1024); #IN GB						
						
						$VOLUME_INFO[$VOLUME_NAME] = $VOLUME_SIZE;					
					}
				break;
				
				case 'Ctyun':
					for ($r=0; $r<count($REPL_DISK); $r++)
					{
						$CLOUD_DISK_UUID = $REPL_DISK[$r]['OPEN_DISK'];
						
						#QUERY VOLUME INFORMATION
						$VOLUME_STATUS = $this -> CtyunMgmt -> describe_volumes($CLUSTER_UUID,$CLOUD_DISK_UUID);
						
						$VOLUME_NAME = $VOLUME_STATUS -> name;
						$VOLUME_SIZE = $VOLUME_STATUS -> size;	#IN GB
						
						$VOLUME_INFO[$VOLUME_NAME] = $VOLUME_SIZE;
					}
				break;
				
				case 'VMWare':
	
					#QUERY VOLUME INFORMATION
					$VOLUME_STATUS = $this -> VMWareMgmt -> getVirtualMachineInfo($CLUSTER_UUID,$REPL_UUID,$REPL_UUID);
					
					foreach( $VOLUME_STATUS->disks as $disk)
						$VOLUME_INFO[$disk->name] = $disk->size_kb / 1024 / 1024;
					
				break;
				
				default:
					for ($r=0; $r<count($REPL_DISK); $r++)
					{
						//$VOLUME_NAME = json_decode($REPL_DISK[$r]['PACK_URI']) -> address;	  #SCSI ADDR
						$VOLUME_NAME = $REPL_DISK[$r]['SCSI_ADDR'];	  					  #SCSI ADDR
						$VOLUME_SIZE = ceil($REPL_DISK[$r]['DISK_SIZE'] / 1024 /1024 / 1024); #IN GB
						
						$VOLUME_INFO[$VOLUME_NAME] = $VOLUME_SIZE;
					}
			}

			if (count($VOLUME_INFO) != 0)
			{				
				$HOST_INFO -> CloudVolume = $VOLUME_INFO;
			}
		}
		
		return $HOST_INFO;
	}
	
	###########################
	# QUERY RECOVERY WORKLOAD INFO
	###########################
	public function query_recovery_workload($SERVICE_UUID)
	{
		#DEFAULE HOST INFO
		$HOST_INFO = (object)array(
								'JobUUID' => $SERVICE_UUID,
								'Hostname' => 'processing..',
								'RecoveryType' => 'processing..',
								'OSName' => 'processing..',
								'OSType' => 'processing..',
								'Disk' => 'processing..',
								'InstanceHostname' => 'processing..',
								'Flavor' => array('name' => 'processing..','vcpus' => 'processing..','ram' => 'processing..'),
								'SecurityGroups' => 'processing..',
								'HypervisorHostname' => 'processing..',
								'NicInfo' => array(array('addr' => 'processing..', 'mac' => 'processing..', 'type' => 'processing..')),
								'CloudType' => 'processing..',
								'AdminPassword' => 'processing..',
								'RecoveryJobSpec' => 'processing..',
								'TimeZone' => 'processing..',
								'LogLocation' => 'processing..',
								'JobTypeX' => false,
								'Status' => false
						);
		
		#QUERY SERVICE INFO
		$QUERY_SERVICE = $this -> query_service($SERVICE_UUID);
	
		#GET USER SET TIME ZONE
		$QUERY_REPLICA = $this -> ReplMgmt -> query_replica($QUERY_SERVICE['REPL_UUID']);
		$HOST_INFO -> TimeZone = $QUERY_REPLICA['JOBS_JSON'] -> timezone;
	
		#LOG LOCATION
		$HOST_INFO -> LogLocation = $QUERY_REPLICA['LOG_LOCATION'];		
		
		#APPEND HOSTNAME
		$HOST_INFO -> Hostname = $QUERY_SERVICE['HOST_NAME'];
		
		#SERVICE JOB JSON
		$SERV_JOBS_JSON = json_decode($QUERY_SERVICE['JOBS_JSON']);
		
		#RECOVERY JOB SPEC
		if (isset($SERV_JOBS_JSON -> datamode_instance))
		{
			$DATAMODE_INSTANCE = ($SERV_JOBS_JSON -> datamode_instance == '')?'NoAssociatedDataModeInstance':$SERV_JOBS_JSON -> datamode_instance;
			$IS_DATAMODE_BOOT = ($SERV_JOBS_JSON -> is_datamode_boot == '')?'false':$SERV_JOBS_JSON -> is_datamode_boot;
			$RCVY_PRE_SCRIPT = $SERV_JOBS_JSON -> rcvy_pre_script;
			$RCVY_POST_SCRIPT = $SERV_JOBS_JSON -> rcvy_pre_script;
		}
		else
		{
			$DATAMODE_INSTANCE = "NoAssociatedDataModeInstance";
			$IS_DATAMODE_BOOT = false;
			$RCVY_PRE_SCRIPT = '';
			$RCVY_POST_SCRIPT = '';
		}
		
		$HOST_INFO -> RecoveryJobSpec = array(
												'rcvy_pre_script' 	=> $RCVY_PRE_SCRIPT,
												'rcvy_post_script' 	=> $RCVY_POST_SCRIPT,
												'datamode_instance' => $DATAMODE_INSTANCE,
												'is_datamode_boot'	=> $IS_DATAMODE_BOOT
										);
		
		#DEFINE RECOVERY TYPE
		if ($QUERY_SERVICE['NOVA_VM_UUID'] == 'EXPORT_LOCK' OR $QUERY_SERVICE['WINPE_JOB'] == 'Y')
		{
			#MARK AS SPECIAL TYPE OF JOB
			$JobTypeX = true;
			
			if ($QUERY_SERVICE['NOVA_VM_UUID'] == 'EXPORT_LOCK')
			{
				$RECOVERYTYPE = 'Export';
			}
			else
			{
				$RECOVERYTYPE = 'Recover Kit';
			}
		}
		elseif ($QUERY_SERVICE['CLUSTER_UUID'] == 'ONPREMISE-00000-LOCK-00000-PREMISEON' AND $QUERY_SERVICE['WINPE_JOB'] == 'N')
		{
			#MARK AS SPECIAL TYPE OF JOB
			$JobTypeX = true;
			$RECOVERYTYPE = 'Disk To Disk';
		}
		else
		{
			#MARK AS NORMAL TYPE OF JOB
			$JobTypeX = false;
			
			$RECOVERY_TYPE = $SERV_JOBS_JSON -> recovery_type;
			$SEARCH = array('PlannedMigration','DisasterRecovery','DevelopmentTesting');
			$REPLACE = array('Planned Migration','Disaster Recovery','Development Testing');
			$RECOVERYTYPE = str_replace($SEARCH, $REPLACE, $RECOVERY_TYPE);
		}		
		
		$HOST_INFO -> RecoveryType = $RECOVERYTYPE;

		#APPEND OS TYPE
		if ($QUERY_SERVICE['OS_TYPE'] == 'MS')
		{
			$OS_TYPE = 'Microsoft Windows';
		}
		else
		{
			$OS_TYPE = 'Linux';
		}
		$HOST_INFO -> OSType = $OS_TYPE;
		
		#APPEND PACKER HOST INFO
		$PACKER_UUID = $QUERY_SERVICE['PACK_UUID'];		
		$PACKER_INFO = $this -> ServerMgmt -> query_host_info($PACKER_UUID);
		
		if ($PACKER_INFO['HOST_TYPE'] == 'Physical' OR $PACKER_INFO['HOST_TYPE'] == 'Offline')
		{
			$HOST_INFO -> OSName = $PACKER_INFO['HOST_INFO']['os_name'];
		}
		else
		{
			$HOST_INFO -> OSName = $PACKER_INFO['HOST_INFO']['guest_os_name'];
		}
	
		#QUERY SERVICE DISK INFO
		$QUERY_SERVICE_DISK = $this -> query_service_disk($SERVICE_UUID);

		if ($QUERY_SERVICE_DISK != false)
		{
			for ($x=0; $x<count($QUERY_SERVICE_DISK); $x++)
			{
				$SERVICE_DISK[] = $QUERY_SERVICE_DISK[$x]['DISK_SIZE'];
			}		
			$HOST_INFO -> Disk = $SERVICE_DISK;
		}
		
		#GET CLOUD INFORMATION
		$CLUSTER_UUID = $QUERY_SERVICE['CLUSTER_UUID'];		
		$CLOUD_INFO	= $this -> query_cloud_info($CLUSTER_UUID);
	
		#GET CONNECTION INFORMATION
		$CONN_INFO = $this -> ServerMgmt -> query_connection_info($QUERY_SERVICE['CONN_UUID']);
		$SERV_INFO = explode("|",$CONN_INFO['LAUN_OPEN']);
		if (isset($SERV_INFO[1]))
		{
			$SERVER_UUID = $SERV_INFO[0];
			$SERVER_ZONE = $SERV_INFO[1];
		}
		else
		{
			$SERVER_UUID = $CONN_INFO['LAUN_OPEN'];
			$SERVER_ZONE = 'Mercury';
		}
		
		#SKIP INSTANCE QUERY ON EXPORT AND WINPE TYPE JOB
		if ($JobTypeX == TRUE)
		{
			$HOST_INFO -> JobTypeX = $JobTypeX; 
			return $HOST_INFO;
		}
		else
		{			
			#DEFINE INSTANCE UUID
			if (isset($SERV_JOBS_JSON -> datamode_instance) AND $SERV_JOBS_JSON -> datamode_instance != 'NoAssociatedDataModeInstance')
			{
				$INSTANCE_UUID = $SERV_JOBS_JSON -> datamode_instance;
			}
			else
			{			
				$INSTANCE_UUID = $QUERY_SERVICE['NOVA_VM_UUID'];
			}		
			
			#ADMINISTRATOR PASSWORD
			$HOST_INFO -> AdminPassword = $QUERY_SERVICE['ADMIN_PASS'];
			
			if ($SERV_JOBS_JSON -> job_status == 'InstanceCreated')
			{
				#BEGIN TO APPEND INSTANCE INFORMATION
				switch($CLOUD_INFO['CLOUD_TYPE'])
				{
					case 'OPENSTACK':
						$QueryHostInformation = $this -> OpenStackMgmt -> get_vm_detail_Info($CLUSTER_UUID,$INSTANCE_UUID);
					
						if (isset($QueryHostInformation -> server -> name))
						{
							#CHANGE INSTANCE HOSTNAME
							$HOST_INFO -> InstanceHostname = $QueryHostInformation -> server -> name;
							
							#CHANGE FAVIOR INFORMATION
							if (isset($QueryHostInformation -> server -> flavor -> id))
							{
								$FLAVOR_ID = $QueryHostInformation -> server -> flavor -> id;								
							}
							else
							{
								$FLAVOR_ID = $QUERY_SERVICE['FLAVOR_ID'];								
							}
							
							$QUERY_FLAVOR_INFO = $this -> OpenStackMgmt -> get_flavor_detail_info($CLUSTER_UUID,$FLAVOR_ID);							
							$FLAVOR = array(
										'name' => $QUERY_FLAVOR_INFO -> flavor -> name,
										'vcpus' => $QUERY_FLAVOR_INFO -> flavor -> vcpus,
										'ram' => $QUERY_FLAVOR_INFO -> flavor -> ram
									);
							
							$HOST_INFO -> Flavor = $FLAVOR;
														
							#CHANGE SECURITY GROUP NAME
							if (isset($QueryHostInformation -> server -> security_groups[0] -> name))
							{
								$HOST_INFO -> SecurityGroups = $QueryHostInformation -> server -> security_groups[0] -> name;
							}
							else
							{
								$SGROUP_ID = $QUERY_SERVICE['SGROUP_UUID'];
								$QUERY_SGROUP_INFO = $this -> OpenStackMgmt -> get_security_group_detail_info($CLUSTER_UUID,$SGROUP_ID);
								$HOST_INFO -> SecurityGroups = $QUERY_SGROUP_INFO -> security_group -> name;
							}
							
							#CHANGE HYPERVISOR HOSTNAME
							$HOST_INFO -> HypervisorHostname = $QueryHostInformation -> server -> {'OS-EXT-SRV-ATTR:hypervisor_hostname'};
						
							#CHANGE NIC INFORMATION
							if (isset($QueryHostInformation -> server -> addresses -> admin_internal_net))
							{
								$ADDR_INFO = $QueryHostInformation -> server -> addresses -> admin_internal_net;
								$PUBLIC_ADDR = '';
								for ($i=0; $i<count($ADDR_INFO); $i++)
								{
									if ($ADDR_INFO[$i] -> {'OS-EXT-IPS:type'} == 'floating')
									{
										$PUBLIC_ADDR = ' ('.$ADDR_INFO[$i] -> addr.')';
									}
									
									if ($ADDR_INFO[$i] -> {'OS-EXT-IPS:type'} == 'fixed')
									{
										$NIC_INFO[] = array(
															'addr' 	=> $ADDR_INFO[$i] -> addr,
															'mac' 	=> $ADDR_INFO[$i] -> {'OS-EXT-IPS-MAC:mac_addr'},
															'type' 	=> $ADDR_INFO[$i] -> {'OS-EXT-IPS:type'}
														);
									}
								}
								
								foreach($NIC_INFO as $NIC_KEY => $NIC_VALUE)
								{
									$NIC_INFO[0]['type'] = $NIC_VALUE['type'].$PUBLIC_ADDR;
								}
							}
							else
							{
								$ADDR_INFO = $QueryHostInformation -> server -> addresses;
				
								foreach ($ADDR_INFO as $ADDR_KEY => $ADDR_DATA)
								{
									for ($n=0; $n<count($ADDR_DATA); $n++)
									{
										if ($ADDR_DATA[$n] -> {'OS-EXT-IPS:type'} == 'floating')
										{
											$PUBLIC_ADDR = ' ('.$ADDR_DATA[$n] -> addr.')';
										}
										
										if ($ADDR_DATA[$n] -> {'OS-EXT-IPS:type'} == 'fixed')
										{
											$NIC_INFO[] = array(
															'addr' 	=> $ADDR_DATA[$n] -> addr,
															'mac' 	=> $ADDR_DATA[$n] -> {'OS-EXT-IPS-MAC:mac_addr'},
															'type' 	=> $ADDR_DATA[$n] -> {'OS-EXT-IPS:type'}
														);
										}
									}
								}
								foreach($NIC_INFO as $NIC_KEY => $NIC_VALUE)
								{
									$NIC_INFO[0]['type'] = $NIC_VALUE['type'].$PUBLIC_ADDR;
								}
							}
							$HOST_INFO -> NicInfo = $NIC_INFO;
							
							#CHANGE STATUS
							$HOST_INFO -> Status = true;
							
							#CHANGE CLOUD TYPE
							if ($CLOUD_INFO['VENDOR_NAME'] == 'OPENSTACK')
							{
								$HOST_INFO -> CloudType = 'OpenStack';
							} 
							else
							{							
								$HOST_INFO -> CloudType = $CLOUD_INFO['VENDOR_NAME'];	
							}
						}
						return $HOST_INFO;
					break;
				
					case 'AWS':
						#QUERY INSTANCE INFORMATION
						$QueryHostInformation = $this -> AwsMgmt -> describe_instance($CLUSTER_UUID,$SERVER_ZONE,$INSTANCE_UUID);
						
						#CHANGE INSTANCE HOSTNAME
						if (isset($QueryHostInformation[0]['Instances'][0]['Tags']))
						{
							for ($n=0; $n<count($QueryHostInformation[0]['Instances'][0]['Tags']); $n++)
							{
								if ($QueryHostInformation[0]['Instances'][0]['Tags'][$n]['Key'] == 'Name')
								{
									$HOST_INFO -> InstanceHostname = $QueryHostInformation[0]['Instances'][0]['Tags'][$n]['Value'];
								}
								else
								{
									$HOST_INFO -> InstanceHostname = $QueryHostInformation[0]['Instances'][0]['Tags'][0]['Value'];
								}							
							}
						
							$FLAVOR_TYPE = $QueryHostInformation[0]['Instances'][0]['InstanceType'];
							
							#CHANGE FAVIOR INFORMATION
							$QUERY_FLAVOR_INFO = $this -> AwsMgmt -> describe_instance_types($FLAVOR_TYPE);
							$FLAVOR = array(
											'name' => $QUERY_FLAVOR_INFO['Name'],
											'vcpus' => $QUERY_FLAVOR_INFO['vCPU'],
											'ram' => str_replace(' GiB','',$QUERY_FLAVOR_INFO['Memory'])*1024
										);			
							$HOST_INFO -> Flavor = $FLAVOR;
							
							#CHANGE SECURITY GROUP NAME
							$HOST_INFO -> SecurityGroups = $QueryHostInformation[0]['Instances'][0]['SecurityGroups'][0]['GroupName'];
							
							#CHANGE HYPERVISOR HOSTNAME
							$HOST_INFO -> HypervisorHostname = $QueryHostInformation[0]['Instances'][0]['Placement']['AvailabilityZone'];
							
							#CHANGE NIC INFORMATION
							$ADDR_INFO = $QueryHostInformation[0]['Instances'][0]['NetworkInterfaces'];
							for ($i=0; $i<count($ADDR_INFO); $i++)
							{
								$NIC_INFO[] = array(
													'addr' 	=> $ADDR_INFO[$i]['PrivateIpAddress'],
													'mac' 	=> $ADDR_INFO[$i]['MacAddress'],											
													'type' 	=> $ADDR_INFO[$i]['Association']['PublicIp']
												);
							}
							$HOST_INFO -> NicInfo = $NIC_INFO;
							
							#CHANGE STATUS
							$HOST_INFO -> Status = true;
							
							$HOST_INFO -> CloudType = 'Amazon Web Services';
						}
						
						return $HOST_INFO;
					break;
					
					case 'Azure':
						$HOST_INFO -> CloudType = 'Azure';
						
						$this -> AzureMgmt -> getVMByServUUID( $CONN_INFO['LAUN_UUID'], $CLUSTER_UUID);
						
						$instance = $this -> AzureMgmt -> describe_instance($CLUSTER_UUID, $SERVER_ZONE, $INSTANCE_UUID);
						if (isset($instance))
						{
							$HOST_INFO -> InstanceHostname = $instance["name"];
							$HOST_INFO -> SecurityGroups = $instance["security_group"];
							$HOST_INFO -> HypervisorHostname = $SERVER_ZONE;
							
							$ADDR_INFO = $instance["netInterface"];

							for ($i=0; $i<count($ADDR_INFO); $i++)
							{	
								$type = "Public : None";
								
								if(isset( $ADDR_INFO[$i]['public_ip'] ))
									$type = "Public : ".$ADDR_INFO[$i]['public_ip'];
							
								$NIC_INFO[] = array(
													'addr' 	=> "Private : ".$ADDR_INFO[$i]['private_ip'],
													'mac' 	=> $ADDR_INFO[$i]['mac'],
													'type'  => $type
												);

							}
							$HOST_INFO -> NicInfo = $NIC_INFO;
							
							$vm_type = $this -> AzureMgmt -> describe_instance_types( $CLUSTER_UUID, $SERVER_ZONE, $instance["type"]);
							
							$FLAVOR = array(
											'name' => $vm_type["name"],
											'vcpus' => $vm_type["numberOfCores"],
											'ram' => $vm_type["memoryInMB"]
										);			
							$HOST_INFO -> Flavor = $FLAVOR;
						}
						return $HOST_INFO;
					break;
					
					case 'Aliyun':
						$HOST_INFO -> CloudType = 'Aliyun';
		
						$instance = $this -> AliMgmt -> describe_instance($CLUSTER_UUID, $this->AliMgmt->getCurrectRegion( substr($SERVER_ZONE, 0, -1) ),$INSTANCE_UUID);
					
						if (isset($instance))
						{
							$disks = $this->AliMgmt->DescribeSystemDiskInfoFromInstance($CLUSTER_UUID, $SERVER_ZONE, $INSTANCE_UUID);
								
							foreach( $disks as $disk ) {
								if( $disk["diskType"] == "system" )
									$HOST_INFO->Disk[0] = $disk["size"];
							}
							
							$HOST_INFO -> InstanceHostname = $instance["InstanceName"];
							$HOST_INFO -> SecurityGroups = $instance["SecurityGroup"];
							$HOST_INFO -> HypervisorHostname = $this->AliMgmt->getCurrectRegion( substr($SERVER_ZONE, 0, -1) );
							
							$ADDR_INFO = $instance["NetworkInterfaces"];
			
							for ($i=0; $i<count($ADDR_INFO); $i++)
							{
								$NIC_INFO[] = array(
													'addr' 	=> $ADDR_INFO[$i]->{'PrimaryIpAddress'},
													'mac' 	=> $ADDR_INFO[$i]->{'MacAddress'},											
													'type' 	=> $ADDR_INFO[$i]->{'NetworkInterfaceId'}.' / '.$instance["PublicIpAddress"]
												);
							}
							$HOST_INFO -> NicInfo = $NIC_INFO;
							
							$FLAVOR = array(
											'name' => $instance["InstanceType"],
											'vcpus' => $instance["CPU"],
											'ram' => $instance["Memory"]
										);			
							$HOST_INFO -> Flavor = $FLAVOR;
						}
						return $HOST_INFO;
					break;
				
					case 'Tencent':
						$HOST_INFO -> CloudType = 'Tencent';
						
						$instance = $this -> TencentMgmt -> describe_instance($CLUSTER_UUID, $this->TencentMgmt->getCurrectRegion( $SERVER_ZONE ),$INSTANCE_UUID);
		
						if (isset($instance))
						{
							/*$disks = $this->AliMgmt->DescribeDiskInfoFromInstance($CLUSTER_UUID, $SERVER_ZONE, $INSTANCE_UUID);
							
							foreach( $disks as $disk ) {
								if( $disk["diskType"] == "system" )
									$HOST_INFO->Disk[0] = $disk["size"];
							}*/
							
							$HOST_INFO -> InstanceHostname = $instance["InstanceName"];
							$HOST_INFO -> SecurityGroups = $instance["SecurityGroup"];
							$HOST_INFO -> HypervisorHostname = $this->TencentMgmt->getCurrectRegion( $SERVER_ZONE );
							
							$ADDR_INFO = $instance["NetworkInterfaces"];
			
							for ($i=0; $i<count($ADDR_INFO); $i++)
							{
								$NIC_INFO[] = array(
													'addr' 	=> $ADDR_INFO[$i]->{'PrimaryIpAddress'},
													'mac' 	=> $ADDR_INFO[$i]->{'MacAddress'},											
													'type' 	=> $ADDR_INFO[$i]->{'NetworkInterfaceId'}
												);
							}
							$HOST_INFO -> NicInfo = $NIC_INFO;
							
							$FLAVOR = array(
											'name' => $instance["InstanceType"],
											'vcpus' => $instance["CPU"],
											'ram' => $instance["Memory"]
										);			
							$HOST_INFO -> Flavor = $FLAVOR;
						}
						return $HOST_INFO;
					break;
					
					case 'Ctyun':
					
						$QueryHostInformation = $this -> CtyunMgmt -> describe_instance($CLUSTER_UUID,$INSTANCE_UUID) -> server;
					
						if ($QueryHostInformation != false)
						{
							#CHANGE INSTANCE HOSTNAME
							$HOST_INFO -> InstanceHostname = $QueryHostInformation -> name;
							
							#CHANGE FAVIOR INFORMATION
							if (isset($QueryHostInformation -> flavor -> id))
							{
								$FLAVOR_ID = $QueryHostInformation -> flavor -> id;								
							}
							else
							{
								$FLAVOR_ID = $QUERY_SERVICE['FLAVOR_ID'];								
							}
							
							$QUERY_FLAVOR_INFO = $this -> CtyunMgmt -> describe_instance_types($CLUSTER_UUID,$FLAVOR_ID);	

							$FLAVOR = array(
										'name' => $QUERY_FLAVOR_INFO -> id,
										'vcpus' => $QUERY_FLAVOR_INFO -> cpuNum,
										'ram' => $QUERY_FLAVOR_INFO -> memSize*1024 #MB
									);
							
							$HOST_INFO -> Flavor = $FLAVOR;
							
							$HOST_INFO -> SecurityGroups = $QueryHostInformation -> security_groups[0] -> name;
							
							$HOST_INFO -> HypervisorHostname = $QueryHostInformation -> {'OS-EXT-AZ:availability_zone'};
														
							$ADDR_INFO = $QueryHostInformation -> addresses;
							
							foreach ($ADDR_INFO as $ADDR_KEY => $ADDR_INFO)
							{
								for ($i=0; $i<count($ADDR_INFO); $i++)
								{
									$NIC_INFO[] = array(
													'addr' 	=> $ADDR_INFO[$i] -> addr,
													'mac' 	=> $ADDR_INFO[$i] -> {'OS-EXT-IPS-MAC:mac_addr'},											
													'type' 	=> $ADDR_INFO[$i] -> {'OS-EXT-IPS:type'}
												);
								}
							}
							$HOST_INFO -> NicInfo = $NIC_INFO;
							
							$HOST_INFO -> CloudType = '天翼云';
						}
						return $HOST_INFO;
					
					break;
					
					case 'VMWare':
						$HOST_INFO -> CloudType = 'VMware';
					
						$instance = $this -> VMWareMgmt -> getVirtualMachineInfo($CLUSTER_UUID, $QUERY_SERVICE['REPL_UUID'],$INSTANCE_UUID);

						if (isset($instance))
						{
							/*$disks = $this->AliMgmt->DescribeDiskInfoFromInstance($CLUSTER_UUID, $SERVER_ZONE, $INSTANCE_UUID);
							
							foreach( $disks as $disk ) {
								if( $disk["diskType"] == "system" )
									$HOST_INFO->Disk[0] = $disk["size"];
							}*/
							
							$HOST_INFO -> InstanceHostname = $instance->name;
							$HOST_INFO -> SecurityGroups = "None";//$instance["SecurityGroup"];
							$HOST_INFO -> HypervisorHostname = "VMware";
			
							for ($i=0; $i<count($instance->network_adapters); $i++)
							{
								$IP_ADDR = ($instance->network_adapters[$i]->ip_addresses == "")?"0.0.0.0":implode(",",$instance->network_adapters[$i]->ip_addresses);
								
								$NIC_INFO[] = array(
													'addr' 	=> $IP_ADDR,
													'mac' 	=> $instance->network_adapters[$i]->mac_address,											
													'type' 	=> $instance->network_adapters[$i]->network
												);
							}
							$HOST_INFO -> NicInfo = $NIC_INFO;
							
							$FLAVOR = array(
											'name' => $instance -> guest_os_name,
											'vcpus' => $instance -> number_of_cpu,
											'ram' => $instance -> memory_mb
										);			
							$HOST_INFO -> Flavor = $FLAVOR;
						}
						return $HOST_INFO;
					break;
					default:
						return $HOST_INFO;
				}
			}
			else
			{
				return $HOST_INFO;
			}
		}
	}
	
	###########################
	#GET REPLICA CUSTOMIZE DISK INFO 
	###########################
	private function replica_customize_disk_info($PACK_INFO,$SKIP_DISK)
	{
		$HOST_DISK = $PACK_INFO['HOST_DISK'];
		$SKIP_DISK = explode(",",$SKIP_DISK);
		
		for ($i=0; $i<count($HOST_DISK); $i++)
		{
			if ($PACK_INFO['HOST_TYPE'] == 'Physical')
			{
				$DISK_INFO = $PACK_INFO['HOST_INFO']['disk_infos'];
				if ($DISK_INFO[$i]['boot_from_disk'] == true)
				{
					$HOST_DISK[$i]['IS_BOOT'] = true;
				}
				else
				{
					$HOST_DISK[$i]['IS_BOOT'] = false;
				}		
			}
			else		
			{
				$HOST_DISK[$i]['IS_BOOT'] = false;
			}			
			
			if($SKIP_DISK[$i] == 'false')
			{
				$HOST_DISK[$i]['IS_SKIP'] = true;
			}
			else
			{
				$HOST_DISK[$i]['IS_SKIP'] = false;
			}
		}
		return $HOST_DISK;
	}
	
	###########################
	#QUERY SERVICE PLAN
	###########################
	public function query_service_plan($ACCT_UUID,$PLAN_UUID)
	{
		#DEFAULE PLAN INFO
		$TEMP_INFO = (object)array(
							'PlanUUID' => $PLAN_UUID,
							'Hostname' => 'processing..',
							'RecoveryType' => 'processing..',
							'HostType' => 'processing..',
							'OSName' => 'processing..',
							'OSType' => 'processing..',
							'Disk' => 'processing..',
							'Flavor' => array('name' => 'processing..','vcpus' => 'processing..','ram' => 'processing..'),
							'SecurityGroups' => 'processing..',
							'HypervisorHostname' => 'processing..',
							'Network' => array('name' => 'processing..', 'status' => 'processing..'),
							'CloudType' => 'processing..',
							'Status' => false
					);
		
		$EXEC_PLAN = "SELECT * FROM _SERVICE_PLAN WHERE _ACCT_UUID = '".$ACCT_UUID."' AND _PLAN_UUID = '".$PLAN_UUID."' AND _STATUS = 'Y'";
		$QUERY_PLAN = $this -> DBCON -> prepare($EXEC_PLAN);
		$QUERY_PLAN -> execute();
		$COUNT_ROWS = $QUERY_PLAN -> rowCount();
		if ($COUNT_ROWS != 0)
		{
			#BEGIN TO QUERY PLAN
			$PLAN_QUERY = $QUERY_PLAN -> fetch(PDO::FETCH_ASSOC);			
		
			#REPLICA INFOMATION
			$REPL_UUID = $PLAN_QUERY['_REPL_UUID'];
			$REPL_INFO = $this -> ReplMgmt -> query_replica($REPL_UUID);
			$PACK_INFO = $this -> ServerMgmt -> query_host_info($REPL_INFO['PACK_UUID']);
			
			#PLAN CONFIGURE
			$PLAN_CONFIG 	= json_decode($PLAN_QUERY['_PLAN_JSON']);
			
			$SKIP_DISK		= $PLAN_CONFIG -> volume_uuid;
			$CLOUD_DISK = $this -> replica_customize_disk_info($PACK_INFO,$SKIP_DISK);
			$TEMP_INFO -> Disk = $CLOUD_DISK;
			
			$CLOUD_TYPE 	= $PLAN_CONFIG -> CloudType;
			$RECOVERY_TYPE 	= $PLAN_CONFIG -> RecoverType;
			$CLUSTER_UUID 	= $PLAN_CONFIG -> ClusterUUID;
			
			if( isset($PLAN_CONFIG -> flavor_id) )
			$FLAVOR_ID	 	= $PLAN_CONFIG -> flavor_id;
			
			$NETWORK_ID		= $PLAN_CONFIG -> network_uuid;
			
			if( isset($PLAN_CONFIG -> sgroup_uuid) )
			$SGROUP_ID		= $PLAN_CONFIG -> sgroup_uuid;
		
			$SERVER_ZONE	= $PLAN_CONFIG -> ServiceRegin;
			
			#CLOUD INFORMATION
			$CLOUD_INFO	= $this -> query_cloud_info($CLUSTER_UUID);
		
			#SET CLOUD TYPE
			$TEMP_INFO -> CloudType = $CLOUD_INFO['VENDOR_NAME'];
	
			#SET DEFAULT METADATA
			$TEMP_INFO -> Hostname = $REPL_INFO['HOST_NAME'];
			
			#APPEND HOST TYPE
			$TEMP_INFO -> HostType = $PACK_INFO['HOST_TYPE'];
			if ($PACK_INFO['HOST_TYPE'] == 'Physical')
			{				
				$TEMP_INFO -> OSName = $PACK_INFO['HOST_INFO']['os_name'];
			}
			else
			{
				$TEMP_INFO -> OSName = $PACK_INFO['HOST_INFO']['guest_os_name'];
			}
			$SEARCH = array('RECOVERY_PM','RECOVERY_DR','RECOVERY_DT');
			$REPLACE = array('Planned Migration','Disaster Recovery','Development Testing');
			$RECOVERYTYPE = str_replace($SEARCH, $REPLACE, $RECOVERY_TYPE);
			$TEMP_INFO -> RecoveryType = $RECOVERYTYPE;
	
			#APPEND OS TYPE
			if ($PACK_INFO['HOST_SERV']['SYST_TYPE'] == 'WINDOWS' OR $PACK_INFO['HOST_INFO']['guest_os_type'] == 1)
			{
				$OS_TYPE = 'Microsoft Windows';
			}
			else
			{
				$OS_TYPE = 'Linux';
			}
			$TEMP_INFO -> OSType = $OS_TYPE;

			switch ($CLOUD_TYPE)
			{
				case 'OPENSTACK':					
					#FLAVOR INFO
					$QUERY_FLAVOR_INFO = $this -> OpenStackMgmt -> get_flavor_detail_info($CLUSTER_UUID,$FLAVOR_ID);							
					$FLAVOR = array(
									'name' => $QUERY_FLAVOR_INFO -> flavor -> name,
									'vcpus' => $QUERY_FLAVOR_INFO -> flavor -> vcpus,
									'ram' => $QUERY_FLAVOR_INFO -> flavor -> ram.'MB'
								);
					$TEMP_INFO -> Flavor = $FLAVOR;
					
					#NETWORK INFO
					$QUERY_NETWORK_INFO = $this -> OpenStackMgmt -> get_network_detail_info($CLUSTER_UUID,$NETWORK_ID);
					$NETWORK = array(
									'name' => $QUERY_NETWORK_INFO -> network -> name,
									'status' => $QUERY_NETWORK_INFO -> network -> status
								);
					$TEMP_INFO -> Network = $NETWORK;
				
					#SECURITY GROUP INFO					
					$QUERY_SGROUP_INFO = $this -> OpenStackMgmt -> get_security_group_detail_info($CLUSTER_UUID,$SGROUP_ID);
					$TEMP_INFO -> SecurityGroups = $QUERY_SGROUP_INFO -> security_group -> description;
					
					return $TEMP_INFO;				
				break;
				
				case 'AWS':					
					#FLAVOR INFO
					$QUERY_FLAVOR_INFO = $this -> AwsMgmt -> describe_instance_types($FLAVOR_ID);
					$FLAVOR = array(
									'name' => $QUERY_FLAVOR_INFO['Name'],
									'vcpus' => $QUERY_FLAVOR_INFO['vCPU'],
									'ram' => $QUERY_FLAVOR_INFO['Memory']
								);
					$TEMP_INFO -> Flavor = $FLAVOR;
						
					#NETWORK INFO
					$QUERY_NETWORK_INFO = $this -> AwsMgmt -> describe_available_network($CLUSTER_UUID,$SERVER_ZONE,$NETWORK_ID);
					$NETWORK = array(
									'name' => $QUERY_NETWORK_INFO[0]['VpcId'],
									'status' => $QUERY_NETWORK_INFO[0]['State']
								);
					$TEMP_INFO -> Network = $NETWORK;
					
					#SECURITY GROUP INFO		
					$QUERY_SGROUP_INFO = $this -> AwsMgmt -> describe_security_group($CLUSTER_UUID,$SERVER_ZONE,$SGROUP_ID);
					$TEMP_INFO -> SecurityGroups = $QUERY_SGROUP_INFO[0]['Description'];
					
					return $TEMP_INFO;
				break;
				
				case 'Azure':

				break;
				
				case 'Aliyun':

				break;
				
				case 'VMWare':
					$FLAVOR = array(
									'name' => "",
									'vcpus' => $PLAN_CONFIG->CPU,
									'ram' => $PLAN_CONFIG->Memory
								);
					$TEMP_INFO -> Flavor = $FLAVOR;
					
					$NETWORK = array(
									'name' => $PLAN_CONFIG->network_uuid,
									'status' => ""
								);
					$TEMP_INFO -> Network = $NETWORK;
					
				break;
			}		
			
			return $TEMP_INFO;
		}
	}
		
	###########################
	# LIST CLOUD
	###########################
	public function list_cloud($ACCT_UUID)
	{
		$GET_EXEC = "SELECT 
							TA.*, TB.*, TC._HOST_NAME as TransportName FROM _CLOUD_MGMT as TA
						JOIN 
							_SYS_CLOUD_TYPE as TB on TA._CLOUD_TYPE = TB._ID
							left join _SERVER as TC on TC._OPEN_UUID = TA._CLUSTER_UUID
						WHERE 
							TA._ACCT_UUID = '".strtoupper($ACCT_UUID)."' AND TA._STATUS = 'Y'
						group by TA._CLUSTER_UUID
						ORDER by TA._TIMESTAMP ASC";
	
		$QUERY = $this -> DBCON -> prepare($GET_EXEC);
		$QUERY -> execute();
		$COUNT_ROWS = $QUERY -> rowCount();
		
		if ($COUNT_ROWS != 0)
		{
			foreach($QUERY as $QueryResult)
			{
				$AUTH_DATA = json_decode($QueryResult['_AUTH_TOKEN']);
				
				if (isset($AUTH_DATA -> auth_token)){$AUTH_TOKEN = $AUTH_DATA -> auth_token;}else{$AUTH_TOKEN = $QueryResult['_AUTH_TOKEN'];}
				if (isset($AUTH_DATA -> project_id)){$PROJECT_ID = $AUTH_DATA -> project_id;}else{$PROJECT_ID = null;}
				if (isset($AUTH_DATA -> vendor_name)){$VENDOR_NAME = $AUTH_DATA -> vendor_name;}else{$VENDOR_NAME = $QueryResult['_CLOUD_TYPE'];}
				if (isset($AUTH_DATA -> project_region)){$REGION = $AUTH_DATA -> project_region;}else{$REGION = null;}			
				
				$HOTHATCH_DATA[] = array(
								"ACCT_UUID" 	=> $QueryResult['_ACCT_UUID'],
								"REGN_UUID" 	=> $QueryResult['_REGN_UUID'],
								"CLUSTER_UUID" 	=> $QueryResult['_CLUSTER_UUID'],
								
								"PROJECT_ID" 	=> $PROJECT_ID,
								"PROJECT_NAME" 	=> $QueryResult['_PROJECT_NAME'],
								"CLUSTER_USER" 	=> Misc_Class::encrypt_decrypt('decrypt',$QueryResult['_CLUSTER_USER']),
								"CLUSTER_PASS" 	=> Misc_Class::encrypt_decrypt('decrypt',$QueryResult['_CLUSTER_PASS']),
								"CLUSTER_ADDR"	=> $QueryResult['_CLUSTER_ADDR'],
								
								"AUTH_TOKEN" 	=> $AUTH_TOKEN,
								"TIMESTAMP" 	=> strtotime($QueryResult['_TIMESTAMP']),
								"CLOUD_TYPE"	=> $QueryResult['_CLOUD_TYPE'],
								"VENDOR"		=> $VENDOR_NAME,
								"REGION"		=> $REGION,
								"TransportName"	=> $QueryResult['TransportName']
							);	
			}
			
			return $HOTHATCH_DATA;
		}
		else
		{
			return false;
		}
	}
	
	###########################
	# QUERY CLOUD INFO
	###########################
	public function query_cloud_info($CLUSTER_UUID)
	{
		$GET_EXEC 	= "SELECT * FROM 
							_CLOUD_MGMT  as TA 
						JOIN 
							_SYS_CLOUD_TYPE as TB on TA._CLOUD_TYPE = TB._ID
						WHERE 
							TA._CLUSTER_UUID = '".strtoupper($CLUSTER_UUID)."' AND _STATUS = 'Y'";
	
		$QUERY = $this -> DBCON -> prepare($GET_EXEC);
		$QUERY -> execute();
		$COUNT_ROWS = $QUERY -> rowCount();
		
		if ($COUNT_ROWS != 0)
		{
			foreach($QUERY as $QueryResult)
			{
				$AUTH_DATA = json_decode($QueryResult['_AUTH_TOKEN']);
				
				if (isset($AUTH_DATA -> auth_token)){$AUTH_TOKEN = $AUTH_DATA -> auth_token;}else{$AUTH_TOKEN = $QueryResult['_AUTH_TOKEN'];}
				if (isset($AUTH_DATA -> project_id)){$PROJECT_ID = $AUTH_DATA -> project_id;}else{$PROJECT_ID = null;}
				if (isset($AUTH_DATA -> vendor_name)){$VENDOR_NAME = $AUTH_DATA -> vendor_name;}else{$VENDOR_NAME = $QueryResult['_CLOUD_TYPE'];}
				if (isset($AUTH_DATA -> project_region)){$REGION = $AUTH_DATA -> project_region;}else{$REGION = null;}
								
				$HOTHATCH_DATA = array(
									"ACCT_UUID" 	 => $QueryResult['_ACCT_UUID'],
									"REGN_UUID" 	 => $QueryResult['_REGN_UUID'],
									"CLUSTER_UUID" 	 => $QueryResult['_CLUSTER_UUID'],
									
									"PROJECT_ID" 	 => $PROJECT_ID,
									"PROJECT_NAME" 	 => $QueryResult['_PROJECT_NAME'],
								
									"CLUSTER_USER" 	 => Misc_Class::encrypt_decrypt('decrypt',$QueryResult['_CLUSTER_USER']),
									"CLUSTER_PASS" 	 => Misc_Class::encrypt_decrypt('decrypt',$QueryResult['_CLUSTER_PASS']),
									"CLUSTER_ADDR"	 => $QueryResult['_CLUSTER_ADDR'],
								
									"AUTH_TOKEN" 	 => $AUTH_TOKEN,
									"TIMESTAMP"		 => $QueryResult['_TIMESTAMP'],
									"CLOUD_TYPE"	 => $QueryResult['_CLOUD_TYPE'],
									"VENDOR_NAME"	 => $VENDOR_NAME,
									"REGION"		 => $REGION
								);	
			}
			
			return $HOTHATCH_DATA;
		}
		else
		{
			return false;
		}
	}
	
	###########################
	# DELETE SELECT CLOUD
	###########################
	public function delete_cloud($CLUSTER_UUID)
	{
		$CHECK_REGISTER_TRANSPORT = "SELECT * FROM _SERVER WHERE _OPEN_UUID = '".$CLUSTER_UUID."' AND _STATUS = 'Y'";
		$QUERY = $this -> DBCON -> prepare($CHECK_REGISTER_TRANSPORT);
		$QUERY -> execute();
		$COUNT_ROWS = $QUERY -> rowCount();
		
		if ($COUNT_ROWS == 0)
		{	
			$DELETE_EXEC = "UPDATE _CLOUD_MGMT
							SET
								_TIMESTAMP 		= '".Misc_Class::current_utc_time()."',
								_STATUS			= 'X'
							WHERE
								_CLUSTER_UUID 	= '".$CLUSTER_UUID."'";
			
			$QUERY = $this -> DBCON -> prepare($DELETE_EXEC) -> execute();
			
			#REMOVE OPENSTACK REFERENCE FILE
			$REF_FILE = $_SERVER['DOCUMENT_ROOT'].'/_include/_inc/_endpoints/'.$CLUSTER_UUID.'.ref';
			@unlink($REF_FILE);
			
			return true;
		}
		else
		{
			return false;
		}
	}
	
	###########################
	# QUERY CLOUD DISK
	###########################
	public function query_cloud_disk($REPL_UUID)
	{
		$QUERY_EXEC = "SELECT * FROM _CLOUD_DISK WHERE _REPL_UUID = '".$REPL_UUID."' AND _STATUS = 'Y'";
		
		$QUERY = $this -> DBCON -> prepare($QUERY_EXEC);
		$QUERY -> execute();		
		$COUNT_ROWS = $QUERY -> rowCount();
			
		if ($COUNT_ROWS != 0)
		{
			foreach($QUERY as $QueryResult)
			{
				$OPEN_DISK_ARRAY[] = array(
										'OPEN_DISK_UUID' 	=> $QueryResult['_OPEN_DISK_UUID'],
										'OPEN_SERV_UUID' 	=> $QueryResult['_OPEN_SERV_UUID'],
										'OPEN_DISK_ID' 		=> $QueryResult['_OPEN_DISK_ID'],
										'DEVICE_PATH' 		=> $QueryResult['_DEVICE_PATH'],
										'REPL_UUID'	 		=> $QueryResult['_REPL_UUID'],
										'REPL_DISK_UUID'	=> $QueryResult['_REPL_DISK_UUID'],
										'TIMESTAMP' 		=> $QueryResult['_TIMESTAMP'],
										'STATUS' 			=> $QueryResult['_STATUS']
									);								
			}			
			return $OPEN_DISK_ARRAY;
		}
		else
		{
			return false;
		}
	}
	
	###########################
	# MGMT GENERATE SESSION
	###########################
	public function mgmt_generate_session($ADDR_UUID)
	{
		$PORT_NUM = \saasame\transport\Constant::get('TRANSPORT_SERVICE_PORT');
		
		$OpenProtocol = $this -> common_connection('127.0.0.1',$PORT_NUM);
		
		$ClientCall = new saasame\transport\transport_serviceClient($OpenProtocol);	
			
		#response FORWARD
		$response = $ClientCall -> generate_session($ADDR_UUID);
		
		return $response;
	}
	
	
	###########################
	# GET CLOUD CONTROLLER
	###########################
	public function getCloudController( $cloudType ) {
		
		$Controller = $cloudType.'_Controller';
		
		$cloudController = new $Controller();

        return $cloudController;
    }
	
	
	###########################
	# CONVERT TO BYTES
	###########################
	public function convert_to_bytes($INPUT)
	{
		$FORMAT_INPUT = preg_split('/(?<=[0-9])(?=[a-z]+)/i',$INPUT);
		
		$NUMBER = $FORMAT_INPUT[0];
		$UNITS  = $FORMAT_INPUT[1];
		
		switch($UNITS)
		{
			case "KB":
			case "K":
				return $NUMBER * 1024;
			case "MB":
			case "M":
				return $NUMBER * pow(1024,2);
			case "GB":
			case "G":
				return $NUMBER * pow(1024,3);
			case "TB":
			case "T":
				return $NUMBER * pow(1024,4);
			case "PB":
			case "P":
				return $NUMBER * pow(1024,5);
			default:
				return $INPUT;
		}
	}
	
	
	###########################
	# GET MGMT TRANSPORT UUID 
	# PASSIVE PACKER REGISTRATION PAIRED WITH THE MANAGEMENT SERVER
	###########################
	public function get_mgmt_transport_uuid($ACCT_UUID,$SERV_ADDR)
	{
		$PORT_NUM = \saasame\transport\Constant::get('TRANSPORT_SERVICE_PORT');		
		$OpenProtocol = $this -> common_connection('127.0.0.1',$PORT_NUM);		
		$ClientCall = new saasame\transport\transport_serviceClient($OpenProtocol);	
		
		$HostDetail = $ClientCall -> get_host_detail_p($SERV_ADDR,null);		
		$MACHINE_ID = $HostDetail -> machine_id;		
		return $this -> ServerMgmt -> select_self_transport_info($ACCT_UUID,$MACHINE_ID);
	}
	
	###########################
	# GET_VHD_DISK_SNAPSHOTS
	###########################
	public function get_vhd_disk_snapshots($CONN_STRING,$REPL_UUID,$DISK_ID)
	{	
		$PORT_NUM = \saasame\transport\Constant::get('TRANSPORT_SERVICE_PORT');
		$OpenProtocol = $this -> common_connection('127.0.0.1', $PORT_NUM);
		$ClientCall = new saasame\transport\transport_serviceClient($OpenProtocol);
		
		$LIST_SNPASHOT = $ClientCall -> get_vhd_disk_snapshots($CONN_STRING,$REPL_UUID,$DISK_ID);
		return $LIST_SNPASHOT;
	}
	
	###########################
	# DELETE_VHD_DISK_SNAPSHOT
	###########################
	public function delete_vhd_disk_snapshot($CONN_STRING,$REPL_UUID,$DISK_ID,$SNAPSHOT_ID)
	{
		$PORT_NUM = \saasame\transport\Constant::get('TRANSPORT_SERVICE_PORT');
		$OpenProtocol = $this -> common_connection('127.0.0.1', $PORT_NUM);
		$ClientCall = new saasame\transport\transport_serviceClient($OpenProtocol);
		
		$DELETE_SNPASHOT = $ClientCall -> delete_vhd_disk_snapshot($CONN_STRING,$REPL_UUID,$DISK_ID,$SNAPSHOT_ID);
		return $DELETE_SNPASHOT;
	}
	
	###########################
	# CREATE_VHD_DISK_FROM_SNAPSHOT
	###########################
	public function create_vhd_disk_from_snapshot($CONN_STRING,$REPL_UUID,$ORIGINAL_DISK_NAME,$TARGET_DISK_NAME,$SNAPSHOT_ID)
	{
		$PORT_NUM = \saasame\transport\Constant::get('TRANSPORT_SERVICE_PORT');
		$OpenProtocol = $this -> common_connection('127.0.0.1', $PORT_NUM);
		$ClientCall = new saasame\transport\transport_serviceClient($OpenProtocol);
		
		$VOLUME_ID = $ClientCall -> create_vhd_disk_from_snapshot($CONN_STRING,$REPL_UUID,$ORIGINAL_DISK_NAME,$TARGET_DISK_NAME,$SNAPSHOT_ID);
		return $VOLUME_ID;
	}
	
	###########################
	# DELETE_VHD_DISK
	###########################
	public function delete_vhd_disk($CONN_STRING,$REPL_UUID,$DISK_NAME)
	{
		$PORT_NUM = \saasame\transport\Constant::get('TRANSPORT_SERVICE_PORT');
		$OpenProtocol = $this -> common_connection('127.0.0.1', $PORT_NUM);
		$ClientCall = new saasame\transport\transport_serviceClient($OpenProtocol);
		
		$VOLUME_ID = $ClientCall -> delete_vhd_disk($CONN_STRING,$REPL_UUID,$DISK_NAME);
		return $VOLUME_ID;
	}
		
	###########################
	# VERIFY_CONNECTION_STRING
	###########################
	public function verify_connection_string($CONN_STRING)
	{
		$PORT_NUM = \saasame\transport\Constant::get('TRANSPORT_SERVICE_PORT');
		$OpenProtocol = $this -> common_connection('127.0.0.1', $PORT_NUM);
		$ClientCall = new saasame\transport\transport_serviceClient($OpenProtocol);
		
		$VERIFY_CONN = $ClientCall -> verify_connection_string($CONN_STRING);
		return $VERIFY_CONN;
	}
	
	###########################
	# IS_SNAPSHOT_VHD_DISK_READY
	###########################
	public function is_snapshot_vhd_disk_ready($TASK_ID)
	{
		$PORT_NUM = \saasame\transport\Constant::get('TRANSPORT_SERVICE_PORT');
		$OpenProtocol = $this -> common_connection('127.0.0.1', $PORT_NUM);
		$ClientCall = new saasame\transport\transport_serviceClient($OpenProtocol);
		
		$CHECK_TASK = $ClientCall -> is_snapshot_vhd_disk_ready($TASK_ID);
		return $CHECK_TASK;
	}
	
	##########################
	# CHECK SECURITY CODE
	##########################
	public function check_security_code($INPUT_STRING)
	{
		if (strlen($INPUT_STRING) == 8)
		{
			$GET_EXEC = "SELECT * FROM _ACCOUNT WHERE _ACCT_DATA LIKE '%".$INPUT_STRING."%' AND _STATUS = 'Y' LIMIT 1";
			$QUERY = $this -> DBCON -> prepare($GET_EXEC);
			$QUERY -> execute();
			$COUNT_ROWS = $QUERY -> rowCount();
					
			if ($COUNT_ROWS == 1)
			{
				$QueryResult = $QUERY->fetchAll(PDO::FETCH_ASSOC);
				return (object)array('ACCT_UUID' => $QueryResult[0]['_ACCT_UUID'], 'REGN_UUID' => json_decode($QueryResult[0]['_ACCT_DATA']) -> region);	
			}
			else
			{
				return false;
			}
		}
		else
		{
			return false;
		}
	}
	
	##########################
	# GENERATE DATEMODE AGENT
	##########################
	public function generate_datemode_agent($AGENT_PATH,$INPUT_STRING,$OS_TYPE)
	{
		#DEFINE AGENT FILE NAME
		$AGENT_FILE_NAME = 'datemode_agent_'.date('Ymd_Hi_s').'.zip';
	
		#NEW PHP ZIP ARCHIVE CLASS
		$zip = new ZipArchive();

		#CREATE PHP ZIP ARCHIVE FILE INTO MEMORY
		$zip -> open($AGENT_PATH.'\\'.$AGENT_FILE_NAME, ZipArchive::CREATE);
		
		$zip -> addFromString("agent.cfg",$INPUT_STRING);
		if ($OS_TYPE == 'WINDOWS')
		{
			$zip -> addFile($AGENT_PATH.'\irm_agent.exe','irm_agent.exe');
		}
		else
		{
			$zip -> addFile($AGENT_PATH.'\irm_agent','irm_agent');
		}
		$zip -> close();

		return $AGENT_FILE_NAME;
	}	
}