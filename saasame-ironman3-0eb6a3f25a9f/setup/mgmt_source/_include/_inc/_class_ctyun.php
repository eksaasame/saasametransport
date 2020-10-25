<?php
###########################
#
# Ctyun Server Class
#
###########################
class Ctyun_Server_Class
{	
	###########################
	#CONSTRUCT FUNCTION
	###########################
	protected $CtyunWebSevMgmt;	
	public function __construct()
	{
		$this -> CtyunWebSevMgmt = new Ctyun_Query_Class();
	}
	
	###########################
	# Ctyun Debug
	###########################
	public function ctyun_debug($POST_DATA,$DEBUG_OUTPUT,$STATUS_CODE)
	{
		$folder =  $_SERVER['DOCUMENT_ROOT'].'/_include/_debug/_mgmt';
		$file = $_SERVER['DOCUMENT_ROOT'].'/_include/_debug/_mgmt/CtyunDebug.txt';
		
		if (!file_exists($folder)){mkdir($folder, 0777, true);}

		#CHECK AND CREATE FILE EXISTS
		if(!file_exists($file))
		{
			$fp = fopen($file,'w');
			if(!$fp)
			{
				throw new Exception('File open failed.');
			}
			else
			{
				fclose($fp);
			}
		}
			
		#CHECK AND CREATE FILE EXISTS
		if(!file_exists($file)){fopen($file,'w');}	
		
		#APPEND STATUS CODE
		$POST_DATA -> status_code = $STATUS_CODE;
		
		$current = file_get_contents($file);
		$current .= 'POST_DATA:'."\n";
		$current .= print_r($POST_DATA,TRUE);
		$current .= 'RESPONSE:'."\n";
		$current .= print_r($DEBUG_OUTPUT,TRUE);
		$current .= "\n".'----------------------------------------------------------'."\n";
		file_put_contents($file, $current);
	}
	
	###########################
	# EXECUTE CURL
	###########################
	private function exec_curl($POST_DATA)
	{
		$POST_DATA = (object)$POST_DATA;
	
		$REQUEST_URL = "https://api.ctyun.cn".$POST_DATA -> request_url;
	
		$EXE_CURL = curl_init($REQUEST_URL);
		curl_setopt($EXE_CURL, CURLOPT_CUSTOMREQUEST, $POST_DATA -> request_method);

		curl_setopt($EXE_CURL, CURLOPT_SSL_VERIFYHOST, false);
		curl_setopt($EXE_CURL, CURLOPT_SSL_VERIFYPEER, false);
		curl_setopt($EXE_CURL, CURLOPT_RETURNTRANSFER, true);
		curl_setopt($EXE_CURL, CURLOPT_VERBOSE, false);
		
		$SetHeader[] = 'Content-Type: application/x-www-form-urlencoded';
		
		foreach ($POST_DATA -> set_header as $Key => $Value){$SetHeader[] = $Key.":".$Value;}
		curl_setopt($EXE_CURL, CURLOPT_HTTPHEADER, $SetHeader);
		
		if ($POST_DATA -> request_method == 'POST')
		{
			curl_setopt($EXE_CURL, CURLOPT_POSTFIELDS, http_build_query($POST_DATA -> request_data));	
		}
		
		$RESP_MESG   = curl_exec($EXE_CURL);
		$HTTP_CODE   = curl_getinfo($EXE_CURL, CURLINFO_HTTP_CODE);
		curl_close($EXE_CURL);
	
		#QUERY STATUS CODE
		$STATUS_CODE = json_decode($RESP_MESG) -> statusCode;
	
		switch ($STATUS_CODE)
		{
			case 400:
				$RESPONSE = 'Incorrect API request parameters.';
			break;
			
			case 800:
				if (isset(json_decode($RESP_MESG) -> returnObj))
				{
					$RESPONSE = json_decode($RESP_MESG) -> returnObj;
				}
				else
				{
					$RESPONSE = 'No return object.';
				}
			break;
			
			case 900:
				$RESPONSE = str_replace(';','',json_decode($RESP_MESG) -> message);
			break;
			
			default:
				$RESPONSE = array('ResponseMsg' => $RESP_MESG, 'HttpCode' => $HTTP_CODE);
		}
		
		$DEBUG_LEVEL = Misc_Class::define_mgmt_setting() -> ctyun -> debug_level;
		
		if ($DEBUG_LEVEL == 0)
		{
			if (strpos($POST_DATA -> request_url, 'queryJobStatus') == FALSE AND $STATUS_CODE != 800)
			{
				$this -> ctyun_debug($POST_DATA,$RESPONSE,$STATUS_CODE);
			}
		}
		else
		{
			$this -> ctyun_debug($POST_DATA,$RESPONSE,$STATUS_CODE);
		}
		
		#DEBUG OUTPUT
		/*if (json_decode($RESP_MESG) -> statusCode != 800 AND $POST_DATA -> request_url != '/apiproxy/v3/queryJobStatus')
		{
			$this -> ctyun_debug($POST_DATA,$RESPONSE);
		}*/
		
		return $RESPONSE;
	}
	
	###########################
	# REQUEST METHOD END POINT
	###########################
	private function request_method($METHOD_NAME)
	{
		switch ($METHOD_NAME)
		{
			/* JOB STATUS */
			case 'queryJobStatus':
				$PARAMETERS = array(
									'url' => '/apiproxy/v3/queryJobStatus',
									'request_type' => 'GET'
				);
			break;
			/* JOB STATUS */
			
			
			/* AUTHENTICATION */
			case 'getZoneConfig':				
				$PARAMETERS = array(				
									'url' => '/apiproxy/v3/order/getZoneConfig',
									'request_type' => 'GET'
				);
			break;
			/* AUTHENTICATION */
			
			
			/* INSTANCE */
			case 'getImages':
				$PARAMETERS = array(				
									'url' => '/apiproxy/v3/order/getImages',
									'request_type' => 'GET'
				);
			break;
			
			case 'createVM':
				$PARAMETERS = array(				
									'url' => '/apiproxy/v3/ondemand/createVM',
									'request_type' => 'POST'
				);			
			break;
			
			case 'startVM':
				$PARAMETERS = array(				
									'url' => '/apiproxy/v3/ondemand/startVM',
									'request_type' => 'POST'
				);			
			break;
			
			case 'stopVM':
				$PARAMETERS = array(				
									'url' => '/apiproxy/v3/ondemand/stopVM',
									'request_type' => 'POST'
				);			
			break;
			
			case 'deleteVM':
				$PARAMETERS = array(				
									'url' => '/apiproxy/v3/ondemand/deleteVM',
									'request_type' => 'POST'
				);			
			break;			
			
			case 'queryVMs':
				$PARAMETERS = array(				
									'url' => '/apiproxy/v3/ondemand/queryVMs',
									'request_type' => 'GET'
				);			
			break;
			
			case 'queryAvailableDevice':
				$PARAMETERS = array(				
									'url' => '/apiproxy/v3/queryAvailableDevice',
									'request_type' => 'GET'
				);
			break;
			
			case 'queryDataDiskByVMId':
				$PARAMETERS = array(				
									'url' => '/apiproxy/v3/queryDataDiskByVMId',
									'request_type' => 'GET'
				);			
			break;
			
			case 'getFlavors':
				$PARAMETERS = array(
									'url' => '/apiproxy/v3/order/getFlavors',
									'request_type' => 'GET'
				);
			break;		
			/* INSTANCE */
			
					
			/* VOLUME */
			case 'createVolume':
				$PARAMETERS = array(
									'url' => '/apiproxy/v3/ondemand/createVolume',
									'request_type' => 'POST'
				);
			break;
			
			case 'attachVolume':
				$PARAMETERS = array(
									'url' => '/apiproxy/v3/ondemand/attachVolume',
									'request_type' => 'POST'
				);
			break;			
			
			case 'uninstallVolume':
				$PARAMETERS = array(
									'url' => '/apiproxy/v3/ondemand/uninstallVolume',
									'request_type' => 'POST'
				);
			break;
			
			case 'deleteVolume':
				$PARAMETERS = array(
									'url' => '/apiproxy/v3/ondemand/deleteVolume',
									'request_type' => 'POST'
				);
			break;			
			
			case 'queryVolumes':
				$PARAMETERS = array(
									'url' => '/apiproxy/v3/ondemand/queryVolumes',
									'request_type' => 'GET'
				);
			break;
			
			case 'expandVolumeSize':
				$PARAMETERS = array(
									'url' => '/apiproxy/v3/ondemand/expandVolumeSize',
									'request_type' => 'POST'
				);
			break;
			
			case 'restoreDiskBackup':
				$PARAMETERS = array(
									'url' => '/apiproxy/v3/restoreDiskBackup',
									'request_type' => 'GET'
				);
			break;
			/* VOLUME */
			
			
			/* SNAPSHOT */
			case 'createVBS':
				$PARAMETERS = array(
									'url' => '/apiproxy/v3/ondemand/createVBS',
									'request_type' => 'POST'
				);			
			break;

			case 'deleteVBS':
				$PARAMETERS = array(
									'url' => '/apiproxy/v3/ondemand/deleteVBS',
									'request_type' => 'POST'
				);
			break;
			
			case 'queryVBSs':
				$PARAMETERS = array(
									'url' => '/apiproxy/v3/ondemand/queryVBSs',
									'request_type' => 'GET'
				);
			break;
			
			case 'queryVBSDetails':
				$PARAMETERS = array(
									'url' => '/apiproxy/v3/ondemand/queryVBSDetails',
									'request_type' => 'GET'
				);
			break;

			case 'queryVBSDetail':
				$PARAMETERS = array(
									'url' => '/apiproxy/v3/ondemand/queryVBSDetail',
									'request_type' => 'GET'
				);
			break;			
			/* SNAPSHOT */
			
			
			/* NETWORK */
			case 'getVpcs':
				$PARAMETERS = array(
									'url' => '/apiproxy/v3/getVpcs',
									'request_type' => 'GET'
				);
			break;
			
			case 'getSubnets':
				$PARAMETERS = array(
									'url' => '/apiproxy/v3/getSubnets',
									'request_type' => 'GET'
				);			
			break;
			
			case 'getSecurityGroups':
				$PARAMETERS = array(
									'url' => '/apiproxy/v3/getSecurityGroups',
									'request_type' => 'GET'
				);
			break;
			
			case 'querySecurityGroupDetail':
				$PARAMETERS = array(
									'url' => '/apiproxy/v3/querySecurityGroupDetail',
									'request_type' => 'GET'
				);
			break;
			
			case 'queryIps':
				$PARAMETERS = array(
									'url' => '/apiproxy/v3/ondemand/queryIps',
									'request_type' => 'GET'
				);
			break;
			/* NETWORK */
		}
		
		return $PARAMETERS;
	}
	
	
	###########################
	# Generate Auth Header
	###########################
	private function auth_header($POST_DATA)
	{
		$METHOD_DATA = $this -> request_method($POST_DATA['method_name']);

		$REQUEST_DATE = date("D, j M Y h:i:s T");
		
		$MD5_DATA = base64_encode(md5("",TRUE));
		$HMAC_DATA = $MD5_DATA."\n".$REQUEST_DATE."\n".$METHOD_DATA['url'];
		$HMAC = hash_hmac("sha1",$HMAC_DATA,$POST_DATA['secret_key'],true);
		
		$HEADER_DATA = array(
							"request_url" 	 => $METHOD_DATA['url'],
							"request_method" => $METHOD_DATA['request_type'],
							"set_header" 	 => array
												(
													"accessKey" => $POST_DATA['public_key'],
													"contentMD5" => $MD5_DATA, 
													"requestDate" => $REQUEST_DATE,
													"hmac" => base64_encode($HMAC), 
													"platform" => 3											
												)
						);
		
		
		foreach ($POST_DATA as $Key => $Value)
		{
			if (strpos($Key, '_') == FALSE)
			{
				$HEADER_DATA['set_header'][$Key] = $Value;
			}
			
			if ($Key == 'request_data')
			{
				$HEADER_DATA['request_data'] = $POST_DATA['request_data'];
			}			
		}		
		return $HEADER_DATA;
	}
	
	
	###########################
	# Execute Restful API
	###########################
	public function RestfulOperation($POST_DATA)
	{
		$REQUEST_DATA = $this -> auth_header($POST_DATA);

		$RESTFUL_RESPONSE = $this -> exec_curl($REQUEST_DATA);
				
		return $RESTFUL_RESPONSE;
	}
}





