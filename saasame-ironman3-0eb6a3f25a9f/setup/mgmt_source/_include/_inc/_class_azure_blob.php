<?php
###########################
#
# Azure Blob Action Class
#
###########################
class Azure_Blob_Action_Class extends Azure_Blob_Query_Class
{
	###########################
	#CONSTRUCT FUNCTION
	###########################
	public function __construct()
	{
		parent::__construct();	
	}	
	
	###########################
	# BLOB SNAPSHOT CONTROL
	###########################
	public function snapshot_control($REPL_UUID,$DISK_ID,$NUMBER_SNAPSHOT)
	{
		$ServiceMgmt = new Service_Class();
			
		$DISK_NAME = $DISK_ID.'.vhd';
		
		$CONN_STRING = $this -> get_blob_connection_string($REPL_UUID) -> ConnectionString;
		
		$LIST_SNAPSHOTS = $ServiceMgmt -> get_vhd_disk_snapshots($CONN_STRING,$REPL_UUID,$DISK_NAME);
		
		$SLICE_SNAPSHOT = array_slice(array_reverse($LIST_SNAPSHOTS),$NUMBER_SNAPSHOT);
		
		for ($x=0; $x<count($SLICE_SNAPSHOT); $x++)
		{
			$REMOVE_SNAPSHOT_ID = $SLICE_SNAPSHOT[$x] -> id;
			$ServiceMgmt -> delete_vhd_disk_snapshot($CONN_STRING,$REPL_UUID,$DISK_NAME,$REMOVE_SNAPSHOT_ID);
		}		
		return true;
	}

