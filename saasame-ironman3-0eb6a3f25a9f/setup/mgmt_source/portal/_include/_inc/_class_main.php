<?php
###################################
#
#	PDO DB CONNECTION CLASS
#
###################################
class Db_Connection extends PDO
{
	public $DBCON;	
	
    public function __construct()
	{
		// Create (connect to) SQLite database in file
		try {
			$this -> DBCON = new PDO('sqlite:_include/_inc/pixiu.sqlite');
		}
		catch (Exception $e){
			$this -> DBCON = new PDO('sqlite:../_inc/pixiu.sqlite');
		}
		
		
		// Set errormode to exceptions
		$this -> DBCON -> setAttribute(PDO::ATTR_ERRMODE, 
										PDO::ERRMODE_EXCEPTION);
 
 		// Create new database in memory
		$memory_db = new PDO('sqlite::memory:');
    
		// Set errormode to exceptions
		$memory_db -> setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);
	}
}



###################################
#
#	PAGE PASS VIA CLASS
#
###################################
class Pages_Passvia extends Db_Connection
{
	#GET PAGE REF POINT FROM DATABASE
	private function get_page_ref_point($URI_TOKEN)
	{
		$GET_EXEC = "SELECT * FROM _PAGE WHERE _URI_NAME = '".$URI_TOKEN."' AND _STATUS = 'Y' LIMIT 1";
		$QUERY = $this -> DBCON -> prepare($GET_EXEC);
		$QUERY -> execute();
		
		$QueryResult = $QUERY->fetchAll(PDO::FETCH_ASSOC);		
		$COUNT_ROWS = count($QueryResult);

		
		if ($COUNT_ROWS == 1)
		{
			$REF_FILE_NAME 	= $QueryResult[0]['_FILE_NAME'];									
		}
		else
		{
			$REF_FILE_NAME = false;
		}
		
		return $REF_FILE_NAME;
	}
	
	#QUERY AND LINK URI VALUES
	public function query_restful_value($REQUEST_URI)
	{
		if (isset($_SESSION['admin']) and $_SESSION['admin'] != FALSE)
		{
			$URI_TOKEN = array_filter(explode("/", $REQUEST_URI));	
		
			$URI_INFO = $this -> get_page_ref_point(end($URI_TOKEN));
		
			define('ROOT_PATH', __DIR__);
			require_once(ROOT_PATH . "/languages/setlang.php");

			include '_include/_pages/_headernavi.php';
			include '_include/_pages/_leftnavi.php';
			if ($URI_INFO != FALSE)
			{
				include '_include/_pages/_inc/'.$URI_INFO;
			}
			else
			{
				include '_include/_pages/_inc/_MgmtPrepareWorkload.php';
			}
		}
		else
		{
			include '_include/_pages/_inc/_login.php';
		}
	}
}


###################################
#
#	Administrator Operation Class
#
###################################
class AdminOperation extends Db_Connection
{
	#READ VERSION
	private function read_version()
	{
		$Read_Version = fgets(fopen(__DIR__ .'\_version.txt', 'r'));
		return $Read_Version;
	}
	
	#GUID V4
	private function guid_v4()
	{
		$Random_Data = random_bytes(16);
		assert(strlen($Random_Data) == 16);

		$Random_Data[6] = chr(ord($Random_Data[6]) & 0x0f | 0x40); // set version to 0100
		$Random_Data[8] = chr(ord($Random_Data[8]) & 0x3f | 0x80); // set bits 6-7 to 10

		return vsprintf('%s%s-%s-%s-%s-%s%s%s', str_split(bin2hex($Random_Data), 4));
	}
	
	#UPDATE FIRST TIME ACCT UUID
	private function update_acct_uuid($ACCT_UUID)
	{
		$UPDATE_UUID = "UPDATE _ACCT SET _UUID = '".$ACCT_UUID."' WHERE _ID = '1'";
		$QUERY = $this -> DBCON -> prepare($UPDATE_UUID);
		$QUERY -> execute();
	}
	