###########################
#
# Ctyun Action Class
#
###########################
class Ctyun_Action_Class extends Ctyun_Server_Class
{
	###########################
	# QUERY JOB STATUS
	###########################
	public function query_job_status($CLOUD_UUID,$JOB_DATA,$JOB_UUID)
	{
		if (isset($JOB_DATA -> data))
		{		
			$JOB_ID = $JOB_DATA -> data;
		
			$REQUEST_DATA = $this -> CtyunWebSevMgmt -> query_ctyun_connection_information($CLOUD_UUID);
					
			$AUTH_DATA = array(
								'public_key'   => $REQUEST_DATA['ACCESS_KEY'],
								'secret_key'   => $REQUEST_DATA['SECRET_KEY'],
								'regionId'	   => $REQUEST_DATA['REGION_ID'],
								'jobId'	   	   => $JOB_ID,
								'method_name'  => 'queryJobStatus'
								
							);						
				
			$REQUEST = $this -> RestfulOperation($AUTH_DATA);
		
			$REQUEST = false;
			for ($i=0; $i<80; $i++)
			{		
				$REQUEST = $this -> RestfulOperation($AUTH_DATA);
				if (isset($REQUEST -> job_id))
				{
					if ($REQUEST -> status == 'FAILED')
					{
						$REQUEST = false;
					}
					break;
				}
				else
				{
					if ($JOB_UUID != null)
					{
						$ReplMgmt = new Replica_Class();
						
						if (!isset($COUNT))
						{
							$COUNT = 1;
							$MESSAGE = $ReplMgmt -> job_msg('Checking job status.<sup>%1%</sup>',array($COUNT));
							$ReplMgmt -> update_job_msg($JOB_UUID,$MESSAGE,'Service');
						}
						elseif ($i%4 == 0)
						{
							$COUNT++;
							$MESSAGE = $ReplMgmt -> job_msg('Checking job status.<sup>%1%</sup>',array($COUNT));
							$ReplMgmt -> update_job_msg($JOB_UUID,$MESSAGE,'Service');
						}					
					}
					sleep(15);
				}			
			}		
			return $REQUEST;
		}
		else
		{
			//return json_decode(str_replace(';','',$REQUEST)) -> error -> message;
			return false;
		}
	}	
	
	###########################
	# GENERATE AUTH CREDENTIAL
	###########################
	public function verify_ctyun_credential($POST_DATA)
	{
		$AUTH_DATA = array(
							'public_key'   => $POST_DATA['AccessKey'],	#INPUT
							'secret_key'   => $POST_DATA['SecretKey'],	#INPUT							
							'method_name'  => 'getZoneConfig'
						);
		
		return $this -> RestfulOperation($AUTH_DATA);
	}
		
	
		
	/* 
		VOLUME PART
	*/	
	###########################
	# ATTACH VOLUMES
	###########################
	public function attach_volume($JOB_UUID,$CLOUD_UUID,$HOST_UUID,$VOLUME_ID,$MOUNT_POINT)
	{
		#DETACH VOLUME IF NEED
		$ATTACHMENTS_STATUS = $this -> describe_volumes($CLOUD_UUID,$VOLUME_ID) -> attachments;
		if (count($ATTACHMENTS_STATUS) != 0)
		{
			$this -> detach_volume($CLOUD_UUID,$VOLUME_ID);
		}	
		
		if ($MOUNT_POINT == FALSE)
		{
			$MOUNT_POINT = $this -> get_new_mount_point($CLOUD_UUID,$HOST_UUID);
		}
		
		$REQUEST_DATA = $this -> CtyunWebSevMgmt -> query_ctyun_connection_information($CLOUD_UUID);
		
		$AUTH_DATA = array(
							'public_key'   	=> $REQUEST_DATA['ACCESS_KEY'],
							'secret_key'   	=> $REQUEST_DATA['SECRET_KEY'],
							'method_name'  	=> 'attachVolume'
						);
						
		$AUTH_DATA["request_data"] = array(
								'regionId' 	=> $REQUEST_DATA['REGION_ID'],
								'volumeId' 	=> $VOLUME_ID,
								'vmId'		=> $HOST_UUID,
								'device'	=> $MOUNT_POINT
							);
		
		$REQUEST = $this -> RestfulOperation($AUTH_DATA);
		$REQUEST -> method_name = 'attachVolume';
		
		$JOB_STATUS = $this -> query_job_status($CLOUD_UUID,$REQUEST,$JOB_UUID);
				
		if ($JOB_STATUS != false)
		{
			$VOLUME_INFO = (object)array('serverId' => $HOST_UUID, 'volumeId' => $VOLUME_ID, 'volumePath' => $MOUNT_POINT);
			return $VOLUME_INFO;
		}
		else
		{
			return false;
		}
	}
	
	###########################
	# DETACH VOLUMES
	###########################
	public function detach_volume($CLOUD_UUID,$VOLUME_ID)
	{
		$VOLUME_INFO = $this -> describe_volumes($CLOUD_UUID,$VOLUME_ID) -> attachments[0];
		
		$MOUNT_POINT = $VOLUME_INFO -> device;
		
		$HOST_UUID = $VOLUME_INFO -> server_id;
		
		$REQUEST_DATA = $this -> CtyunWebSevMgmt -> query_ctyun_connection_information($CLOUD_UUID);
		
		$AUTH_DATA = array(
							'public_key'   	=> $REQUEST_DATA['ACCESS_KEY'],
							'secret_key'   	=> $REQUEST_DATA['SECRET_KEY'],
							'method_name'  	=> 'uninstallVolume'
						);
						
		$AUTH_DATA["request_data"] = array(
								'regionId' 	=> $REQUEST_DATA['REGION_ID'],
								'volumeId' 	=> $VOLUME_ID,
								'vmId'		=> $HOST_UUID,
								'device'	=> $MOUNT_POINT
							);
		
		$REQUEST = $this -> RestfulOperation($AUTH_DATA);
		$REQUEST -> method_name = 'uninstallVolume';
		
		$JOB_STATUS = $this -> query_job_status($CLOUD_UUID,$REQUEST,null);
		
		return $JOB_STATUS;
	}
	
