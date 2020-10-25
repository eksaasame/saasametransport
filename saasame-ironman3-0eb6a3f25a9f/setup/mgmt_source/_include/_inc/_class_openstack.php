<?php
###########################
#
# OpenStack Server Class
#
###########################
class OpenStack_Server_Class
{
	###########################
	# Construct Function
	###########################
	protected $OpenStackQuery;
	public function __construct()
	{
		$this -> OpenStackQuery  = new OpenStack_Query_Class();
	}
	
	###########################
	# Execute Restful API
	###########################	
	private function ExecuteRestfulApi($REQUEST_INFO)
	{
		$EXE_CURL = curl_init($REQUEST_INFO -> RESTFUL_ADDRESS);
		curl_setopt($EXE_CURL, CURLOPT_CUSTOMREQUEST, $REQUEST_INFO -> REQUEST_METHOD);

		curl_setopt($EXE_CURL, CURLOPT_SSL_VERIFYHOST, false);
		curl_setopt($EXE_CURL, CURLOPT_SSL_VERIFYPEER, false);
		curl_setopt($EXE_CURL, CURLOPT_RETURNTRANSFER, true);
		curl_setopt($EXE_CURL, CURLOPT_VERBOSE, false);
		if (strpos($REQUEST_INFO -> REQUEST_NAME, 'GenerateAuthToken') !== FALSE)
		{
			curl_setopt($EXE_CURL, CURLOPT_HEADER, true);
		}
		
		curl_setopt($EXE_CURL, CURLOPT_POSTFIELDS, $REQUEST_INFO -> REQUEST_DATA);

		$SetHeader = array();
		$SetHeader[] = 'X-AUTH-TOKEN:'.$REQUEST_INFO -> AUTH_TOKEN;
		if ($REQUEST_INFO -> REQUEST_NAME == 'UploadImage')
		{
			$SetHeader[] = 'Content-Type: application/octet-stream';
			
			$FILE_NAME_WITH_PATH = dirname(__FILE__).'\_qcow2\vMotionSystemDisk.qcow2';
			$FILE_POST = array('file' => new CurlFile($FILE_NAME_WITH_PATH,'application/octet-stream'));
			curl_setopt($EXE_CURL, CURLOPT_POST, true);
			curl_setopt($EXE_CURL, CURLOPT_POSTFIELDS, $FILE_POST);
			curl_setopt($EXE_CURL, CURLOPT_RETURNTRANSFER, true);
		}
		else
		{
			$SetHeader[] = 'Content-Type: application/json';
		}
		curl_setopt($EXE_CURL, CURLOPT_HTTPHEADER, $SetHeader);
		
		$RESP_MESG   = curl_exec($EXE_CURL);
		$HTTP_CODE   = curl_getinfo($EXE_CURL, CURLINFO_HTTP_CODE);
		curl_close($EXE_CURL);
	
		$DEBUG_LEVEL = Misc_Class::define_mgmt_setting() -> openstack -> debug_level;
	
		#FOR GenerateAuthToken
		if (strpos($REQUEST_INFO -> REQUEST_NAME, 'GenerateAuthToken') !== FALSE)
		{	
			if ($HTTP_CODE == 200 OR $HTTP_CODE == 201)
			{
				$CURL_HEADER = $this -> get_headers_from_curl_response($RESP_MESG);
			
				if (isset($CURL_HEADER['X-Subject-Token']))
				{
					$CURL_BODY  = str_replace(' ','',explode(PHP_EOL, $RESP_MESG));
					$AUTH_TOKEN  = $CURL_HEADER['X-Subject-Token'];
					$ENDPOINT_REF = json_decode(end($CURL_BODY)) -> token -> catalog;
					$DOMAIN_ID = json_decode(end($CURL_BODY)) -> token -> user -> domain -> id;
				}
				else
				{				
					$CURL_HEADER  = str_replace(' ','',explode(PHP_EOL, $RESP_MESG));
					$AUTH_TOKEN  = json_decode(end($CURL_HEADER)) -> access -> token -> id;
					$ENDPOINT_REF = json_decode(end($CURL_HEADER)) -> access -> serviceCatalog;
					$DOMAIN_ID = false;
				}
				
				if ($DEBUG_LEVEL == 1)
				{
					Misc_Class::openstack_debug('OpenStackDebug',$REQUEST_INFO,json_encode($RESP_MESG));
				}
				
				return (object)array('AUTH_TOKEN' => $AUTH_TOKEN, 'AUTH_ADDR' => $REQUEST_INFO -> RESTFUL_ADDRESS, 'ENDPOINT_REF' => $ENDPOINT_REF, 'DOMAIN_ID' => $DOMAIN_ID);
			}
			else
			{				
				Misc_Class::openstack_debug('OpenStackDebug',$REQUEST_INFO,json_encode($RESP_MESG));
				return $HTTP_CODE;
			}			
		}
		else
		{
			#DEBUG
			if ($DEBUG_LEVEL == 0)
			{
				if ($HTTP_CODE == 400 OR $HTTP_CODE == 401 OR $HTTP_CODE == 403 OR $HTTP_CODE == 404 OR $HTTP_CODE == 409 OR $HTTP_CODE == 412 OR $HTTP_CODE == 413 OR $HTTP_CODE == 500 OR $HTTP_CODE == 503)
				#if ($HTTP_CODE != 200 OR $HTTP_CODE != 201 OR $HTTP_CODE != 202)
				{
					#ADD HTTP RETURN CODE TO REQUEST
					$REQUEST_INFO -> HTTP_CODE = $HTTP_CODE;
					Misc_Class::openstack_debug('OpenStackDebug',$REQUEST_INFO,$RESP_MESG);
					$RESP_MESG = json_encode(array('error' => $RESP_MESG, 'response_code' => $HTTP_CODE));
				}
			}
			else
			{			
				Misc_Class::openstack_debug('OpenStackDebug',$REQUEST_INFO,$RESP_MESG);
			}
			return json_decode($RESP_MESG,false);
		}
	}
	
	###########################
	# Get Headers from CURL resposne
	###########################
	private function get_headers_from_curl_response($response)
	{
		$headers = array();
		$header_text = substr($response, 0, strpos($response, "\r\n\r\n"));
		
		foreach (explode("\r\n", $header_text) as $i => $line)
		{
			if ($i === 0)
			{
				$headers['http_code'] = $line;
			}
			else
			{
				if (strpos($line, 'X-Subject-Token') !== FALSE)
				{
					list ($key, $value) = explode(': ', $line);
					$headers[$key] = $value;
				}
			}
		}
		return $headers;
	}

	###########################
	# Validate Auth Token
	###########################
	public function ValidateAuthToken($CLUSTER_UUID)
	{
		$OpenStackInfo = $this -> OpenStackQuery -> query_openstack_connection_information($CLUSTER_UUID);
	
		if ($OpenStackInfo != FALSE)
		{
			$PROJECT_ID   		 = $OpenStackInfo['PROJECT_ID'];
			$TENANT_NAME  		 = $OpenStackInfo['PROJECT_NAME'];
			$USER_NAME 	  		 = $OpenStackInfo['CLUSTER_USER'];
			$PASS_WORD    		 = $OpenStackInfo['CLUSTER_PASS'];
			$AUTH_TOKEN	  		 = $OpenStackInfo['AUTH_TOKEN'];
			$VENDOR_NAME  		 = $OpenStackInfo['VENDOR_NAME'];
			$IDENTITY_PROTOCOL	 = $OpenStackInfo['IDENTITY_PROTOCOL'];
			$VIP_ADDRESS  		 = $OpenStackInfo['CLUSTER_ADDR'];
			$IDENTITY_PORT		 = $OpenStackInfo['IDENTITY_PORT'];
			$AUTH_PROJECT_ID	 = $OpenStackInfo['AUTH_PROJECT_ID'];
			$AUTH_PROJECT_REGION = $OpenStackInfo['AUTH_PROJECT_REGION'];
		
			$VALIDATE_ADDRESS = $IDENTITY_PROTOCOL.'://'.$VIP_ADDRESS.':'.$IDENTITY_PORT;
			
			$RESTFUL_PARAMETERS = $this -> RestfulParser('ValidateToken',$AUTH_TOKEN,null,$VALIDATE_ADDRESS);
			
			$REQUEST_INFO = (object)array(
											'REQUEST_NAME'		=> $RESTFUL_PARAMETERS -> REQUEST_NAME,
											'AUTH_TOKEN'   		=> $AUTH_TOKEN, 
											'REQUEST_METHOD' 	=> $RESTFUL_PARAMETERS -> REQUEST_METHOD,
											'RESTFUL_ADDRESS'	=> $RESTFUL_PARAMETERS -> RESTFUL_ADDRESS,
											'REQUEST_DATA' 		=> null);
			#CHECK OR GEN NEW TOKEN
			$REQUEST_RESPONSE = $this -> ExecuteRestfulApi($REQUEST_INFO);
	
			if (isset($REQUEST_RESPONSE -> error) || isset($REQUEST_RESPONSE -> Message))
			{
				$AUTH_DATA = (object)array(
									'projectId' => $PROJECT_ID,
									'domain'    => $TENANT_NAME,
									'protocol'  => $IDENTITY_PROTOCOL,
									'address'	=> $VIP_ADDRESS,
									'port'		=> $IDENTITY_PORT,
									'username'  => $USER_NAME,
									'password'  => $PASS_WORD,
									'endpoints' => true);
				
				$NEW_AUTH = $this -> RestfulOperation('','GenerateAuthToken','','','',$AUTH_DATA);
			
				if (!isset($NEW_AUTH -> AUTH_TOKEN))
				{
					$NEW_AUTH = $this -> RestfulOperation('','GenerateAuthTokenV3','','','',$AUTH_DATA);
				}
				
				#NEW AUTH_TOKEN
				$NEW_AUTH_TOKEN = $NEW_AUTH -> AUTH_TOKEN;
				
				#REF_URL_INFO
				#$ENDPOINT_REF = $NEW_AUTH -> ENDPOINT_REF;
				
				#UPDATE ENDPOINT REFERENCE 
				#$this -> GenerateEndPointReference($CLUSTER_UUID,$ENDPOINT_REF,'New');

				#UPDATE TOKEN INFOMATION 				
				$NEW_AUTH_DATA = array('auth_token' => $NEW_AUTH_TOKEN, 'project_id' => $AUTH_PROJECT_ID, 'vendor_name' => $VENDOR_NAME, 'project_region' => $AUTH_PROJECT_REGION, 'identity_protocol' => $IDENTITY_PROTOCOL, 'identity_port' => $IDENTITY_PORT);
				
				$this -> OpenStackQuery -> update_openstack_auth_token_info($CLUSTER_UUID,$NEW_AUTH_DATA);
				
				return $NEW_AUTH_TOKEN;	
			}
			else
			{
				return $AUTH_TOKEN;
			}
		}
		else
		{
			return false;
		}
	}
	
