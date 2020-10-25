<?php
###################################
#
#	PDO DB CONNECTION CLASS
#
###################################
class Db_Connection
{
    public $DBCON;
	
	public function __construct()
	{
		require '_config.php';
		
		try {
			$this -> DBCON = new PDO("mysql:host=".$db['sa_db']['hostname'].";dbname=".$db['sa_db']['database'].";charset=".$db['sa_db']['charset'], $db['sa_db']['username'],$db['sa_db']['password']);
		}
		catch(PDOException $Exception ) {
			return false;			
		}
		
		// Set errormode to exceptions
		$this -> DBCON -> setAttribute(PDO::ATTR_ERRMODE,PDO::ERRMODE_EXCEPTION);
 
		// Create new database in memory
		$memory_db = new PDO('sqlite::memory:');
    
		// Set errormode to exceptions
		$memory_db -> setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);
	}
}


###################################
#
#	RESTFUL PASS VIA CLASS
#
###################################
class Restful_Passvia extends Db_Connection
{
	#GET API REF POINT FROM DATABASE
	private function get_api_ref_point($URI_TOKEN)
	{
		$GET_EXEC 	 = "SELECT * FROM _SYS_RESTFUL_REF WHERE _URI_NAME = '".$URI_TOKEN."' AND _STATUS = 'Y'";
		$QUERY = $this -> DBCON -> prepare($GET_EXEC);
		$QUERY -> execute();
		$COUNT_ROWS = $QUERY->rowCount();

		if ($COUNT_ROWS == 1)
		{
			foreach($QUERY as $QueryResult)
			{
				$REF_FILE_NAME 	 = $QueryResult['_REF_NAME'];
				$REF_FILE_STATUS = $QueryResult['_STATUS'];
			}
						
			return array('NAME'   => $REF_FILE_NAME,
						 'STATUS' => $REF_FILE_STATUS);
		}
		else
		{
			return array('NAME' => 'NA', 'STATUS' => 'N');
		}	
	}
	
	#QUERY AND MAP RESTFUL VALUE
	public function query_restful_value($REQUEST_URI)
	{
		$REQUEST_URI = ltrim($REQUEST_URI, '/');
		$URI_TOKEN = array_filter(explode("/", $REQUEST_URI));

		if (isset($URI_TOKEN[0]) and $URI_TOKEN[0] == 'restful')
		{
			if (isset($URI_TOKEN[1]))
			{
				$RESTFUL_URI = array_filter(explode("?", $URI_TOKEN[1]));
			
				$URI_INFO = $this -> GET_API_REF_POINT($RESTFUL_URI[0]);
				
				if ($URI_INFO['STATUS'] == 'Y')
				{
					include '_include/_exec/'.$URI_INFO['NAME'];
				}
				else
				{
					echo 'Module Not Found';
				}
			}
		}			
	}
}