	###########################
	# DELETE VOLUME
	###########################
	public function delete_volume($CLOUD_UUID,$VOLUME_ID)
	{
		$REQUEST_DATA = $this -> CtyunWebSevMgmt -> query_ctyun_connection_information($CLOUD_UUID);
		
		$AUTH_DATA = array(
							'public_key'   	=> $REQUEST_DATA['ACCESS_KEY'],
							'secret_key'   	=> $REQUEST_DATA['SECRET_KEY'],
							'method_name'  	=> 'deleteVolume'
						);
						
		$AUTH_DATA["request_data"] = array(
								'regionId' 	=> $REQUEST_DATA['REGION_ID'],
								'volumeId' 	=> $VOLUME_ID
							);
		
		$REQUEST = $this -> RestfulOperation($AUTH_DATA);
		$REQUEST -> method_name = 'deleteVolume';
		
		$JOB_STATUS = $this -> query_job_status($CLOUD_UUID,$REQUEST,null);
		
		return $JOB_STATUS;
	}
	
	###########################
	# DESCRIBE VOLUME
	###########################
	public function describe_volumes($CLOUD_UUID,$VOLUME_ID)
	{
		$REQUEST_DATA = $this -> CtyunWebSevMgmt -> query_ctyun_connection_information($CLOUD_UUID);
		
		$AUTH_DATA = array(
							'public_key'   	=> $REQUEST_DATA['ACCESS_KEY'],
							'secret_key'   	=> $REQUEST_DATA['SECRET_KEY'],
							'regionId' 		=> $REQUEST_DATA['REGION_ID'],
							'volumeId' 		=> $VOLUME_ID,
							'method_name'  	=> 'queryVolumes'
						);
				
		if ($VOLUME_ID == null) #LIST ALL VOLUMES
		{
			unset($AUTH_DATA['volumeId']);
			return $this -> RestfulOperation($AUTH_DATA) -> volumes;
		}
		else
		{
			$VOLUME_INFO = $this -> RestfulOperation($AUTH_DATA);
			
			if (isset($VOLUME_INFO -> volume))
			{
				return $VOLUME_INFO -> volume;
			}
			else
			{
				return $VOLUME_INFO -> volumes[0];
			}
		}
	}
	
	###########################
	# EXPAND VOLUME
	###########################
	public function expand_volume($CLOUD_UUID,$VOLUME_ID,$VOLUME_SIZE)
	{
		$REQUEST_DATA = $this -> CtyunWebSevMgmt -> query_ctyun_connection_information($CLOUD_UUID);
		
		$AUTH_DATA = array(
							'public_key'   	=> $REQUEST_DATA['ACCESS_KEY'],
							'secret_key'   	=> $REQUEST_DATA['SECRET_KEY'],
							'method_name'  	=> 'expandVolumeSize'
						);
						
		$AUTH_DATA["request_data"] = array(
								'regionId' 	=> $REQUEST_DATA['REGION_ID'],
								'volumeId' 	=> $VOLUME_ID,
								'newSize'	=> $VOLUME_SIZE
							);
		
		$REQUEST = $this -> RestfulOperation($AUTH_DATA);
		$REQUEST -> method_name = 'expandVolumeSize';
		
		$JOB_STATUS = $this -> query_job_status($CLOUD_UUID,$REQUEST,null);
		
		return $JOB_STATUS;
	}
		
	###########################
	# RESTORE DISK BACKUP
	###########################
	public function restore_disk_backup($CLOUD_UUID,$JOB_UUID,$VOLUME_ID,$SNAPSHOT_ID)
	{
		$REQUEST_DATA = $this -> CtyunWebSevMgmt -> query_ctyun_connection_information($CLOUD_UUID);
		
		$AUTH_DATA = array(
							'public_key'   	=> $REQUEST_DATA['ACCESS_KEY'],
							'secret_key'   	=> $REQUEST_DATA['SECRET_KEY'],
							'method_name'  	=> 'restoreDiskBackup'
						);
		
		$AUTH_DATA["diskBackup"] = json_encode
									(
										array
											(
												'regionId' 	=> $REQUEST_DATA['REGION_ID'],
												'backupId'	=> $SNAPSHOT_ID,
												'volumeId' 	=> $VOLUME_ID									
											)
									);
	
		$REQUEST = $this -> RestfulOperation($AUTH_DATA);

		$REQUEST -> method_name = 'restoreDiskBackup';
		
		$JOB_STATUS = $this -> query_job_status($CLOUD_UUID,$REQUEST,$JOB_UUID);
		
		return $JOB_STATUS;
	}
	
	
	/*
		SNAPSHOT PART	
	*/	
	###########################
	# SNAPSHOT CONTROL
	###########################
	public function snapshot_control($CLOUD_UUID,$VOLUME_ID,$NUMBER_SNAPSHOT)
	{
		$LIST_SNAPSHOTS = $this -> list_available_snapshot($CLOUD_UUID,$VOLUME_ID);
		
		if (count($LIST_SNAPSHOTS) != 0)
		{
			$SLICE_SNAPSHOT = array_slice($LIST_SNAPSHOTS,($NUMBER_SNAPSHOT-1));
			
			for ($x=0; $x<count($SLICE_SNAPSHOT); $x++)
			{
				$REMOVE_SNAPSHOT_ID = $SLICE_SNAPSHOT[$x] -> id;
				$this -> delete_snapshot($CLOUD_UUID,$REMOVE_SNAPSHOT_ID);
			}
		}	
	}	
	
	
	###########################
	# LIST SNAPSHOT VOLUME ID
	###########################
	public function list_available_snapshot($CLOUD_UUID,$VOLUME_ID)
	{
		$REQUEST_DATA = $this -> CtyunWebSevMgmt -> query_ctyun_connection_information($CLOUD_UUID);
			
		$AUTH_DATA = array(
							'public_key'  => $REQUEST_DATA['ACCESS_KEY'],
							'secret_key'  => $REQUEST_DATA['SECRET_KEY'],
							'regionId' 	  => $REQUEST_DATA['REGION_ID'],
							'volumeId' 	  => $VOLUME_ID,
							'method_name' => 'queryVBSDetails',
						);
			
		return $this -> RestfulOperation($AUTH_DATA) -> backups;	
	}
	
	###########################
	# TAKE SNAPSHOT
	###########################
	public function take_snapshot($CLOUD_UUID,$VOLUME_ID,$HOST_NAME,$SNAP_TIME)
	{				
		$REQUEST_DATA = $this -> CtyunWebSevMgmt -> query_ctyun_connection_information($CLOUD_UUID);
			
		$AUTH_DATA = array(
							'public_key'   => $REQUEST_DATA['ACCESS_KEY'],
							'secret_key'   => $REQUEST_DATA['SECRET_KEY'],
							'method_name'  => 'createVBS'
						);		

		$SNAPSHOT_NAME = $HOST_NAME.'-'.strtotime($SNAP_TIME);
		$SNAPSHOT_META = 'Snapshot Created By SaaSaMe Transport Service';
		
		$AUTH_DATA["request_data"] = array(
								'regionId' => $REQUEST_DATA['REGION_ID'],
								'volumeId' => $VOLUME_ID,
								'name' => $SNAPSHOT_NAME,
								'description' => $SNAPSHOT_META
							);
						
		return $this -> RestfulOperation($AUTH_DATA);
	}	
	
	
	
	###########################
	# DELETE SNAPSHOT
	###########################
	public function delete_snapshot($CLOUD_UUID,$SNAPSHOT_ID)
	{
		$REQUEST_DATA = $this -> CtyunWebSevMgmt -> query_ctyun_connection_information($CLOUD_UUID);
				
		$AUTH_DATA = array(
							'public_key'   => $REQUEST_DATA['ACCESS_KEY'],
							'secret_key'   => $REQUEST_DATA['SECRET_KEY'],
							'method_name'  => 'deleteVBS'
						);
	
		$AUTH_DATA["request_data"] = array(
								'regionId' 	=> $REQUEST_DATA['REGION_ID'],
								'vbsId' 	=> $SNAPSHOT_ID
							);
	
		$this -> RestfulOperation($AUTH_DATA);
	}
	
	###########################
	# DESCRIBE SNAPSHOT
	###########################
	public function describe_snapshot($CLOUD_UUID,$SNAPSHOT_ID)
	{
		$REQUEST_DATA = $this -> CtyunWebSevMgmt -> query_ctyun_connection_information($CLOUD_UUID);
			
		$AUTH_DATA = array(
							'public_key'  => $REQUEST_DATA['ACCESS_KEY'],
							'secret_key'  => $REQUEST_DATA['SECRET_KEY'],
							'regionId' 	  => $REQUEST_DATA['REGION_ID'],
							'vbsId' 	  => $SNAPSHOT_ID,
							'method_name' => 'queryVBSDetail',
						);
			
		return $this -> RestfulOperation($AUTH_DATA);	
	}
	
	
	
	
	/*	
		NETWORK PART	
	*/
	###########################
	# QUERY AVAILABLE NETWORK
	###########################
	public function describe_available_network($CLOUD_UUID)
	{
		$REQUEST_DATA = $this -> CtyunWebSevMgmt -> query_ctyun_connection_information($CLOUD_UUID);
		
		$AUTH_DATA = array(
							'public_key'   => $REQUEST_DATA['ACCESS_KEY'],
							'secret_key'   => $REQUEST_DATA['SECRET_KEY'],
							'regionId'	   => $REQUEST_DATA['REGION_ID'],
							'method_name'  => 'getSubnets'
						);
			
		return $this -> RestfulOperation($AUTH_DATA);
	}
	
	
	###########################
	# DESCRIBE NETWORK
	###########################
	public function describe_network($CLOUD_UUID,$NETWORK_ID)
	{
		$REQUEST_DATA = $this -> CtyunWebSevMgmt -> query_ctyun_connection_information($CLOUD_UUID);
		
		$AUTH_DATA = array(
							'public_key'   => $REQUEST_DATA['ACCESS_KEY'],
							'secret_key'   => $REQUEST_DATA['SECRET_KEY'],
							'regionId'	   => $REQUEST_DATA['REGION_ID'],
							'vpcId'		   => $NETWORK_ID,
							'method_name'  => 'getVpcs'
						);
			
		return $this -> RestfulOperation($AUTH_DATA);
	}
	