	###########################
	# Format EndPoint Reference
	###########################
	public function QueryEndPointReference($CLOUD_UUID,$ENDPOINT_REF)
	{
		$REF_INFO = '';
		if ($CLOUD_UUID != false)
		{
			#GET OUTPUT FILE LOCATION
			$REF_FILE = $_SERVER['DOCUMENT_ROOT'].'/_include/_inc/_endpoints/'.$CLOUD_UUID.'.ref';

			if (!file_exists($REF_FILE) OR filesize($REF_FILE) == 0)
			{
				$AUTH_TOKEN = $this -> ValidateAuthToken($CLOUD_UUID);
				$this -> GetServiceEndPointURL($CLOUD_UUID,$AUTH_TOKEN,null);
			}		

			$REF_INFO = file_get_contents($REF_FILE);
			#$REF_INFO = file($REF_FILE, FILE_IGNORE_NEW_LINES|FILE_SKIP_EMPTY_LINES);
		}
		else
		{
			#ENDPOINT INTERFACE
			$DEFINE_INTERFACE = Misc_Class::define_mgmt_setting() -> openstack -> endpoint_interface;
	
			for ($i=0; $i<count($ENDPOINT_REF); $i++)
			{
				$TYPE = $ENDPOINT_REF[$i] -> type;
				
				if (isset($ENDPOINT_REF[$i] -> endpoints[0] -> publicURL))
				{
					$ADMIN_URL = $ENDPOINT_REF[$i] -> endpoints[0] -> publicURL;
				}
				else
				{
					for ($x=0; $x<count($ENDPOINT_REF[$i] -> endpoints); $x++)
					{
						if ($ENDPOINT_REF[$i] -> endpoints[$x] -> interface == $DEFINE_INTERFACE)
						{
							$ADMIN_URL = $ENDPOINT_REF[$i] -> endpoints[$x] -> url;
							break;
						}
						else
						{
							$ADMIN_URL = $ENDPOINT_REF[$i] -> endpoints[0] -> url;
						}
					}
				}
				$REF_INFO .= $TYPE.",".$ADMIN_URL."\n";
			}		
			
			/*for ($i=0; $i<count($ENDPOINT_REF); $i++)
			{
				$TYPE = $ENDPOINT_REF[$i] -> type;
					
				if (isset($ENDPOINT_REF[$i] -> endpoints[0] -> publicURL))
				{
					$ADMIN_URL = $ENDPOINT_REF[$i] -> endpoints[0] -> publicURL;
				}
				else
				{
					$ADMIN_URL = $ENDPOINT_REF[$i] -> endpoints[0] -> url;
				}
				
				$REF_INFO .= $TYPE.",".$ADMIN_URL."\n";		
			}*/
		}
		
		$REF_INFO = strip_tags(json_encode( array_values(array_filter(explode("\n",$REF_INFO)))));
		
		return $REF_INFO;
	}
	
	
	###########################
	# Generate EndPoint Reference
	###########################
	public function GenerateEndPointReference($CLUSTER_UUID,$ENDPOINT_REF,$REGEN_TYPE)
	{
		#DEFINE OUTPUT FILE LOCATION
		$file = $_SERVER['DOCUMENT_ROOT'].'/_include/_inc/_endpoints/'.$CLUSTER_UUID.'.ref';
	
		#EMPTY FILE CONTENT
		file_put_contents($file, '');
				
		$FILE_HANDLE = fopen($file,'w');		
		
		if ($REGEN_TYPE == 'New')
		{
			//@unlink($REF_FILE);
			
			#ENDPOINT INTERFACE
			$DEFINE_INTERFACE = Misc_Class::define_mgmt_setting() -> openstack -> endpoint_interface;
	
			for ($i=0; $i<count($ENDPOINT_REF); $i++)
			{
				$TYPE = $ENDPOINT_REF[$i] -> type;
			
				if (isset($ENDPOINT_REF[$i] -> endpoints[0] -> publicURL))
				{
					$ADMIN_URL = $ENDPOINT_REF[$i] -> endpoints[0] -> publicURL;
				}
				else
				{
					for ($x=0; $x<count($ENDPOINT_REF[$i] -> endpoints); $x++)
					{
						if ($ENDPOINT_REF[$i] -> endpoints[$x] -> interface == $DEFINE_INTERFACE)
						{
							$ADMIN_URL = $ENDPOINT_REF[$i] -> endpoints[$x] -> url;
							break;
						}
						else
						{
							$ADMIN_URL = $ENDPOINT_REF[$i] -> endpoints[0] -> url;
							break;
						}
					}
				}
				
				$REF_PATH = $TYPE.",".$ADMIN_URL."\n";
				
				$current  = file_get_contents($file);
				$current .= $REF_PATH;
				file_put_contents($file, strip_tags($current));
			}			
		}
		elseif ($REGEN_TYPE == 'JSON')
		{
			$ENDPOINT_REF = json_decode($ENDPOINT_REF);
			for ($i=0; $i<count($ENDPOINT_REF); $i++)
			{
				$current  = file_get_contents($file);
				$current .= $ENDPOINT_REF[$i]."\n";
				file_put_contents($file, strip_tags($current));
			}
		}
		else
		{
			foreach ($ENDPOINT_REF as $SERVICE_TYPE => $SERVICE_URL)
			{
				$REF_PATH  = $SERVICE_TYPE.",".$SERVICE_URL."\n";			
							
				$current  = file_get_contents($file);
				$current .= $REF_PATH;
				file_put_contents($file, strip_tags($current));
			}
		}
		
		fclose($FILE_HANDLE);
	}
	
	###########################
	# Get Service EndPoint URL
	###########################
	public function GetServiceEndPointURL($CLUSTER_UUID,$AUTH_TOKEN,$SERV_TYPE)
	{
		$ENDPOINT_FILE = $_SERVER['DOCUMENT_ROOT'].'/_include/_inc/_endpoints/'.$CLUSTER_UUID.'.ref';
		
		if(!file_exists($ENDPOINT_FILE) OR filesize($ENDPOINT_FILE) == 0)
		{		
			$OpenStackInfo = $this -> OpenStackQuery -> query_openstack_connection_information($CLUSTER_UUID);
			$PROJECT_ID 		= $OpenStackInfo['PROJECT_ID'];
			$IDENTITY_PROTOCOL 	= $OpenStackInfo['IDENTITY_PROTOCOL'];
			$VIP_ADDRESS 		= $OpenStackInfo['CLUSTER_ADDR'];
			$IDENTITY_PORT 		= $OpenStackInfo['IDENTITY_PORT'];
			
			$IDENTITY_ADDRESS 	= $IDENTITY_PROTOCOL.'://'.$VIP_ADDRESS.':'.$IDENTITY_PORT;
			
			#TRY API_V2 FIRST
			$LIST_ENDPOINT = $this -> RestfulParser('ListEndpointsForToken',$AUTH_TOKEN,null,$IDENTITY_ADDRESS);			
			$ENDPOINT_INFO = (object)array(
										  'REQUEST_NAME'	=> $LIST_ENDPOINT -> REQUEST_NAME,
										  'AUTH_TOKEN'   	=> $AUTH_TOKEN, 
										  'REQUEST_METHOD' 	=> $LIST_ENDPOINT -> REQUEST_METHOD,
										  'RESTFUL_ADDRESS'	=> $LIST_ENDPOINT -> RESTFUL_ADDRESS,
										  'REQUEST_DATA' 	=> null);
			
			$ENDPOINT_LIST = $this -> ExecuteRestfulApi($ENDPOINT_INFO);
			if (isset($ENDPOINT_LIST -> endpoints))
			{			
				for ($x=0; $x<count($ENDPOINT_LIST -> endpoints); $x++)
				{
					$REGION_ENDPOINT[$ENDPOINT_LIST -> endpoints[$x] -> type] = $ENDPOINT_LIST -> endpoints[$x] -> publicURL;
				}
			}
			
			#TRY API_V3
			if (!isset($REGION_ENDPOINT))
			{
				$LIST_SERVICES = $this -> RestfulParser('ListServicesV3',$AUTH_TOKEN,null,$IDENTITY_ADDRESS);
				$SERVICE_INFO = (object)array(
											  'REQUEST_NAME'	=> $LIST_SERVICES -> REQUEST_NAME,
											  'AUTH_TOKEN'   	=> $AUTH_TOKEN, 
											  'REQUEST_METHOD' 	=> $LIST_SERVICES -> REQUEST_METHOD,
											  'RESTFUL_ADDRESS'	=> $LIST_SERVICES -> RESTFUL_ADDRESS,
											  'REQUEST_DATA' 	=> null);
											  
				$LIST_SERVICES_RESPONSE = $this -> ExecuteRestfulApi($SERVICE_INFO);

				$SERVICE_REGION = explode(".",parse_url($LIST_SERVICES_RESPONSE -> links -> self)['host'])[1];
				$SERVICE_LIST = $LIST_SERVICES_RESPONSE -> services;
			
				$LIST_ENDPOINT = $this -> RestfulParser('ListEndpointsForTokenV3',$AUTH_TOKEN,null,$IDENTITY_ADDRESS);
			
				$ENDPOINT_INFO = (object)array(
											  'REQUEST_NAME'	=> $LIST_ENDPOINT -> REQUEST_NAME,
											  'AUTH_TOKEN'   	=> $AUTH_TOKEN, 
											  'REQUEST_METHOD' 	=> $LIST_ENDPOINT -> REQUEST_METHOD,
											  'RESTFUL_ADDRESS'	=> $LIST_ENDPOINT -> RESTFUL_ADDRESS,
											  'REQUEST_DATA' 	=> null);
				
				$ENDPOINT_LIST = $this -> ExecuteRestfulApi($ENDPOINT_INFO) -> endpoints;
				
				for ($i=0; $i<count($SERVICE_LIST); $i++)
				{
					for ($w=0; $w<count($ENDPOINT_LIST); $w++)
					{
						if ($SERVICE_LIST[$i] -> id == $ENDPOINT_LIST[$w] -> service_id AND ((!isset($ENDPOINT_LIST[$w] -> region_id)) OR $SERVICE_REGION == $ENDPOINT_LIST[$w] -> region_id))
						{
							$REGION_ENDPOINT[$SERVICE_LIST[$i] -> type] = str_replace('$(tenant_id)s',$PROJECT_ID,$ENDPOINT_LIST[$w] -> url);
						}		
					}
				}				
			}
			
			if (isset($REGION_ENDPOINT))
			{
				$this -> GenerateEndPointReference($CLUSTER_UUID,$REGION_ENDPOINT,'ReNew');
				if ($SERV_TYPE != null){return $REGION_ENDPOINT[$SERV_TYPE];}
			}
			else
			{
				return false;
			}			
		}
		else
		{			
			$READ_FILE = file($ENDPOINT_FILE,FILE_IGNORE_NEW_LINES);
			
			for ($r=0; $r<count($READ_FILE); $r++)
			{
				$URL_INFO = explode(',',$READ_FILE[$r]);
				$ENDPOINT_TYPE = strtolower($URL_INFO[0]);
				$ENDPOINT_TYPE = str_replace(' ','',$ENDPOINT_TYPE);
			
				if ($SERV_TYPE == $ENDPOINT_TYPE)
				{
					return $URL_INFO[1];
					break;
				}
			}
			return false;
		}
	}
		