	###########################
	# LIST DISK SNAPSHOT
	###########################
	public function list_snapshot($REPL_UUID,$DISK_ID)
	{
		$ServiceMgmt = new Service_Class();
			
		$DISK_NAME = $DISK_ID.'.vhd';
		
		$CONN_STRING = $this -> get_blob_connection_string($REPL_UUID) -> ConnectionString;
		
		$LIST_SNAPSHOTS = $ServiceMgmt -> get_vhd_disk_snapshots($CONN_STRING,$REPL_UUID,$DISK_NAME);
		
		if (count($LIST_SNAPSHOTS) != 0)
		{
			for ($i=0; $i<count($LIST_SNAPSHOTS); $i++)
			{
				$LIST_SNAPSHOTS[$i] -> description = 'Snapshot Created By SaaSaMe Transport Service @ '.$LIST_SNAPSHOTS[$i] -> datetime;				
			}
		}
		return $LIST_SNAPSHOTS;
	}
	
	
	###########################
	# GET SNAPSHOT INFORMATION
	###########################
	public function get_snapshot_info($REPL_UUID,$SNAPSHOT_ID)
	{
		#GET CONNECTION STRING INFORMATION
		$ServiceMgmt = new Service_Class();		
		$CONN_STRING = $this -> get_blob_connection_string($REPL_UUID) -> ConnectionString;
			
		#GET REPLICA DISK INFORMATION
		$ReplicaMgmt = new Replica_Class();		
		$REPL_DISK = $ReplicaMgmt -> query_replica_disk($REPL_UUID);

		for ($i=0; $i<count($REPL_DISK); $i++)
		{
			$DISK_NAME = $REPL_DISK[$i]['OPEN_DISK'].'.vhd';
			$DISK_SIZE = ceil($REPL_DISK[$i]['DISK_SIZE']/1024/1024/1024);
			$LIST_SNAPSHOTS = $ServiceMgmt -> get_vhd_disk_snapshots($CONN_STRING,$REPL_UUID,$DISK_NAME);

			if ($SNAPSHOT_ID == 'UseLastSnapshot')
			{
					$LAST_SNAPSHOT[] = array(
											'snapshot_id' => end($LIST_SNAPSHOTS) -> id,
											'tags' 		  => array('Description' => 'Snapshot Created By SaaSaMe Transport Service @ '.end($LIST_SNAPSHOTS) -> datetime),
											'properties'  => array('diskSizeGB'  => $DISK_SIZE, 
															   'timeCreated' => end($LIST_SNAPSHOTS) -> datetime,
															   'diskName'	 => $DISK_NAME)
										);
			}
			else
			{
				$SNAP_ID = explode(',',$SNAPSHOT_ID);
				for ($w=0; $w<count($SNAP_ID); $w++)
				{
					for ($x=0; $x<count($LIST_SNAPSHOTS); $x++)
					{
						if ($LIST_SNAPSHOTS[$x] -> id == $SNAP_ID[$w])
						{
							$LAST_SNAPSHOT[] = array(
													'snapshot_id' => $LIST_SNAPSHOTS[$x] -> id,
													'tags' 		  => array('Description' => 'Snapshot Created By SaaSaMe Transport Service @ '.$LIST_SNAPSHOTS[$x] -> datetime),
													'properties'  => array('diskSizeGB'  => $DISK_SIZE, 
																		   'timeCreated' => $LIST_SNAPSHOTS[$x] -> datetime,
																		   'diskName'	 => $DISK_NAME)
												);
							break;
						}
					}
				}
			}
		}
		return $LAST_SNAPSHOT;
	}
	
	
	###########################
	# CREATE_UNMGNT_VOLUME_FROM_SNAPSHOT
	###########################
	public function create_volume_from_snapshot($CLOUD_UUID,$SERVER_UUID,$SERVER_ZONE,$REPL_UUID,$IS_MGMT_DISK,$DISK_COUNT,$DISK_NAME,$DISK_SIZE,$SNAPSHOT_ID,$BLOB_DIRECT)
	{
		//Misc_Class::function_debug('_mgmt',__FUNCTION__,func_get_args());
		
		$ServiceMgmt = new Service_Class();
		
		$CONN_STRING = $this -> get_blob_connection_string($REPL_UUID) -> ConnectionString;
	
		$TEMP_DISK_NAME = time();
		
		$TASK_ID = $ServiceMgmt -> create_vhd_disk_from_snapshot($CONN_STRING,$REPL_UUID,$DISK_NAME,$TEMP_DISK_NAME.'.vhd',$SNAPSHOT_ID);
		
		$TASK_READY = false;
		$TRY_COUNT = 3;
		while(!$TASK_READY OR $TRY_COUNT < 0)
		{
			try{			
				$TASK_READY = $ServiceMgmt -> is_snapshot_vhd_disk_ready($TASK_ID);
			}
			catch (Exception $e){
				break;
			}
			sleep(5);
			$TRY_COUNT--;
		}
		
		if($IS_MGMT_DISK == FALSE OR $BLOB_DIRECT == TRUE)
		{
			$VOLUME_UUID = $TEMP_DISK_NAME.".vhd";
		}
		else
		{
			$AzureMgmt = new Azure_Controller();
			
			$AzureMgmt -> getVMByServUUID($SERVER_UUID,$CLOUD_UUID);
			
			#VOLUME NAME
			$VOLUME_UUID = "SaaSaMe-".$REPL_UUID.'-'.$DISK_COUNT.'-'.time();			
		
			$TRYCOUNT = 3;
			while($TRYCOUNT > 0)
			{
				try{
					$AzureMgmt -> CreateDisk_BlobMode($CLOUD_UUID,$VOLUME_UUID,$CONN_STRING,$REPL_UUID,$TEMP_DISK_NAME,$SERVER_ZONE,$DISK_SIZE);
					break;
				}
				catch (Exception $e){
					$VOLUME_UUID = false;
				}
				sleep(5);
				$TRYCOUNT--;
			}								
			$this -> delete_temp_vhd_disk_in_container($REPL_UUID,$TEMP_DISK_NAME);
		}
		return $VOLUME_UUID;
	}
	
	###########################
	# ATTACH UNMANAGEMENT DISK
	###########################
	public function attach_volume($CLOUD_UUID,$SERVER_UUID,$SERVER_NAME,$SERVER_ZONE,$REPL_UUID,$VOLUME_UUID,$DISK_SIZE)
	{
		$AZURE_CONN_INFO = $this -> get_blob_connection_string($REPL_UUID);
		$ACCT_NAME = $AZURE_CONN_INFO -> AccountName;
		$CONN_STRING = $AZURE_CONN_INFO -> ConnectionString;
									
		$DISK_INFO = array("diskSize"=>$DISK_SIZE,
						   "storageAccount"=> $ACCT_NAME,
						   "container"=> $REPL_UUID,
						   "filename"=>$VOLUME_UUID
		);
	
		$AzureMgmt = new Azure_Controller();
		$AzureMgmt -> getVMByServUUID($SERVER_UUID,$CLOUD_UUID);
		$ATTACH_VOLUME = $AzureMgmt -> attach_vhd($CLOUD_UUID,$SERVER_ZONE,$SERVER_NAME,$DISK_INFO,$CONN_STRING);
		return $ATTACH_VOLUME;
	}
	