	###########################
	# QUERY AVAILABLE SECURITY GROUPS
	###########################
	public function describe_available_security_groups($CLOUD_UUID)
	{
		$REQUEST_DATA = $this -> CtyunWebSevMgmt -> query_ctyun_connection_information($CLOUD_UUID);
		
		$AUTH_DATA = array(
							'public_key'   => $REQUEST_DATA['ACCESS_KEY'],
							'secret_key'   => $REQUEST_DATA['SECRET_KEY'],
							'regionId'	   => $REQUEST_DATA['REGION_ID'],							
							'method_name'  => 'getSecurityGroups'
						);
			
		return $this -> RestfulOperation($AUTH_DATA);
	}
	
	###########################
	# QUERY AVAILABLE PUBLIC ADDRESS
	###########################
	public function describe_available_public_address($CLOUD_UUID)
	{
		$REQUEST_DATA = $this -> CtyunWebSevMgmt -> query_ctyun_connection_information($CLOUD_UUID);
		
		$AUTH_DATA = array(
							'public_key'   => $REQUEST_DATA['ACCESS_KEY'],
							'secret_key'   => $REQUEST_DATA['SECRET_KEY'],
							'regionId'	   => $REQUEST_DATA['REGION_ID'],							
							'method_name'  => 'queryIps'
						);
			
		return $this -> RestfulOperation($AUTH_DATA);
	}
		
	###########################
	# DESCRIBE PUBLIC ADDRESS
	###########################
	public function describe_public_address($CLOUD_UUID,$NETWORK_ID)
	{
		$REQUEST_DATA = $this -> CtyunWebSevMgmt -> query_ctyun_connection_information($CLOUD_UUID);
		
		$AUTH_DATA = array(
							'public_key'   => $REQUEST_DATA['ACCESS_KEY'],
							'secret_key'   => $REQUEST_DATA['SECRET_KEY'],
							'regionId'	   => $REQUEST_DATA['REGION_ID'],
							'publicIpId'   => $NETWORK_ID,
							'method_name'  => 'queryIps'
						);
			
		return $this -> RestfulOperation($AUTH_DATA);
	}
	
	###########################
	# DESCRIBE SECURITY GROUP
	###########################
	public function describe_security_group($CLOUD_UUID,$SGROUP_ID)
	{
		$REQUEST_DATA = $this -> CtyunWebSevMgmt -> query_ctyun_connection_information($CLOUD_UUID);
		
		$AUTH_DATA = array(
							'public_key'		=> $REQUEST_DATA['ACCESS_KEY'],
							'secret_key'  		=> $REQUEST_DATA['SECRET_KEY'],
							'regionId'	   		=> $REQUEST_DATA['REGION_ID'],
							'securityGroupId'	=> $SGROUP_ID,							
							'method_name'  		=> 'querySecurityGroupDetail'
						);			
		return $this -> RestfulOperation($AUTH_DATA);
	}
		

	
	###########################
	# DESCRIBE SUBNET INFORMATION
	###########################
	public function describe_subnet($CLOUD_UUID,$VPC_UUID)
	{
		$REQUEST_DATA = $this -> CtyunWebSevMgmt -> query_ctyun_connection_information($CLOUD_UUID);
			
		$AUTH_DATA = array(
							'public_key'  => $REQUEST_DATA['ACCESS_KEY'],
							'secret_key'  => $REQUEST_DATA['SECRET_KEY'],
							'regionId' 	  => $REQUEST_DATA['REGION_ID'],
							'vpcid'		  => $VPC_UUID,
							'method_name' => 'getSubnets',
						);
			
		return $this -> RestfulOperation($AUTH_DATA);
	}
	
	
	
	
	