	#############################################
	# Parser Require Restful URL And Method
	#############################################
	private function RestfulParser($METHOD_NAME,$SERV_UUID=null,$ATTH_UUID=null,$END_POINT_URL=null)
	{
		switch ($METHOD_NAME)
		{
			##################################
			# List Projects
			##################################
			case 'ListProject':
				if ($SERV_UUID == '_default') #DISABLE FOR DEFAULT PROJECT
				{
					$URL = $END_POINT_URL.'/v3/projects';
				}
				elseif($SERV_UUID != false)
				{
					$URL = $END_POINT_URL.'/v3/auth/projects?domain_id='.$SERV_UUID;
				}
				else
				{			
					$URL = $END_POINT_URL.'/v3/auth/projects';
				}
				
				$PARAMETERS = (object)array('REQUEST_NAME'	  => $METHOD_NAME,
											'RESTFUL_ADDRESS' => $URL,
											'REQUEST_METHOD'  => 'GET');
				
				return $PARAMETERS;
			break;
			
			##################################
			# Show Proejct Detail
			##################################
			case 'ShowProjectDetail':
				$URL = $END_POINT_URL.'/v3/projects/'.$SERV_UUID;
						
				$PARAMETERS = (object)array('REQUEST_NAME'	  => $METHOD_NAME,
											'RESTFUL_ADDRESS' => $URL,
											'REQUEST_METHOD'  => 'GET');
				return $PARAMETERS;
			break;
			
			##################################
			# List Versions
			##################################
			case 'ListVersions':
				$URL = $END_POINT_URL.'/';
				
				$PARAMETERS = (object)array('REQUEST_NAME'	  => $METHOD_NAME,
											'RESTFUL_ADDRESS' => $URL,
											'REQUEST_METHOD'  => 'GET');
				
				return $PARAMETERS;
			break;
						
			##################################
			# Show version details
			##################################
			case 'ShowVersionDetails':
				$URL = $END_POINT_URL.'/v3';			
				
				$PARAMETERS = (object)array('REQUEST_NAME'	  => $METHOD_NAME,
											'RESTFUL_ADDRESS' => $URL,
											'REQUEST_METHOD'  => 'GET');
				
				return $PARAMETERS;
			break;
			
			##################################
			# Generate Auth Token
			##################################
			case 'ValidateToken':
				$URL = $END_POINT_URL.'/v3/auth/domains';
							
				$PARAMETERS = (object)array('REQUEST_NAME'	  => $METHOD_NAME,
											'RESTFUL_ADDRESS' => $URL,
											'REQUEST_METHOD'  => 'GET');
				
				return $PARAMETERS;				
			break;			
			
			case 'ValidateTokenV3':
				$URL = $END_POINT_URL.'/v3/projects';
				
				$PARAMETERS = (object)array('REQUEST_NAME'	  => $METHOD_NAME,
											'RESTFUL_ADDRESS' => $URL,
											'REQUEST_METHOD'  => 'HEAD');
				
				return $PARAMETERS;				
			break;
			
			case 'GenerateAuthToken':
				$URL = $END_POINT_URL.'/v2.0/tokens';
			
				$PARAMETERS = (object)array('REQUEST_NAME'	  => $METHOD_NAME,
											'RESTFUL_ADDRESS' => $URL,
											'REQUEST_METHOD'  => 'POST');
				
				return $PARAMETERS;
			break;
			
			case 'GenerateAuthTokenV3':
				$URL = $END_POINT_URL.'/v3/auth/tokens';
				
				$PARAMETERS = (object)array('REQUEST_NAME'	  => $METHOD_NAME,
											'RESTFUL_ADDRESS' => $URL,
											'REQUEST_METHOD'  => 'POST');
				
				return $PARAMETERS;
			break;
			
			case 'GetAvailableProjectScopes':
				$URL = $END_POINT_URL.'/v3/auth/projects';
				
				$PARAMETERS = (object)array('REQUEST_NAME'	  => $METHOD_NAME,
											'RESTFUL_ADDRESS' => $URL,
											'REQUEST_METHOD'  => 'GET');
				
				return $PARAMETERS;				
			break;
			
			##################################
			# Regions
			##################################
			case 'ListRegions':
				$URL = $END_POINT_URL.'/v3/regions';
				
				$PARAMETERS = (object)array('REQUEST_NAME'	  => $METHOD_NAME,
											'RESTFUL_ADDRESS' => $URL,
											'REQUEST_METHOD'  => 'GET');
				
				return $PARAMETERS;
			break;
		
			##################################
			# Endoints For Token
			##################################
			case "ListEndpointsForToken":
				$URL = $END_POINT_URL.'/v2.0/tokens/'.$SERV_UUID.'/endpoints';
							
				$PARAMETERS = (object)array('REQUEST_NAME'	  => $METHOD_NAME,
											'RESTFUL_ADDRESS' => $URL,
											'REQUEST_METHOD'  => 'GET');
				
				return $PARAMETERS;
			break;
			
			
			case "ListEndpointsForTokenV3":
				$URL = $END_POINT_URL.'/v3/endpoints';
			
				$PARAMETERS = (object)array('REQUEST_NAME'	  => $METHOD_NAME,
											'RESTFUL_ADDRESS' => $URL,
											'REQUEST_METHOD'  => 'GET');
				
				return $PARAMETERS;
			break;
			
			
			case "ListServicesV3":
				$URL = $END_POINT_URL.'/v3/services';
			
				$PARAMETERS = (object)array('REQUEST_NAME'	  => $METHOD_NAME,
											'RESTFUL_ADDRESS' => $URL,
											'REQUEST_METHOD'  => 'GET');
				
				return $PARAMETERS;
			break;
			
			
			
			##################################
			# Keystone Identity Information API
			##################################
			case 'ListTenants':
				$URL = $END_POINT_URL.'/tenants';
								
				$PARAMETERS = (object)array('REQUEST_NAME'	  => $METHOD_NAME,
											'RESTFUL_ADDRESS' => $URL,
											'REQUEST_METHOD'  => 'GET');
				
				return $PARAMETERS;
			break;
			
			
			##################################
			# Cinder Volume API
			##################################
			
			#Cinder Volume Query#
			case 'GetVolumeList':
				$URL = $END_POINT_URL.'/volumes';
				
				$PARAMETERS = (object)array('REQUEST_NAME'	  => $METHOD_NAME,
											'RESTFUL_ADDRESS' => $URL,
											'REQUEST_METHOD'  => 'GET');
				
				return $PARAMETERS;
			break;
			
			case 'GetVolumeDetailList':
				$URL = $END_POINT_URL.'/volumes/detail';
				
				$PARAMETERS = (object)array('REQUEST_NAME'	  => $METHOD_NAME,
											'RESTFUL_ADDRESS' => $URL,
											'REQUEST_METHOD'  => 'GET');
				
				return $PARAMETERS;
			break;
			
			case 'GetVolumeDetailInfo':
				$URL = $END_POINT_URL.'/volumes/'.$SERV_UUID;
				
				$PARAMETERS = (object)array('REQUEST_NAME'	  => $METHOD_NAME,
											'RESTFUL_ADDRESS' => $URL,
											'REQUEST_METHOD'  => 'GET');
				
				return $PARAMETERS;
			break;
			#Cinder Volume Query#
			
			
			#Cinder Snapshot Query#
			case 'GetSnapshotList':
				$URL = $END_POINT_URL.'/snapshots';
				
				$PARAMETERS = (object)array('REQUEST_NAME'	  => $METHOD_NAME,
											'RESTFUL_ADDRESS' => $URL,
											'REQUEST_METHOD'  => 'GET');
				
				return $PARAMETERS;
			break;
			
			case 'GetSnapshotDetailInfo':
				$URL = $END_POINT_URL.'/snapshots/'.$SERV_UUID;
				
				$PARAMETERS = (object)array('REQUEST_NAME'	  => $METHOD_NAME,
											'RESTFUL_ADDRESS' => $URL,
											'REQUEST_METHOD'  => 'GET');
				
				return $PARAMETERS;
			break;
			#Cinder Snapshot Query#

			case 'GetVolumeTypeList':
				$URL = $END_POINT_URL.'/types';
				
				$PARAMETERS = (object)array('REQUEST_NAME'	  => $METHOD_NAME,
											'RESTFUL_ADDRESS' => $URL,
											'REQUEST_METHOD'  => 'GET');
				
				return $PARAMETERS;
			break;
						
			#Cinder Volume Operation#
			case 'CreateVolume':
				$URL = $END_POINT_URL.'/volumes';
				
				$PARAMETERS = (object)array('REQUEST_NAME'	  => $METHOD_NAME,
											'RESTFUL_ADDRESS' => $URL,
											'REQUEST_METHOD'  => 'POST');
				
				return $PARAMETERS;
			break;
			
			case 'DeleteVolume':
				$URL = $END_POINT_URL.'/volumes/'.$SERV_UUID;
				
				$PARAMETERS = (object)array('REQUEST_NAME'	  => $METHOD_NAME,
											'RESTFUL_ADDRESS' => $URL,
											'REQUEST_METHOD'  => 'DELETE');
				
				return $PARAMETERS;
			break;
			
			case 'SetBootable':
				$URL = $END_POINT_URL.'/volumes/'.$SERV_UUID.'/action';
				
				$PARAMETERS = (object)array('REQUEST_NAME'	  => $METHOD_NAME,
											'RESTFUL_ADDRESS' => $URL,
											'REQUEST_METHOD'  => 'POST');
				
				return $PARAMETERS;
			break;
			#Cinder Volume Operation#
			
			
			#Cinder Snapshot Operation#
			case 'CreateSnapshot':
				$URL = $END_POINT_URL.'/snapshots';
				
				$PARAMETERS = (object)array('REQUEST_NAME'	  => $METHOD_NAME,
											'RESTFUL_ADDRESS' => $URL,
											'REQUEST_METHOD'  => 'POST');
				
				return $PARAMETERS;
			break;
		
			case 'DeleteSnapshot':
				$URL = $END_POINT_URL.'/snapshots/'.$SERV_UUID;
				
				$PARAMETERS = (object)array('REQUEST_NAME'	  => $METHOD_NAME,
											'RESTFUL_ADDRESS' => $URL,
											'REQUEST_METHOD'  => 'DELETE');
				
				return $PARAMETERS;
			break;
			
			case 'CreateSnapshotMetadata':
				$URL = $END_POINT_URL.'/snapshots/'.$SERV_UUID.'/metadata';
				
				$PARAMETERS = (object)array('REQUEST_NAME'	  => $METHOD_NAME,
											'RESTFUL_ADDRESS' => $URL,
											'REQUEST_METHOD'  => 'POST');
				
				return $PARAMETERS;
			break;
			#Cinder Snapshot Operation#
			
			
			##################################
			# Glance Image API
			##################################
			case 'GetImageList':
				$URL = $END_POINT_URL.'/v2/images';
				
				$PARAMETERS = (object)array('REQUEST_NAME'	  => $METHOD_NAME,
											'RESTFUL_ADDRESS' => $URL,
											'REQUEST_METHOD'  => 'GET');
				
				return $PARAMETERS;
			break;
			
			case 'CreateImage':
				$URL = $END_POINT_URL.'/v2/images';
				
				$PARAMETERS = (object)array('REQUEST_NAME'	  => $METHOD_NAME,
											'RESTFUL_ADDRESS' => $URL,
											'REQUEST_METHOD'  => 'POST');
				
				return $PARAMETERS;
			break;

			case 'GetImage':
				$URL = $END_POINT_URL.'/v2/images/'.$SERV_UUID;
				
				$PARAMETERS = (object)array('REQUEST_NAME'	  => $METHOD_NAME,
											'RESTFUL_ADDRESS' => $URL,
											'REQUEST_METHOD'  => 'GET');
				
				return $PARAMETERS;			
			break;
			
			case 'UploadImage':
				$URL = $END_POINT_URL.'/v2/images/'.$SERV_UUID.'/file';
				
				$PARAMETERS = (object)array('REQUEST_NAME'	  => $METHOD_NAME,
											'RESTFUL_ADDRESS' => $URL,
											'REQUEST_METHOD'  => 'PUT');
				
				return $PARAMETERS;			
			break;
			
			
			##################################
			# Nova Compute API
			##################################
			case 'GetVmDetailList':
				$URL = $END_POINT_URL.'/servers/detail';
				
				$PARAMETERS = (object)array('REQUEST_NAME'	  => $METHOD_NAME,
											'RESTFUL_ADDRESS' => $URL,
											'REQUEST_METHOD'  => 'GET');
				
				return $PARAMETERS;
			break;
			
			case 'GetVmDetailInfo':
				$URL = $END_POINT_URL.'/servers/'.$SERV_UUID;
				
				$PARAMETERS = (object)array('REQUEST_NAME'	  => $METHOD_NAME,
											'RESTFUL_ADDRESS' => $URL,
											'REQUEST_METHOD'  => 'GET');
				
				return $PARAMETERS;
			break;
			
			case 'GetVolumeAttachments':
				$URL = $END_POINT_URL.'/servers/'.$SERV_UUID.'/os-volume_attachments';
				
				$PARAMETERS = (object)array('REQUEST_NAME'	  => $METHOD_NAME,
											'RESTFUL_ADDRESS' => $URL,
											'REQUEST_METHOD'  => 'GET');
				
				return $PARAMETERS;		
			break;
					
			case 'GetFlavorList':
				$URL = $END_POINT_URL.'/flavors/detail';
				
				$PARAMETERS = (object)array('REQUEST_NAME'	  => $METHOD_NAME,
											'RESTFUL_ADDRESS' => $URL,
											'REQUEST_METHOD'  => 'GET');
				
				return $PARAMETERS;
			break;
			
			case 'ListSecurityGroupsByTenant':
				$URL = $END_POINT_URL.'/os-security-groups';
				
				$PARAMETERS = (object)array('REQUEST_NAME'	  => $METHOD_NAME,
											'RESTFUL_ADDRESS' => $URL,
											'REQUEST_METHOD'  => 'GET');
				
				return $PARAMETERS;
			break;
		
			case 'GetFlavorDetailInfo':
				$URL = $END_POINT_URL.'/flavors/'.$SERV_UUID.'';
				
				$PARAMETERS = (object)array('REQUEST_NAME'	  => $METHOD_NAME,
											'RESTFUL_ADDRESS' => $URL,
											'REQUEST_METHOD'  => 'GET');
				
				return $PARAMETERS;
			break;
			
			case 'ShowVmDetails':
				$URL = $END_POINT_URL.'/servers/'.$SERV_UUID;
				
				$PARAMETERS = (object)array('REQUEST_NAME'	  => $METHOD_NAME,
											'RESTFUL_ADDRESS' => $URL,
											'REQUEST_METHOD'  => 'GET');
				
				return $PARAMETERS;
			break;
			
			case 'CreateVm':
				$URL = $END_POINT_URL.'/servers';
				
				$PARAMETERS = (object)array('REQUEST_NAME'	  => $METHOD_NAME,
											'RESTFUL_ADDRESS' => $URL,
											'REQUEST_METHOD'  => 'POST');
				
				return $PARAMETERS;
			break;
			
			case 'StartVm':
				$URL = $END_POINT_URL.'/servers/'.$SERV_UUID.'/action';
				
				$PARAMETERS = (object)array('REQUEST_NAME'	  => $METHOD_NAME,
											'RESTFUL_ADDRESS' => $URL,
											'REQUEST_METHOD'  => 'POST');
				
				return $PARAMETERS;			
			break;
			
			case 'StopVm':
				$URL = $END_POINT_URL.'/servers/'.$SERV_UUID.'/action';
				
				$PARAMETERS = (object)array('REQUEST_NAME'	  => $METHOD_NAME,
											'RESTFUL_ADDRESS' => $URL,
											'REQUEST_METHOD'  => 'POST');
				
				return $PARAMETERS;			
			break;
			
			case 'AttachVolumeToServer':
				$URL = $END_POINT_URL.'/servers/'.$SERV_UUID.'/os-volume_attachments';
				
				$PARAMETERS = (object)array('REQUEST_NAME'	  => $METHOD_NAME,
											'RESTFUL_ADDRESS' => $URL,
											'REQUEST_METHOD'  => 'POST');
				
				return $PARAMETERS;		
			break;
			
			case 'DetachVolumeFromServer':
				$URL = $END_POINT_URL.'/servers/'.$SERV_UUID.'/os-volume_attachments/'.$ATTH_UUID.'';
				
				$PARAMETERS = (object)array('REQUEST_NAME'	  => $METHOD_NAME,
											'RESTFUL_ADDRESS' => $URL,
											'REQUEST_METHOD'  => 'DELETE');
				
				return $PARAMETERS;		
			break;
			
			case 'VolumeAttach':
				$URL = $END_POINT_URL.'/volumes/'.$SERV_UUID.'/action';
				
				$PARAMETERS = (object)array('REQUEST_NAME'	  => $METHOD_NAME,
											'RESTFUL_ADDRESS' => $URL,
											'REQUEST_METHOD'  => 'POST');
				
				return $PARAMETERS;		
			break;
	
			case 'DeleteVmAttachmentVolume':
				$URL = $END_POINT_URL.'/volumes/'.$SERV_UUID.'/action';
			
				$PARAMETERS = (object)array('REQUEST_NAME'	  => $METHOD_NAME,
											'RESTFUL_ADDRESS' => $URL,
											'REQUEST_METHOD'  => 'DELETE');
				
				return $PARAMETERS;			
			break;
			
			case 'DeleteVm':
				$URL = $END_POINT_URL.'/servers/'.$SERV_UUID;
				
				$PARAMETERS = (object)array('REQUEST_NAME'	  => $METHOD_NAME,
											'RESTFUL_ADDRESS' => $URL,
											'REQUEST_METHOD'  => 'DELETE');
				
				return $PARAMETERS;
			break;
			
			case 'ForceDeleteVm':
				$URL = $END_POINT_URL.'/servers/'.$SERV_UUID.'/action';
				
				$PARAMETERS = (object)array('REQUEST_NAME'	  => $METHOD_NAME,
											'RESTFUL_ADDRESS' => $URL,
											'REQUEST_METHOD'  => 'POST');
				
				return $PARAMETERS;
			break;
		
			case 'ListPortInterfaces':
				$URL = $END_POINT_URL.'/servers/'.$SERV_UUID.'/os-interface';
					
				$PARAMETERS = (object)array('REQUEST_NAME'	  => $METHOD_NAME,
											'RESTFUL_ADDRESS' => $URL,
											'REQUEST_METHOD'  => 'GET');
				
				return $PARAMETERS;
			break;
		
			##################################
			# Neutron Network API
			##################################
			case 'GetNetworkList':
				$URL = $END_POINT_URL.'/v2.0/networks';
				
				$PARAMETERS = (object)array('REQUEST_NAME'	  => $METHOD_NAME,
											'RESTFUL_ADDRESS' => $URL,
											'REQUEST_METHOD'  => 'GET');
				
				return $PARAMETERS;
			break;
			
			case 'GetNetworkDetailInfo':
				$URL = $END_POINT_URL.'/v2.0/networks/'.$SERV_UUID.'';
				
				$PARAMETERS = (object)array('REQUEST_NAME'	  => $METHOD_NAME,
											'RESTFUL_ADDRESS' => $URL,
											'REQUEST_METHOD'  => 'GET');
				
				return $PARAMETERS;
			break;
			
			case 'GetSubnetList':
				$URL = $END_POINT_URL.'/v2.0/subnets';
				
				$PARAMETERS = (object)array('REQUEST_NAME'	  => $METHOD_NAME,
											'RESTFUL_ADDRESS' => $URL,
											'REQUEST_METHOD'  => 'GET');
				
				return $PARAMETERS;			
			break;
			
			case 'GetSubnetDetailInfo':
				$URL = $END_POINT_URL.'/v2.0/subnets/'.$SERV_UUID.'';
				
				$PARAMETERS = (object)array('REQUEST_NAME'	  => $METHOD_NAME,
											'RESTFUL_ADDRESS' => $URL,
											'REQUEST_METHOD'  => 'GET');
				
				return $PARAMETERS;			
			break;
			
			case 'GetSecurityGroupList':
				$URL = $END_POINT_URL.'/v2.0/security-groups';
				
				$PARAMETERS = (object)array('REQUEST_NAME'	  => $METHOD_NAME,
											'RESTFUL_ADDRESS' => $URL,
											'REQUEST_METHOD'  => 'GET');
				
				return $PARAMETERS;			
			break;
			
			case 'GetSecurityGroupDetailInfo':
				$URL = $END_POINT_URL.'/v2.0/security-groups/'.$SERV_UUID.'';
				
				$PARAMETERS = (object)array('REQUEST_NAME'	  => $METHOD_NAME,
											'RESTFUL_ADDRESS' => $URL,
											'REQUEST_METHOD'  => 'GET');
				
				return $PARAMETERS;			
			break;
			
			case 'ListFloatingIPs':
				$URL = $END_POINT_URL.'/v2.0/floatingips';
				
				$PARAMETERS = (object)array('REQUEST_NAME'	  => $METHOD_NAME,
											'RESTFUL_ADDRESS' => $URL,
											'REQUEST_METHOD'  => 'GET');
				
				return $PARAMETERS;
			break;
			
			case 'ListPort':
				$URL = $END_POINT_URL.'/v2.0/ports';
				
				$PARAMETERS = (object)array('REQUEST_NAME'	  => $METHOD_NAME,
											'RESTFUL_ADDRESS' => $URL,
											'REQUEST_METHOD'  => 'GET');
				
				return $PARAMETERS;			
			break;
			
			case 'GetFloatingIpDetails':
				$URL = $END_POINT_URL.'/v2.0/floatingips/'.$SERV_UUID;
			
				$PARAMETERS = (object)array('REQUEST_NAME'	  => $METHOD_NAME,
											'RESTFUL_ADDRESS' => $URL,
											'REQUEST_METHOD'  => 'GET');
				return $PARAMETERS;
			break;
			
			case 'UpdateFloatingIP':
				$URL = $END_POINT_URL.'/v2.0/floatingips/'.$SERV_UUID;
			
				$PARAMETERS = (object)array('REQUEST_NAME'	  => $METHOD_NAME,
											'RESTFUL_ADDRESS' => $URL,
											'REQUEST_METHOD'  => 'PUT');
				return $PARAMETERS;
			break;
			
			##################################
			# Guest Configure Network
			##################################
			case 'GuestConfigureNetwork':
				$URL = $END_POINT_URL.'/servers/'.$SERV_UUID.'/action';
				
				$PARAMETERS = (object)array('REQUEST_NAME'	  => $METHOD_NAME,
											'RESTFUL_ADDRESS' => $URL,
											'REQUEST_METHOD'  => 'POST');
				
				return $PARAMETERS;
			break;			
		}
	}
	