	#UPDATE FIRST TIME REGN UUID
	private function update_regn_uuid($REGN_UUID)
	{
		$UPDATE_REGN_UUID = "UPDATE _ACCT SET _REGION = '".$REGN_UUID."' WHERE _ID = '1'";
		$QUERY = $this -> DBCON -> prepare($UPDATE_REGN_UUID);
		$QUERY -> execute();
		
		$UPDATE_REGN_INFO = "UPDATE _ACCT_REGN SET _UUID = '".$REGN_UUID."' WHERE _ID = '1'";
		$QUERY = $this -> DBCON -> prepare($UPDATE_REGN_INFO);
		$QUERY -> execute();
	}
	
	#DEFAULT ACCT MISC SETTING
	private function default_acct_misc($ACCT_UUID,$MISC_INFO)
	{
		if (!isset($MISC_INFO -> Timezone)){$MISC_INFO -> Timezone = 'UTC';}
		if (!isset($MISC_INFO -> WebPort)){$MISC_INFO -> WebPort = 443;}
		if (!isset($MISC_INFO -> Language)){$MISC_INFO -> Language = 'ENG';}
		if (!isset($MISC_INFO -> OptionLevel)){$MISC_INFO -> OptionLevel = 'USER';}
		
		return $MISC_INFO;
	}
	
	#LOGIN CHECK
	public function check_login($USERNAME,$PASSWORD)
	{
		$GET_EXEC = "SELECT * FROM _ACCT WHERE _USERNAME = '".$USERNAME."' AND _PASSWORD = '".md5($PASSWORD)."' AND _STATUS = 'Y'";
		$QUERY = $this -> DBCON -> prepare($GET_EXEC);
		$QUERY -> execute();
			
		$QueryResult = $QUERY->fetchAll(PDO::FETCH_ASSOC);		
		$COUNT_ROWS = count($QueryResult);
		
		if ($COUNT_ROWS == 1)
		{
			#REGEN CHECK ACCT UUID
			if ($QueryResult[0]['_UUID'] == '')
			{
				$ACCT_UUID = $this -> guid_v4();
				$REGN_UUID = $this -> guid_v4();
				
				$this -> update_acct_uuid($ACCT_UUID);
				$this -> update_regn_uuid($REGN_UUID);
			}
			else
			{
				$ACCT_UUID = $QueryResult[0]['_UUID'];
				$REGN_UUID = $QueryResult[0]['_REGION'];
			}
			
			$MISC_INFO = (object)json_decode($QueryResult[0]['_MISC']);
			$ACCT_INFO = $this -> default_acct_misc($ACCT_UUID,$MISC_INFO);
			
			$_SESSION['timezone'] 	 = $ACCT_INFO -> Timezone;
			$_SESSION['webport'] 	 = $ACCT_INFO -> WebPort;	
			$_SESSION['language'] 	 = $ACCT_INFO -> Language;			
			$_SESSION['optionlevel'] = $ACCT_INFO -> OptionLevel;
						
			$_SESSION['version'] = $this -> read_version();
			$_SESSION['admin'] = array('ACCT_NAME' => $USERNAME, 'ACCT_UUID' => $ACCT_UUID, 'REGN_UUID' => $REGN_UUID);
			
			#UPDATE PORTAL PAGE URI
			$this -> portal_page_update();
			
			#USER LOGS
			$this -> add_user_logs('UserLogin','Success');
			
			#RENEW WEBPORT
			Misc::renew_htaccess_port($_SESSION['webport']);
			return true;							
		}
		else
		{
			#USER LOGS
			$INPUT = json_encode(array($USERNAME => $PASSWORD));
			$this -> add_user_logs('UserLogin',$INPUT);
			return false;
		}
	}
	