	/*	
		INSTANCE PART
	*/
	###########################
	# LIST ALL INSTANCE
	###########################
	public function describe_all_instances($CLOUD_UUID,$FILTER_UUID)
	{
		$REQUEST_DATA = $this -> CtyunWebSevMgmt -> query_ctyun_connection_information($CLOUD_UUID);
	
		$AUTH_DATA = array(
							'public_key'   => $REQUEST_DATA['ACCESS_KEY'],
							'secret_key'   => $REQUEST_DATA['SECRET_KEY'],													
							'method_name'  => 'queryVMs',
							'regionId'	   => $REQUEST_DATA['REGION_ID']
						);
		
		$GetVmDetailList = $this -> RestfulOperation($AUTH_DATA) -> servers;
		
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
	# DESCRIBE INSTANCE
	###########################
	public function describe_instance($CLOUD_UUID,$HOST_UUID)
	{
		$REQUEST_DATA = $this -> CtyunWebSevMgmt -> query_ctyun_connection_information($CLOUD_UUID);
		
		$AUTH_DATA = array(
							'public_key'   => $REQUEST_DATA['ACCESS_KEY'],
							'secret_key'   => $REQUEST_DATA['SECRET_KEY'],							
							'method_name'  => 'queryVMs',
							'regionId'	   => $REQUEST_DATA['REGION_ID'],
							'vmId'		   => $HOST_UUID					
						);
	
		$INSTANCE_INFO = $this -> RestfulOperation($AUTH_DATA);
		
		if (!isset($INSTANCE_INFO -> server))
		{
			$INSTANCE_INFO -> server = $INSTANCE_INFO -> servers[0];
			unset($INSTANCE_INFO -> servers);
		}		
		return $INSTANCE_INFO;
	}
	
	###########################
	# GET IMAGE ID
	###########################
	private function get_image_id($CLOUD_UUID,$HOST_TYPE)
	{
		if ($HOST_TYPE == 'MS')
		{
			$IMAGE_NAME = 'Windows 2012 Standard 64位 英文版';
			//return 'f35c510c-c0d1-482b-ba70-0e5348e822c8';
		}
		else
		{
			$IMAGE_NAME = 'Ubuntu16.04 64位';
			//$IMAGE_NAME = 'CentOS6.8 64位';
			//return '7a5c4128-d921-4541-980c-92e0c6da5478';
		}
		
		$REQUEST_DATA = $this -> CtyunWebSevMgmt -> query_ctyun_connection_information($CLOUD_UUID);
	
		$AUTH_DATA = array(
							'public_key'  => $REQUEST_DATA['ACCESS_KEY'],
							'secret_key'  => $REQUEST_DATA['SECRET_KEY'],
							'imageType'   => 'gold',
							'regionId'	  => $REQUEST_DATA['REGION_ID'],
							'method_name' => 'getImages',
						);
						
		$IMAGE_LIST = $this -> RestfulOperation($AUTH_DATA);
				
		for ($i=0; $i<count($IMAGE_LIST); $i++)
		{
			if ($IMAGE_LIST[$i] -> name == $IMAGE_NAME)
			{
				return $IMAGE_LIST[$i] -> id;
			}
		}		
	}

	###########################
	# FORMAT HOSTNAME
	###########################
	private function format_hostname($HOST_NAME)
	{
		if(is_numeric($HOST_NAME))
		{
			$HOST_NAME = 'CANT-BE-NUM-ONLY';
		}
		else
		{
			$SEARCH  = array('	', ' ', ',', '.', '|');

			$HOST_NAME = str_replace($SEARCH,'_',$HOST_NAME);
			
			if(substr($HOST_NAME,0,1) === '.') #REPLACE DOT AT BEGIN
			{
				$HOST_NAME = str_replace(substr($HOST_NAME,0,1),'s',$HOST_NAME);
			}
			
			if(substr($HOST_NAME,0,1) === '-') #REPLACE DASH AT BEGIN
			{
				$HOST_NAME = str_replace(substr($HOST_NAME,0,1),'s',$HOST_NAME);
			}
				
			if(is_numeric(substr($HOST_NAME,0,1))) #REPLACE NUMBER AT BEGIN
			{
				$HOST_NAME = str_replace(substr($HOST_NAME,0,1),'s',$HOST_NAME);
			}
		}
		return $HOST_NAME;
	}	
	
	###########################
	# CREATE INSTANCE
	###########################
	public function create_instance($JOB_UUID,$CLOUD_UUID,$TRANSPORT_UUID,$HOST_NAME)
	{
		$ServiceMgmt = new Service_Class();
		$SERV_INFO 	= $ServiceMgmt -> query_service($JOB_UUID);
				
		#DEFAULT DATA
		$TRANSPORT_INFO		= $this -> describe_instance($CLOUD_UUID,$TRANSPORT_UUID) -> server;
		$AVAILABILITY_ZONE 	= $TRANSPORT_INFO -> {'OS-EXT-AZ:availability_zone'};
		$ADMIN_PASS			= Misc_Class::password_generator(13);
		$HOST_NAME 			= $this -> format_hostname($HOST_NAME);
	
		if ($SERV_INFO == FALSE)
		{
			$ReplMgmt 			= new Replica_Class();
			$TYPE_OS 			= $ReplMgmt -> query_replica($JOB_UUID)['OS_TYPE'];
				
			$HOST_NAME 			= $HOST_NAME.'-'.time();
			$IMAGE_ID 			= $this -> get_image_id($CLOUD_UUID,($TYPE_OS == 'WINDOWS' ? 'MS' : 'LX'));
			$OS_TYPE 			= ($TYPE_OS == 'WINDOWS' ? 'windows' : 'linux');
			$FLAVOR_TYPE 		= 's3.medium.2';
			$VPC_ID 			= key($TRANSPORT_INFO -> addresses);
			$SECURITY_GROUPS_ID = $this -> describe_available_security_groups($CLOUD_UUID)[0] -> resSecurityGroupId;
			$SUBNET_ID 			= $this -> describe_subnet($CLOUD_UUID,$VPC_ID)[0] -> resVlanId;
			
			$PRIVATE_IP			= 'DynamicAssign';
			$PUBLIC_IP			= 'DynamicAssign';
		}
		else
		{			
			$HOST_NAME 			= "SERV_".$HOST_NAME."-".time();
			$IMAGE_ID 			= $this -> get_image_id($CLOUD_UUID,$SERV_INFO['OS_TYPE']);
			$OS_TYPE 			= ($SERV_INFO['OS_TYPE'] == 'MS' ? 'windows' : 'linux');
			$FLAVOR_TYPE 		= $SERV_INFO['FLAVOR_ID'];
			$VPC_ID				= $SERV_INFO['NETWORK_UUID'];
			$SECURITY_GROUPS_ID = explode('|',$SERV_INFO['SGROUP_UUID'])[0];
			$SUBNET_ID			= json_decode($SERV_INFO['JOBS_JSON']) -> subnet_uuid;
			
			$PRIVATE_IP			= json_decode($SERV_INFO['JOBS_JSON']) -> private_address_id;
			$PUBLIC_IP			= json_decode($SERV_INFO['JOBS_JSON']) -> elastic_address_id;
		}
		
		$REQUEST_DATA = $this -> CtyunWebSevMgmt -> query_ctyun_connection_information($CLOUD_UUID);
		
		$AUTH_DATA = array(
							'public_key'  => $REQUEST_DATA['ACCESS_KEY'],
							'secret_key'  => $REQUEST_DATA['SECRET_KEY'],						
							'method_name' => 'createVM',
						);
		
		$AUTH_DATA['request_data'] = array('createVMInfo' =>
											array(
												'server' => (
														array(
																'availability_zone' => $AVAILABILITY_ZONE,
																'name' => $HOST_NAME,
																'imageRef' => $IMAGE_ID,
																'osType' => $OS_TYPE,
																'root_volume' => array('volumetype' => 'SATA'),
																'flavorRef' => $FLAVOR_TYPE,
																'vpcid' => $VPC_ID,
																'security_groups' => array(array('id' => $SECURITY_GROUPS_ID)),
																'nics' => array(array('subnet_id' => $SUBNET_ID)),
																'adminPass' => $ADMIN_PASS,
																'count' => 1,
																'extendparam' => array('regionID' => $REQUEST_DATA['REGION_ID'])
														)
													)
												)
											);
		
		if ($PRIVATE_IP != 'DynamicAssign')
		{
			$AUTH_DATA['request_data']['createVMInfo']['server']['nics'][0]['ip_address'] = $PRIVATE_IP;
		}
		
		if ($PUBLIC_IP != 'DynamicAssign')
		{
			$AUTH_DATA['request_data']['createVMInfo']['server']['publicip'] = array('id' => $PUBLIC_IP);
		}
		
		$AUTH_DATA['request_data']['createVMInfo'] = json_encode($AUTH_DATA['request_data']['createVMInfo']);
	
		$REQUEST = $this -> RestfulOperation($AUTH_DATA);
		$REQUEST -> method_name = 'createVM';
	
		$JOB_STATUS = $this -> query_job_status($CLOUD_UUID,$REQUEST,$JOB_UUID);
		
		if ($JOB_STATUS != false)
		{
			return $JOB_STATUS -> entities -> sub_jobs[0] -> entities -> server_id;
		}
		else
		{
			return false;
		}
	}
	
	###########################
	# START INSTANCE
	###########################
	public function start_instance($CLOUD_UUID,$HOST_UUID)
	{
		$REQUEST_DATA = $this -> CtyunWebSevMgmt -> query_ctyun_connection_information($CLOUD_UUID);
			
		$AUTH_DATA = array(
							'public_key'  => $REQUEST_DATA['ACCESS_KEY'],
							'secret_key'  => $REQUEST_DATA['SECRET_KEY'],
							'method_name' => 'startVM',
						);
		
		$AUTH_DATA["request_data"] = array(
								'regionId' 	=> $REQUEST_DATA['REGION_ID'],
								'vmId' 		=> $HOST_UUID
							);
	
		$REQUEST = $this -> RestfulOperation($AUTH_DATA);
		$REQUEST -> method_name = 'startVM';
		
		$JOB_STATUS = $this -> query_job_status($CLOUD_UUID,$REQUEST,null);
		
		if ($JOB_STATUS != false)
		{
			return $JOB_STATUS;
		}
		else
		{
			return false;
		}
	}
	
	###########################
	# STOP INSTANCE
	###########################
	public function stop_instance($JOB_UUID,$CLOUD_UUID,$HOST_UUID)
	{
		$REQUEST_DATA = $this -> CtyunWebSevMgmt -> query_ctyun_connection_information($CLOUD_UUID);
			
		$AUTH_DATA = array(
							'public_key'  => $REQUEST_DATA['ACCESS_KEY'],
							'secret_key'  => $REQUEST_DATA['SECRET_KEY'],
							'method_name' => 'stopVM',
						);
						
		$AUTH_DATA["request_data"] = array(
								'regionId' 	=> $REQUEST_DATA['REGION_ID'],
								'vmId' 		=> $HOST_UUID
							);
	
		$REQUEST = $this -> RestfulOperation($AUTH_DATA);
		$REQUEST -> method_name = 'stopVM';
	
		$JOB_STATUS = $this -> query_job_status($CLOUD_UUID,$REQUEST,$JOB_UUID);
		
		if ($JOB_STATUS != false)
		{
			return $JOB_STATUS;
		}
		else
		{
			return false;
		}
	}
	
	###########################
	# DELETE INSTANCE
	###########################
	public function terminate_instances($CLOUD_UUID,$HOST_UUID,$JOB_UUID,$DATAMODE_BOOTABLE)
	{
		if ($JOB_UUID != FALSE)
		{
			$INSTANCE_INFO = $this -> describe_instance($CLOUD_UUID,$HOST_UUID) -> server;
			
			$ReplMgmt = new Replica_Class();
			
			$POWER_STATUS = $INSTANCE_INFO -> status;
			
			if ($POWER_STATUS != 'SHUTOFF')
			{
				$ServiceMgmt = new Service_Class();
				
				$SERV_INFO 	= $ServiceMgmt -> query_service($JOB_UUID);
				$JOB_INFO	= json_decode($SERV_INFO['JOBS_JSON']);
				
				$JOB_INFO -> mark_delete = false;
				$ServiceMgmt -> update_trigger_info($JOB_UUID,$JOB_INFO,'SERVICE');
		
				$MESSAGE = $ReplMgmt -> job_msg('Please power off recovery instance before delete recovery process.');
				$ReplMgmt -> update_job_msg($JOB_UUID,$MESSAGE,'Service');
				exit;
			}
			
			$this -> stop_instance($JOB_UUID,$CLOUD_UUID,$HOST_UUID);
			
			$VOLUMES = $INSTANCE_INFO -> {'os-extended-volumes:volumes_attached'};
			
			$MESSAGE = $ReplMgmt -> job_msg('Detaching volumes from recovery instance.');
			$ReplMgmt -> update_job_msg($JOB_UUID,$MESSAGE,'Service');
			
			for($i=0; $i<count($VOLUMES); $i++)
			{
				$this -> detach_volume($CLOUD_UUID,$VOLUMES[$i] -> id);
			}
		}
		
		if ($DATAMODE_BOOTABLE == FALSE)
		{
			$REQUEST_DATA = $this -> CtyunWebSevMgmt -> query_ctyun_connection_information($CLOUD_UUID);
				
			$AUTH_DATA = array(
								'public_key'  => $REQUEST_DATA['ACCESS_KEY'],
								'secret_key'  => $REQUEST_DATA['SECRET_KEY'],
								'method_name' => 'deleteVM',
							);
							
			$AUTH_DATA["request_data"] = array(
									'regionId' 	=> $REQUEST_DATA['REGION_ID'],
									'vmId' 		=> $HOST_UUID
								);
		
			$REQUEST = $this -> RestfulOperation($AUTH_DATA);
			$REQUEST -> method_name = 'deleteVM';
			
			$JOB_STATUS = $this -> query_job_status($CLOUD_UUID,$REQUEST,null);
			
			if ($JOB_STATUS != false)
			{
				return $JOB_STATUS;
			}
			else
			{
				return false;
			}
		}
	}
	
	###########################
	# QUERY FLAVOR LIST
	###########################
	public function get_flavor_list_detail($CLOUD_UUID)
	{
		$REQUEST_DATA = $this -> CtyunWebSevMgmt -> query_ctyun_connection_information($CLOUD_UUID);
		
		$AUTH_DATA = array(
							'public_key'   => $REQUEST_DATA['ACCESS_KEY'],
							'secret_key'   => $REQUEST_DATA['SECRET_KEY'],
							'regionId'	   => $REQUEST_DATA['REGION_ID'],
							'method_name'  => 'getFlavors'
						);
			
		return $this -> RestfulOperation($AUTH_DATA);
	}
	
	###########################
	# DESCRIBE INSTANCE TYPES
	###########################
	public function describe_instance_types($CLOUD_UUID,$FLAVOR_ID)
	{
		$FLAVOR_LIST = $this -> get_flavor_list_detail($CLOUD_UUID);
		
		for ($i=0; $i<count($FLAVOR_LIST); $i++)
		{
			if ($FLAVOR_LIST[$i] -> id == $FLAVOR_ID)
			{
				return $FLAVOR_LIST[$i];				
			}			
		}
	}
	
	###########################
	# GET NEW MOUNT POINT
	###########################
	public function get_new_mount_point($CLOUD_UUID,$HOST_UUID)
	{
		$REQUEST_DATA = $this -> CtyunWebSevMgmt -> query_ctyun_connection_information($CLOUD_UUID);
		
		$AUTH_DATA = array(
							'public_key'    => $REQUEST_DATA['ACCESS_KEY'],
							'secret_key'    => $REQUEST_DATA['SECRET_KEY'],
							'regionId'	    => $REQUEST_DATA['REGION_ID'],
							'vmId'			=> $HOST_UUID,
							'method_name'   => 'queryAvailableDevice'
						);
			
		return $this -> RestfulOperation($AUTH_DATA)[0];
	}
	
	
	
	
	
	
	
	###########################
	# BEGIN TO CREATE VOLUME FOR LOADER
	###########################
	public function begin_volume_for_loader($CLOUD_UUID,$HOST_UUID,$DISK_SIZE,$JOB_UUID,$TAG_NAME,$IS_BOOT)
	{
		if ($IS_BOOT == TRUE)
		{
			$ReplMgmt = new Replica_Class();

			#CREATE TEMPORARY INSTANCE
			$MESSAGE = $ReplMgmt -> job_msg('Creating temporary instance.');
			$ReplMgmt -> update_job_msg($JOB_UUID,$MESSAGE,'Replica');
			$TEMP_VM_UUID = $this -> create_instance($JOB_UUID,$CLOUD_UUID,$HOST_UUID,$TAG_NAME);			
			if ($TEMP_VM_UUID == FALSE)
			{
				$MESSAGE = $ReplMgmt -> job_msg('Failed to create instance.');
				$ReplMgmt -> update_job_msg($JOB_UUID,$MESSAGE,'Replica');
				exit;
			}
					
			#SLEEP
			$MESSAGE = $ReplMgmt -> job_msg('Waiting for instance ready.');
			$ReplMgmt -> update_job_msg($JOB_UUID,$MESSAGE,'Replica');
			sleep(30*3);
			
			#STOP TEMPORARY INSTANCE
			$MESSAGE = $ReplMgmt -> job_msg('Power off instance.');
			$ReplMgmt -> update_job_msg($JOB_UUID,$MESSAGE,'Replica');
			$STOP_INSTANCE = $this -> stop_instance($JOB_UUID,$CLOUD_UUID,$TEMP_VM_UUID);
			if ($STOP_INSTANCE == FALSE)
			{
				$MESSAGE = $ReplMgmt -> job_msg('Failed to power off instance.');
				$ReplMgmt -> update_job_msg($JOB_UUID,$MESSAGE,'Replica');
				exit;
			}			
			
			#SLEEP
			sleep(30*1);
			
			#GET ROOT DEVICE ID
			$MESSAGE = $ReplMgmt -> job_msg('Getting instance root volume id.');
			$ReplMgmt -> update_job_msg($JOB_UUID,$MESSAGE,'Replica');
			$VOLUME_ID = $this -> describe_instance($CLOUD_UUID,$TEMP_VM_UUID) -> server -> {'os-extended-volumes:volumes_attached'}[0] -> id;
			
			if ($VOLUME_ID == FALSE)
			{
				$MESSAGE = $ReplMgmt -> job_msg('Cannot get instance root id.');
				$ReplMgmt -> update_job_msg($JOB_UUID,$MESSAGE,'Replica');
				exit;			
			}
			
			#DETACH TEMP ROOT VOLUME
			$MESSAGE = $ReplMgmt -> job_msg('Detach root volume from instance.');
			$ReplMgmt -> update_job_msg($JOB_UUID,$MESSAGE,'Replica');
			$DETACH_VOLUME = $this -> detach_volume($CLOUD_UUID,$VOLUME_ID);
			if ($DETACH_VOLUME == FALSE)
			{
				$MESSAGE = $ReplMgmt -> job_msg('Failed to detach root volume from instance.');
				$ReplMgmt -> update_job_msg($JOB_UUID,$MESSAGE,'Replica');
				exit;			
			}

			#EXPEND VOLUME SIZE
			if ($DISK_SIZE > 40)
			{
				$MESSAGE = $ReplMgmt -> job_msg('Expend root volume size to %1%',array($DISK_SIZE.'GB.'));
				$ReplMgmt -> update_job_msg($JOB_UUID,$MESSAGE,'Replica');
				$EXPAND_VOLUME = $this -> expand_volume($CLOUD_UUID,$VOLUME_ID,$DISK_SIZE);
				if ($EXPAND_VOLUME == FALSE)
				{
					$MESSAGE = $ReplMgmt -> job_msg('Failed to expend root volume size.');
					$ReplMgmt -> update_job_msg($JOB_UUID,$MESSAGE,'Replica');
					exit;			
				}
			}			
			
			#DELETE TEMPORARY INSTANCE
			$MESSAGE = $ReplMgmt -> job_msg('Delete temporary instance.');
			$ReplMgmt -> update_job_msg($JOB_UUID,$MESSAGE,'Replica');
			$TERMINATE_INSTANCES = $this -> terminate_instances($CLOUD_UUID,$TEMP_VM_UUID,false,false);
			if ($TERMINATE_INSTANCES == FALSE)
			{
				$MESSAGE = $ReplMgmt -> job_msg('Failed to delete temporary instance.');
				$ReplMgmt -> update_job_msg($JOB_UUID,$MESSAGE,'Replica');
				exit;			
			}
		}
		else
		{
			#GET TANSPORT SERVER INFORMATION
			$INSTANCE_INFO = $this -> describe_instance($CLOUD_UUID,$HOST_UUID) -> server;
						
			#GET TANSPORT SERVER ZONE ID
			$REQUEST_ZONE_ID = $INSTANCE_INFO -> {'OS-EXT-AZ:availability_zone'};
			
			#QUERY CLOUD INFO
			$REQUEST_DATA = $this -> CtyunWebSevMgmt -> query_ctyun_connection_information($CLOUD_UUID);
					
			$AUTH_DATA = array(
								'public_key'   => $REQUEST_DATA['ACCESS_KEY'],
								'secret_key'   => $REQUEST_DATA['SECRET_KEY'],
								'method_name'  => 'createVolume'
							);
			
			#VOLUME METADATA
			$NOW_TIME = Misc_Class::current_utc_time();
			$DISK_NAME = $TAG_NAME."-".time();
			$DISK_META = "Volume Created By SaaSaMe Transport Service @ ".$NOW_TIME;
			$DISK_TYPE = 'SATA';
					
			$AUTH_DATA["request_data"] = array("createVolumeInfo" => 
												json_encode
												(
													array
													(
														'regionId' 		=> $REQUEST_DATA['REGION_ID'],
														'zoneId'		=> $REQUEST_ZONE_ID,	
														'name'			=> $DISK_NAME,
														'type'			=> $DISK_TYPE,													
														'size'			=> $DISK_SIZE
													)
												)
											);
			
			$REQUEST = $this -> RestfulOperation($AUTH_DATA);
			$REQUEST -> method_name = 'createVolume';
			
			$JOB_STATUS = $this -> query_job_status($CLOUD_UUID,$REQUEST,null);
			
			if ($JOB_STATUS != false)
			{
				$VOLUME_ID = $JOB_STATUS -> entities -> volume_id;
			}
			else
			{
				return false;
			}
		}
		
		return $this -> attach_volume($JOB_UUID,$CLOUD_UUID,$HOST_UUID,$VOLUME_ID,false);
	}	
	
	
	###########################
	# ATTACH SUBSCRIPTION VOLUME
	###########################
	public function attach_subscription_volume($JOB_UUID,$CLOUD_UUID,$HOST_UUID,$VOLUME_ID)
	{
		return $this -> attach_volume($JOB_UUID,$CLOUD_UUID,$HOST_UUID,$VOLUME_ID,false);
	}
	
	
	###########################
	# CREATE VOLUME BY SNAPSHOT
	###########################
	public function create_volume_by_snapshot($CLOUD_UUID,$JOB_UUID,$SNAPSHOT_ID)
	{
		$ReplMgmt = new Replica_Class();
		$ServiceMgmt = new Service_Class();
				
		$SERV_INFO 	= $ServiceMgmt -> query_service($JOB_UUID);
		$JOB_INFO	= json_decode($SERV_INFO['JOBS_JSON']);

		#GET VOLUME ID FROM SNAPSHOT
		$VOLUME_ID = $this -> describe_snapshot($CLOUD_UUID,$SNAPSHOT_ID) -> backup -> volume_id;
		
		#DETACH VOLUME FROM TRANSPORT SERVER
		$MESSAGE = $ReplMgmt -> job_msg('Detach Volume from Transport Server.');
		$ReplMgmt -> update_job_msg($JOB_UUID,$MESSAGE,'Service');
		$this -> detach_volume($CLOUD_UUID,$VOLUME_ID);		
		
		#CREATE TEMPORARY RECOVER SNAPSHOT
		$MESSAGE = $ReplMgmt -> job_msg('Creating recovery rollback snapshot.');
		$ReplMgmt -> update_job_msg($JOB_UUID,$MESSAGE,'Service');
		
		$HOST_NAME = "RecoverRollback-".explode("-",$JOB_UUID)[0];
		$SNAP_TIME = date("Y-m-d h:i:s");
		
		$RESPONSE = $this -> take_snapshot($CLOUD_UUID,$VOLUME_ID,$HOST_NAME,$SNAP_TIME);
		$JOB_STATUS = $this -> query_job_status($CLOUD_UUID,$RESPONSE,$JOB_UUID);
		$ROLLBACK_ID = $JOB_STATUS -> entities -> backup_id;
		
		if (!isset($JOB_INFO -> rollback_id))
		{
			$JOB_INFO -> rollback_id = new stdClass();
		}
		$JOB_INFO -> rollback_id -> $VOLUME_ID = $ROLLBACK_ID;
		$ServiceMgmt -> update_trigger_info($JOB_UUID,$JOB_INFO,'SERVICE');

		#BEGIN TO RESTORE SELECTED SNAPSHOT
		$MESSAGE = $ReplMgmt -> job_msg('Restoring selected recover snapshot to disk.');
		$ReplMgmt -> update_job_msg($JOB_UUID,$MESSAGE,'Service');
		$this -> restore_disk_backup($CLOUD_UUID,$JOB_UUID,$VOLUME_ID,$SNAPSHOT_ID);
	
		return $VOLUME_ID;
		
		/*
		#GET TANSPORT SERVER INFORMATION
		$INSTANCE_INFO = $this -> describe_instance($CLOUD_UUID,$HOST_UUID) -> server;

		#GET TANSPORT SERVER ZONE ID
		$REQUEST_ZONE_ID = $INSTANCE_INFO -> {'OS-EXT-AZ:availability_zone'};
				
		#QUERY CLOUD INFO
		$REQUEST_DATA = $this -> CtyunWebSevMgmt -> query_ctyun_connection_information($CLOUD_UUID);
				
		$AUTH_DATA = array(
							'public_key'   => $REQUEST_DATA['ACCESS_KEY'],
							'secret_key'   => $REQUEST_DATA['SECRET_KEY'],
							'regionId'	   => $REQUEST_DATA['REGION_ID'],
							'method_name'  => 'createVolume'
						);
		
		#VOLUME METADATA
		$NOW_TIME = Misc_Class::current_utc_time();
		$DISK_NAME = "Snap-".$HOST_NAME."@".$NOW_TIME;
		$DISK_META = "Volume Created By SaaSaMe Transport Service @ ".$NOW_TIME;
		$DISK_TYPE = 'SATA';
			
		if ($DISK_SIZE <= 40){$DISK_SIZE = 100;}
			
		$AUTH_DATA["request_data"] = array("createVolumeInfo" => 
											json_encode
											(
												array
												(
													'regionId' 		=> $REQUEST_DATA['REGION_ID'],
													'zoneId'		=> $REQUEST_ZONE_ID,	
													'name'			=> $DISK_NAME,
													'type'			=> $DISK_TYPE,
													'size'			=> $DISK_SIZE,
													'backupId'		=> $SNAPSHOT_ID
												)
											)
										);
		$REQUEST = $this -> RestfulOperation($AUTH_DATA);
		$REQUEST -> method_name = 'createVolumeFromSnapshot';
		
		$JOB_STATUS = $this -> query_job_status($CLOUD_UUID,$REQUEST,null);
		
		if ($JOB_STATUS != false)
		{
			return $JOB_STATUS -> entities -> volume_id;
		}
		else
		{
			return false;
		}
		*/
	}
	
	###########################
	# ROLLBACK BACKUP TO VOLUME
	###########################
	public function rollback_backup_to_volume($CLOUD_UUID,$JOB_UUID,$SERVER_UUID,$VOLUME_ID)
	{
		$ATTACHMENTS_STATUS = $this -> describe_volumes($CLOUD_UUID,$VOLUME_ID) -> attachments;
		if (count($ATTACHMENTS_STATUS) != 0)
		{
			$this -> detach_volume($CLOUD_UUID,$VOLUME_ID);
		}
		
		$ReplMgmt = new Replica_Class();
		$ServiceMgmt = new Service_Class();
		
		$SERV_INFO 	 = $ServiceMgmt -> query_service($JOB_UUID);
		$ROLLBACK_ID = json_decode($SERV_INFO['JOBS_JSON']) -> rollback_id -> $VOLUME_ID;
		
		$MESSAGE = $ReplMgmt -> job_msg('Restoring rollback snapshot to disk.');
		$ReplMgmt -> update_job_msg($JOB_UUID,$MESSAGE,'Service');
		$this -> restore_disk_backup($CLOUD_UUID,$JOB_UUID,$VOLUME_ID,$ROLLBACK_ID);

		$MESSAGE = $ReplMgmt -> job_msg('Attach volume back to transport server.');
		$ReplMgmt -> update_job_msg($JOB_UUID,$MESSAGE,'Service');
		$this -> attach_volume($JOB_UUID,$CLOUD_UUID,$SERVER_UUID,$VOLUME_ID,false);
	
		$MESSAGE = $ReplMgmt -> job_msg('Delete rollback snapshot.');
		$ReplMgmt -> update_job_msg($JOB_UUID,$MESSAGE,'Service');
		$this -> delete_snapshot($CLOUD_UUID,$ROLLBACK_ID);	
	}
	
	
	###########################
	# BEGIN TO RUN RECOVERY INSTANCE
	###########################	
	public function begin_to_run_recovery_instance($JOB_UUID,$CLOUD_UUID,$TRAN_UUID,$HOST_NAME,$VOLUME_INFO)
	{
		$ReplMgmt = new Replica_Class();
		$ServiceMgmt = new Service_Class();
		
		$SERV_QUERY	= $ServiceMgmt -> query_service($JOB_UUID);
		
		$DATAMODE_INSTANCE = json_decode($SERV_QUERY['JOBS_JSON']) -> datamode_instance;
		$DATAMODE_BOOTABLE = json_decode($SERV_QUERY['JOBS_JSON']) -> is_datamode_boot;
		
		if ($DATAMODE_INSTANCE != 'NoAssociatedDataModeInstance' AND $DATAMODE_BOOTABLE == TRUE)
		{
			#ATTACH CONVERT VOLUME
			for ($i=0; $i<count($VOLUME_INFO); $i++)
			{
				$MOUNT_POINT = ($i == 0 ? '/dev/sda' : false);
			
				$MESSAGE = $ReplMgmt -> job_msg('Attach recovery volume %1% to instance.',array($i));
				$ReplMgmt -> update_job_msg($JOB_UUID,$MESSAGE,'Service');
						
				$ATTACH_VOLUME = $this -> attach_volume($JOB_UUID,$CLOUD_UUID,$DATAMODE_INSTANCE,$VOLUME_INFO[$i],$MOUNT_POINT);
				if ($ATTACH_VOLUME == FALSE)
				{
					$MESSAGE = $ReplMgmt -> job_msg('Cannot get attach volume to the recovery instance.');
					$ReplMgmt -> update_job_msg($JOB_UUID,$MESSAGE,'Service');
					exit;	
				}
			}
			
			#POWER ON INSTANCE
			$MESSAGE = $ReplMgmt -> job_msg('Power on instance.');
			$ReplMgmt -> update_job_msg($JOB_UUID,$MESSAGE,'Service');
			$START_INSTANCE = $this -> start_instance($CLOUD_UUID,$DATAMODE_INSTANCE);
			if ($START_INSTANCE == FALSE)
			{
				$MESSAGE = $ReplMgmt -> job_msg('Failed to power on recovery instance.');
				$ReplMgmt -> update_job_msg($JOB_UUID,$MESSAGE,'Service');
				exit;			
			}	
			return $DATAMODE_INSTANCE;
		}	
		
		$MESSAGE = $ReplMgmt -> job_msg('Getting instance ready.');
		$ReplMgmt -> update_job_msg($JOB_UUID,$MESSAGE,'Service');
		
		$INSTANCE_UUID = $this -> create_instance($JOB_UUID,$CLOUD_UUID,$TRAN_UUID,$HOST_NAME);
		if ($INSTANCE_UUID == FALSE)
		{
			$MESSAGE = $ReplMgmt -> job_msg('Failed to create instance.');
			$ReplMgmt -> update_job_msg($JOB_UUID,$MESSAGE,'Service');
			exit;			
		}
		
		#SLEEP 60*2 SECONDS
		sleep(60*2);
		
		#STOP INSTANCE
		$MESSAGE = $ReplMgmt -> job_msg('Power off instance.');
		$ReplMgmt -> update_job_msg($JOB_UUID,$MESSAGE,'Service');
		$STOP_INSTANCE = $this -> stop_instance($JOB_UUID,$CLOUD_UUID,$INSTANCE_UUID);
		if ($STOP_INSTANCE == FALSE)
		{
			$MESSAGE = $ReplMgmt -> job_msg('Failed to stop instance.');
			$ReplMgmt -> update_job_msg($JOB_UUID,$MESSAGE,'Service');
			exit;			
		}		
			
		#SLEEP 30 SECONDS
		sleep(30*1);
				
		#GET ROOT DEVICE ID
		$MESSAGE = $ReplMgmt -> job_msg('Getting instance temporary root volume id.');
		$ReplMgmt -> update_job_msg($JOB_UUID,$MESSAGE,'Service');
		
		$ROOT_VOLUME = $this -> describe_instance($CLOUD_UUID,$INSTANCE_UUID) -> server -> {'os-extended-volumes:volumes_attached'}[0] -> id;
		if ($ROOT_VOLUME == FALSE)
		{
			$MESSAGE = $ReplMgmt -> job_msg('Cannot get instance temporary root volume ID.');
			$ReplMgmt -> update_job_msg($JOB_UUID,$MESSAGE,'Service');
			exit;			
		}		
		
		#SLEEP ONE MINUTES
		sleep(30*1);
		
		#DETACH TEMP ROOT VOLUME
		$MESSAGE = $ReplMgmt -> job_msg('Detach temporary root volume from instance.');
		$ReplMgmt -> update_job_msg($JOB_UUID,$MESSAGE,'Service');
		$DETACH_VOLUME = $this -> detach_volume($CLOUD_UUID,$ROOT_VOLUME);
		if ($DETACH_VOLUME == FALSE)
		{
			$MESSAGE = $ReplMgmt -> job_msg('Cannot get temporary volume id.');
			$ReplMgmt -> update_job_msg($JOB_UUID,$MESSAGE,'Service');
			exit;			
		}	
				
		#DELETE TEMP ROOT VOLUME
		$MESSAGE = $ReplMgmt -> job_msg('Delete temporary root volume.');
		$ReplMgmt -> update_job_msg($JOB_UUID,$MESSAGE,'Service');
		$DELETE_TEMP = $this -> delete_volume($CLOUD_UUID,$ROOT_VOLUME);
		if ($DELETE_TEMP == FALSE)
		{
			$MESSAGE = $ReplMgmt -> job_msg('Failed to delete temporary root volume.');
			$ReplMgmt -> update_job_msg($JOB_UUID,$MESSAGE,'Service');
			exit;			
		}		
		
		#ATTACH VOLUME
		$MESSAGE = $ReplMgmt -> job_msg('Attach recovery volumes to instance.');
		$ReplMgmt -> update_job_msg($JOB_UUID,$MESSAGE,'Service');
		for ($i=0; $i<count($VOLUME_INFO); $i++)
		{
			$MOUNT_POINT = ($i == 0 ? '/dev/sda' : false);
			
			$ATTACH_VOLUME = $this -> attach_volume($JOB_UUID,$CLOUD_UUID,$INSTANCE_UUID,$VOLUME_INFO[$i],$MOUNT_POINT);
			if ($ATTACH_VOLUME == FALSE)
			{
				$MESSAGE = $ReplMgmt -> job_msg('Cannot get attach volume to the recovery instance.');
				$ReplMgmt -> update_job_msg($JOB_UUID,$MESSAGE,'Service');
				exit;			
			}
		}
		
		#POWER ON INSTANCE
		$MESSAGE = $ReplMgmt -> job_msg('Power on instance.');
		$ReplMgmt -> update_job_msg($JOB_UUID,$MESSAGE,'Service');
		$START_INSTANCE = $this -> start_instance($CLOUD_UUID,$INSTANCE_UUID);
		if ($START_INSTANCE == FALSE)
		{
			$MESSAGE = $ReplMgmt -> job_msg('Failed to power on recovery instance.');
			$ReplMgmt -> update_job_msg($JOB_UUID,$MESSAGE,'Service');
			exit;			
		}
	
		return $INSTANCE_UUID;
	}
	
	
		
	###########################
	# DELETE REPLICA JOB
	###########################
	public function delete_replica_job($CLOUD_UUID,$HOST_UUID,$DISK_UUID)
	{
		for ($i=0; $i<count($DISK_UUID); $i++)
		{			
			$VOLUME_ID = $DISK_UUID[$i]['OPEN_DISK_ID'];
						
			$LIST_SNAPSHOT = $this -> list_available_snapshot($CLOUD_UUID,$VOLUME_ID);
			if (count($LIST_SNAPSHOT) != 0)
			{
				for ($x=0; $x<count($LIST_SNAPSHOT); $x++)
				{
					$SNAPSHOT_ID = $LIST_SNAPSHOT[$x] -> id;
					$this -> delete_snapshot($CLOUD_UUID,$SNAPSHOT_ID);
				}
			}
					
			$this -> detach_volume($CLOUD_UUID,$VOLUME_ID);
			
			$volume_status = $this -> describe_volumes($CLOUD_UUID,$VOLUME_ID);
			
			if (!isset($volume_status -> metadata -> billing))
			{
				$this -> delete_volume($CLOUD_UUID,$VOLUME_ID);	
			}
		}
	}	
}





###################################
#
#	CTYUN SERVICES QUERY CLASS
#
###################################
class Ctyun_Query_Class extends Db_Connection
{
	###########################
	#CREATE NEW CTYUN CONNECTION
	###########################
	public function create_ctyun_connection($POST_DATA)
	{
		$ACCT_UUID 	 = $POST_DATA['AcctUUID'];
		$REGN_UUID 	 = $POST_DATA['RegnUUID'];
		$CLOUD_UUID  = Misc_Class::guid_v4();
		$ACCESS_KEY  = $POST_DATA['AccessKey'];
		$SECRET_KEY  = $POST_DATA['SecretKey'];		
		$REGION_NAME = explode("|",$POST_DATA['SelectRegion'])[0];
		$REGION_ID   = explode("|",$POST_DATA['SelectRegion'])[1];
		$AUTH_TOKEN  = json_encode(array('project_region' => $REGION_ID, 'vendor_name' => 'Ctyun'));
	
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
								_CLOUD_TYPE,
								_STATUS)
							VALUE(
								'',
								
								'".$ACCT_UUID."',
								'".$REGN_UUID."',
								'".$CLOUD_UUID."',
								
								'".$REGION_NAME."',								
								'".Misc_Class::encrypt_decrypt('encrypt',$ACCESS_KEY)."',
								'".Misc_Class::encrypt_decrypt('encrypt',$SECRET_KEY)."',
								'None',
								
								'".$AUTH_TOKEN."',
								
								'".Misc_Class::current_utc_time()."',
								'5',
								'Y')";
								