	###########################
	# Execute Restful API
	###########################
	public function RestfulOperation($CLUSTER_UUID=null,$METHOD_NAME,$SERV_TYPE=null,$SERV_UUID=null,$ATTH_UUID=null,$INFO_DATA=null)
	{
		if ($METHOD_NAME != '')
		{
			sleep(1);
			if ($METHOD_NAME == 'GenerateAuthToken')
			{
				$END_POINT_URL = $INFO_DATA -> protocol.'://'.$INFO_DATA -> address.':'.$INFO_DATA -> port;
				$AUTH_TOKEN = null;
				
				$INFO_DATA = '{
						"auth": {
								"tenantName": "'.$INFO_DATA -> domain.'",
								"passwordCredentials": {
									"username": "'.$INFO_DATA -> username.'",
									"password": "'.$INFO_DATA -> password.'"
								}
							}
						}';				
			}
			elseif ($METHOD_NAME == 'GenerateAuthTokenV3')
			{
				$END_POINT_URL = $INFO_DATA -> protocol.'://'.$INFO_DATA -> address.':'.$INFO_DATA -> port;
				$AUTH_TOKEN = null;
				
				$AUTH_DATA = array("auth" => 
									array
									(
										"identity" => array
										(
											"methods" => array("password"),
											"password" => array
											(
												"user" => array
												(
													"name" => $INFO_DATA -> username, 
													"password" => $INFO_DATA -> password,
													"domain" => array("name" => $INFO_DATA -> domain)
												)
											)
										)
									)
								);
								
				if ($INFO_DATA -> endpoints == false)
				{
					$AUTH_DATA["auth"]["scope"] = array("domain" => array("name" => $INFO_DATA -> domain));
				}

				if ($INFO_DATA -> projectId != false)
				{
					$AUTH_DATA["auth"]["scope"] = array("project" => array("id" => $INFO_DATA -> projectId));
				}
			
				$INFO_DATA = json_encode($AUTH_DATA);
			}
			elseif ($METHOD_NAME == 'ListRegions' OR $METHOD_NAME == 'ListProject' OR $METHOD_NAME == 'ShowProjectDetail')
			{
				#$END_POINT_URL = $INFO_DATA -> vip_address;
				$END_POINT_URL = $INFO_DATA -> protocol.'://'.$INFO_DATA -> vip_address.':'.$INFO_DATA -> port;
				$AUTH_TOKEN = $INFO_DATA -> AUTH_TOKEN;
				$INFO_DATA = '';
			}
			else
			{
				$AUTH_TOKEN = $this -> ValidateAuthToken($CLUSTER_UUID);				
				$END_POINT_URL = $this -> GetServiceEndPointURL($CLUSTER_UUID,$AUTH_TOKEN,$SERV_TYPE);				
			}
			
			$RESTFUL_PARAMETERS = $this -> RestfulParser($METHOD_NAME,$SERV_UUID,$ATTH_UUID,$END_POINT_URL);
			
			$REQUEST_INFO = (object)array(
										'REQUEST_NAME'		=> $RESTFUL_PARAMETERS -> REQUEST_NAME,
										'AUTH_TOKEN'   		=> $AUTH_TOKEN, 
										'REQUEST_METHOD' 	=> $RESTFUL_PARAMETERS -> REQUEST_METHOD,
										'RESTFUL_ADDRESS'	=> $RESTFUL_PARAMETERS -> RESTFUL_ADDRESS,
										'REQUEST_DATA' 		=> $INFO_DATA
										);
		
			$REQUEST_RESPONSE = $this -> ExecuteRestfulApi($REQUEST_INFO);
			
			return $REQUEST_RESPONSE;
		}
		else
		{
			return false;
		}
	}
}










###########################
#
# OpenStack Query Class
#
###########################
class OpenStack_Query_Class extends Db_Connection
{
	###########################
	# LIST OPENSTACK CONNECTION
	###########################
	public function list_openstack_connection($ACCT_UUID)
	{
		$GET_EXEC 	= "SELECT 
							* FROM _CLOUD_MGMT as TA
						JOIN 
							_SYS_CLOUD_TYPE as TB on TA._CLOUD_TYPE = TB._ID
						WHERE 
							_ACCT_UUID = '".strtoupper($ACCT_UUID)."' AND _STATUS = 'Y'";
		
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
								"REGION"		=> $REGION
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
	# CREATE NEW OPENSTACK CONNECTION
	###########################
	public function create_openstack_connection($ACCT_UUID,$REGN_UUID,$PROJECT_NAME,$CLUSTER_USER,$CLUSTER_PASS,$CLUSTER_ADDR,$AUTH_DATA)
	{
		$CLUSTER_UUID  = Misc_Class::guid_v4();
		
		$INSERT_EXEC = "INSERT 
							INTO _CLOUD_MGMT(
								_ID,
							
								_ACCT_UUID,
								_REGN_UUID,
								_CLUSTER_UUID,
								
								_PROJECT_NAME,
								_CLUSTER_USER,
								_CLUSTER_PASS,								
								_CLUSTER_ADDR,		
								
								_AUTH_TOKEN,
								
								_TIMESTAMP,
								_STATUS)
							VALUE(
								'',
								
								'".$ACCT_UUID."',
								'".$REGN_UUID."',
								'".$CLUSTER_UUID."',
								
								'".$PROJECT_NAME."',								
								'".Misc_Class::encrypt_decrypt('encrypt',$CLUSTER_USER)."',
								'".Misc_Class::encrypt_decrypt('encrypt',$CLUSTER_PASS)."',
								'".$CLUSTER_ADDR."',
								
								'".json_encode($AUTH_DATA)."',
								
								'".Misc_Class::current_utc_time()."',
								'Y')";
		
		$this -> DBCON -> prepare($INSERT_EXEC) -> execute();
		
		return $CLUSTER_UUID;
	}
	
	###########################
	# DELETE SELECT OPENSTACK CONNECTION
	###########################
	public function delete_connection($CLUSTER_UUID)
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
			return true;
		}
		else
		{
			return false;
		}
	}

	###########################
	# QUERY OPENSTACK CONNECTION INFORMATION
	###########################
	public function query_openstack_connection_information($CLUSTER_UUID)
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
				
				if (isset($AUTH_DATA -> identity_protocol)){$PROTOCOL = $AUTH_DATA -> identity_protocol;}else{$PROTOCOL = 'http';}
				if (isset($AUTH_DATA -> identity_port)){$PORT = $AUTH_DATA -> identity_port;}else{$PORT = '5000';}
				
				if (isset($AUTH_DATA -> domain_name)){$AUTH_DOMAIN_NAME = $AUTH_DATA -> domain_name;}else{$AUTH_DOMAIN_NAME = null;}
				if (isset($AUTH_DATA -> project_region)){$AUTH_PROJECT_REGION = $AUTH_DATA -> project_region;}else{$AUTH_PROJECT_REGION = null;}
				if (isset($AUTH_DATA -> project_id)){$AUTH_PROJECT_ID = $AUTH_DATA -> project_id;}else{$AUTH_PROJECT_ID = null;}
				
				$HOTHATCH_DATA = array(
									"ACCT_UUID" 	 		=> $QueryResult['_ACCT_UUID'],
									"REGN_UUID" 	 		=> $QueryResult['_REGN_UUID'],
									"CLUSTER_UUID" 	 		=> $QueryResult['_CLUSTER_UUID'],
										
									"PROJECT_ID" 	 		=> $PROJECT_ID,
									"PROJECT_NAME" 	 		=> $QueryResult['_PROJECT_NAME'],
									
									"CLUSTER_USER" 	 		=> Misc_Class::encrypt_decrypt('decrypt',$QueryResult['_CLUSTER_USER']),
									"CLUSTER_PASS" 	 		=> Misc_Class::encrypt_decrypt('decrypt',$QueryResult['_CLUSTER_PASS']),
									"CLUSTER_ADDR"	 		=> $QueryResult['_CLUSTER_ADDR'],
									"IDENTITY_PROTOCOL" 	=> $PROTOCOL,
									"IDENTITY_PORT"			=> $PORT,
								
									"AUTH_TOKEN" 	 		=> $AUTH_TOKEN,
									"TIMESTAMP"				=> $QueryResult['_TIMESTAMP'],
									"CLOUD_TYPE"	 		=> $QueryResult['_CLOUD_TYPE'],
									"VENDOR_NAME"	 		=> $VENDOR_NAME,
									"AUTH_DOMAIN_NAME"		=> $AUTH_DOMAIN_NAME,
									"AUTH_PROJECT_REGION"	=> $AUTH_PROJECT_REGION,
									"AUTH_PROJECT_ID"		=> $AUTH_PROJECT_ID
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
	# UPDATE OPENSTACK CONNECTION INFO
	###########################
	public function update_openstack_connection($CLUSTER_UUID,$PROJECT_NAME,$CLUSTER_USER,$CLUSTER_PASS,$CLUSTER_ADDR,$AUTH_DATA)
	{
		$UPDATE_EXEC = "UPDATE _CLOUD_MGMT
							SET
								_PROJECT_NAME = '".$PROJECT_NAME."',
								_CLUSTER_USER = '".Misc_Class::encrypt_decrypt('encrypt',$CLUSTER_USER)."',
								_CLUSTER_PASS = '".Misc_Class::encrypt_decrypt('encrypt',$CLUSTER_PASS)."',					
								_CLUSTER_ADDR = '".$CLUSTER_ADDR."',
								_AUTH_TOKEN   = '".json_encode($AUTH_DATA)."',
								_TIMESTAMP	  = '".Misc_Class::current_utc_time()."'		
							WHERE
								_CLUSTER_UUID 	= '".$CLUSTER_UUID."'";
		
		$QUERY = $this -> DBCON -> prepare($UPDATE_EXEC) -> execute();
		return true;
	}
	
	
	###########################
	# UPDATE OPENSTACK AUTH TOKEN INFO
	###########################
	public function update_openstack_auth_token_info($CLUSTER_UUID,$AUTH_TOEKN)
	{
		$TOKEN_UPDATE = "UPDATE _CLOUD_MGMT
							SET
								_AUTH_TOKEN		= '".json_encode($AUTH_TOEKN)."',
								_TIMESTAMP 		= '".Misc_Class::current_utc_time()."'							
							WHERE
								_CLUSTER_UUID 	= '".$CLUSTER_UUID."'";
		
		$QUERY = $this -> DBCON -> prepare($TOKEN_UPDATE) -> execute();
		return true;
	}
	
	###########################
	# QUERY OPENSTACK DISK INFO
	###########################
	public function get_openstack_disk($REPL_UUID)
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
}









