<?php

class Azure_Model extends Db_Connection
{
    # write cloud information into _CLOUD_MGMT in database
    public function create_cloud_connection($ACCT_UUID,$REGN_UUID,$ACCESS_KEY,$SECRET_KEY,$USER_UUID,$Endpoint)
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
								
								'Azure',								
								'".Misc_Class::encrypt_decrypt('encrypt',$ACCESS_KEY)."',
								'".Misc_Class::encrypt_decrypt('encrypt',$SECRET_KEY)."',
								'".$Endpoint."',
								
								'".$USER_UUID."',
								
								'".Misc_Class::current_utc_time()."',
								'Y',
								3)";

		$this -> DBCON -> prepare($INSERT_EXEC) -> execute();
		
		return $INSERT_EXEC;
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
									"SERV_ADDR" 	 => $QueryResult['_SERV_ADDR']
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
}

?>