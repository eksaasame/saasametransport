<?php
###################################
#
#	ACCOUNT MANAGEMENT
#
###################################
class Account_Class extends Db_Connection
{
	# TO INITIALIZE ACCOUNT
	public function initialize_account($ACCT_UUID,$REGN_UUID,$USERNAME,$PASSWORD)
	{
		$CHECK_NEW = $this -> query_account($ACCT_UUID);
		if ($CHECK_NEW == FALSE)
		{
			$IDENTITY = mt_rand(10000000, 99999999);
			
			$ACCT_DATA = json_encode(array('region' => $REGN_UUID, 'identity' => $IDENTITY));
		
			$INSERT_EXEC = "INSERT 
								INTO _ACCOUNT(
									_ID,
									_ACCT_UUID,
									_ACCT_DATA,
									_TIMESTAMP,
									_STATUS)
								VALUE(
									'',
									'".$ACCT_UUID."',
									'".$ACCT_DATA."',
									'".Misc_Class::current_utc_time()."',
									'Y')";
			$this -> DBCON -> prepare($INSERT_EXEC) -> execute();
			return true;	
		}
		else
		{
			#FOR UPGRADE FROM PREVIOUS VERSION
			if (strlen($CHECK_NEW['ACCT_DATA'] -> identity) != 8)
			{
				$GEN_NEW_KEY = array('Action' => 'ReGenHostAccessCode', 'AcctUUID' => $ACCT_UUID);
				$this -> update_account_data($GEN_NEW_KEY);
			}
		}
	}
	
	# LIST ALL ACTIVE ACCOUNT
	public function list_all_accounts()
	{
		$GET_EXEC 	= "SELECT * FROM _ACCOUNT WHERE _STATUS = 'Y'";
		$QUERY = $this -> DBCON -> prepare($GET_EXEC);
		$QUERY -> execute();
		$COUNT_ROWS = $QUERY -> rowCount();
		if ($COUNT_ROWS != 0)
		{
			foreach($QUERY as $QueryResult)
			{
				$ResultQuery[] = array(
									'ACCT_UUID'     => $QueryResult['_ACCT_UUID'],
									'ACCT_DATA'   	=> $QueryResult['_ACCT_DATA'],
									'TIMESTAMP'		=> $QueryResult['_TIMESTAMP']
									);
			}
			return $ResultQuery;
		}
		else
		{
			return false;
		}
	}
	
	# QUERY ACTIVE ACCOUNT
	public function query_account($ACCT_UUID)
	{
		$GET_EXEC 	= "SELECT * FROM _ACCOUNT WHERE _ACCT_UUID = '".$ACCT_UUID."' AND _STATUS = 'Y'";
		$QUERY = $this -> DBCON -> prepare($GET_EXEC);
		$QUERY -> execute();
		$COUNT_ROWS = $QUERY -> rowCount();
		
		if ($COUNT_ROWS != 0)
		{
			foreach($QUERY as $QueryResult)
			{
				$ACCT_DATA = json_decode($QueryResult['_ACCT_DATA']);
				
				if (isset($ACCT_DATA -> smtp_settings))
				{
					$SMTP_INFO = Misc_Class::encrypt_decrypt('decrypt',$ACCT_DATA -> smtp_settings);
					$ACCT_DATA -> smtp_settings = json_decode($SMTP_INFO);
				}
				
				$ResultQuery = array(
								'ACCT_UUID'		=> $QueryResult['_ACCT_UUID'],
								'ACCT_DATA'		=> $ACCT_DATA,
								'TIMESTAMP'		=> $QueryResult['_TIMESTAMP']
								);
			}
			return $ResultQuery;
		}
		else
		{
			return false;
		}
	}
		