###########################
#
# OpenStack Action Class
#
###########################
class OpenStack_Action_Class extends OpenStack_Server_Class
{
	###########################
	# Construct Function
	###########################
	protected $OpenStackQuery;
	public function __construct()
	{
		$this -> OpenStackQuery  = new OpenStack_Query_Class();
	}	
		
	###########################
	# GENERATE AUTH TOKEN
	###########################
	public function generate_auth_token($POST_DATA)
	{
		$AUTH_DATA = (object)array(
									'domain' => $POST_DATA['ProjectName'],
									'protocol' => $POST_DATA['IdentityProtocol'],
									'address' => $POST_DATA['ClusterVipAddr'],
									'port' => $POST_DATA['IdentityPort'],
									'username' => $POST_DATA['ClusterUsername'],
									'password' => $POST_DATA['ClusterPassword'],
									'endpoints' => true
								);
	
		if (!isset($POST_DATA['ProjectId']) OR $POST_DATA['ProjectId'] == '' OR $POST_DATA['ProjectId'] == 'false')
		{
			$AUTH_DATA -> projectId = false;
		}
		else
		{
			$AUTH_DATA -> projectId = $POST_DATA['ProjectId'];
		}
	
		$AUTH_INFO = $this -> RestfulOperation('','GenerateAuthTokenV3','','','',$AUTH_DATA);
		
		if (!is_object($AUTH_INFO))
		{
			$AUTH_INFO = $this -> RestfulOperation('','GenerateAuthToken','','','',$AUTH_DATA);
			$AUTH_INFO -> auth_version = 'v2';
		}
		elseif ($AUTH_INFO -> ENDPOINT_REF == '')
		{
			$AUTH_DATA -> endpoints = false;
			$AUTH_INFO = $this -> RestfulOperation('','GenerateAuthTokenV3','','','',$AUTH_DATA);
			$AUTH_INFO -> auth_version = 'v3';
		}		
		else
		{
			$AUTH_INFO -> auth_version = 'v3';
		}
		
		$AUTH_INFO -> protocol = $POST_DATA['IdentityProtocol'];
		$AUTH_INFO -> vip_address = $POST_DATA['ClusterVipAddr'];
		$AUTH_INFO -> port = $POST_DATA['IdentityPort'];
		
		return $AUTH_INFO;
	}
	
	###########################
	# LIST PROJECT
	###########################
	public function list_projects($AUTH_INFO)
	{
		$PROJECT_LIST = $this -> RestfulOperation('','ListProject','',$AUTH_INFO -> DOMAIN_ID,'',$AUTH_INFO) -> projects;
		
		//$LIST_PROJECT = false;
		
		for($i=0; $i<count($PROJECT_LIST); $i++)
		{
			$LIST_PROJECT[$PROJECT_LIST[$i] -> id] = $PROJECT_LIST[$i] -> name;
		}
		
		return $LIST_PROJECT;		
	}
	
	###########################
	# GENERATE PROJECT REGION
	###########################
	public function generate_project_id($AUTH_INFO,$AUTH_REGION)
	{
		$PROJECT_LIST = $this -> RestfulOperation('','ListProject','',$AUTH_INFO -> DOMAIN_ID,'',$AUTH_INFO) -> projects;
			
		$PROJECT_ID = false;
		for ($i=0; $i<count($PROJECT_LIST); $i++)
		{
			if ($AUTH_REGION == $PROJECT_LIST[$i] -> name)
			{
				$PROJECT_ID = $PROJECT_LIST[$i] -> id;
				break;
			}			
		}		
		return $PROJECT_ID;		
	}	
	
	###########################
	# GENERATE PROJECT REGION
	###########################
	public function query_project_name($AUTH_INFO,$PROJECT_ID)
	{
		$PROJECT_INFO = $this -> RestfulOperation('','ShowProjectDetail','',$PROJECT_ID,'',$AUTH_INFO);
		
		if (isset($PROJECT_INFO -> project -> name))
		{
			$PROJECT_NAME = $PROJECT_INFO -> project -> name;
			return $PROJECT_NAME;
		}
		else
		{
			return false;
		}
	}
	
	###########################
	# LIST REGION INFORMATION
	###########################
	public function list_regions($AUTH_INFO)
	{
		$LIST_REGIONS = $this -> RestfulOperation('','ListRegions','','','',$AUTH_INFO) -> regions;
		
		for($i=0; $i<count($LIST_REGIONS); $i++)
		{
			if (isset($LIST_REGIONS[$i] -> locales -> {'zh-cn'}))
			{
				$REGION_NAME = $LIST_REGIONS[$i] -> locales -> {'zh-cn'};
			}
			else
			{
				$REGION_NAME = $LIST_REGIONS[$i] -> id;
			}
			
			$REGIONS[$REGION_NAME] = $LIST_REGIONS[$i] -> id;			
		}		
		return $REGIONS;
	}
		
	###########################
	# GET VM DETAIL LIST
	###########################
	public function get_vm_detail_list($CLUSTER_UUID,$FILTER_UUID)
	{
		$GetVmDetailList = $this -> RestfulOperation($CLUSTER_UUID,'GetVmDetailList','compute') -> servers;
	
		for ($i=0; $i<count($GetVmDetailList); $i++)
		{
			if ($GetVmDetailList[$i] -> id != $FILTER_UUID)
			{
				$VmList[] = $GetVmDetailList[$i];
			}
		}
		
		return (object)array('servers' => $VmList);
	}
	
	###########################
	# QUERY VM DETAIL INFO
	###########################
	public function query_vm_detail_info($CLUSTER_UUID,$HOST_UUID)
	{
		$QueryHostInformation = $this -> RestfulOperation($CLUSTER_UUID,'GetVmDetailInfo','compute',$HOST_UUID);
		return $QueryHostInformation;
	}
	
	###########################
	# CREATE NULL SYSTEM IMAGE
	###########################
	public function create_null_imgage($CLUSTER_UUID,$REPL_UUID)
	{		
		$CLUSTER_ADDR = $this -> OpenStackQuery -> query_openstack_connection_information($CLUSTER_UUID)['CLUSTER_ADDR'];
		if (strpos($CLUSTER_ADDR, 'myhuaweicloud') !== false)
		{
			$CREATE_BY_IMAGE = TRUE;
		}
		else
		{
			$CREATE_BY_IMAGE = Misc_Class::define_mgmt_setting() -> openstack -> create_volume_from_image;
		}
		
		if ($CREATE_BY_IMAGE == TRUE)
		{	
			$CHECK_NULL_IMAGE = $this -> get_null_system_object_path($CLUSTER_UUID);
			
			if($CHECK_NULL_IMAGE == FALSE)
			{
				$ReplMgmt = new Replica_Class();				
				$MESSAGE = $ReplMgmt -> job_msg('Create and upload SaaSaMe private image for volume creation reference.');
				$ReplMgmt -> update_job_msg($REPL_UUID,$MESSAGE,'Replica');				
								
				$IMAGE_UUID = Misc_Class::guid_v4();
				
				$IMAGE_ARRAY = array(
										"container_format" 	=> "bare",
										"disk_format" 		=> "qcow2",
										"name" 				=> "vMotionSystemDisk",
										"id" 				=> $IMAGE_UUID,
										"visibility"		=> "private",
										"min_disk"			=> 1
									);
									
				$IMAGE_JSON = json_encode($IMAGE_ARRAY);
				
				$this -> RestfulOperation($CLUSTER_UUID,'CreateImage','image','','',$IMAGE_JSON);
				
				$this -> RestfulOperation($CLUSTER_UUID,'UploadImage','image',$IMAGE_UUID);
				
				$this -> check_image_upload_status($CLUSTER_UUID,$IMAGE_UUID,$REPL_UUID);				
				
				return $this -> get_null_system_object_path($CLUSTER_UUID);
			}
			else
			{
				return $CHECK_NULL_IMAGE;
			}
		}
		else
		{
			return true;
		}
	}
	
	###########################
	# CHECK IMAGE READY STATUS
	###########################
	private function check_image_upload_status($CLUSTER_UUID,$IMAGE_UUID,$REPL_UUID)
	{
		$ReplMgmt = new Replica_Class();
		
		for ($i=0; $i<=20; $i++)
		{
			$GetImageStatus = $this -> RestfulOperation($CLUSTER_UUID,'GetImage','image',$IMAGE_UUID) -> status;
			if ($GetImageStatus == 'active')
			{
				break;
			}
			else
			{
				$MESSAGE = $ReplMgmt -> job_msg('The uploaded image still in processing.<sup>%1%</sup>',array(($i+1)));
				$ReplMgmt -> update_job_msg($REPL_UUID,$MESSAGE,'Replica');
				
				sleep(30);
			}
		}
	}
	
	
	###########################
	# GET NEW DISK MOUNT POINT
	###########################
	private function get_new_mount_point($MOUNT_ARRAY)
	{
		$MountPoint = $MOUNT_ARRAY -> volumeAttachments;
		if (sizeof($MountPoint) == 0)
		{
			return '/dev/vdb';
		}
		else
		{
			for ($i=0; $i<count($MountPoint); $i++)
			{
				$DEVICE = $MountPoint[$i] -> device;
				$DEVICE_TOKEN = explode('/', $DEVICE);		
				$DEVICE_TOKEN = end($DEVICE_TOKEN);
				$DEVICE_LIST[] = $DEVICE_TOKEN;
			}
			
			sort($DEVICE_LIST);
			$LAST_DEVICE = end($DEVICE_LIST);
			$LAST_DEVICE++;
			
			return '/dev/'.$LAST_DEVICE;
		}
	}
	
	
	###########################
	# GET VOLUME TYPE
	###########################
	public function get_volume_type($CLUSTER_UUID)
	{
		$VOLUME_CREATE_TYPE = Misc_Class::define_mgmt_setting() -> openstack -> volume_create_type;

		$ListVolumeType = $this -> RestfulOperation($CLUSTER_UUID,'GetVolumeTypeList','volumev2');
		
		if (count($ListVolumeType -> volume_types) != 0)
		{
			#DEFAULT TYPE
			$VolumeTypeName = $ListVolumeType -> volume_types[0] -> name;
	
			for ($i=0; $i<count($ListVolumeType -> volume_types); $i++)
			{
				if (stripos($ListVolumeType -> volume_types[$i] -> name, $VOLUME_CREATE_TYPE) !== false)
				{
					$VolumeTypeName = $ListVolumeType -> volume_types[$i] -> name;
					break;
				}
			}			
			return $VolumeTypeName;
		}
		else
		{
			return false;
		}
	}
	
	
	###########################
	# GET NULL SYSTEM OBJECT PATH
	###########################
	public function get_null_system_object_path($CLUSTER_UUID)
	{
		$ListObjectImage = $this -> RestfulOperation($CLUSTER_UUID,'GetImageList','image');
		
		foreach ($ListObjectImage -> images as $Object)
		{
			if ($Object -> name == 'vMotionSystemDisk' and $Object -> visibility == 'private')
			{
				$NullImagePath = $Object -> id;
				
				return $NullImagePath;
			}
		}
		return false;
	}
	