	###########################
	# CREATE BLOB VOLUME AND CLEANUP
	###########################
	public function detach_volume($CLOUD_UUID,$SERVER_UUID,$SERVER_ZONE,$SERVICE_UUID,$VHD_NAME,$DISK_COUNT,$DISK_SIZE)
	{
		//Misc_Class::function_debug('_mgmt',__FUNCTION__,func_get_args());
		
		$ServiceMgmt = new Service_Class();
		$SERVICE_INFO = $ServiceMgmt -> query_service($SERVICE_UUID);
		$IS_PROMOTE = json_decode($SERVICE_INFO['JOBS_JSON']) -> is_promote;
		$IS_MGMT_DISK = json_decode($SERVICE_INFO['JOBS_JSON']) -> is_azure_mgmt_disk;
		
		$AzureMgmt = new Azure_Controller();
		$AzureMgmt -> getVMByServUUID($SERVER_UUID,$CLOUD_UUID);
		
		if ($AzureMgmt -> IsAzureStack($CLOUD_UUID) == TRUE)
		{		
			return $VHD_NAME;
		}
		else
		{
			if ($IS_PROMOTE == FALSE AND $IS_MGMT_DISK == FALSE)
			{
				return $VHD_NAME;
			}
			else
			{			
				$CONTAINER_ID = $SERVICE_INFO['REPL_UUID'];
				$HOST_NAME = $SERVICE_INFO['HOST_NAME'];
				$CONN_STRING = $this -> get_blob_connection_string($CONTAINER_ID) -> ConnectionString;
				$DISK_SIZE = ceil($DISK_SIZE / 1024 / 1024);
				$VOLUME_UUID = 'SNAP-'.$HOST_NAME.'-'.$DISK_COUNT.'-'.time();
				$DISK_NAME = str_replace(".vhd","",$VHD_NAME);
				
				$AzureMgmt -> CreateDisk_BlobMode($CLOUD_UUID,$VOLUME_UUID,$CONN_STRING,$CONTAINER_ID,$DISK_NAME,$SERVER_ZONE,$DISK_SIZE);
				
				$this -> update_direct_blob_disk($VHD_NAME,$VOLUME_UUID);
					
				$this -> delete_temp_vhd_disk_in_container($CONTAINER_ID,$VHD_NAME);
				return $VOLUME_UUID;			
			}
		}
	}
	
	###########################
	# DELETE VHD DISK FROM CONTAINER
	###########################
	public function delete_temp_vhd_disk_in_container($REPL_UUID,$TEMP_DISK_NAME)
	{
		$ServiceMgmt = new Service_Class();
		
		$CONN_INFO = $this -> get_blob_connection_string($REPL_UUID);
		$CONN_STRING = $CONN_INFO -> ConnectionString;
		
		if( strpos( $TEMP_DISK_NAME, ".vhd" ) !== false )
			$ServiceMgmt -> delete_vhd_disk($CONN_STRING,$REPL_UUID,$TEMP_DISK_NAME);
		else
			$ServiceMgmt -> delete_vhd_disk($CONN_STRING,$REPL_UUID,$TEMP_DISK_NAME.'.vhd');
		
		return true;		
	}
}