	#UPDATE ACCOUNT DATA
	public function update_account_data($ACCT_SETTINGS)
	{
		#GET ACCT UUID
		$ACCT_UUID = $ACCT_SETTINGS['AcctUUID'];				
		
		#QUERY ACCOUNT INFO
		$QUERY_ACCT = $this -> query_account($ACCT_UUID);
		$ACCT_DATA = $QUERY_ACCT['ACCT_DATA'];
	
		#QUERY DEFINE SMTP SETTINGS
		if (isset($QUERY_ACCT['ACCT_DATA'] -> smtp_settings))
		{
			$ACCT_DATA -> smtp_settings = Misc_Class::encrypt_decrypt('encrypt',json_encode($QUERY_ACCT['ACCT_DATA'] -> smtp_settings));
		}
		
		#SPECIFIC SWITCH CASE OPTIONS
		switch ($ACCT_SETTINGS['Action'])
		{
			case 'SaveSMTPSettings':
				#UNSET UNNECESSARY ITEM
				unset($ACCT_SETTINGS['Action']);
				unset($ACCT_SETTINGS['AcctUUID']);
				unset($ACCT_SETTINGS['EncryptKey']);
						
				#ENCRYPT SMTP INFO
				$SMTP_INFO = Misc_Class::encrypt_decrypt('encrypt',json_encode($ACCT_SETTINGS));
			
				#UPDATE SMTP INFO INFO ACCT DATA ARRAY
				$ACCT_DATA -> smtp_settings = $SMTP_INFO;
				$RESPONSE = true;
			break;
			
			case 'DeleteSMTPSettings':
				unset($ACCT_DATA -> smtp_settings);
				unset($ACCT_DATA -> notification_type);
				unset($ACCT_DATA -> notification_time);
				$RESPONSE = true;
			break;
			
			case 'UpdateNotificationType':
				$ACCT_DATA -> notification_type = $ACCT_SETTINGS['NotificationType'];
				$ACCT_DATA -> notification_time = $ACCT_SETTINGS['NotificationTime'];
				$RESPONSE = true;
			break;
			
			case 'AcctTimeZoneAndLanguage':
				$ACCT_DATA -> account_language = $ACCT_SETTINGS['AcctLang'];
				$ACCT_DATA -> account_timezone = $ACCT_SETTINGS['TimeZone'];
				$RESPONSE = true;
			break;
			
			case 'ReGenHostAccessCode':
				$IDENTITY_CODE = mt_rand(10000000, 99999999);
				$ACCT_DATA -> identity = $IDENTITY_CODE;
				$RESPONSE = $IDENTITY_CODE;
			break;
		}
		$UPDATE_EXEC = "UPDATE _ACCOUNT
							SET
								_ACCT_DATA 	= '".json_encode($ACCT_DATA)."',					
								_TIMESTAMP	= '".Misc_Class::current_utc_time()."'
							WHERE
								_ACCT_UUID 	= '".$ACCT_UUID."'";
		
		$this -> DBCON -> prepare($UPDATE_EXEC) -> execute();
		return $RESPONSE;
	}
	
	# DELETE ACCOUNT
	public function delete_account($ACCT_UUID)
	{
		$CHECKACCT = $this->query_account($ACCT_UUID);
		
		if ($CHECKACCT != FALSE)
		{
			$DELETE_EXEC = "UPDATE _ACCOUNT
							SET
								_TIMESTAMP	= '".Misc_Class::current_utc_time()."',
								_STATUS		= 'X'
							WHERE
								_ACCT_UUID 	= '".$ACCT_UUID."'";
		
			$QUERY = $this -> DBCON -> prepare($DELETE_EXEC);
			$QUERY -> execute();
			return true;
		}
		else
		{
			return false;
		}
	}		
}



###################################
#
#	REGION MANAGEMENT
#
###################################
class Region_Class extends Db_Connection
{	
	# TO INITIALIZE NEW REGION
	public function initialize_region($ACCT_UUID,$REGION_NAME,$REGION_DATA)
	{
		$REGION_UUID = Misc_Class::guid_v4();
		$INSERT_EXEC = "INSERT 
							INTO _ACCOUNT_REGN(
								_ID,
								_ACCT_UUID,
								_REGN_UUID,
								_REGN_NAME,
								_REGN_DATA,
								_TIMESTAMP,
								_STATUS)
							VALUE(
								'',
								'".$ACCT_UUID."',
								'".$REGION_UUID."',
								'".$REGION_NAME."',
								'".$REGION_DATA."',
								'".Misc_Class::current_utc_time()."',
								'Y')";
		$QUERY = $this -> DBCON -> prepare($INSERT_EXEC);
		$QUERY -> execute();
		return $REGION_UUID;		
	}
	
