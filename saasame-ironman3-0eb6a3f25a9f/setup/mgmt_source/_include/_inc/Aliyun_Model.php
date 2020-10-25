<?php

class Aliyun_Model extends Db_Connection
{
    # write cloud information into _CLOUD_MGMT in database
    public function create_cloud_connection($ACCT_UUID,$REGN_UUID,$ACCESS_KEY,$SECRET_KEY,$USER_UUID)
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
				
				'Aliyun',								
				'".Misc_Class::encrypt_decrypt('encrypt',$ACCESS_KEY)."',
				'".Misc_Class::encrypt_decrypt('encrypt',$SECRET_KEY)."',
				'None',
				
				'".$USER_UUID."',
				
				'".Misc_Class::current_utc_time()."',
				'Y',
				4)";

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
	public function update_cloud_connection($CLUSTER_UUID,$ACCESS_KEY,$SECRET_KEY)
	{
		$UPDATE_EXEC = "UPDATE _CLOUD_MGMT
						SET
							_CLUSTER_USER 	= '".Misc_Class::encrypt_decrypt('encrypt',$ACCESS_KEY)."',
							_CLUSTER_PASS	= '".Misc_Class::encrypt_decrypt('encrypt',$SECRET_KEY)."',
							_TIMESTAMP		= '".Misc_Class::current_utc_time()."'
						WHERE
							_CLUSTER_UUID 	= '".$CLUSTER_UUID."'";
		
		$QUERY = $this -> DBCON -> prepare($UPDATE_EXEC) -> execute();
		return true;
	}

	public function getServerHostInfo( $HOST_UUID )
	{
		$GET_EXEC 	= "SELECT * FROM _SERVER_HOST WHERE _HOST_UUID = '".strtoupper($HOST_UUID)."' AND _STATUS = 'Y'";
		
		$QUERY = $this -> DBCON -> prepare($GET_EXEC);

		$QUERY -> execute();

		$data = $QUERY->fetchAll();

		return $data;
	}
	
}

?>