		$this -> DBCON -> prepare($INSERT_EXEC) -> execute();
		
		return $INSERT_EXEC;
	}
	
	
	
	###########################
	#QUERY CTYUN KEY INFORMATION
	###########################
	public function query_ctyun_connection_information($CLUSTER_UUID)
	{
		$GET_EXEC 	= "SELECT * FROM _CLOUD_MGMT WHERE _CLUSTER_UUID = '".strtoupper($CLUSTER_UUID)."' AND _STATUS = 'Y'";
		
		$QUERY = $this -> DBCON -> prepare($GET_EXEC);
		$QUERY -> execute();
		$COUNT_ROWS = $QUERY -> rowCount();
		
		if ($COUNT_ROWS != 0)
		{
			foreach($QUERY as $QueryResult)
			{
				$CLOUD_DATA = array(
									"ACCT_UUID" 	 => $QueryResult['_ACCT_UUID'],
									"REGN_UUID" 	 => $QueryResult['_REGN_UUID'],
									"CLUSTER_UUID" 	 => $QueryResult['_CLUSTER_UUID'],
									"PROJECT_NAME" 	 => $QueryResult['_PROJECT_NAME'],
								
									"ACCESS_KEY" 	 => Misc_Class::encrypt_decrypt('decrypt',$QueryResult['_CLUSTER_USER']),
									"SECRET_KEY" 	 => Misc_Class::encrypt_decrypt('decrypt',$QueryResult['_CLUSTER_PASS']),
									"DEFAULT_ADDR"	 => $QueryResult['_CLUSTER_ADDR'],
								
									"USER_UUID" 	 => $QueryResult['_AUTH_TOKEN'],
									"REGION_ID"		 => json_decode($QueryResult['_AUTH_TOKEN']) -> project_region,
									"TIMESTAMP"		 => $QueryResult['_TIMESTAMP']
								);	
			}
			
			return $CLOUD_DATA;
		}
		else
		{
			return false;
		}	
	}
	
	
	###########################
	#UPDATE CTYUN KEY INFORMATION
	###########################
	public function update_ctyun_connection($POST_DATA)
	{
		$CLUSTER_UUID = $POST_DATA['ClusterUUID'];
		$ACCESS_KEY   = $POST_DATA['AccessKey'];
		$SECRET_KEY   = $POST_DATA['SecretKey'];		
		$REGION_NAME  = explode("|",$POST_DATA['SelectRegion'])[0];
		$REGION_ID    = explode("|",$POST_DATA['SelectRegion'])[1];
		$AUTH_TOKEN   = json_encode(array('project_region' => $REGION_ID, 'vendor_name' => 'Ctyun'));		
		
		$UPDATE_EXEC = "UPDATE _CLOUD_MGMT
						SET
							_PROJECT_NAME	= '".$REGION_NAME."',
							_CLUSTER_USER 	= '".Misc_Class::encrypt_decrypt('encrypt',$ACCESS_KEY)."',
							_CLUSTER_PASS	= '".Misc_Class::encrypt_decrypt('encrypt',$SECRET_KEY)."',
							_AUTH_TOKEN		= '".$AUTH_TOKEN."',
							_TIMESTAMP		= '".Misc_Class::current_utc_time()."'
						WHERE
							_CLUSTER_UUID 	= '".$CLUSTER_UUID."'";
		
		$QUERY = $this -> DBCON -> prepare($UPDATE_EXEC) -> execute();
		return true;
	}	
}