	#UPDATE SETTINGS
	public function update_user_setting($ACCT_UUID,$TIMEZONE,$WEBPORT,$LANGUAGE,$OPTIONLEVEL)
	{
		$SET_INFO = json_encode(array('Timezone' => $TIMEZONE, 'WebPort' => $WEBPORT, 'Language' => $LANGUAGE, 'OptionLevel' => $OPTIONLEVEL));
			
		$UPDATE_EXEC = "UPDATE _ACCT SET _MISC = '".$SET_INFO."' WHERE _UUID = '".$ACCT_UUID."'";
		$QUERY = $this -> DBCON -> prepare($UPDATE_EXEC);
		$QUERY -> execute();
		
		$_SESSION['timezone'] 	 = $TIMEZONE;
		$_SESSION['webport'] 	 = $WEBPORT;
		$_SESSION['language'] 	 = $LANGUAGE;
		$_SESSION['optionlevel'] = $OPTIONLEVEL;
		
		#USER LOGS
		$this -> add_user_logs('ChangeTimeZone',$TIMEZONE);

		#USER LOGS
		$this -> add_user_logs('ChangeWebPort',$WEBPORT);
		
		#USER LOGS
		$this -> add_user_logs('ChangeLanguage',$LANGUAGE);
		
		#USER LOGS
		$this -> add_user_logs('ChangeOptionLevel',$OPTIONLEVEL);
		return true;
	}
	
	#CHANGE ADMIN PASSWORD
	public function change_password($ACCT_UUID,$PASSWORD)
	{
		$UPDATE_EXEC = "UPDATE _ACCT SET _PASSWORD = '".md5($PASSWORD)."' WHERE _UUID = '".$ACCT_UUID."'";
		$this -> DBCON -> prepare($UPDATE_EXEC) -> execute();
		
		return true;
	}
	