	###########################
	# CREATE NOVA VOLUME
	###########################
	private function create_cinder_volume($CLUSTER_UUID,$DISK_SIZE_GB,$HOST_NAME,$REPL_UUID,$AZ_ZONE)
	{
		$NULL_IMAGE_PATH = $this -> create_null_imgage($CLUSTER_UUID,$REPL_UUID);
		
		if ($NULL_IMAGE_PATH == FALSE)
		{
			return false;
		}
		else
		{	
			#CREATE CINDER VOLUME
			$NOW_TIME = Misc_Class::current_utc_time();
			
			$VOLUME_TYPE = $this -> get_volume_type($CLUSTER_UUID);
			if ($VOLUME_TYPE === FALSE)
			{
				return false;
			}
			else
			{
				$VOLUME_CREATE_ARRAY['volume'] = array(
													"size" => $DISK_SIZE_GB,
													"description" => "Volume Created By SaaSaMe Transport Service @ ".$NOW_TIME,
													"name" => $HOST_NAME."@".$NOW_TIME,
													"availability_zone" => $AZ_ZONE
												);
				
				#TRUE NOT TO CREATE VOLUME BY IMAGE
				if ($NULL_IMAGE_PATH !== TRUE)
				{
					$VOLUME_CREATE_ARRAY['volume']['imageRef'] = $NULL_IMAGE_PATH;
				}
				
				#SET DEFAULT VOLUME TYPE
				if ($VOLUME_TYPE !== TRUE)
				{
					$VOLUME_CREATE_ARRAY['volume']['volume_type'] = $VOLUME_TYPE;
				}
				
				#VOLUME ARRAY TO JSON
				$VOLUME_CREATE_JSON = json_encode($VOLUME_CREATE_ARRAY);
			}

			$VolumeMessage = $this -> RestfulOperation($CLUSTER_UUID,'CreateVolume','volumev2','','',$VOLUME_CREATE_JSON);
			
			if (isset($VolumeMessage -> volume -> id))
			{
				$VolumeUUID = $VolumeMessage -> volume -> id;
				return $VolumeUUID;
			}
			else
			{
				return false;
			}			
		}	
	}	
	
	###########################	
	#BEGIN TO CREATE OPENSTACK VOLUME
	###########################
	public function begin_volume_for_loader($CLUSTER_UUID,$SERVER_UUID,$DISK_SIZE,$REPL_UUID,$HOST_NAME)
	{
		$VOLUME_STATUS = $this -> attach_volume_to_server($CLUSTER_UUID,$SERVER_UUID,$DISK_SIZE,$REPL_UUID,$HOST_NAME);
		
		return $VOLUME_STATUS;
	}
	
	
	###########################
	# CREATE AND ATTACH VOLUME
	###########################
	private function attach_volume_to_server($CLUSTER_UUID,$SERVER_UUID,$DISK_SIZE_GB,$REPL_UUID,$HOST_NAME)
	{
		$ReplMgmt = new Replica_Class();
		
		#DISK WAITER
		$CREATE_COUNT = Misc_Class::define_mgmt_setting() -> openstack -> disk_create_retry;
		$MOUNT_COUNT  = Misc_Class::define_mgmt_setting() -> openstack -> disk_mount_retry;
		
		#GET AZ ZONE
		$AZ_ZONE = $this -> query_vm_detail_info($CLUSTER_UUID,$SERVER_UUID) -> server -> {'OS-EXT-AZ:availability_zone'};
		
		
		#CREATE VOLUME
		$VOLUME_UUID = $this -> create_cinder_volume($CLUSTER_UUID,$DISK_SIZE_GB,$HOST_NAME,$REPL_UUID,$AZ_ZONE);
		if ($VOLUME_UUID == FALSE)
		{
			return false;
		}
		
		sleep(5);
		
		#CHECK AND WAIT VOLUME STATUS - 'available'
		$this -> get_volume_status($CLUSTER_UUID,$VOLUME_UUID,$CREATE_COUNT,$REPL_UUID,'Replica','available');
		
		#SET BOOTABLE
		$BOOT_JSON = '{"os-set_bootable":{"bootable": "True"}}';
		$this -> RestfulOperation($CLUSTER_UUID,'SetBootable','volumev2',$VOLUME_UUID,'',$BOOT_JSON);
		
		$MESSAGE = $ReplMgmt -> job_msg('Attaching the volume to the Transport server');
		$ReplMgmt -> update_job_msg($REPL_UUID,$MESSAGE,'Replica');
				
		#GEN NEW MOUNT POINT
		$MountPointList = $this -> RestfulOperation($CLUSTER_UUID,'GetVolumeAttachments','compute',$SERVER_UUID);
		$MOUNT_POINT = $this -> get_new_mount_point($MountPointList);
		
		#ATTACH VOLUME TO TRANSPORT SERVER
		$MOUNT_JSON = '{
						"volumeAttachment": {
							"volumeId": "'.$VOLUME_UUID.'",
							"device": "'.$MOUNT_POINT.'"
						}
					}';

		$ATTACH_VOLUME = $this -> RestfulOperation($CLUSTER_UUID,'AttachVolumeToServer','compute',$SERVER_UUID,'',$MOUNT_JSON);
		
		sleep(5);
		
		#CHECK AND WAIT VOLUME STATUS - 'in-use'
		$this -> get_volume_status($CLUSTER_UUID,$VOLUME_UUID,$MOUNT_COUNT,$REPL_UUID,'Replica','in-use');
		
		$VOLUME_INFO = $this -> RestfulOperation($CLUSTER_UUID,'GetVolumeDetailInfo','volumev2',$VOLUME_UUID);
		$VOLUME_STATUS = $VOLUME_INFO -> volume -> status;
				
		if ($VOLUME_STATUS == 'in-use')
		{
			$VOLUME_STATUS = true;
		}
		else
		{
			$VOLUME_STATUS = false;
		}
				
		$VOLUME_STATUS = (object)array('volumeMountStatus' => $VOLUME_STATUS);

		$OBJECT_MERGED = (object)array_merge((array)$ATTACH_VOLUME,(array)$VOLUME_STATUS);
			