###########################
#
# Azure Blob Query Class
#
###########################
class Azure_Blob_Query_Class extends Db_Connection
{
	###########################
	# INSERT BLOB CLOUD_DISK
	###########################
	public function insert_blob_cloud_disk($REPL_UUID,$SOUCE_DISK_ID,$CLOUD_DISK_INFO)
	{
		$INSERT_EXEC =  "INSERT INTO _CLOUD_DISK
				(
					_OPEN_DISK_UUID,
					_OPEN_SERV_UUID,
					_OPEN_DISK_ID,
					_DEVICE_PATH,
					_REPL_UUID,
					_REPL_DISK_UUID,
					_TIMESTAMP,
					_STATUS
				)
				VALUE
				(
					'".$CLOUD_DISK_INFO["DiskId"]."',
					'".$CLOUD_DISK_INFO["TransportServerId"]."',
					'".$CLOUD_DISK_INFO["DiskId"]."',
					'None',
					'".$REPL_UUID."',
					'".$SOUCE_DISK_ID."',				
					'".Misc_Class::current_utc_time()."',
					'Y'
				)";
				
		$this -> DBCON -> prepare($INSERT_EXEC) -> execute();
		
		return true;		
	}
	
	###########################
	# GET_BLOB_CONNECTION_STRING
	###########################
	public function get_blob_connection_string($REPL_UUID)
	{
		$CONN_STRING_SQL = "SELECT TB._CLUSTER_USER	FROM _REPLICA as TA JOIN _CLOUD_MGMT as TB on TA._CLUSTER_UUID = TB._CLUSTER_UUID WHERE TA._REPL_UUID = '".$REPL_UUID."'";
		
		$CONN_DATA = $this -> DBCON -> prepare($CONN_STRING_SQL);
		$CONN_DATA -> execute();
		
		$CLUSTER_USER = json_decode(Misc_Class::encrypt_decrypt('decrypt',$CONN_DATA -> fetch()['_CLUSTER_USER']));
		
		if (isset($CLUSTER_USER -> CONNECTION_STRING))
		{
			$CONN_STRING = $CLUSTER_USER -> CONNECTION_STRING; 
			
			$CONN_ARRAY = explode( ';', $CONN_STRING);
			foreach( $CONN_ARRAY as $CONN_DATA )
			{
				$CONN_RETURN = explode( '=', $CONN_DATA);
				$CONN_INFO[$CONN_RETURN[0]] = $CONN_RETURN[1];
			}
			
			$CONN_INFO['ConnectionString'] = $CONN_STRING;			
			return (object)$CONN_INFO;
		}
		else
		{
			return false;
		}
		
	}
		
	###########################
	# GET_SNAPSHOT_DISK_INFO
	###########################
	public function get_snapshot_disk_info($SNAPSHOT_ID)
	{
		$QUERY_SNAPSHOT_INFO = "SELECT * FROM _REPLICA_SNAP WHERE _SNAP_UUID = '".$SNAPSHOT_ID."' AND _STATUS = 'Y'";
		$SNAPSHOT_DISK_INFO = $this -> DBCON -> prepare($QUERY_SNAPSHOT_INFO);
		$SNAPSHOT_DISK_INFO -> execute();
		
		$SNAPSHOT_INFO = $SNAPSHOT_DISK_INFO -> fetch(PDO::FETCH_OBJ);
		
		$SNAP_DISK = new stdClass();
		$SNAP_DISK -> DISK_UUID = $SNAPSHOT_INFO -> _DISK_UUID;
		$SNAP_DISK -> SNAP_UUID = $SNAPSHOT_INFO -> _SNAP_UUID;
		$SNAP_DISK -> DISK_NAME = explode('_', $SNAPSHOT_INFO -> _SNAP_NAME)[0];
		$SNAP_DISK -> DISK_SIZE = ceil($SNAPSHOT_INFO -> _ORIGINAL_SIZE / 1024 / 1024 / 1024) + 1;
		$SNAP_DISK -> SNAP_TIME = $SNAPSHOT_INFO -> _SNAP_TIME;

		return $SNAP_DISK;
	}
	
	###########################
	# UPDATE DIRECT BLOB DISK INFO
	###########################
	public function update_direct_blob_disk($BLOB_DISK_NAME,$AZURE_DISK_NAME)
	{
		$UPDATE_EXEC = "UPDATE 
							_SERVICE_DISK
						SET
							_OPEN_DISK = '".$AZURE_DISK_NAME."'
						WHERE
							_SCSI_ADDR = '".$BLOB_DISK_NAME."'";
							
		$this -> DBCON -> prepare($UPDATE_EXEC) -> execute();
	}
}