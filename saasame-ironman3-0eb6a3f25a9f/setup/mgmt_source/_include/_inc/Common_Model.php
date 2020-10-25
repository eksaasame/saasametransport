<?php

require_once '_class_main.php';

class Common_Model extends Db_Connection
{
    # write cloud information into _CLOUD_MGMT in database
    public function create_cloud_connection($ACCT_UUID,$REGN_UUID,$ACCESS_KEY,$SECRET_KEY,$USER_UUID,$Endpoint, $PROJECT_NAME = "Azure", $CLOUD_TYPE = 3)
	{
		$CLOUD_UUID  = Misc_Class::guid_v4();
	
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
								_STATUS,
								_CLOUD_TYPE)
							VALUE(
								'',
								
								'".$ACCT_UUID."',
								'".$REGN_UUID."',
								'".$CLOUD_UUID."',
								
								'".$PROJECT_NAME."',								
								'".Misc_Class::encrypt_decrypt('encrypt',$ACCESS_KEY)."',
								'".Misc_Class::encrypt_decrypt('encrypt',$SECRET_KEY)."',
								'".$Endpoint."',
								
								'".$USER_UUID."',
								
								'".Misc_Class::current_utc_time()."',
								'Y',
								".$CLOUD_TYPE.")";

		$this -> DBCON -> prepare($INSERT_EXEC) -> execute();
		