	#PORTAL PAGE UPDATER
	private function portal_page_update()
	{
		#SET PAGES
		$PAGE_ARRAY = array(
						'EditConfigureSchedule' => '_ConfigureSchedule.php',
						'AddAliyunConnection' => '_MgmtAliyunConnection.php',
						'EditAliyunConnection' => '_MgmtAliyunConnection.php',
						'AddAzureConnection' => '_MgmtAzureConnection.php',
						'EditAzureConnection' => '_MgmtAzureConnection.php',
						'SelectAzureTransportInstance' => '_SelectAzureTransportInstance.php',
						'VerifyAzureTransportServices' => '_VerifyAzureTransportServices.php',
						'EditAzureTransportServices' => '_VerifyAzureTransportServices.php',
						'SelectAzureHostTransportInstance' => '_SelectAzureHostTransportInstance.php',
						'VerifyAzureHostTransportType' => '_VerifyAzureHostTransportType.php',
						'RecoverExportSummary' => '_RecoverExportSummary.php',
					    'RecoverAzureSummary' => '_RecoverAzureSummary.php',
                        'InstanceAzureConfigurations' => '_InstanceAzureConfigurations.php',
                        'SelectListAzureSnapshot' => '_SelectListAzureSnapshot.php',
						'SelectAwsLinuxLauncher' => '_SelectAwsLinuxLauncher.php',
						'VerifyAwsLinuxLauncherService' => '_VerifyAwsLinuxLauncherService.php',
						'EditAwsLinuxLauncherService' => '_VerifyAwsLinuxLauncherService.php',
						'InstanceAliyunConfigurations' => '_InstanceAliyunConfigurations.php',
						'RecoverAliyunSummary' => '_RecoverAliyunSummary.php',
						'SelectAliyunTransportInstance' => '_SelectAliyunTransportInstance.php',
						'SelectAliyunHostTransportInstance' => '_SelectAliyunHostTransportInstance.php',
						'SelectListAliyunSnapshot' => '_SelectListAliyunSnapshot.php',
						'VerifyAliyunHostTransportType' => '_VerifyAliyunHostTransportType.php',
						'VerifyAliyunTransportServices' => '_VerifyAliyunTransportServices.php',
						'EditAliyunTransportServices' => '_VerifyAliyunTransportServices.php',
						'AddTencentConnection' => '_MgmtTencentConnection.php',
						'EditTencentConnection'	=> '_MgmtTencentConnection.php',
						'InstanceTencentConfigurations'	=> '_InstanceTencentConfigurations.php',
						'RecoverTencentSummary'	=> '_RecoverTencentSummary.php',
						'SelectTencentTransportInstance' => '_SelectTencentTransportInstance.php',
						'SelectTencentHostTransportInstance' => '_SelectTencentHostTransportInstance.php',
						'SelectListTencentSnapshot' => '_SelectListTencentSnapshot.php',
						'VerifyTencentHostTransportType' => '_VerifyTencentHostTransportType.php',
						'VerifyTencentTransportServices' => '_VerifyTencentTransportServices.php',
						'EditTencentTransportServices' => '_VerifyTencentTransportServices.php',
						'EditAliyunTransportServices' => '_VerifyAliyunTransportServices.php',
						'ConfigureWorkload' => '_ConfigureWorkload.php',
						'MgmtRecoverPlan' => '_MgmtRecoverPlan.php',						
						'PlanSelectRecoverReplica' => '_PlanSelectRecoverReplica.php',
						'PlanSelectRecoverType' => '_PlanSelectRecoverType.php',
						'EditPlanSelectRecoverType' => '_PlanSelectRecoverType.php',
						'PlanInstanceConfigurations' => '_PlanInstanceConfigurations.php',
						'PlanInstanceAwsConfigurations' => '_PlanInstanceAwsConfigurations.php',
						'PlanInstanceAzureConfigurations' => '_PlanInstanceAzureConfigurations.php',
						'PlanInstanceAliyunConfigurations' => '_PlanInstanceAliyunConfigurations.php',
						'PlanInstanceTencentConfigurations' => '_PlanInstanceTencentConfigurations.php',
						'EditPlanInstanceConfigurations' => '_PlanInstanceConfigurations.php',
						'EditPlanInstanceAwsConfigurations' => '_PlanInstanceAwsConfigurations.php',
						'EditPlanInstanceAzureConfigurations' => '_PlanInstanceAzureConfigurations.php',
						'EditPlanInstanceAliyunConfigurations' => '_PlanInstanceAliyunConfigurations.php',
						'EditPlanInstanceTencentConfigurations' => '_PlanInstanceTencentConfigurations.php',
						'PlanRecoverSummary' => '_PlanRecoverSummary.php',
						'PlanRecoverAwsSummary' => '_PlanRecoverAwsSummary.php',
						'PlanRecoverAzureSummary' => '_PlanRecoverAzureSummary.php',
						'PlanRecoverAliyunSummary' => '_PlanRecoverAliyunSummary.php',
						'PlanRecoverTencentSummary' => '_PlanRecoverTencentSummary.php',
						'EditPlanRecoverSummary' => '_PlanRecoverSummary.php',
						'EditPlanRecoverAwsSummary' => '_PlanRecoverAwsSummary.php',
						'EditPlanRecoverAzureSummary' => '_PlanRecoverAzureSummary.php',
						'EditPlanRecoverAliyunSummary' => '_PlanRecoverAliyunSummary.php',
						'EditPlanRecoverTencentSummary' => '_PlanRecoverTencentSummary.php',
						'AddHuaweiCloudConnection' => '_MgmtHuaweiCloudConnection.php',
						'EditHuaweiCloudConnection' => '_MgmtHuaweiCloudConnection.php',
						
						'AddCtyunConnection' => '_MgmtCtyunConnection.php',
						'EditCtyunConnection' => '_MgmtCtyunConnection.php',
						'SelectCtyunTransportInstance' => '_SelectCtyunTransportInstance.php',
						'VerifyCtyunTransportServices' => '_VerifyCtyunTransportServices.php',
						'EditCtyunTransportServices' => '_VerifyCtyunTransportServices.php',
						'SelectCtyunHostTransportInstance' => '_SelectCtyunHostTransportInstance.php',
						'VerifyCtyunHostTransportType' => '_VerifyCtyunHostTransportType.php',
						'EditCtyunHostTransportType' => '_VerifyCtyunHostTransportType.php',
						'SelectListCtyunSnapshot' => '_SelectListCtyunSnapshot.php',
						'InstanceCtyunConfigurations' => '_InstanceCtyunConfigurations.php',
						'RecoverCtyunSummary' => '_RecoverCtyunSummary.php',
						
						'AddVMWareConnection' => '_MgmtVMWareConnection.php',
						'SelectListVMWareSnapshot' => '_SelectListVMWareSnapshot.php',
						'InstanceVMWareConfigurations' => '_InstanceVMWareConfigurations.php',
						'RecoverVMWareSummary' => '_RecoverVMWareSummary.php',
						'EditVMWareConnection' => '_MgmtVMWareConnection.php',
						'PlanInstanceVMWareConfigurations' => '_PlanInstanceVMWareConfigurations.php',
						'PlanRecoverVMWareSummary' => '_PlanRecoverVMWareSummary.php',
						'EditPlanInstanceVMWareConfigurations' => '_PlanInstanceVMWareConfigurations.php',
						'EditPlanRecoverVMWareSummary' => '_PlanRecoverVMWareSummary.php',
						
						'PlanSelectListSnapshot' => '_PlanSelectListSnapshot.php',
						'EditPlanSelectListSnapshot' => '_PlanSelectListSnapshot.php',
						
						'PlanSelectListEbsSnapshot' => '_PlanSelectListEbsSnapshot.php',
						'EditPlanSelectListEbsSnapshot' => '_PlanSelectListEbsSnapshot.php',
						
						'PlanInstanceCtyunConfigurations' => '_PlanInstanceCtyunConfigurations.php',
						'EditPlanInstanceCtyunConfigurations' => '_PlanInstanceCtyunConfigurations.php',
						
						'PlanRecoverCtyunSummary' => '_PlanRecoverCtyunSummary.php',
						'EditPlanRecoverCtyunSummary' => '_PlanRecoverCtyunSummary.php'
						);
		
		#LOOP TO CHECK AND INSERT PAGE
		foreach ($PAGE_ARRAY as $URL_NAME => $FILE_NAME)
		{
			$CHECK_PAGE = "SELECT * FROM _PAGE WHERE _URI_NAME = '".$URL_NAME."' AND _STATUS = 'Y'";
			$QUERY = $this -> DBCON -> prepare($CHECK_PAGE);
			$QUERY -> execute();
		
			$QueryResult = $QUERY->fetchAll(PDO::FETCH_ASSOC);		
			$COUNT_ROWS = count($QueryResult);
			
			if ($COUNT_ROWS == 0)
			{
				$INSERT_PAGE = "INSERT INTO _PAGE ('_URI_NAME','_FILE_NAME','_STATUS') VALUES ('".$URL_NAME."','".$FILE_NAME."','Y')";
				$this -> DBCON -> prepare($INSERT_PAGE) -> execute();
			}
		}
		
		#CREATE LOG TABLES
		$CREATE_TABLE = "CREATE TABLE IF NOT EXISTS '_LOGS'('_ID' INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL UNIQUE,
														    '_ACCT_UUID' VARCHAR NOT NULL,
														    '_ACTION' VARCHAR NOT NULL,
														    '_STATUS' VARCHAR NOT NULL,
															'_IP_ADDR' VARCHAR NOT NULL,
														    '_DATETIME' DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP)";
		$this -> DBCON -> prepare($CREATE_TABLE) -> execute();
				