###################################
#
#	VERIFY CLASS
#
###################################
class Verify_Class extends Db_Connection
{
	public static function check_uuid_status($TYPE_UUID,$TYPE)
	{
		switch ($TYPE)
		{
			case "ACCT":
				$GET_EXEC 	= "SELECT * FROM _ACCOUNT WHERE _ACCT_UUID = '".$TYPE_UUID."' AND _STATUS = 'Y'";
			break;
					
			case "REGN":
				$GET_EXEC 	= "SELECT * FROM _ACCOUNT_REGN WHERE _REGN_UUID = '".$TYPE_UUID."' AND _STATUS = 'Y'";
			break;
		}
		$QUERY = $this -> DBCON -> prepare($GET_EXEC);
		$QUERY -> execute();
		$COUNT_ROWS = $QUERY->rowCount();
		
		if ($COUNT_ROWS == 1)
		{
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
#	RESET DATABASE PASSWORD
#
###################################
class Reset_Class extends Db_Connection
{
	public function ResetDatabase($REST_TYPE)
	{
		$REST_TYPE = strtoupper($REST_TYPE);		
		
		if ($REST_TYPE == 'GLOBAL')
		{
			//$TRUNCATE_ARY = array('_ACCOUNT','_ACCOUNT_REGN','_REPLICA','_REPLICA_DISK','_JOBS_HISTORY','_REPLICA_SNAP','_SERVER','_SERVER_CONN','_SERVER_DISK','_SERVER_HOST','_CLOUD_MGMT','_CLOUD_DISK','_SERVICE','_SERVICE_DISK');
			$TRUNCATE_ARY = array('_ACCOUNT','_REPLICA','_REPLICA_DISK','_JOBS_HISTORY','_REPLICA_SNAP','_SERVER','_SERVER_CONN','_SERVER_DISK','_SERVER_HOST','_CLOUD_MGMT','_CLOUD_DISK','_SERVICE','_SERVICE_DISK','_SERVICE_PLAN');
			//$TRUNCATE_ARY = array('_REPLICA','_REPLICA_DISK','_JOBS_HISTORY','_REPLICA_SNAP','_SERVER','_SERVER_CONN','_SERVER_DISK','_SERVER_HOST','_CLOUD_MGMT','_CLOUD_DISK','_SERVICE','_SERVICE_DISK');
		}
		elseif ($REST_TYPE == 'JOBS')
		{
			$TRUNCATE_ARY = array('_REPLICA','_REPLICA_DISK','_JOBS_HISTORY','_REPLICA_SNAP','_CLOUD_DISK','_SERVICE','_SERVICE_DISK');
		}
		else
		{
			return false;
		}
			
		for ($i=0; $i<count($TRUNCATE_ARY); $i++)
		{
			$TRUNCATE_TABLE = "TRUNCATE TABLE ".$TRUNCATE_ARY[$i]."";
			$QUERY = $this -> DBCON -> prepare($TRUNCATE_TABLE);
			$QUERY -> execute();
		}
		return true;
	}	
}

function maskdata( &$item, $key ){
	$mk = array(
		"access_key",
		"secret_key",
		"azure_storage_connection_string",
		"username",
		"password"
		);
	
	if( $key != 0 && array_search( $key, $mk ) !== FALSE ){
		$item = "string_masking".json_encode($mk);
	}
	
}

###################################
#
#	MISC CLASS
#
###################################
class Misc_Class
{
	#FATAL ERROR HANDLER
	public function __construct()
	{
		error_reporting(0);
		#register_shutdown_function(array(&$this, 'fatal_error_handler'));
	}
	
	#FATAL ERROR HANDLER
	/*
	public function fatal_error_handler()
	{
		$error = error_get_last();
			
		if(($error['type'] === E_ERROR) || ($error['type'] === E_USER_ERROR))
		{
			if (strpos($error['message'],'writeMessageBegin') == true)
			{
				$FatalErrorMsg = array('Code' => false, 'Msg' => 'Please check connection; failed to connect Saasame Services.');
				echo json_encode($FatalErrorMsg);
			}
			else
			{
				$FatalErrorMsg = array('Code' => false, 'Msg' => 'Sorry, a serious error has occured about' . $error['message']);
				echo json_encode($FatalErrorMsg);
			}
		}
	}
	*/

	#ENCRYPT LOOP
	private static function encrypt( $source, $key )
	{
		$maxlength = 128;
		
		$output = '';
		
		while($source){
			
			$input= substr( $source, 0, $maxlength );
			
			$source=substr( $source, $maxlength );
			
			openssl_public_encrypt( $input, $encrypted, $key, OPENSSL_PKCS1_PADDING );

			$output.=$encrypted;
		}
		
		$output = base64_encode($output);
		
		return $output;
	}
	
	#DECRYPT LOOP
	private static function decrypt( $source, $key )
	{
		$maxlength = 256;
		
		$output = '';
		
		$source = base64_decode($source);

		while($source){
			
			$input= substr($source,0,$maxlength);
			
			$source=substr($source,$maxlength);
			
			openssl_private_decrypt($input,$out,$key);

			$output.=$out;
		}
		
		return $output;

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
				#FILE OPEN PRIVATE KEY
				$OpenPublicKey = fopen($PUBLIC_KEY_PATH,"r");
				$PublicKey = fread($OpenPublicKey,8192);
				fclose($OpenPublicKey);
				
				return self::encrypt( $STRING, $PublicKey);
			break;
			
			case 'decrypt':
				#OPEN PRIVATE KEY FILE
				$OpenPrivatKey = fopen($PRIVATE_KEY_PATH,"r");
				$PrivateKey = fread($OpenPrivatKey,8192);
				fclose($OpenPrivatKey);
				
				return self::decrypt( $STRING, $PrivateKey);		
			break;
		}
	}
	
	/*public static function encrypt_decrypt($ACTION,$STRING)
	{
		$CERT_ROOT = getenv('WEBROOT').'/apache24/conf/ssl';				
		$PUBLIC_KEY_PATH = $CERT_ROOT.'/server.crt';
		$PRIVATE_KEY_PATH = $CERT_ROOT.'/server.key';
		
		switch ($ACTION)
		{
			case 'encrypt':
				#FILE OPEN PRIVATE KEY
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
				$Decrypt = openssl_private_decrypt($CipherText,$plaintext,$PrivateKey);
				if ($Decrypt == FALSE)
				{
					#COMPATIBLE WITH LAST VERSION
					$openssl_key = 'bSJRDOcGC6c0WG5o5SuA';
					$openssl_iv = 'xT4FvqRyaNjcDDWa';
					$plaintext = openssl_decrypt($CipherText, 'AES-256-CBC', md5($openssl_key), OPENSSL_RAW_DATA, $openssl_iv);
					if ($plaintext == FALSE)
					{
						#COMPATIBLE WITH LAST LAST VERSION
						$mcrypt_key = 'SaasameForTheWin';
						$mcrypt_iv = md5(md5($mcrypt_key));
						$ciphertext = base64_decode($STRING);
						$plaintext = mcrypt_decrypt(MCRYPT_RIJNDAEL_256, md5($mcrypt_key), $ciphertext, MCRYPT_MODE_CBC, $mcrypt_iv);
						return rtrim($plaintext, "\0");
					}
					else
					{
						return $plaintext;
					}
				}
				else
				{
					return $plaintext;
				}				
			break;
		}
	}*/
	
	#GENERATE GUID VERSION 4
	public static function guid_v4()
	{
		$Random_Data = random_bytes(16);
		assert(strlen($Random_Data) == 16);

		$Random_Data[6] = chr(ord($Random_Data[6]) & 0x0f | 0x40); // set version to 0100
		$Random_Data[8] = chr(ord($Random_Data[8]) & 0x3f | 0x80); // set bits 6-7 to 10

		return vsprintf('%s%s-%s-%s-%s-%s%s%s', str_split(bin2hex($Random_Data), 4));
		
		/*
		return strtoupper(sprintf('%04x%04x-%04x-%04x-%04x-%04x%04x%04x',

		mt_rand(0, 0xffff), mt_rand(0, 0xffff),

		mt_rand(0, 0xffff),

		mt_rand(0, 0x0fff) | 0x4000,

		mt_rand(0, 0x3fff) | 0x8000,

		mt_rand(0, 0xffff), mt_rand(0, 0xffff), mt_rand(0, 0xffff)
		));
		*/
	}
	
	#CHECK IS VALID UUID
	public static function is_valid_uuid($UUID)
	{
		if (!is_string($UUID) || (preg_match('/^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$/', $UUID) !== 1))
		{
			return false;
		}
		else
		{
			return true;
		}
	}

	#CURRENT UTC TIME IN SQL TIMESTAMP
	public static function current_utc_time()
	{
		return gmdate("Y-m-d H:i:s", time());		
	}
	
	#STR_REPLACE HOSTNAME
	public static function str_replace_hostname($HOSTNAME)
	{
		$STR = array(" ","(",")","'");
		$REPLACE = array("-","-","-","-");
		
		return str_replace($STR,$REPLACE,$HOSTNAME);		
	}
		
	#SQL CLEAN UP
	private static function sql_backup_cleanup($CURRENT_TIME)
	{
		$UNLINK_BACKUP = $CURRENT_TIME - (86400 * 30);
		$ARCHIVE_PATH = getenv('WEBROOT').'\apache24\htdocs\_include\_debug\_archive\\';
		
		#UNLINK OLD .SQL FILES
		foreach (glob($ARCHIVE_PATH.'*.sql') as $OLD_ARCHIVE_FILE) 
		{
			unlink($OLD_ARCHIVE_FILE);
		}
		
		#REMOVE SQL OLDER THAN 30 DAYS
		foreach (glob($ARCHIVE_PATH.'_sqlbackup\\'.'*.sql') as $ARCHIVE_FILE) 
		{
			$FILE_NAME = explode('.',str_replace($ARCHIVE_PATH.'_sqlbackup\\','',$ARCHIVE_FILE));
			if ($UNLINK_BACKUP > $FILE_NAME[0])
			{
				unlink($ARCHIVE_FILE);
			}
		}
	}
	
	#BACKUP MYSQL DATABSE
	public static function backup_database($ROTATION)
	{
		require '_config.php';
		
		$CURRENT_TIME = time();													
		$FILE_NAME = array();
		$ARCHIVE_PATH = getenv('WEBROOT').'apache24\htdocs\_include\_debug\_archive\\';
		$DEBUG_PATH  = getenv('WEBROOT').'apache24\htdocs\_include\_debug\\';
		$EXEDUMP_PATH = getenv('WEBROOT').'mariadb\bin\mysqldump.exe';
		$DBA_USERNAME = $db['sa_db']['username'];
		$DBA_PASSWORD = $db['sa_db']['password'];
		$DBA_DATABASE = $db['sa_db']['database'];
		
		#CREATE SQL BACKUP ARCHIVE FOLDER
		if (is_dir($ARCHIVE_PATH.'_sqlbackup\\') == false)
		{
			mkdir($ARCHIVE_PATH.'_sqlbackup\\', 0777, true);
		}
	
		#LIST ALL .SQL FILES
		foreach (glob($ARCHIVE_PATH.'_sqlbackup\\'.'*.sql') as $ARCHIVE_FILE) 
		{
			$FILE_ARCH = explode('.',str_replace($ARCHIVE_PATH.'_sqlbackup\\','',$ARCHIVE_FILE));
			$FILE_NAME[] = $FILE_ARCH[0];
		}
		
		if ($ROTATION == 'SYSTEM')
		{		
			$LAST_BACKUP = $CURRENT_TIME - end($FILE_NAME);
			$ROTATION_DAYS = 86400 * 1;
		
			if ($LAST_BACKUP > $ROTATION_DAYS)
			{
				$ARCHIVE_NAME = $ARCHIVE_PATH.'_sqlbackup\\'.$CURRENT_TIME.'.sql';
				$mysql_dump = '"'.$EXEDUMP_PATH.'" --user='.$DBA_USERNAME.' --password='.$DBA_PASSWORD.' --databases '.$DBA_DATABASE.' > "'.$ARCHIVE_NAME.'"';
				exec($mysql_dump,$output,$return_var);
				Misc_Class::function_debug('_mgmt','database_backup',array('scheduler_sql_backup' => $ARCHIVE_NAME));
			}
			else
			{
				Misc_Class::function_debug('_mgmt','database_backup',array('last_sql_backup' => end($FILE_NAME)));
			}
			
			#CLEAN UP SQL
			self::sql_backup_cleanup($CURRENT_TIME);
		}
		else if($ROTATION == 'NOW')
		{
			$ARCHIVE_NAME = $ARCHIVE_PATH.'_sqlbackup\\'.$CURRENT_TIME.'.sql';
			$mysql_dump = '"'.$EXEDUMP_PATH.'" --user='.$DBA_USERNAME.' --password='.$DBA_PASSWORD.' --databases '.$DBA_DATABASE.' > "'.$ARCHIVE_NAME.'"';
			exec($mysql_dump,$output,$return_var);
			Misc_Class::function_debug('_mgmt','database_backup',array('now_sql_backup' => $ARCHIVE_NAME));
			return $ARCHIVE_NAME;
		}
		else
		{
			$ARCHIVE_NAME = $DEBUG_PATH.'xray_backup_'.$CURRENT_TIME.'.sql';
			$mysql_dump = '"'.$EXEDUMP_PATH.'" --user='.$DBA_USERNAME.' --password='.$DBA_PASSWORD.' --databases '.$DBA_DATABASE.' > "'.$ARCHIVE_NAME.'"';
			exec($mysql_dump,$output,$return_var);
			Misc_Class::function_debug('_mgmt','database_backup',array('xray_sql_backup' => $ARCHIVE_NAME));
		}
	}
	
	#CHECK JOB CREATE VERSION
	public static function compatibility_version()
	{
		$IS_ENABLE = true;
		$XRAY_WIN_VERSION = '1.0.301.0';
		$XRAY_LX_VERSION  = '1.6.5';
		
		return (object)array('is_enable' => $IS_ENABLE, 'xray_win_version' => $XRAY_WIN_VERSION, 'xray_lx_version' => $XRAY_LX_VERSION);
	}
	
	
	public static function transport_report_async($REPORT_DATA)
	{
		#READ REFERENCE
		$REPORT_REF = self::define_mgmt_setting();
		
		#UNSET DATA
		unset($REPORT_DATA['EncryptKey']);
		unset($REPORT_DATA['Action']);
		
		#DEFINE SERVER LOCATION		
		if ($REPORT_REF -> report -> address == '')
		{				
			$SERV_LOCATION = array('https://report.saasame.com','https://report.saasame');
		}
		else
		{
			$SERV_LOCATION = explode(',', $REPORT_REF -> report -> address);
		}
			
		#CONVERT REPORT TO JSON FORMAT
		$REPORT_JSON = json_encode($REPORT_DATA);

		for ($i=0; $i<count($SERV_LOCATION); $i++)
		{
			sleep(5);
			
			$URL = $SERV_LOCATION[$i].'/api/v1/records';
		
			$curl = curl_init($URL); 
			curl_setopt($curl, CURLOPT_SSL_VERIFYHOST, 0);
			curl_setopt($curl, CURLOPT_SSL_VERIFYPEER, 0);
			curl_setopt($curl, CURLOPT_HEADER, 0);
			
			curl_setopt($curl, CURLOPT_FRESH_CONNECT, true);
			curl_setopt($curl, CURLOPT_CONNECTTIMEOUT ,5); 
			curl_setopt($curl, CURLOPT_TIMEOUT, 5);
			
			curl_setopt($curl, CURLOPT_POSTFIELDS, $REPORT_JSON);
			curl_setopt($curl, CURLOPT_POST, 1);
			curl_setopt($curl, CURLOPT_RETURNTRANSFER, 1);
			curl_setopt($curl, CURLOPT_FAILONERROR,true);
			
			$RESPONSES = curl_exec($curl);
			$HTTP_CODE = curl_getinfo($curl,CURLINFO_HTTP_CODE);
			
			if ($RESPONSES === FALSE)
			{
				$RESPONSES = json_encode(array('curl_error' => curl_error($curl)));
			}
			
			$ReportMgmtFile = json_decode($RESPONSES);
			$ReportMgmtFile -> http_code = $HTTP_CODE;
			$ReportMgmtFile -> submit_url = $URL;
			$LogLocation = $REPORT_DATA['LogLocation'];
			
			if (!isset($LogLocation))
			{
				$LogLocation = '_mgmt';
			}
				
			Misc_Class::function_debug($LogLocation,'reporting_management',$ReportMgmtFile);
			curl_close($curl);	
		}
	}	
	
	#TRANSPORT REPORT CALLBACK
	public static function transport_report($REPORT_DATA)
	{
		#READ REFERENCE
		$REPORT_REF = self::define_mgmt_setting();
		
		#GET ON/OFF STATUS
		$IS_ENABLE = $REPORT_REF -> report -> enable;
				
		#BEGIN TO REPORT
		if ($IS_ENABLE == TRUE)
		{
			$POST_DATA['EncryptKey'] = Misc_Class::encrypt_decrypt('encrypt',time());
			$POST_DATA['Action'] = 'ReportMgmtAsync';
			$POST_DATA['ReportData'] = $REPORT_DATA;
	
			$curl = curl_init();
			curl_setopt($curl, CURLOPT_URL, 'http://127.0.0.1:8080/restful/ServiceManagement');
			curl_setopt($curl, CURLOPT_FRESH_CONNECT, true);
			curl_setopt($curl, CURLOPT_POSTFIELDS, json_encode($POST_DATA));
			curl_setopt($curl, CURLOPT_TIMEOUT_MS, 2500);
	
			curl_exec($curl);
			curl_close($curl);
		}
	}

	#SET DEFAULT THRIFT SECURE SCOKET COMMUNICATION
	public static function secure_socket_config()
	{
		$IS_SECURE_SOCKET = true;
		
		return $IS_SECURE_SOCKET;
	}
	
	#SET QGA CONFIGURATION
	public static function qga_config()
	{
		/* Timeout Control */
		$IS_ENABLE 		 = false;
		$MS_WAIT_TIME 	 = 240;
		$LX_WAIT_TIME 	 = 180;
		$RETRY_WAIT_TIME = 30;
		$RETYR_COUNT 	 = 10;
		
		/* Network Configuration */
		$NETWORK_TYPE 	 = 'public';
		$ONBOOT 		 = 'yes';
		
		return array('is_enable'		=> $IS_ENABLE,
					 'ms_wait_time'		=> $MS_WAIT_TIME,
					 'lx_wait_time'		=> $LX_WAIT_TIME,
					 'retry_wait_time'	=> $RETRY_WAIT_TIME,
					 'retry_count'		=> $RETYR_COUNT,
					 'network_type'		=> $NETWORK_TYPE,
					 'onboot'			=> $ONBOOT);
	}
	
	#DEFINE MGMT INI
	private static function define_mgmt_ini()
	{
		$REF_SETUP  = "[report]\r\n";
		$REF_SETUP .= "address=\r\n";
		$REF_SETUP .= "enable=0\r\n";	
		$REF_SETUP .= "\r\n";
		
		/* Web Sockets */
		$REF_SETUP .= "[webdav]\r\n";
		$REF_SETUP .= "port=443\r\n";
		$REF_SETUP .= "ssl=1\r\n";
		$REF_SETUP .= "\r\n";
		
		$REF_SETUP .= "[verify]\r\n";
		$REF_SETUP .= "port=443\r\n";
		$REF_SETUP .= "ssl=1\r\n";
		$REF_SETUP .= "\r\n";
		
		$REF_SETUP .= "[scheduler]\r\n";
		$REF_SETUP .= "port=443\r\n";
		$REF_SETUP .= "ssl=1\r\n";
		$REF_SETUP .= "\r\n";
		
		$REF_SETUP .= "[loader]\r\n";
		$REF_SETUP .= "port=443\r\n";
		$REF_SETUP .= "ssl=1\r\n";
		$REF_SETUP .= "\r\n";
		
		$REF_SETUP .= "[launcher]\r\n";
		$REF_SETUP .= "port=443\r\n";
		$REF_SETUP .= "ssl=1\r\n";
		$REF_SETUP .= "\r\n";
		/* Web Sockets */
		
		/* Azure */
		$REF_SETUP .= "[azure_china]\r\n";
		$REF_SETUP .= "Auzre_ControlUrl=management.chinacloudapi.cn\r\n";
		$REF_SETUP .= "Auzre_LoginUrl=login.chinacloudapi.cn\r\n";
		$REF_SETUP .= "Azure_Resource=management.core.chinacloudapi.cn/\r\n";
		$REF_SETUP .= "\r\n";
		
		$REF_SETUP .= "[azure_international]\r\n";
		$REF_SETUP .= "Auzre_ControlUrl=management.azure.com\r\n";
		$REF_SETUP .= "Auzre_LoginUrl=login.microsoftonline.com\r\n";
		$REF_SETUP .= "Azure_Resource=management.core.windows.net/\r\n";
		$REF_SETUP .= "\r\n";
		
		$REF_SETUP .= "[azure_enpoint]\r\n";
		$REF_SETUP .= "International=azure_international\r\n";
		$REF_SETUP .= "China=azure_china\r\n";
		$REF_SETUP .= "\r\n";
		/* Azure */
		
		/* AWS */
		#$REF_SETUP .= "[aws_region]\r\n";
		#$REF_SETUP .= "default=us-east-1\r\n";
		#$REF_SETUP .= "\r\n";
		
		$REF_SETUP .= "[aws_ec2_type]\r\n";
		$REF_SETUP .= "t2.nano='{\"Name\":\"t2.nano\",\"vCPU\":1,\"ECU\":\"Variable\",\"Memory\":\"0.5 GiB\",\"InstanceStorage\":\"EBS Only\"}'\r\n";
		$REF_SETUP .= "t2.micro='{\"Name\":\"t2.micro\",\"vCPU\":1,\"ECU\":\"Variable\",\"Memory\":\"1 GiB\",\"InstanceStorage\":\"EBS Only\"}'\r\n";
		$REF_SETUP .= "t2.small='{\"Name\":\"t2.small\",\"vCPU\":1,\"ECU\":\"Variable\",\"Memory\":\"2 GiB\",\"InstanceStorage\":\"EBS Only\"}'\r\n";
		$REF_SETUP .= "t2.medium='{\"Name\":\"t2.medium\",\"vCPU\":2,\"ECU\":\"Variable\",\"Memory\":\"4 GiB\",\"InstanceStorage\":\"EBS Only\"}'\r\n";
		$REF_SETUP .= "t2.large='{\"Name\":\"t2.large\",\"vCPU\":2,\"ECU\":\"Variable\",\"Memory\":\"8 GiB\",\"InstanceStorage\":\"EBS Only\"}'\r\n";
		$REF_SETUP .= "\r\n";		
		/* AWS */
				
		/* Alibaba Cloud */
		$REF_SETUP .= "[alibaba_cloud]\r\n";
		$REF_SETUP .= "recover_mode=1\r\n";
		$REF_SETUP .= "\r\n";
		/* Alibaba Cloud  */
		
		/* OpenStack */
		$REF_SETUP .= "[openstack]\r\n";
		$REF_SETUP .= "create_volume_from_image=1\r\n";
		$REF_SETUP .= "volume_create_type=SATA\r\n";
		$REF_SETUP .= "volume_create_az=''\r\n";
		$REF_SETUP .= "debug_level=0\r\n";
		$REF_SETUP .= "endpoint_interface=public\r\n";		
		$REF_SETUP .= "disk_create_retry=20\r\n";
		$REF_SETUP .= "disk_mount_retry=5\r\n";
		$REF_SETUP .= "disk_unmount_retry=5\r\n";
		$REF_SETUP .= "disk_wait_time_retry=30\r\n";	
 		/* OpenStack */
		
		/* CTyun */
		$REF_SETUP .= "[ctyun]\r\n";
		$REF_SETUP .= "debug_level=0\r\n";
		
		return $REF_SETUP;
	}
		
	#SET DEFAULT MGMT COMMUNICATION TYPE
	public static function mgmt_comm_type($SERV_TYPE)
	{		
		$WEB_SOCKET = self::define_mgmt_setting();
		
		$RESPONSE_WEB_SOCKET = $WEB_SOCKET -> $SERV_TYPE;
		
		return array('mgmt_port' => $RESPONSE_WEB_SOCKET -> port, 'is_ssl' => $RESPONSE_WEB_SOCKET -> ssl);		
	}
	
	#UPGRAGE MGMT SETTINGS
	private static function upgrade_mgmt_setting()
	{
		#DEFINE DEFAULT REGION
		$DEFINE_INI_FILE = __DIR__ .'\_transport\transport_mgmt.ini';
		
		$REGEN_INI = false;		
		$MGMT_INI = parse_ini_file($DEFINE_INI_FILE,true);
		
		if (isset($MGMT_INI['azure_select_enpoint']))
		{
			unset($MGMT_INI['azure_china']);
			unset($MGMT_INI['azure_international']);
			unset($MGMT_INI['azure_select_enpoint']);
			$REGEN_INI = true;
		}
		
		if (!isset($MGMT_INI['azure_china']) OR !isset($MGMT_INI['azure_international']) OR !isset($MGMT_INI['azure_enpoint']))
		{
			$MGMT_INI['azure_china']['Auzre_ControlUrl'] = 'management.chinacloudapi.cn';
			$MGMT_INI['azure_china']['Auzre_LoginUrl']   = 'login.chinacloudapi.cn';
			$MGMT_INI['azure_china']['Azure_Resource'] 	 = 'management.core.chinacloudapi.cn/';
			
			$MGMT_INI['azure_international']['Auzre_ControlUrl'] = 'management.azure.com';
			$MGMT_INI['azure_international']['Auzre_LoginUrl']   = 'login.microsoftonline.com';
			$MGMT_INI['azure_international']['Azure_Resource'] 	 = 'management.core.windows.net/';
			
			$MGMT_INI['azure_enpoint']['International'] = 'azure_international';
			$MGMT_INI['azure_enpoint']['China'] = 'azure_china';			
			$REGEN_INI = true;
		}
			
		if (isset($MGMT_INI['aws_region']))
		{
			unset($MGMT_INI['aws_region']);
			unset($MGMT_INI['aws_region']['default']);			
			$REGEN_INI = true;
		}

		if (!isset($MGMT_INI['aws_ec2_type']) OR count($MGMT_INI['aws_ec2_type']) == 0)
		{
			$MGMT_INI['aws_ec2_type']['t2.nano'] = "{\"Name\":\"t2.nano\",\"vCPU\":1,\"ECU\":\"Variable\",\"Memory\":\"0.5 GiB\",\"InstanceStorage\":\"EBS Only\"}";
			$MGMT_INI['aws_ec2_type']['t2.micro'] = "{\"Name\":\"t2.micro\",\"vCPU\":1,\"ECU\":\"Variable\",\"Memory\":\"1 GiB\",\"InstanceStorage\":\"EBS Only\"}";
			$MGMT_INI['aws_ec2_type']['t2.small'] = "{\"Name\":\"t2.small\",\"vCPU\":1,\"ECU\":\"Variable\",\"Memory\":\"2 GiB\",\"InstanceStorage\":\"EBS Only\"}";
			$MGMT_INI['aws_ec2_type']['t2.medium'] = "{\"Name\":\"t2.medium\",\"vCPU\":2,\"ECU\":\"Variable\",\"Memory\":\"4 GiB\",\"InstanceStorage\":\"EBS Only\"}";
			$MGMT_INI['aws_ec2_type']['t2.large'] = "{\"Name\":\"t2.large\",\"vCPU\":2,\"ECU\":\"Variable\",\"Memory\":\"8 GiB\",\"InstanceStorage\":\"EBS Only\"}";
			$REGEN_INI = true;
		}
		
		if (!isset($MGMT_INI['openstack']) OR !isset($MGMT_INI['openstack']['create_volume_from_image']))
		{
			$MGMT_INI['openstack']['create_volume_from_image'] = 0;			
			$REGEN_INI = true;
		}
		
		if (!isset($MGMT_INI['openstack']['volume_create_type']))
		{
			$MGMT_INI['openstack']['volume_create_type'] = 'SATA';			
			$REGEN_INI = true;
		}
				
		if (isset($MGMT_INI['openstack']['identity_port']))
		{
			unset($MGMT_INI['openstack']['identity_port']);			
			$REGEN_INI = true;
		}
		
		if (!isset($MGMT_INI['openstack']['volume_create_az']))
		{
			$MGMT_INI['openstack']['volume_create_az'] = '';			
			$REGEN_INI = true;
		}
		
		if (!isset($MGMT_INI['openstack']['debug_level']))
		{
			$MGMT_INI['openstack']['debug_level'] = 0;			
			$REGEN_INI = true;
		}
		
		if (!isset($MGMT_INI['openstack']['endpoint_interface']))
		{
			$MGMT_INI['openstack']['endpoint_interface'] = 'public';			
			$REGEN_INI = true;
		}
		
		if (!isset($MGMT_INI['openstack']['disk_create_retry']))
		{
			$MGMT_INI['openstack']['disk_create_retry'] = 20;			
			$REGEN_INI = true;
		}
		
		if (!isset($MGMT_INI['openstack']['disk_mount_retry']))
		{
			$MGMT_INI['openstack']['disk_mount_retry'] = 5;			
			$REGEN_INI = true;
		}
		
		if (!isset($MGMT_INI['openstack']['disk_unmount_retry']))
		{
			$MGMT_INI['openstack']['disk_unmount_retry'] = 5;			
			$REGEN_INI = true;
		}
		
		if (!isset($MGMT_INI['openstack']['disk_wait_time_retry']))
		{
			$MGMT_INI['openstack']['disk_wait_time_retry'] = 30;			
			$REGEN_INI = true;
		}
		
		if (!isset($MGMT_INI['ctyun']) OR !isset($MGMT_INI['ctyun']['debug_level']))
		{
			$MGMT_INI['ctyun']['debug_level'] = 0;			
			$REGEN_INI = true;
		}
		
		if ($REGEN_INI == TRUE)
		{
			$UPGRADE_INI = '';
			foreach ($MGMT_INI as $INI_TYPE => $INI_ITEM)
			{
				$UPGRADE_INI .= "[".$INI_TYPE."]\r\n";
				foreach ($INI_ITEM as $INI_KEY => $INI_VALUE)
				{
					if ($INI_TYPE == 'aws_ec2_type')
					{
						$UPGRADE_INI .= $INI_KEY."='".$INI_VALUE."'\r\n";
					}
					else
					{
						$UPGRADE_INI .= $INI_KEY."=".$INI_VALUE."\r\n";
					}
				}
				$UPGRADE_INI .= "\r\n";
			}

			file_put_contents($DEFINE_INI_FILE,$UPGRADE_INI);
		}
	}
	
	#DEFINE MGMT INI
	public static function define_mgmt_setting()
	{
		#DEFINE DEFAULT REGION
		$DEFINE_INI_FILE = __DIR__ .'\_transport\transport_mgmt.ini';

		if (file_exists($DEFINE_INI_FILE) != TRUE)
		{
			fopen($DEFINE_INI_FILE, 'w');
		}
		
		#IF THE FILE CANNOT BE OPENED REGEN THE INI
		if ((@parse_ini_file($DEFINE_INI_FILE, true)) == FALSE)
		{
			unlink($DEFINE_INI_FILE);
			$DEFAULT_REF = self::define_mgmt_ini();
			file_put_contents($DEFINE_INI_FILE,$DEFAULT_REF);
		}
		else
		{
			self::upgrade_mgmt_setting(); #CHECK FOR THE UPGRADE
		}
	
		return json_decode(json_encode(parse_ini_file($DEFINE_INI_FILE,true)),false);
	}
	
	#UPDATE PORT NUMBER
	public static function update_port_number($PORT_NUMBER=443)
	{
		$READ_INI = self::define_mgmt_setting();
	
		$READ_INI -> webdav -> port = $PORT_NUMBER;
		$READ_INI -> verify -> port = $PORT_NUMBER;
		$READ_INI -> scheduler -> port = $PORT_NUMBER;
		$READ_INI -> loader -> port = $PORT_NUMBER;
		$READ_INI -> launcher -> port = $PORT_NUMBER;
		
		$FILE_INI = '';
		foreach ($READ_INI as $NAME => $READ_VALUE)
		{
			$FILE_INI .= "[".$NAME."]\r\n";
			foreach ($READ_VALUE as $VALUE_NAME => $VALUE)
			{
				if ($VALUE_NAME == 'enable' OR $VALUE_NAME == 'ssl')
				{
					if ($VALUE == 1)
					{
						$VALUE = 'true';
					}
					else
					{
						$VALUE = 'false';
					}
				}
				
				if ($NAME == 'aws_ec2_type')
				{
					$FILE_INI .= (string)$VALUE_NAME."='".$VALUE."'\r\n";	#JSON TYPE
				}
				else
				{
					$FILE_INI .= (string)$VALUE_NAME."=".$VALUE."\r\n";
				}
			}			
			$FILE_INI .= "\r\n";
		}

		$DEFINE_INI_FILE = __DIR__ .'\_transport\transport_mgmt.ini';
		file_put_contents($DEFINE_INI_FILE, $FILE_INI);	
	}
	
	#CONNECTION PARAMETER
	public static function connection_parameter()
	{
		/* Connection Parameter */
		$IS_COMPRESSED 	 = true;
		$IS_CHECKSUM	 = false;
		$IS_ENCRYPTED 	 = false;
		$WEBDAV_USER 	 = 'admin';
		$WEBDAV_PASS 	 = 'abc@123';
		
		return array('is_compressed' => $IS_COMPRESSED,
					 'is_checksum'   => $IS_CHECKSUM,
					 'is_encrypted'  => $IS_ENCRYPTED,
					 'webdav_user'   => $WEBDAV_USER,
					 'webdav_pass'   => $WEBDAV_PASS);		
	}
	
	#DISK ENUMERATE USING SCSI OR SERIAL NUMBER MODE
	public static function enumerate_disks_action_parameter()
	{
		$IS_SCSI_MODE = false;
		
		return $IS_SCSI_MODE;
	}
	
	#DISK ACTION WAITER CONFIGURATION
	public static function disk_action_waiter()
	{
		$LOADER_CREATE_COUNT	 = 20;
		$LOADER_MOUNT_COUNT 	 = 5;
		$LAUNCHER_CREATE_COUNT	 = 20;
		$LAUNCHER_MOUNT_COUNT 	 = 5;
		$REPLICA_UNMOUNT_COUNT	 = 5;
		$SERVICE_UNMOUNT_COUNT	 = 5;
		$WAIT_TIME				 = 30;
		
		return array('loader_create_count'		=> $LOADER_CREATE_COUNT,
					 'loader_mount_count' 		=> $LOADER_MOUNT_COUNT,
					 'launcher_create_count'	=> $LAUNCHER_CREATE_COUNT,
					 'launcher_mount_count' 	=> $LAUNCHER_MOUNT_COUNT,
					 'replica_unmount_count'	=> $REPLICA_UNMOUNT_COUNT,
					 'service_unmount_count'	=> $SERVICE_UNMOUNT_COUNT,
					 'wait_time'				=> $WAIT_TIME);
	}
	
	#AUTOMATIC LOG ARCHIVE
	public static function smart_log_archive($FULL_FILE_PATH)
	{
		$FILE_SIZE 	  = filesize($FULL_FILE_PATH);
		$FILE_INFO 	  = explode('.',substr($FULL_FILE_PATH,strrpos($FULL_FILE_PATH,'/') + 1));
		$FILE_NAME    = $FILE_INFO[0];
		$FILE_EXT	  = $FILE_INFO[1];
		
		$ARCHIVE_PATH = $_SERVER['DOCUMENT_ROOT'].'/_include/_debug/_archive';
		$ARCHIVE_NAME = $ARCHIVE_PATH.'/'.$FILE_NAME.'-'.time().'.zip';
		
		#CHECK AND CREATE PATH EXISTS
		if(!file_exists($ARCHIVE_PATH))
		{
			mkdir($ARCHIVE_PATH);
		}
		
		$COMPARE_SIZE = 33 * 1000000; # 33MB

		if ($FILE_SIZE > $COMPARE_SIZE)
		{	
			$Zip = new ZipArchive();		
			
			if ($Zip -> open($ARCHIVE_NAME, ZipArchive::CREATE) === TRUE)
			{			
				$Zip -> addFile($FULL_FILE_PATH,$FILE_NAME.'.'.$FILE_EXT);
				$Zip -> close();

				#RESET FILE TO EMPTY
				$fp = @fopen($FULL_FILE_PATH, "r+");
				if ($fp !== false)
				{
					ftruncate($fp, 0);
					fclose($fp);
				}
			}
		}
	}
	
	#PASSWORD GENERATOR
	public static function password_generator($length)
	{
		$Characters = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
		$Numbers 	= "0123456789";
		$sCharacter = "abcdefghijklmnopqrstuvwxyz";
		//$rCharacter = "!@#$%^&*";
	
		$NumOfChar = $length - 4;
		$Characters = substr( str_shuffle( $Characters ), 0, $NumOfChar);
		$Numbers = substr( str_shuffle( $Numbers ), 0, 2);
		$sCharacter = substr( str_shuffle( $sCharacter ), 0, 2);
		//$rCharacter = substr( str_shuffle( $rCharacter ), 0, 1);
		
		$password = substr( str_shuffle( $Characters.$Numbers.$sCharacter ), 0, $length );
		return $password;
	}
	
	#DEBUG FUNCTION OUTPUT
	public static function function_debug($FOLDERNAME,$FILENAME,$DETAIL_INFO1)
	{
		$DETAIL_INFO = json_decode( json_encode( $DETAIL_INFO1 ), true );
		
		if( is_array($DETAIL_INFO) ){
			array_walk_recursive($DETAIL_INFO, 'maskdata');
		}
		else{
			$DETAIL_INFO = array( $DETAIL_INFO );
		}
		
		#FLAG
		$IS_ENABLE = true;
		
		if ($IS_ENABLE == TRUE)
		{
			try
			{
				$LOG_PATH = $_SERVER['DOCUMENT_ROOT'].'/_include/_debug/'.$FOLDERNAME;
				$LOG_FILE = $_SERVER['DOCUMENT_ROOT'].'/_include/_debug/'.$FOLDERNAME.'/'.$FILENAME.'.txt';
				
				#CHECK AND CREATE FOLDER EXISTS
				if(!file_exists($LOG_PATH))
				{
					mkdir($LOG_PATH);
				}				
				
				#CHECK AND CREATE FILE EXISTS
				if(!file_exists($LOG_FILE))
				{
					$fp = fopen($LOG_FILE,'w');
					if(!$fp)
					{
						throw new Exception('File open failed.');
					}
					else
					{
						fclose($fp);
					}
				}
				else
				{
					#LOG ARCHIVE
					self::smart_log_archive($LOG_FILE);
				}
								
				#ADD MGMT LOG TIME
				$DETAIL_INFO["mgmt_log_time"] = date('Y-m-d H:i:s');
				
				#MASKING PASSWORD
				
				$current  = file_get_contents($LOG_FILE);
				$current .= print_r($DETAIL_INFO,TRUE);
				$current .= "\n";
				file_put_contents($LOG_FILE, $current);				
			}
			catch (Throwable $e)
			{
				return false;
			}
		}
	}
		
	#ARCHIVE LOG FOLDER
	public static function archive_log_folder($FOLDERNAME)
	{
		$LOG_FOLDER = $_SERVER['DOCUMENT_ROOT'].'/_include/_debug/'.$FOLDERNAME;
		
		$ARCHIVE_PATH = $_SERVER['DOCUMENT_ROOT'].'/_include/_debug/_archive';
		$ARCHIVE_NAME = $ARCHIVE_PATH.'/'.$FOLDERNAME.'.zip';
		
		#CHECK AND CREATE PATH EXISTS
		if(!file_exists($ARCHIVE_PATH))
		{
			mkdir($ARCHIVE_PATH);
		}
		
		$zip = new ZipArchive();
		$zip -> open($ARCHIVE_NAME, ZipArchive::CREATE | ZipArchive::OVERWRITE);	
			
		$DIR_FILES = new RecursiveIteratorIterator(new RecursiveDirectoryIterator($LOG_FOLDER),RecursiveIteratorIterator::LEAVES_ONLY);
			
		foreach ($DIR_FILES as $FILE_NAME => $FILE)
		{
			if (!$FILE->isDir())
			{
				$FILE_PATH = $FILE -> getRealPath();
				$RELATIVE_PATH = substr($FILE_PATH, strlen($LOG_FOLDER) + 1);

				$zip -> addFile($FILE_PATH, $RELATIVE_PATH);
			}
		}
		$zip -> close();
		
		#DELETE ALL FILES IN THE FOLDER
		array_map('unlink', glob($LOG_FOLDER.'/*.*'));
		
		#REMNOVE FOLDER
		rmdir($LOG_FOLDER);
	}	
	
	#OPENSTACK DEBUG OUTPUT
	public static function openstack_debug($FILE_NAME,$DETAIL_INFO,$REQUEST_RESPONSE)
	{
		try
		{
			$PATH = $_SERVER['DOCUMENT_ROOT'].'/_include/_debug/_mgmt/';
			$FILE = $_SERVER['DOCUMENT_ROOT'].'/_include/_debug/_mgmt/'.$FILE_NAME.'.txt';
	
			if(!file_exists($PATH))
			{
				mkdir($PATH);
			}		
	
			#CHECK AND CREATE FILE EXISTS
			if(!file_exists($FILE))
			{
				$fp = fopen($FILE,'w');
				if(!$fp)
				{
					throw new Exception('File open failed.');
				}
				else
				{
					fclose($fp);
				}
			}
			else
			{
				#LOG ARCHIVE
				self::smart_log_archive($FILE);
			}
						
			if (isset($DETAIL_INFO -> REQUEST_NAME) and strpos($DETAIL_INFO -> REQUEST_NAME, 'GenerateAuthToken') !== FALSE)
			{
				$REQUEST_DATA = json_decode($DETAIL_INFO -> REQUEST_DATA , FALSE);
				
				if (isset($REQUEST_DATA -> auth -> tenantName))
				{					
					$REQUEST_DATA -> auth -> tenantName = 'string_masking';
					$REQUEST_DATA -> auth -> passwordCredentials -> username = 'string_masking';
					$REQUEST_DATA -> auth -> passwordCredentials -> password = 'string_masking';
				}
				else
				{
					$REQUEST_DATA -> auth -> identity -> password -> user -> name= 'string_masking';
					$REQUEST_DATA -> auth -> identity -> password -> user -> password = 'string_masking';
					$REQUEST_DATA -> auth -> identity -> password -> user -> domain -> name = 'string_masking';
				}					
				$DETAIL_INFO -> REQUEST_DATA = $REQUEST_DATA;
			}
			
			#ADD MGMT LOG TIME
			$DETAIL_INFO -> mgmt_log_time = date('Y-m-d H:i:s');
			
			$current = file_get_contents($FILE);
			$current .= 'DETAIL_INFO:'."\n";
			$current .= print_r(json_decode(json_encode($DETAIL_INFO),TRUE),TRUE);
			$current .= 'REQUEST_RESPONSE:'."\n";
			$current .= print_r(json_decode($REQUEST_RESPONSE,TRUE),TRUE);				
			$current .= "\n".'----------------------------------------------------------'."\n";
			file_put_contents($FILE, $current);				
		}
		catch (Throwable $e)
		{
			return false;
		}
	}
	
	#RESTORE SQL DATABASE
	public static function retore_database($RETORE_FILE)
	{
		require '_config.php';
		$SQLFILE_PATH = getenv('WEBROOT').'apache24\htdocs\_include\_debug\_mgmt\_restoreDB\\'.$RETORE_FILE;
		$MYSQL_PATH = getenv('WEBROOT').'mariadb\bin\mysql.exe';
		$DBA_HOSTNAME = $db['sa_db']['hostname'];
		$DBA_DATABASE = $db['sa_db']['database'];
		$DBA_USERNAME = $db['sa_db']['username'];
		$DBA_PASSWORD = $db['sa_db']['password'];
		$DBA_CHARSETS = $db['sa_db']['charset'];
		
		$FILE_OBJECT = new SplFileObject($SQLFILE_PATH, "r");
		$FILE_LINE = 30;
		while ($FILE_LINE != 0)
		{
			if (strpos($FILE_OBJECT -> fgets(), 'USE `management`;') !== false)
			{
				$FILE_LINE;
				break;
			}
			$FILE_LINE--;
		}
		
		if ($FILE_LINE != 0)
		{
			self::backup_database('NOW');
			
			$RESTORE_DB = new PDO("mysql:host=".$DBA_HOSTNAME.";dbname=".$DBA_DATABASE.";charset=".$DBA_CHARSETS,$DBA_USERNAME,$DBA_PASSWORD);
			
			#LIST TABLES IN MANAGEMENT
			$LIST_TABLE = "SHOW TABLES";
			$TABLE_LIST = $RESTORE_DB -> prepare($LIST_TABLE);
			$TABLE_LIST -> execute();
			$TABLES = $TABLE_LIST -> fetchAll(PDO::FETCH_ASSOC);
		
			#EMPTY TABLES IN MANAGEMENT
			for ($i=0; $i<count($TABLES); $i++)
			{
				$TRUNCATE_TABLE = "TRUNCATE `".$TABLES[$i]['Tables_in_management']."`";
				$TABLE_TRUNCATE = $RESTORE_DB -> prepare($TRUNCATE_TABLE);
				$TABLE_TRUNCATE -> execute();
			}
			
			#RESTORE SQL BACKUP TO MANAGEMENT
			$MYSQL_RESTORE = '"'.$MYSQL_PATH.'" --user='.$DBA_USERNAME.' --password='.$DBA_PASSWORD.' --database '.$DBA_DATABASE.' < "'.$SQLFILE_PATH.'"';
			exec($MYSQL_RESTORE,$MYSQL_OUTPUT,$MYSQL_RETURN_VAR);
			
			#QUERY ACCOUNT UUID
			$ACCT_EXEC = "SELECT * FROM _ACCOUNT WHERE _STATUS = 'Y' LIMIT 1";
			$ACCT_INFO = $RESTORE_DB -> prepare($ACCT_EXEC);
			$ACCT_INFO -> execute();
			$FETCH_DATA = $ACCT_INFO -> fetch();			
			$ACCT_UUID = $FETCH_DATA['_ACCT_UUID'];
			$REGN_UUID = json_decode($FETCH_DATA['_ACCT_DATA']) -> region;
			
			return array('Status' => true, 'Msg' => 'Success restore the database.','AcctUUID' => $ACCT_UUID, 'RegnUUID' => $REGN_UUID);
		}
		else
		{
			return array('Status' => false, 'Msg' => 'Faled to restore database.');
		}
	}
	
}