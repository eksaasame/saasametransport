<?php
class Synchronization_Class extends Db_Connection
{
	#INITIALIZE SYNC SERVER
	public function initialize_sync_server($SERV_TOKEN)
	{
		$SYNC_UUID  = Misc_Class::guid_v4();
		
		$INSERT_EXEC = "INSERT 
							INTO _SYS_SYNC_SERVER(
								_ID,
								_SYNC_UUID,
								_SYNC_TOKEN,
								_TIMESTAMP,
								_STATUS)
							VALUE(
								'',
								'".$SYNC_UUID."',
								'".$SERV_TOKEN."',
								'".Misc_Class::current_utc_time()."',
								'Y')";
								
		$QUERY = $this -> DBCON -> prepare($INSERT_EXEC);
		$QUERY -> execute();
		
		return array('SYNC_SERVER_UUID' => $SYNC_UUID);		
	}

	#QUERY SYNC SERVER INFORMATION
	public function query_sync_server($SYNC_UUID)
	{
		$GET_EXEC = "SELECT * FROM _SYS_SERVICES WHERE _STATUS = 'Y'";
		$QUERY = $this -> DBCON -> prepare($GET_EXEC);
		$QUERY -> execute();
		$COUNT_ROWS = $QUERY -> rowCount();
		if ($COUNT_ROWS != 0)
		{
			foreach($QUERY as $QueryResult)
			{
				$SERV_DATA[$QueryResult['_SERV_UUID']] = $QueryResult['_SERV_NAME'];
			}
			return $SERV_DATA;
		}
		else
		{
			return false;
		}		
	}
	
	public function edit_sync_server()
	{
		
	}
		
	public function delete_sync_server()
	{
		
	}
}