		#DELETE PAGE
		$DEL_PAGE_ARRAY = array('Reset');
		
		#LOOP TO CHECK AND DELETE PAGE
		for ($i=0; $i<count($DEL_PAGE_ARRAY); $i++)
		{
			$UPDATE_PAGE = "UPDATE _PAGE SET _STATUS = 'Y' WHERE _URI_NAME = '".$DEL_PAGE_ARRAY[$i]."'";
			$this -> DBCON -> prepare($UPDATE_PAGE) -> execute();			
		}
	}
	
	#ADD USER LOGS
	public function add_user_logs($ACTION,$STATUS)
	{
		if (isset($_SESSION['admin']['ACCT_UUID']))
		{
			$ACCT_UUID = $_SESSION['admin']['ACCT_UUID'];
		}
		else
		{
			$ACCT_UUID = '00000000-1111-2222-3333-444444444444';
		}
		
		#REMOTE IP ADDRESS
		$USER_IP = $_SERVER['REMOTE_ADDR'];
		
		$INSERT_LOG = "INSERT INTO _LOGS ('_ACCT_UUID','_ACTION','_STATUS','_IP_ADDR') VALUES ('".$ACCT_UUID."','".$ACTION."','".$STATUS."','".$USER_IP."')";
		$this -> DBCON -> prepare($INSERT_LOG) -> execute();		
	}
	
	#RENEW ACCOUNT DATA AFTER RESTORE
	public function renew_account_data($ACCT_UUID,$RENEW_DATA)
	{
		$DEFAULT_NAME = 'admin';
		$NEW_ACCT_UUID = $RENEW_DATA['AcctUUID'];
		$NEW_REGN_UUID = $RENEW_DATA['RegnUUID'];

		$UPDATE_ACCT = "UPDATE _ACCT SET _UUID = '".$NEW_ACCT_UUID."', _REGION = '".$NEW_REGN_UUID."' WHERE _UUID = '".$ACCT_UUID."'";
		$this -> DBCON -> prepare($UPDATE_ACCT) -> execute();
		
		$UPDATE_REGION = "UPDATE _ACCT_REGN SET _UUID = '".$NEW_REGN_UUID."' WHERE _ID = 1";		
		$this -> DBCON -> prepare($UPDATE_REGION) -> execute();
		
		$_SESSION['admin'] = array('ACCT_NAME' => $DEFAULT_NAME, 'ACCT_UUID' => $NEW_ACCT_UUID, 'REGN_UUID' => $NEW_REGN_UUID);
		
		return true;		
	}
}