		return $CLOUD_UUID;
	}

	#query cloud information from _CLOUD_MGMT table
	public function query_cloud_connection_information($CLUSTER_UUID)
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

	#update cloud from _CLOUD_MGMT table
	public function update_cloud_connection($CLUSTER_UUID,$ACCESS_KEY,$SECRET_KEY,$USER_UUID)
	{
		$UPDATE_EXEC = "UPDATE _CLOUD_MGMT
						SET
							_CLUSTER_USER 	= '".Misc_Class::encrypt_decrypt('encrypt',$ACCESS_KEY)."',
							_CLUSTER_PASS	= '".Misc_Class::encrypt_decrypt('encrypt',$SECRET_KEY)."',
							_AUTH_TOKEN		= '".$USER_UUID."',
							_TIMESTAMP		= '".Misc_Class::current_utc_time()."'
						WHERE
							_CLUSTER_UUID 	= '".$CLUSTER_UUID."'";
		
		$QUERY = $this -> DBCON -> prepare($UPDATE_EXEC) -> execute();
		return true;
	}

	public function update_cloud_token_info($CLUSTER_UUID,$TOKEN_INFO)
	{
		$UPDATE_EXEC = "UPDATE _CLOUD_MGMT
						SET
							_AUTH_TOKEN 	= '".$TOKEN_INFO."'
						WHERE
							_CLUSTER_UUID 	= '".$CLUSTER_UUID."'";
		
		$QUERY = $this -> DBCON -> prepare($UPDATE_EXEC) -> execute();
		return true;
	}

	public function getServerInfo( $ServUUID )
	{
		$GET_EXEC 	= "SELECT * FROM _SERVER WHERE _SERV_UUID = '".strtoupper($ServUUID)."' AND _STATUS = 'Y'";
		
		$QUERY = $this -> DBCON -> prepare($GET_EXEC);
		$QUERY -> execute();
		$COUNT_ROWS = $QUERY -> rowCount();

		if ($COUNT_ROWS != 0)
		{
			foreach($QUERY as $QueryResult)
			{
				$HOST_DATA = array(
									"HOST_UUID" 	 => $QueryResult['_HOST_UUID'],
									"SERV_ADDR" 	 => $QueryResult['_SERV_ADDR'],
									"serverInfo" 	 => $QueryResult['_SERV_INFO']
								);	
			}
			
			return $HOST_DATA;
		}
		else
		{
			return false;
		}	
	}

	public function getConnectionString( $replicaId )
	{
		$sql 	= 
			"SELECT TB._CLUSTER_USER
				FROM _REPLICA as TA
				JOIN  _CLOUD_MGMT as TB on TA._CLUSTER_UUID = TB._CLUSTER_UUID
				WHERE TA._REPL_UUID = :replicaId";
		
		$sth  = $this->DBCON->prepare( $sql );

		$sth->bindParam(':replicaId', $replicaId, PDO::PARAM_STR);

		$sth-> execute();

		$result = $sth->fetchAll(\PDO::FETCH_ASSOC);

		$data = Misc_Class::encrypt_decrypt('decrypt',$result[0]['_CLUSTER_USER']);

		$json = json_decode( $data, true );

		return $json["CONNECTION_STRING"];
		
	}

	public function insertCloudDisk( $replicaId, $FromDiskId, $diskInfo )
	{
		$sql 	= 
			"INSERT INTO 
				_CLOUD_DISK( 
				_OPEN_DISK_UUID,
				_OPEN_SERV_UUID,
				_OPEN_DISK_ID,
				_DEVICE_PATH,
				_REPL_UUID,
				_REPL_DISK_UUID,
				_TIMESTAMP,
				_STATUS)
			VALUE(
				:DiskId,
				:TransportIdPiResion,
				:DiskId1,
				'None',
				:ReplicaId,
				:FromDiskId,
			
				'".Misc_Class::current_utc_time()."',
				'Y')";
		
		$sth  = $this->DBCON->prepare( $sql );

		$sth->bindParam(':DiskId', 				$diskInfo["DiskId"], PDO::PARAM_STR);
		$sth->bindParam(':TransportIdPiResion', $diskInfo["TransportIdPiResion"], PDO::PARAM_STR);
		$sth->bindParam(':DiskId1', 			$diskInfo["DiskId"], PDO::PARAM_STR);
		$sth->bindParam(':ReplicaId', 			$replicaId , PDO::PARAM_STR);
		$sth->bindParam(':FromDiskId', 			$FromDiskId, PDO::PARAM_STR);

		$sth-> execute();

		return true;
		
	}

	public function getCloudDisks( $replicaId )
	{
		$sql 	= 
			"SELECT _OPEN_DISK_ID as diskId,
					_REPL_DISK_UUID as replicationDiskId
				FROM _CLOUD_DISK 
				WHERE _REPL_UUID = :replicaId";
		
		$sth  = $this->DBCON->prepare( $sql );

		$sth->bindParam(':replicaId', $replicaId, PDO::PARAM_STR);

		$sth-> execute();

		$result = $sth->fetchAll(\PDO::FETCH_ASSOC);

		return $result;
	}

	public function updateServerInfo( $CloudId, $MachineId, $json = false, $location = "REMOTE_HOST" ){

		$sql 	= 
			"UPDATE _SERVER
				SET 
					_OPEN_UUID = :cloudId,
					_LOCATION = :location";

		if( $json != false ){
			$sql .= ",_SERV_INFO = :json";
		}

		$sql .=		
			" WHERE 
					JSON_EXTRACT(_SERV_INFO, '$.machine_id') = :machineId
					AND _STATUS = 'Y'";

		$sth  = $this->DBCON->prepare( $sql );

		$sth->bindParam(':cloudId', $CloudId, PDO::PARAM_STR);
		$sth->bindParam(':machineId', $MachineId, PDO::PARAM_STR);
		$sth->bindParam(':location', $location, PDO::PARAM_STR);

		if( $json != false ){
			$sth->bindParam(':json', $json, PDO::PARAM_STR);
		}

		$sth-> execute();

		return true;
	}

	public function updateServerConn( $CloudId, $TransportId  ){

		$sql 	= 
			"UPDATE _SERVER_CONN
				SET 
					_CLUSTER_UUID = :cloudId
				WHERE 
					_CARR_UUID = :transportId
					AND _STATUS = 'Y'";
		
		$sth  = $this->DBCON->prepare( $sql );

		$sth->bindParam(':cloudId', $CloudId, PDO::PARAM_STR);
		$sth->bindParam(':transportId', $TransportId, PDO::PARAM_STR);

		$sth-> execute();

		return true;
	}

	public function getMachineId( $TransportId )
	{
		$sql 	=
			"SELECT JSON_UNQUOTE( JSON_EXTRACT(TA._SERV_INFO, '$.machine_id') ) as machineId , TA._SERV_INFO as serverInfo
				FROM _SERVER as TA 
				WHERE TA._SERV_UUID = :transportId";
		
		$sth  = $this->DBCON->prepare( $sql );

		$sth->bindParam(':transportId', $TransportId, PDO::PARAM_STR);

		$sth-> execute();

		$result = $sth->fetchAll(\PDO::FETCH_ASSOC);

		return $result[0];
	}

	public function getReplicatInfo( $replicaId ){

		$sql 	= 
			"SELECT 
				TA._CLUSTER_UUID as CloudId,
				TA._JOBS_JSON as rep_job_json,
				TC._SERV_ADDR as TransportIp,
				TE._CLOUD_TYPE as CloudType,
				TA._PACK_UUID as PackId,
				JSON_UNQUOTE( JSON_EXTRACT(TA._JOBS_JSON, '$.boot_disk')) as boot_disk,
				JSON_UNQUOTE( JSON_EXTRACT(TA._JOBS_JSON, '$.CONNECTION_TYPE')) as connectionType,
				JSON_UNQUOTE( JSON_EXTRACT(TA._CONN_UUID, '$.TARGET')) as TargetConnectionId,
				JSON_UNQUOTE( JSON_EXTRACT(TF._HOST_INFO, '$.os_name')) as os_name,
				JSON_UNQUOTE( JSON_EXTRACT(TF._HOST_INFO, '$.guest_os_name')) as guest_os_name,
				JSON_UNQUOTE( JSON_EXTRACT(TF._HOST_INFO, '$.disks')) as disks,
				JSON_UNQUOTE( JSON_EXTRACT(TF._HOST_INFO, '$.disk_infos')) as disk_infos,
				JSON_UNQUOTE( JSON_EXTRACT(TF._HOST_INFO, '$.network_adapters')) as network_adapters,
				JSON_UNQUOTE( JSON_EXTRACT(TF._HOST_INFO, '$.network_infos')) as network_infos,
				JSON_UNQUOTE( JSON_EXTRACT(TF._HOST_INFO, '$.firmware')) as firmware,
				JSON_UNQUOTE( JSON_EXTRACT(TF._HOST_INFO, '$.guest_id')) as guest_id,
				JSON_UNQUOTE( JSON_EXTRACT(TF._HOST_INFO, '$.memory_mb')) as vm_memory,
				JSON_UNQUOTE( JSON_EXTRACT(TF._HOST_INFO, '$.number_of_cpu')) as vm_cpu,
				JSON_UNQUOTE( JSON_EXTRACT(TF._HOST_INFO, '$.physical_memory')) as physical_memory,
				JSON_UNQUOTE( JSON_EXTRACT(TF._HOST_INFO, '$.processors')) as physical_cpu,
				JSON_UNQUOTE( JSON_EXTRACT(TF._HOST_INFO, '$.logical_processors')) as logical_processors,
				JSON_UNQUOTE( JSON_EXTRACT(TF._HOST_INFO, '$.architecture')) as architecture,
				TC._SERV_UUID as ServerId,
				GROUP_CONCAT( TG._DISK_UUID SEPARATOR ',') as replicaDisksId
			FROM _REPLICA as TA
			JOIN _SERVER_CONN as TB on JSON_EXTRACT(TA._CONN_UUID, '$.TARGET') = TB._CONN_UUID
			JOIN _SERVER as TC on TB._LAUN_UUID = TC._SERV_UUID
			LEFT JOIN _CLOUD_MGMT as TD on TA._CLUSTER_UUID = TD._CLUSTER_UUID
            LEFT JOIN _SYS_CLOUD_TYPE as TE on TD._CLOUD_TYPE = TE._ID
			JOIN _SERVER_HOST as TF on TA._PACK_UUID = TF._HOST_UUID
			JOIN _REPLICA_DISK as TG on TA._REPL_UUID = TG._REPL_UUID
			WHERE 
				TA._REPL_UUID = :replicaId
				AND TA._STATUS = 'Y'
				AND TF._STATUS = 'Y'
			GROUP BY TA._REPL_UUID";

		$sth  = $this->DBCON->prepare( $sql );

		$sth->bindParam(':replicaId', $replicaId, PDO::PARAM_STR);

		$sth-> execute();

		$result = $sth->fetchAll(\PDO::FETCH_ASSOC);

		return $result;
	}

	public function getTransportInfo( $servId ){

		$sql 	= 
			"SELECT 
				_OPEN_UUID as CloudId,
				_SERV_ADDR as TransportIp,
				JSON_EXTRACT(_SERV_INFO, '$.direct_mode') as directMode,
				JSON_UNQUOTE(JSON_EXTRACT(_SERV_INFO, '$.machine_id')) as machineId,
				_HOST_UUID as serverInfo
			FROM _SERVER 
			WHERE 
				_SERV_UUID = :servId
				AND _STATUS = 'Y'";

		$sth  = $this->DBCON->prepare( $sql );

		$sth->bindParam(':servId', $servId, PDO::PARAM_STR);

		$sth-> execute();

		$result = $sth->fetchAll(\PDO::FETCH_ASSOC);

		if( $result[0]["directMode"] == "false" )
			$result[0]["ConnectAddr"] = array( $result[0]["machineId"] );
		else
			$result[0]["ConnectAddr"] = json_decode( $result[0]["TransportIp"],true );

		return $result[0];
	}

	public function getRecoveryInfo( $serviceId ){

		$sql 	= 
			"SELECT 
				TA._CLUSTER_UUID as CloudId,
				TA._JOBS_JSON as rep_job_json,
				TC._SERV_ADDR as TransportIp,
				TE._CLOUD_TYPE as CloudType,
				ST._JOBS_JSON as service_job_json,
				ST._REPL_UUID as replicaId,
				ST._SNAP_JSON as snapshots,
				TC._SERV_UUID as serverId,
				ST._NETWORK_UUID as Network,
				ST._NOVA_VM_UUID as instanceId
			FROM _SERVICE as ST
			JOIN _REPLICA as TA on ST._REPL_UUID = TA._REPL_UUID
			JOIN _SERVER_CONN as TB on JSON_EXTRACT(TA._CONN_UUID, '$.TARGET') = TB._CONN_UUID
			JOIN _SERVER as TC on TB._LAUN_UUID = TC._SERV_UUID
			JOIN _CLOUD_MGMT as TD on TA._CLUSTER_UUID = TD._CLUSTER_UUID
            JOIN _SYS_CLOUD_TYPE as TE on TD._CLOUD_TYPE = TE._ID
			WHERE 
				ST._SERV_UUID = :serviceId
				AND ST._STATUS = 'Y'";

		$sth  = $this->DBCON->prepare( $sql );

		$sth->bindParam(':serviceId', $serviceId, PDO::PARAM_STR);

		$sth-> execute();

		$result = $sth->fetchAll(\PDO::FETCH_ASSOC);

		return $result;

	}

	public function insertServiceDisk( $diskId, $serviceId, $packId, $diskSize, $scsiAddr, $volumnId, $snapshotId){

		$sql 	= 
			"INSERT INTO 
				_SERVICE_DISK( 
					_DISK_UUID,
					_SERV_UUID,
					_HOST_UUID,
					_DISK_SIZE,
					_SCSI_ADDR,
					_OPEN_DISK,
					_SNAP_UUID,
					_TIMESTAMP,
					_STATUS)
			VALUE(
				:diskId,
				:serviceId,
				:packId,
				:diskSize,
				:scsiAddr,
				:volumnId,
				:snapshotId,
				'".Misc_Class::current_utc_time()."',
				'Y')";
		
		$sth  = $this->DBCON->prepare( $sql );

		$sth->bindParam(':diskId', 			$diskId, PDO::PARAM_STR);
		$sth->bindParam(':serviceId', 		$serviceId, PDO::PARAM_STR);
		$sth->bindParam(':packId', 			$packId, PDO::PARAM_STR);
		$sth->bindParam(':diskSize', 		$diskSize , PDO::PARAM_STR);
		$sth->bindParam(':scsiAddr', 		$scsiAddr, PDO::PARAM_STR);
		$sth->bindParam(':volumnId', 		$volumnId, PDO::PARAM_STR);
		$sth->bindParam(':snapshotId', 		$snapshotId, PDO::PARAM_STR);

		$sth-> execute();

		return true;
	}

	function updateMachineIdInServiceTable( $machineId, $serviceId){
		
		$sql 	= 
			"UPDATE _SERVICE
				SET 
					_NOVA_VM_UUID = :machineId
				WHERE 
					_SERV_UUID = :serviceId
					AND _STATUS = 'Y'";
		
		$sth  = $this->DBCON->prepare( $sql );

		$sth->bindParam(':machineId', $machineId, PDO::PARAM_STR);
		$sth->bindParam(':serviceId', $serviceId, PDO::PARAM_STR);

		$sth-> execute();

		return true;
	}

	function getRunningSnapshot( $replicaId ){

		$sql 	= 
			"SELECT 
				ST._SNAP_JSON as snapshots
			FROM _SERVICE as ST
			WHERE 
				ST._REPL_UUID = :replicaId
				AND ST._STATUS = 'Y'
				AND JSON_EXTRACT(ST._JOBS_JSON, '$.job_status') != 'LauncherConvertFailed' 
				AND JSON_EXTRACT(ST._JOBS_JSON, '$.job_status') != 'InstanceCreated'";

		$sth  = $this->DBCON->prepare( $sql );

		$sth->bindParam(':replicaId', $replicaId, PDO::PARAM_STR);

		$sth-> execute();

		$result = $sth->fetchAll(\PDO::FETCH_ASSOC);

		return $result;
	}

	function getServerByAddr( $addr, $type ){

		$addr = '%'.$addr.'%';

		$sql 	= 
			"SELECT 
				*
			FROM _SERVER
			WHERE 
				_SERV_ADDR like :addr
				AND _STATUS = 'Y'
				AND _SERV_TYPE = :server_type";

		$sth  = $this->DBCON->prepare( $sql );

		$sth->bindParam(':addr', $addr, PDO::PARAM_STR);
		$sth->bindParam(':server_type', $type, PDO::PARAM_STR);

		$sth-> execute();

		$result = $sth->fetchAll(\PDO::FETCH_ASSOC);

		if( count( $result ) > 1 )
			return false;

		return $result[0];
	}

	function getDRCount( $replicaId, $exit = false ){

		$sql 	= 
			"SELECT 
				COUNT(*) as serviceCount
			FROM _SERVICE
			WHERE 
				JSON_EXTRACT(_JOBS_JSON, '$.recovery_type') = 'DisasterRecovery'
				AND _REPL_UUID = :replicaId";

		if( $exit )
			$sql .= " AND _STATUS = 'Y'";

		$sth  = $this->DBCON->prepare( $sql );

		$sth->bindParam(':replicaId', $replicaId, PDO::PARAM_STR);

		$sth-> execute();

		$result = $sth->fetchAll(\PDO::FETCH_ASSOC);

		return (int)$result[0]["serviceCount"];
	}

	function getReplicaServiceNum( $serverId ){

		$sql 	= 
			"SELECT 
				COUNT(*) as replicaCount
			FROM _REPLICA as TA
			join _SERVER_CONN as TB on JSON_EXTRACT(TA._CONN_UUID, '$.TARGET') = TB._CONN_UUID
			WHERE 
				TB._LAUN_UUID = :serverId
				AND TA._STATUS = 'Y'";

		$sth  = $this->DBCON->prepare( $sql );

		$sth->bindParam(':serverId', $serverId, PDO::PARAM_STR);

		$sth-> execute();

		$result = $sth->fetchAll(\PDO::FETCH_ASSOC);

		return (int)$result[0]["replicaCount"];
	}

	function getAzurePromoteTransport( $cloudId ){

		$sql = "SELECT _SERV_INFO as serverInfo, _SERV_UUID as serverId
				FROM 
					_SERVER
				WHERE 
					_OPEN_UUID = :cloudId
				AND JSON_EXTRACT(_SERV_INFO, '$.is_promote') = true
				AND _STATUS = 'Y'";
		
		$sth  = $this->DBCON->prepare( $sql );

		$sth->bindParam(':cloudId', $cloudId, PDO::PARAM_STR);

		$sth-> execute();

		$result = $sth->fetchAll(\PDO::FETCH_ASSOC);

		return $result;
	}

	function getVMwarePromoteTransport( $cloudId ){

		$sql = "SELECT _SERV_INFO as serverInfo, _SERV_UUID as serverId
				FROM 
					_SERVER
				WHERE 
					_OPEN_UUID = :cloudId
				AND _STATUS = 'Y'";
		
		$sth  = $this->DBCON->prepare( $sql );

		$sth->bindParam(':cloudId', $cloudId, PDO::PARAM_STR);

		$sth-> execute();

		$result = $sth->fetchAll(\PDO::FETCH_ASSOC);

		return $result;
	}
}

?>