		return $OBJECT_MERGED;
	}
	
	
	###########################
	# DETACH VOLUME FROM SERVER
	###########################
	public function detach_volume_from_server($CLUSTER_UUID,$SERV_UUID,$VOLU_UUID)
	{
		$DETACH_VOLUME = $this -> RestfulOperation($CLUSTER_UUID,'DetachVolumeFromServer','compute',$SERV_UUID,$VOLU_UUID);
		
		#DISK WAITER
		$CREATE_COUNT = Misc_Class::define_mgmt_setting() -> openstack -> disk_create_retry;
			
		#CHECK AND WAIT VOLUME STATUS - 'available'
		$this -> get_volume_status($CLUSTER_UUID,$VOLU_UUID,$CREATE_COUNT,$SERV_UUID,'Replica','available');
		
		return $DETACH_VOLUME;
	}
	
	
	###########################
	# DELETE VOLUME
	###########################
	public function delete_volume($SERV_UUID,$CLOUD_UUID,$VOLUME_UUID)
	{
		$VOLUME_INFO = $this -> get_volume_detail_info($CLOUD_UUID,$VOLUME_UUID);
		if ($VOLUME_INFO -> status == 'in-use')
		{				
			#VOLUME SERVER ID
			$VOLUME_SERVER_ID = $VOLUME_INFO -> server_id;
										
			#DETACH VOLUME
			$this -> detach_volume_from_server($CLOUD_UUID,$VOLUME_SERVER_ID,$VOLUME_UUID);
									
			#GET DISK ACTION WAITER
			$UNMOUNT_COUNT = Misc_Class::define_mgmt_setting() -> openstack -> disk_unmount_retry;
					
			#CHECK VOLUME DETACH STATUS
			$WAITER_RESPONSE = $this -> get_volume_status($CLOUD_UUID,$VOLUME_UUID,$UNMOUNT_COUNT,$SERV_UUID,'Service','available');
			if ($WAITER_RESPONSE == FALSE)
			{
				$ReplMgmt = new Replica_Class();
				$MESSAGE = $ReplMgmt -> job_msg('Disk unmount timeout.');
				$ReplMgmt -> update_job_msg($SERV_UUID,$MESSAGE,'Service');
			}
		}								
		$this -> delete_volume_from_cluster($CLOUD_UUID,$VOLUME_UUID);
	}
	
	
	###########################
	# DELETE VOLUME FROM CLUSTER
	###########################
	public function delete_volume_from_cluster($CLUSTER_UUID,$VOLUME_UUID)
	{
		$DELETE_VOL = $this -> RestfulOperation($CLUSTER_UUID,'DeleteVolume','volumev2',$VOLUME_UUID);
		
		return $DELETE_VOL;
	}
	
	
	###########################
	# GET VOLUME DETAIL INFO
	###########################
	public function get_volume_detail_info($CLUSTER_UUID,$VOLUME_UUID)
	{
		$VOLUME_INFO = $this -> RestfulOperation($CLUSTER_UUID,'GetVolumeDetailInfo','volumev2',$VOLUME_UUID);
		
		$VOLUME_STATUS			= $VOLUME_INFO -> volume -> status;		
		$UUID_VOLUME			= $VOLUME_INFO -> volume -> id;
		$VOLUME_NAME			= $VOLUME_INFO -> volume -> name;
		$VOLUME_SIZE			= $VOLUME_INFO -> volume -> size;
		$AVAILABILITY_ZONE		= $VOLUME_INFO -> volume -> availability_zone;
				
		if ($VOLUME_STATUS == 'in-use')
		{
			$VOLUME_SERVER = $VOLUME_INFO -> volume -> attachments[0] -> server_id;
		}
		else
		{
			$VOLUME_SERVER = '';
		}
				
		return (object)array('server_id' => $VOLUME_SERVER, 'volume_id' => $UUID_VOLUME,'volume_name' => $VOLUME_NAME,'volume_size' => $VOLUME_SIZE,'status' => $VOLUME_STATUS,'availability_zone' => $AVAILABILITY_ZONE);
	}	
	
	###########################
	# GET VOLUME STATUS
	###########################
	public function get_volume_status($CLUSTER_UUID,$VOLUME_UUID,$LOOP_COUNT,$SERVICE_UUID,$JOB_TYPE,$STATUS_TYPE)
	{
		$ReplMgmt = new Replica_Class();
		
		#GET DISK ACTION WAITER
		$WAIT_TIME = Misc_Class::define_mgmt_setting() -> openstack -> disk_wait_time_retry;
			
		#SET DEFAULT RESPONSE
		$DEFAULT_RESPONSE = false;
		
		for ($i=0; $i<$LOOP_COUNT; $i++)
		{
			$MESSAGE = $ReplMgmt -> job_msg('Checking the volume '.$STATUS_TYPE.' status<sup>%1%</sup>',array(($i+1)));
			$ReplMgmt -> update_job_msg($SERVICE_UUID,$MESSAGE,$JOB_TYPE);
			
			$VOLUME_INFO = $this -> RestfulOperation($CLUSTER_UUID,'GetVolumeDetailInfo','volumev2',$VOLUME_UUID);
		
			$VOLUME_STATUS = $VOLUME_INFO -> volume -> status;
			
			if ($VOLUME_STATUS == $STATUS_TYPE)
			{
				$DEFAULT_RESPONSE = true;
				break;
			}
			else
			{
				sleep($WAIT_TIME);
			}
		}
		
		if ($DEFAULT_RESPONSE == FALSE)
		{
			$MESSAGE = $ReplMgmt -> job_msg('Disk creation timeout.');
			$ReplMgmt -> update_job_msg($SERVICE_UUID,$MESSAGE,'Service');
		}
		
		return $DEFAULT_RESPONSE;
	}
	
	###########################
	# DELETE REPLICA JOB
	###########################
	public function delete_replica_job($CLOUD_UUID,$REPL_UUID,$INSTANCE_UUID,$DISK_UUID)
	{
		if ($DISK_UUID != FALSE)
		{
			$ReplMgmt = new Replica_Class();
			
			for ($i=0; $i<count($DISK_UUID); $i++)
			{
				$VOLUME_UUID = $DISK_UUID[$i]['OPEN_DISK_UUID'];

				#DETACH DISK
				$this -> detach_volume_from_server($CLOUD_UUID,$INSTANCE_UUID,$VOLUME_UUID);

				#GET MATCH SNAPSHOT LIST
				$LIST_MATCH = $this -> list_available_snapshot($CLOUD_UUID,$VOLUME_UUID);
			
				#BEGIN TO DELETE SNAPSHOT
				if ($LIST_MATCH != '')
				{
					$SNPSHOT_COUNT = count($LIST_MATCH);
					$MESSAGE = $ReplMgmt -> job_msg('Removing the snapshots.');
					$ReplMgmt -> update_job_msg($REPL_UUID,$MESSAGE,'Replica');
					for ($SNAP=0; $SNAP<$SNPSHOT_COUNT; $SNAP++)
					{
						$SNAPSHOT_ID = $LIST_MATCH[$SNAP] -> id;
						
						$this -> delete_snapshot($CLOUD_UUID,$SNAPSHOT_ID);
						
						$MESSAGE = $ReplMgmt -> job_msg('Snapshot %1% removed.',array($SNAPSHOT_ID));
						$ReplMgmt -> update_job_msg($REPL_UUID,$MESSAGE,'Replica');
						
						sleep(3);
					}
				}

				#DELETE VOLUME
				$this -> delete_volume_from_cluster($CLOUD_UUID,$VOLUME_UUID);
				
				#MESSAGE
				$MESSAGE = $ReplMgmt -> job_msg('The volume removed.');
				$ReplMgmt -> update_job_msg($REPL_UUID,$MESSAGE,'Replica');
			}
		}
	}
	
	
	###########################
	# TAKE DEVICE SNAPSHOT
	###########################
	public function take_snapshot($CLUSTER_UUID,$REPL_UUID,$VOLUME_UUID,$SNAP_NAME,$SNAP_TIME)
	{
		$NOW_TIME  		= Misc_Class::current_utc_time();
		$SNAP_SQL_TIME  = gmdate("Y-m-d G:i:s", strtotime($SNAP_TIME));		
		
		$SNAPSHOT_JSON = '{
						"snapshot": {
							"name": "'.$SNAP_NAME.'@'.$NOW_TIME.'",
							"description": "Snapshot Created By SaaSaMe Transport Service @ '.$SNAP_SQL_TIME.' UTC",
							"volume_id": "'.$VOLUME_UUID.'",
							"force": true
						}
					}';
		
		$SNAPSHOT_INFO = $this -> RestfulOperation($CLUSTER_UUID,'CreateSnapshot','volumev2','','',$SNAPSHOT_JSON);
		
		if (isset($SNAPSHOT_INFO -> snapshot -> id))
		{
			$SNAPSHOT_METADATA_JSON = '{
								"metadata":{
									"UUID" : "'.$REPL_UUID.'"
								}
							}';
							
			$this -> RestfulOperation($CLUSTER_UUID,'CreateSnapshotMetadata','volumev2',$SNAPSHOT_INFO -> snapshot -> id,'',$SNAPSHOT_METADATA_JSON);
			return $SNAPSHOT_INFO;
		}
		else
		{		
			if (isset($SNAPSHOT_INFO -> overLimit -> message))
			{
				$ERROR_MSG = $SNAPSHOT_INFO -> overLimit -> message;
			}
			else
			{
				$ERROR_MSG = 'Failed to create snapshot.';
			}
			
			return $ERROR_MSG;
		}
	}

	###########################
	# SNAPSHOT CONTROL
	###########################
	public function snapshot_control($CLOUD_UUID,$VOLUME_ID,$NUMBER_SNAPSHOT)
	{
		$LIST_SNAPSHOTS = $this -> list_available_snapshot($CLOUD_UUID,$VOLUME_ID);
								
		if ($LIST_SNAPSHOTS != FALSE)
		{
			$SLICE_SNAPSHOT = array_slice($LIST_SNAPSHOTS,$NUMBER_SNAPSHOT);
			for ($x=0; $x<count($SLICE_SNAPSHOT); $x++)
			{
				$REMOVE_SNAPSHOT_ID = $SLICE_SNAPSHOT[$x] -> id;
				$this -> delete_snapshot($CLOUD_UUID,$REMOVE_SNAPSHOT_ID);
			}
		}
	}
	
	###########################
	# GET SNAPSHOT DETAIL INFO
	###########################
	public function get_snapshot_detail_list($CLUSTER_UUID,$SNAP_UUID)
	{
		$SNAPSHOT_DETAIL = $this -> RestfulOperation($CLUSTER_UUID,'GetSnapshotDetailInfo','volumev2',$SNAP_UUID);
		
		return $SNAPSHOT_DETAIL;
	}
	
	
	###########################
	# CREATE VOLUME FROM SNAPSHOT
	###########################
	public function create_volume_by_snapshot($CLUSTER_UUID,$SERVICE_UUID,$VOLUME_SIZE,$HOST_NAME,$SNAP_UUID)
	{
		$NOW_TIME = Misc_Class::current_utc_time();	
		
		$VOLUME_TYPE = $this -> get_volume_type($CLUSTER_UUID);
		if ($VOLUME_TYPE == FALSE)
		{
			return false;
		}
		elseif ($VOLUME_TYPE == TRUE)
		{
			$VOLUME_CREATE_JSON = '{
									"volume": {
									"size": "'.$VOLUME_SIZE.'",
									"description": "Volume Created By SaaSaMe Transport Service @ '.$NOW_TIME.'",
									"name": "Snap-'.$HOST_NAME.'@'.$NOW_TIME.'",									
									"snapshot_id": "'.$SNAP_UUID.'"
								}
							}';
		}
		else
		{
			$VOLUME_CREATE_JSON = '{
									"volume": {
									"size": "'.$VOLUME_SIZE.'",
									"description": "Volume Created By SaaSaMe Transport Service @ '.$NOW_TIME.'",
									"name": "Snap-'.$HOST_NAME.'@'.$NOW_TIME.'",
									"volume_type": "'.$VOLUME_TYPE.'",
									"snapshot_id": "'.$SNAP_UUID.'"
								}
							}';			
		}
				
		$VolumeINFO = $this -> RestfulOperation($CLUSTER_UUID,'CreateVolume','volumev2','','',$VOLUME_CREATE_JSON);
		
		if (isset($VolumeINFO -> volume -> id))
		{
			$VOLUME_UUID = $VolumeINFO -> volume -> id;
						
			#GET DISK ACTION WAITER
			$CREATE_COUNT = Misc_Class::define_mgmt_setting() -> openstack -> disk_create_retry;
			
			$this -> get_volume_status($CLUSTER_UUID,$VOLUME_UUID,$CREATE_COUNT,$SERVICE_UUID,'Service','available');
			
			return $VOLUME_UUID;	
		}
		else
		{
			return false;
		}
	}
	
	###########################
	# MOUNT SNAPSHOT VOLUME
	###########################
	public function attach_volume($CLOUD_UUID,$SERVER_UUID,$SERVICE_UUID,$VOLUME_UUID)
	{
		$VOLUME_INFO = $this -> get_volume_detail_info($CLOUD_UUID,$VOLUME_UUID);
		if ($VOLUME_INFO -> status == 'in-use')
		{				
			#VOLUME SERVER ID
			$VOLUME_SERVER_ID = $VOLUME_INFO -> server_id;

			#DETACH VOLUME
			$this -> detach_volume_from_server($CLOUD_UUID,$VOLUME_SERVER_ID,$VOLUME_UUID);
									
			#GET DISK ACTION WAITER
			$UNMOUNT_COUNT = Misc_Class::define_mgmt_setting() -> openstack -> disk_unmount_retry;
	
			#CHECK VOLUME DETACH STATUS
			$WAITER_RESPONSE = $this -> get_volume_status($CLOUD_UUID,$VOLUME_UUID,$UNMOUNT_COUNT,$SERVICE_UUID,'Service','available');
			if ($WAITER_RESPONSE == FALSE)
			{
				$ReplMgmt = new Replica_Class();
				$MESSAGE = $ReplMgmt -> job_msg('Disk unmount timeout.');
				$ReplMgmt -> update_job_msg($SERVICE_UUID,$MESSAGE,'Service');
			}
		}
		
		#DISK WAITER
		$MOUNT_COUNT = Misc_Class::define_mgmt_setting() -> openstack -> disk_mount_retry;
	
		#GET NEW MOUNT POINT
		$MountPointList = $this -> RestfulOperation($CLOUD_UUID,'GetVolumeAttachments','compute',$SERVER_UUID);
		$MOUNT_POINT = $this -> get_new_mount_point($MountPointList);
		
		#BEGIN TO ATTACH VOLUME
		$NOVA_JSON = '{
						"volumeAttachment": {
							"volumeId": "'.$VOLUME_UUID.'",
							"device": "'.$MOUNT_POINT.'"
						}
					}';
					
		$ATTACH_TO = $this -> RestfulOperation($CLOUD_UUID,'AttachVolumeToServer','compute',$SERVER_UUID,'',$NOVA_JSON);
		
		#WAITING FOR VOLUME STATUS
		$WAITER_RESPONSE = $this -> get_volume_status($CLOUD_UUID,$VOLUME_UUID,$MOUNT_COUNT,$SERVICE_UUID,'Service','in-use');
		if ($WAITER_RESPONSE == TRUE)
		{
			return $ATTACH_TO;
		}
		else
		{
			return false;
		}
	}	
	
	
	###########################
	# LIST SNAPSHOT VOLUME ID
	###########################
	private function list_snapshot_volume($CLUSTER_UUID)
	{
		$LIST_VOLUME = $this -> RestfulOperation($CLUSTER_UUID,'GetVolumeDetailList','volumev2') -> volumes;
	
		#SET DEFAULT FILTER ARRAY
		$SNAPSHOT_IN_USE = false;
		
		for ($i=0; $i<count($LIST_VOLUME); $i++)
		{
			if (isset($LIST_VOLUME[$i] -> snapshot_id))
			{
				$SNAPSHOT_IN_USE[] = $LIST_VOLUME[$i] -> snapshot_id;
			}	
		}
		return $SNAPSHOT_IN_USE;
	}
	
	###########################
	# SORT SNAPSHOT BY CREATE TIME
	###########################
	private function unsort_snapshot_by_created_at($value_a,$value_b)
	{
		return strcmp($value_b->created_at,$value_a->created_at);
	}	
	
	###########################
	# LIST AVAILABLE SNAPSHOT
	###########################
	public function list_available_snapshot($CLUSTER_UUID,$DISK_UUID)
	{
		#GET SNAPSHOT LIST
		$LIST_SNAPSHOT = $this -> RestfulOperation($CLUSTER_UUID,'GetSnapshotList','volumev2') -> snapshots;
	
		#SET DEFAULT VALUE
		$LIST_MATCH = false;
	
		if (count($LIST_SNAPSHOT) != 0)
		{
			#SORT SNAPSHOT BY CREATE AT
			usort($LIST_SNAPSHOT, array($this,"unsort_snapshot_by_created_at"));
		
			#LIST SNAPSHOT VOLUME
			#$LIST_SNAPSHOT_VOLUME = $this -> list_snapshot_volume($CLUSTER_UUID);
		
			for ($i=0; $i<count($LIST_SNAPSHOT); $i++)
			{
				if ($LIST_SNAPSHOT[$i] -> volume_id == $DISK_UUID)
				{
					$LIST_MATCH[] = $LIST_SNAPSHOT[$i];
					/*
					if ($LIST_SNAPSHOT_VOLUME != FALSE)
					{
						if (!in_array($LIST_SNAPSHOT[$i] -> id, $LIST_SNAPSHOT_VOLUME))
						{
							$LIST_MATCH[] = $LIST_SNAPSHOT[$i];
						}
						$LIST_MATCH[] = $LIST_SNAPSHOT[$i];
					}
					else
					{
						$LIST_MATCH[] = $LIST_SNAPSHOT[$i];
					}
					*/
				}
			}
		}
		
		return $LIST_MATCH;
	}
	
	###########################
	# DELETE SNAPSHOT
	###########################
	public function delete_snapshot($CLUSTER_UUID,$SNAP_UUID)
	{
		$REMOVE_RESPONSE = $this -> RestfulOperation($CLUSTER_UUID,'DeleteSnapshot','volumev2',$SNAP_UUID);
		
		if (isset($REMOVE_RESPONSE))
		{
			return false;
		}
		else
		{
			return true;
		}		
	}
	
	###########################
	# CREATE INSTANCE FROM VOLUME
	###########################
	public function create_instance_from_volume($SERV_UUID,$CLUSTER_UUID,$HOST_NAME,$DEVICE_MAP_JSON,$API_REFERER_FROM)
	{
		$ServiceMgmt = new Service_Class();
		$SERV_QUERY  = $ServiceMgmt -> query_service($SERV_UUID);

		$FLAVOR_ID	  = $SERV_QUERY['FLAVOR_ID'];
		$NETWORK_UUID = $SERV_QUERY['NETWORK_UUID'];
		$SGROUP_UUID  = $SERV_QUERY['SGROUP_UUID'];
		
		$PRIVATE_ADDR = json_decode($SERV_QUERY['JOBS_JSON']) -> private_address_id;
		
		$FLAVOR_REF = $this -> RestfulOperation($CLUSTER_UUID,'GetFlavorDetailInfo','compute',$FLAVOR_ID);
		$FLAVOR_REF_URL = $FLAVOR_REF -> flavor -> links[0] -> href;
		
		#$SGROUP_REF = $this -> RestfulOperation($CLUSTER_UUID,'GetSecurityGroupDetailInfo','network',$SGROUP_UUID);
		#$SGROUP_NAME = $SGROUP_REF -> security_group -> name;
		
		$BOOT_DISK_UUID = json_decode($DEVICE_MAP_JSON)[0] -> uuid;
		
		#USER DEFINE AZ
		$AZ_ZONE = Misc_Class::define_mgmt_setting() -> openstack -> volume_create_az;
		if ($AZ_ZONE == '')
		{
			$AZ_ZONE = $this -> get_volume_detail_info($CLUSTER_UUID,$BOOT_DISK_UUID) -> availability_zone;
		}
		
		$CREATE_VM_JSON = '{
							"server":
							{
								"flavorRef":"'.$FLAVOR_REF_URL.'",
								"name":"'.$HOST_NAME.'@'.time().'",
								"block_device_mapping_v2":'.$DEVICE_MAP_JSON.',
								"networks":[{"uuid": "'.$NETWORK_UUID.'"}],
								"security_groups":[{"name": "'.$SGROUP_UUID.'"}],
								"availability_zone":"'.$AZ_ZONE.'"
							}
						}';
						
		#Assign Private Address			
		if ($PRIVATE_ADDR != 'DynamicAssign')
		{
			$CREATE_VM = json_decode($CREATE_VM_JSON);
			$CREATE_VM -> server -> networks[0] -> fixed_ip = $PRIVATE_ADDR;
			$CREATE_VM_JSON = json_encode($CREATE_VM,JSON_UNESCAPED_SLASHES);
		}
				
		$CreateVM = $this -> RestfulOperation($CLUSTER_UUID,'CreateVm','compute','','',$CREATE_VM_JSON);
		
		if (!isset($CreateVM -> server))
		{
			if (isset($CreateVM -> error))
			{
				$ErrorMsg = json_decode($CreateVM -> error) -> badRequest -> message;
			}
			else
			{
				$ErrorMsg = $CreateVM -> badRequest -> message;
			}			
			
			$ReplMgmt = new Replica_Class();				
			$MESSAGE = $ReplMgmt -> job_msg($ErrorMsg);
			$ReplMgmt -> update_job_msg($SERV_UUID,$MESSAGE,'Service');		
						
			#details = $CreateVM -> fault -> details;
			#service = $CreateVM -> fault -> service;
			return false;
		}
		else
		{
			$FLOATINF_ID = json_decode($SERV_QUERY['JOBS_JSON']) -> elastic_address_id;
			if ($FLOATINF_ID != 'DynamicAssign')
			{			
				$VM_UUID = $CreateVM -> server -> id;
				for ($i=0; $i<5; $i++)
				{
					$VM_INFO = $this -> get_instance_detail($CLUSTER_UUID,$VM_UUID);				
					$ADDR_INFO = $VM_INFO -> server -> addresses;
					foreach ($ADDR_INFO as $ADDR_KEY => $ADDR_DATA)
					{
						for ($n=0; $n<count($ADDR_DATA); $n++)
						{
							if ($ADDR_DATA[$n] -> {'OS-EXT-IPS:type'} == 'fixed')
							{
								$FIXED_IP = $ADDR_DATA[$n] -> addr;
								break 3;
							}		
						}
					}
					sleep(30);
				}				
				$this -> associate_floating_ip($CLUSTER_UUID,$FIXED_IP,$FLOATINF_ID);
			}
			
			if (isset($CreateVM -> server -> id))
			{
				$VM_ID = $CreateVM -> server -> id;
				$VM_PASSWORD = $CreateVM -> server -> adminPass;
				
				#GET VM DETAIL
				$GetVmDetail = $this -> get_instance_detail($CLUSTER_UUID,$VM_ID);				
				if (isset($GetVmDetail -> server -> status))
				{
					if (isset($GetVmDetail -> server -> fault))
					{
						$message 	= $GetVmDetail -> server -> fault -> message;
						$status 	= false;					
					}
					else
					{
						$message 	= 'The instance created.';
						$status 	= true;						
					}
				}
				else
				{
					$message 	= json_decode($GetVmDetail -> error) -> itemNotFound -> message;
					$status 	= false;
				}
				
				return (object)array('instance_id' => $VM_ID, 'adminPass' => $VM_PASSWORD, 'message' => $message, 'status' => $status);
			}
			else
			{
				return false;
			}
		}
	}
		
	###########################
	# LIST FLAVOR
	###########################
	public function get_flavor_list_detail($CLUSTER_UUID)
	{
		$GetFlavorList = $this -> RestfulOperation($CLUSTER_UUID,'GetFlavorList','compute');			
		return $GetFlavorList;
	}
	
	###########################
	# LIST NETWORK
	###########################
	public function get_network_list_detail($CLUSTER_UUID)
	{
		$GetNetworkList = $this -> RestfulOperation($CLUSTER_UUID,'GetNetworkList','network');
		return $GetNetworkList;		
	}
	
	###########################
	# LIST SUBNET
	###########################
	public function get_subnet_list_detail($CLUSTER_UUID)
	{
		$GetSubnetList = $this -> RestfulOperation($CLUSTER_UUID,'GetSubnetList','network') ;
		return $GetSubnetList;		
	}
	
	###########################
	# GET FLOATING IP DETAILS
	###########################
	public function get_floating_ip_details($CLUSTER_UUID,$FLOATING_IP_ID)
	{
		$FloatingIpDetails = $this -> RestfulOperation($CLUSTER_UUID,'GetFloatingIpDetails','network',$FLOATING_IP_ID);
		return $FloatingIpDetails;
	}
	
	###########################
	# ASSOCIATE FLOATING IP
	###########################
	public function associate_floating_ip($CLUSTER_UUID,$FIXED_IP,$FLOATING_ID)
	{
		$PortList = $this -> RestfulOperation($CLUSTER_UUID,'ListPort','network') -> ports;
		for ($i=0; $i<count($PortList); $i++)
		{
			if ($PortList[$i] -> fixed_ips[0] -> ip_address == $FIXED_IP)
			{
				$PORT_ID = $PortList[$i] -> id;
				break;
			}
		}
		
		$FLOATING_JSON = '{"floatingip":{"port_id":"'.$PORT_ID.'"}}';
		
		return $this -> RestfulOperation($CLUSTER_UUID,'UpdateFloatingIP','network',$FLOATING_ID,'',$FLOATING_JSON);		
	}
	
	###########################
	# GET AVAILABLE NETWORK ADDRESS
	###########################
	public function get_available_network_address($CLUSTER_UUID)
	{
		$PROJECT_ID = $this -> OpenStackQuery -> query_openstack_connection_information($CLUSTER_UUID)['PROJECT_ID'];

		$ListFloatingAddr = $this -> RestfulOperation($CLUSTER_UUID,'ListFloatingIPs','network') -> floatingips;	

		$AVAILABLE_FLOATING_ADDR = false;
		for ($i=0; $i<count($ListFloatingAddr); $i++)
		{
			if ($ListFloatingAddr[$i] -> status == 'DOWN' AND $ListFloatingAddr[$i] -> tenant_id == $PROJECT_ID)
			{
				$AVAILABLE_FLOATING_ADDR[$ListFloatingAddr[$i] -> id] = $ListFloatingAddr[$i] -> floating_ip_address;
			}			
		}		
		return $AVAILABLE_FLOATING_ADDR;
	}
	
	###########################
	# LIST SECURITY GROUP
	###########################
	public function get_security_group_list_detail($CLUSTER_UUID)
	{
		$GetSecurityGroupList = $this -> RestfulOperation($CLUSTER_UUID,'ListSecurityGroupsByTenant','compute');			
		return $GetSecurityGroupList;
	}	
	
	###########################
	# QUERY FLAVOR_DETAIL_INFO
	###########################
	public function get_flavor_detail_info($CLUSTER_UUID,$FLAVOR_ID)
	{
		$FlavorInfo = $this -> RestfulOperation($CLUSTER_UUID,'GetFlavorDetailInfo','compute',$FLAVOR_ID);
		return $FlavorInfo;
	}
	
	###########################
	# QUERY NETWORK DETAIL INFO
	###########################
	public function get_network_detail_info($CLUSTER_UUID,$NETWORK_ID)
	{
		$GetNetworkDetailInfo = $this -> RestfulOperation($CLUSTER_UUID,'GetNetworkDetailInfo','network',$NETWORK_ID);
		return $GetNetworkDetailInfo;
	}
	
	###########################
	# QUERY SUBNET DETAIL INFO
	###########################
	public function get_subnet_detail_info($CLUSTER_UUID,$SUBNET_ID)
	{
		$GetSubnetDetailInfo = $this -> RestfulOperation($CLUSTER_UUID,'GetSubnetDetailInfo','network',$SUBNET_ID);		
		return $GetSubnetDetailInfo;	
	}
	
	
	###########################
	# QUERY SECURITY GROUP DETAIL INFO
	###########################
	public function get_security_group_detail_info($CLUSTER_UUID,$SGROUP_ID)
	{
		$GetSecurityGroupDetailInfo = $this -> RestfulOperation($CLUSTER_UUID,'GetSecurityGroupDetailInfo','network',$SGROUP_ID);
		return $GetSecurityGroupDetailInfo;
	}
	
	###########################
	# QUERY VIRTUAL MACHINE
	###########################
	public function get_instance_detail($CLUSTER_UUID,$SERV_UUID)
	{
		$QueryVM = $this -> RestfulOperation($CLUSTER_UUID,'ShowVmDetails','compute',$SERV_UUID);		
		return $QueryVM;
	}
	
	/*
	###########################
	# DELETE VIRTUAL MACHINE
	###########################
	public function delete_instance($CLUSTER_UUID,$SERV_UUID)
	{
		$DeleteVM = $this -> RestfulOperation($CLUSTER_UUID,'DeleteVm','compute',$SERV_UUID);
		return $DeleteVM;
	}
	*/
	
	###########################
	# DELETE VIRTUAL MACHINE
	###########################
	public function delete_instance($CLUSTER_UUID,$SERV_UUID)
	{
		$POST_JSON = '{"forceDelete": null}';
		
		$DeleteVM = $this -> RestfulOperation($CLUSTER_UUID,'ForceDeleteVm','compute',$SERV_UUID,'',$POST_JSON);
		return $DeleteVM;
	}
	
	###########################
	# GET VM DETAIL INFO
	###########################
	public function get_vm_detail_Info($CLUSTER_UUID,$INSTANCE_UUID)
	{
		$QueryHostInformation = $this -> RestfulOperation($CLUSTER_UUID,'GetVmDetailInfo','compute',$INSTANCE_UUID);
		$QueryHostInformation -> server -> CloudType = 'OpenStack';
		return $QueryHostInformation;
	}

	###########################
	# START VM
	###########################
	public function start_vm($CLUSTER_UUID,$SERV_UUID)
	{
		$POWER_STATUS = $this -> get_instance_detail($CLUSTER_UUID,$SERV_UUID) -> server -> status;
		
		if ($POWER_STATUS == 'SHUTOFF')
		{		
			$POST_JSON = '{"os-start": null}';
		
			$StartVM = $this -> RestfulOperation($CLUSTER_UUID,'StartVm','compute',$SERV_UUID,'',$POST_JSON);
			return true;
		}
		else
		{
			return false;
		}
	}

	###########################
	# STOP VM
	###########################
	public function stop_vm($CLUSTER_UUID,$SERV_UUID)
	{
		$POWER_STATUS = $this -> get_instance_detail($CLUSTER_UUID,$SERV_UUID) -> server -> status;
		
		if ($POWER_STATUS == 'ACTIVE')
		{	
			$POST_JSON = '{"os-stop": null}';
		
			$StartVM = $this -> RestfulOperation($CLUSTER_UUID,'StopVm','compute',$SERV_UUID,'',$POST_JSON);
			return true;
		}
		else
		{
			return false;
		}
	}
		
	###########################
	# MAP QUERY CLASS
	###########################
	public function create_openstack_connection($ACCT_UUID,$REGN_UUID,$PROJECT_NAME,$CLUSTER_USER,$CLUSTER_PASS,$CLUSTER_ADDR,$AUTH_DATA)
	{
		return $this -> OpenStackQuery -> create_openstack_connection($ACCT_UUID,$REGN_UUID,$PROJECT_NAME,$CLUSTER_USER,$CLUSTER_PASS,$CLUSTER_ADDR,$AUTH_DATA);
	}
	
	public function update_openstack_connection($CLUSTER_UUID,$PROJECT_NAME,$CLUSTER_USER,$CLUSTER_PASS,$CLUSTER_ADDR,$AUTH_DATA)
	{
		return $this -> OpenStackQuery -> update_openstack_connection($CLUSTER_UUID,$PROJECT_NAME,$CLUSTER_USER,$CLUSTER_PASS,$CLUSTER_ADDR,$AUTH_DATA);
	}
	
	
	###########################
	# MAP QUERY CLASS (LAGCY)
	###########################
	
	#Map with Service -> list_cloud()
	public function list_openstack_connection($ACCT_UUID)
	{
		return $this -> OpenStackQuery -> list_openstack_connection($ACCT_UUID);
	}
	
	#Map with Service -> query_cloud_info()
	public function query_openstack_connection_information($CLOUD_UUID)
	{
		return $this -> OpenStackQuery -> query_openstack_connection_information($CLOUD_UUID);
	}
	
	#Map with Service -> delete_cloud()
	public function delete_connection($CLOUD_UUID)
	{
		return $this -> OpenStackQuery -> delete_connection($CLOUD_UUID);
	}
	
	#Map with Service -> query_cloud_disk()
	public function get_openstack_disk($REPL_UUID)
	{
		return $this -> OpenStackQuery -> get_openstack_disk($REPL_UUID);
	}		
}