###################################
#
#	Extend Class
#
###################################
final class Misc
{
	#GENERATE GUID VERSION 4
	public static function guid_v4()
	{
		return strtoupper(sprintf('%04x%04x-%04x-%04x-%04x-%04x%04x%04x',

		mt_rand(0, 0xffff), mt_rand(0, 0xffff),

		mt_rand(0, 0xffff),

		mt_rand(0, 0x0fff) | 0x4000,

		mt_rand(0, 0x3fff) | 0x8000,

		mt_rand(0, 0xffff), mt_rand(0, 0xffff), mt_rand(0, 0xffff)
		));
	}
	
	#CURRENT UTC TIME IN SQL TIMESTAMP
	public static function current_utc_time()
	{
		return gmdate("Y-m-d H:i:s", time());		
	}
	
	#CHECK IS JOSN OR NOT
	public static function isJSON($string)
	{
		return is_string($string) && is_array(json_decode($string, true)) && (json_last_error() == JSON_ERROR_NONE) ? true : false;
	}
	
	#TIME WITH ZONE CONVERT
	public static function time_convert_with_zone($unix_time)
	{
		date_default_timezone_set($_SESSION['timezone']);
		$TIME_ZONE = date('Y-m-d H:i:s', $unix_time);
		
		return $TIME_ZONE;
	}
	