	# LIST ALL ACTIVE REGION
	public function list_all_regions()
	{
		$GET_EXEC 	= "SELECT * FROM _ACCOUNT_REGN WHERE _STATUS = 'Y'";
		$QUERY = $this -> DBCON -> prepare($GET_EXEC);
		$QUERY -> execute();
		$COUNT_ROWS = $QUERY -> rowCount();
		if ($COUNT_ROWS != 0)
		{
			foreach($QUERY as $QueryResult)
			{
				$ResultQuery[] = array(
									'ACCT_UUID'     => $QueryResult['_ACCT_UUID'],
									'REGN_UUID'   	=> $QueryResult['_REGN_UUID'],
									'REGN_NAME'		=> $QueryResult['_REGN_NAME'],
									'REGN_DATA'		=> $QueryResult['_REGN_DATA'],
									'TIMESTAMP'		=> $QueryResult['_TIMESTAMP']								
									);
			}
			return $ResultQuery;
		}
		else
		{
			return false;
		}
	}
	
	#QUERY REGION BY ACCOUNT UUID
	public function query_by_acct_id($ACCT_UUID)
	{
		$GET_EXEC 	= "SELECT * FROM _ACCOUNT_REGN WHERE _ACCT_UUID = '".$ACCT_UUID."' AND _STATUS = 'Y'";
		$QUERY = $this -> DBCON -> prepare($GET_EXEC);
		$QUERY -> execute();
		$COUNT_ROWS = $QUERY -> rowCount();

		if ($COUNT_ROWS != 0)
		{
			foreach($QUERY as $QueryResult)
			{
				$ResultQuery[] = array(
									'ACCT_UUID'     => $QueryResult['_ACCT_UUID'],
									'REGN_UUID'   	=> $QueryResult['_REGN_UUID'],
									'REGN_NAME'		=> $QueryResult['_REGN_NAME'],
									'REGN_DATA'		=> $QueryResult['_REGN_DATA'],
									'TIMESTAMP'		=> $QueryResult['_TIMESTAMP']
									);
			}
			return $ResultQuery;
		}
		else
		{
			return false;
		}	
	}
	
	#QUERY REGION BY REGION UUID
	public function query_by_rgnl_id($RGN_UUID)
	{
		$GET_EXEC 	= "SELECT * FROM _ACCOUNT_REGN WHERE _REGN_UUID = '".$RGN_UUID."' AND _STATUS = 'Y'";
		$QUERY = $this -> DBCON -> prepare($GET_EXEC);
		$QUERY -> execute();
		$COUNT_ROWS = $QUERY -> rowCount();

		if ($COUNT_ROWS != 0)
		{
			foreach($QUERY as $QueryResult)
			{
				$ResultQuery[] = array(
									'ACCT_UUID'     => $QueryResult['_ACCT_UUID'],
									'REGN_UUID'   	=> $QueryResult['_REGN_UUID'],
									'REGN_NAME'		=> $QueryResult['_REGN_NAME'],
									'REGN_DATA'		=> $QueryResult['_REGN_DATA'],
									'TIMESTAMP'		=> $QueryResult['_TIMESTAMP']
									);
			}
			return $ResultQuery;
		}
		else
		{
			return false;
		}	
	}
	
	#UPDATE REGION INFORMATION
	public function update_region($RGN_UUID,$RGN_NAME,$RGN_DATA)
	{
		$CHECKROW = $this -> query_by_rgnl_id($RGN_UUID);
		
		if ($CHECKROW != FALSE)
		{
			$UPDATE_EXEC = "UPDATE _ACCOUNT_REGN
							SET
								_REGN_NAME		= '".$RGN_NAME."',
								_REGN_DATA		= '".$RGN_DATA."',
								_TIMESTAMP		= '".Misc_Class::current_utc_time()."'
							WHERE
								_REGN_UUID 		= '".$RGN_UUID."'";
			
			$QUERY = $this -> DBCON -> prepare($UPDATE_EXEC);
			$QUERY -> execute();
			return true;
		}
		else
		{
			return false;
		}
	}
	
	#DELETE REGION
	public function delete_region($RGN_UUID)
	{
		$CHECKROW = $this -> query_by_rgnl_id($RGN_UUID);
		
		if ($CHECKROW != FALSE)
		{
			$DELETE_EXEC = "UPDATE _ACCOUNT_REGN
							SET
								_TIMESTAMP		= '".Misc_Class::current_utc_time()."',
								_STATUS			= 'X'
							WHERE
								_REGN_UUID 		= '".$RGN_UUID."'";
			$QUERY = $this -> DBCON -> prepare($DELETE_EXEC);
			$QUERY -> execute();
			return true;
		}
		else
		{
			return false;
		}
	}
}