	#CONVERT VOLUME TIME WITH TIMEZONE
	public static function convert_volume_time_with_zone($create_time)
	{
		#CONVERT USER SET TIME ZONE TO ABBREVIATION
		$dateTime = new DateTime(); 
		$dateTime -> setTimeZone(new DateTimeZone($_SESSION['timezone'])); 
		$time_abbr = $dateTime->format('T'); 
		
		#SET TIMEZONE TO UTC THAN CONVERT TO UNIX TIMESTAMP
		date_default_timezone_set('UTC');
		$volume_time = Misc::time_convert_with_zone(strtotime($create_time));
		
		#RE TEXT DESCRIPTION
		$description_with_time_format = $volume_time.' '.$time_abbr;
		
		return $description_with_time_format;
	}
	
	#CONVERT SNAPSHOT DESCRIPTION WITH TIMEZONE
	public static function convert_snapshot_time_with_zone($description, $create_time)
	{
		#CONVERT USER SET TIME ZONE TO ABBREVIATION
		$dateTime = new DateTime(); 
		$dateTime -> setTimeZone(new DateTimeZone($_SESSION['timezone'])); 
		$time_abbr = $dateTime->format('T'); 
	
		#RE FORMAT SNAPSHOT DESCRIPTION
		$description = explode('@', $description);
		$snapshot_name = $description[0];
		
		#GET TIME FORMAT FROM DESCRIPTION
		if (isset($description[1]))
		{
			$create_time = $description[1];
		}	
		
		#SET TIMEZONE TO UTC THAN CONVERT TO UNIX TIMESTAMP
		date_default_timezone_set('UTC');
		$snapshot_time = Misc::time_convert_with_zone(strtotime($create_time));

		#RE TEXT DESCRIPTION
		$description_with_time_format = $snapshot_name.'@ '.$snapshot_time.' '.$time_abbr;
		
		return $description_with_time_format;
	}
	
	#ENCRYPT DECRYPT
	public static function encrypt_decrypt($ACTION,$STRING)
	{
		$CERT_ROOT = getenv('WEBROOT').'/apache24/conf/ssl';				
		$PUBLIC_KEY_PATH = $CERT_ROOT.'/server.crt';
		$PRIVATE_KEY_PATH = $CERT_ROOT.'/server.key';
		
		switch ($ACTION)
		{
			case 'encrypt':
				#OPEN PRIVATE KEY FILE
				$OpenPublicKey = fopen($PUBLIC_KEY_PATH,"r");
				$PublicKey = fread($OpenPublicKey,8192);
				fclose($OpenPublicKey);
				
				openssl_public_encrypt($STRING,$CipherText,$PublicKey,OPENSSL_PKCS1_PADDING);
				$CipherText = base64_encode($CipherText);
			
				return $CipherText;
			break;
			
			case 'decrypt':
				#OPEN PRIVATE KEY FILE
				$OpenPrivatKey = fopen($PRIVATE_KEY_PATH,"r");
				$PrivateKey = fread($OpenPrivatKey,8192);
				fclose($OpenPrivatKey);
				
				$CipherText = base64_decode($STRING);
				openssl_private_decrypt($CipherText,$plaintext,$PrivateKey);
				return $plaintext;
			break;
		}
	}
	
	#DEFINE HTACCESS
	private static function define_htaccess($PORT_NUMBER,$URL_PORTAL_PATH)
	{
		$HTACCESS  = "RewriteEngine On\r\n";
		$HTACCESS .= "RewriteBase /\r\n";
		$HTACCESS .= "DirectorySlash On\r\n";
		$HTACCESS .= "\r\n";

		$HTACCESS .= "RewriteCond %{REQUEST_FILENAME} !-f\r\n";
		$HTACCESS .= "RewriteCond %{REQUEST_FILENAME} !-d\r\n";
		$HTACCESS .= "RewriteRule ^([^/?]*)$ ".$URL_PORTAL_PATH."/index.php?path=$1 [NC,L,QSA]\r\n";
		$HTACCESS .= "\r\n";
		
		$HTACCESS .= "RewriteRule ^(.*)/$ ".$URL_PORTAL_PATH."/$1 [R,L]\r\n";
		$HTACCESS .= "\r\n";
		
		/*
		if ($PORT_NUMBER != 443)
		{
			$HTACCESS .= "RewriteCond %{HTTP_HOST} ^[a-z0-9_]+$  [NC,OR]\r\n";
			$HTACCESS .= "RewriteCond %{HTTP_HOST} ^[a-z0-9_]+\.[a-z0-9_]+$  [NC,OR]\r\n";
			$HTACCESS .= "RewriteCond %{HTTP_HOST} ^[a-z0-9_]+\.[a-z0-9_]+\.[a-z0-9_]+$  [NC,OR]\r\n";
			$HTACCESS .= "RewriteCond %{HTTP_HOST} ^[a-z0-9_]+\.[a-z0-9_]+\.[a-z0-9_]+\.[a-z0-9_]+$  [NC]\r\n";
			$HTACCESS .= "RewriteRule ^(.*)$ https://%{SERVER_NAME}:".$PORT_NUMBER.$URL_PORTAL_PATH."/$1 [R=301,L]\r\n";
			$HTACCESS .= "\r\n";
		}

		$HTACCESS .= "RewriteCond %{HTTPS} off\r\n";
		$HTACCESS .= "RewriteRule (.*) https://%{SERVER_NAME}:".$PORT_NUMBER.$URL_PORTAL_PATH."/$1 [R,L]";
		*/
		
		if ($PORT_NUMBER == 443)
		{
			$REDIRECT_PORT = 18443;
		}
		else
		{
			$REDIRECT_PORT = 443;
		}
	
		$HTACCESS .= "RewriteCond %{SERVER_PORT} ^80$";
		$HTACCESS .= "\r\n";
		$HTACCESS .= "RewriteRule (.*) https://%{SERVER_NAME}:".$PORT_NUMBER.$URL_PORTAL_PATH."/$1 [R,L]";
		$HTACCESS .= "\r\n\r\n";
		
		$HTACCESS .= "RewriteCond %{SERVER_PORT} ^".$REDIRECT_PORT."$";
		$HTACCESS .= "\r\n";
		$HTACCESS .= "RewriteRule (.*) https://%{SERVER_NAME}:".$PORT_NUMBER.$URL_PORTAL_PATH."/$1 [R,L]";
		
		return $HTACCESS;
	}
	
	
	#UPDATE HTACCESS PORT
	public static function renew_htaccess_port($PORT_NUMBER)
	{		
		#GET FULL PORTAL PATH
		$REAL_PORTAL_PATH = str_replace('\\','/',dirname(dirname(__DIR__)));
		
		#ADD FULL PATH WITH .htaccess
		$DEFINE_HTACCESS = $REAL_PORTAL_PATH.'/.htaccess';		
	
		#GET PORTAL PATH
		$URL_PORTAL_PATH = str_replace($_SERVER['DOCUMENT_ROOT'],'',$REAL_PORTAL_PATH);

		#DELETE FILE
		if (file_exists($DEFINE_HTACCESS) == TRUE)
		{
			unlink($DEFINE_HTACCESS);
		}
		
		#RECREATE FILE
		$FOPEN = fopen($DEFINE_HTACCESS, 'w');
		$DEFAULT_HTACCESS = self::define_htaccess($PORT_NUMBER,$URL_PORTAL_PATH);
		file_put_contents($DEFINE_HTACCESS,$DEFAULT_HTACCESS);
		fclose($FOPEN);
		
		#CHANGE FILE ATTRIB TO HIDDEN
		system('attrib +H ' . escapeshellarg($DEFINE_HTACCESS));
	}
	
	#FOR FILE MAX UPLOAD SIZE
	public static function convert_to_bytes($INPUT)
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
}