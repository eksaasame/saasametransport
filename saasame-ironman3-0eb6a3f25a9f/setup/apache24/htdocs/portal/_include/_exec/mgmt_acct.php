<?php
error_reporting(0);
session_start();
include '../_inc/_class_main.php';
$MyAdmin = new AdminOperation();
New Misc();

define('ROOT_PATH', __DIR__);
require_once(ROOT_PATH . "\..\_inc\languages\setlang.php");

function CurlPostToRemote($REST_DATA)
{
	$URL = 'http://127.0.0.1:8080/restful/IdentificationRegistration';
	
	$REST_DATA['EncryptKey'] = Misc::encrypt_decrypt('encrypt',time());
	$ENCODE_DATA = json_encode($REST_DATA);
	
	$ch = curl_init($URL);
	curl_setopt($ch, CURLOPT_CUSTOMREQUEST, "POST");
	curl_setopt($ch, CURLOPT_HEADER, false);
	curl_setopt($ch, CURLOPT_POSTFIELDS, $ENCODE_DATA);
	curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);	
	curl_setopt($ch, CURLOPT_HTTPHEADER, array('Content-Type: application/json','Content-Length: ' . strlen($ENCODE_DATA)));
	$output = curl_exec($ch);	

	return $output;
	#return json_decode($output,true);
}

$SelectAction = $_REQUEST["ACTION"];
switch ($SelectAction)
{
	############################
	# Check Administrator Login
	############################
	case "CheckAdministratorLogin":
		$INPUT_USERNAME = $_REQUEST['USERNAME'];
		$INPUT_PASSWORD = $_REQUEST['PASSWORD'];
		
		$LoginCheck = $MyAdmin -> check_login($INPUT_USERNAME,$INPUT_PASSWORD);
		
		print_r(json_encode(array('Code' => $LoginCheck)));	
	break;
	
	############################
	# Logout User
	############################
	case "LogoutUser":
		
		$MyAdmin -> add_user_logs('UserLogout','Success');
		
		$lang = $_SESSION["language"];
		
		session_destroy();
		
		print_r(json_encode(array('Code' => true, "language" => $lang)));
	
	break;
	
	############################
	# List Timezone
	############################
	case "TimeZoneList":
		$Timezones = array(
							'UTC'				=> _('(GMT) Coordinated Universal Time'),
							'US/Pacific'		=> _('(GMT-08:00) Pacific Time (US & Canada)'),
							'US/Mountain'		=> _('(GMT-07:00) Mountain Time (US & Canada)'),
							'US/Central'		=> _('(GMT-06:00) Central Time (US & Canada)'),
							'US/Eastern'		=> _('(GMT-05:00) Eastern Time (US & Canada)'),
							'Asia/Taipei'		=> _('(GMT+08:00) Beijing, Hong Kong, Kuala Lumpur, Singapore, Taipei'),
							'Asia/Tokyo'		=> _('(GMT+09:00) Seoul, Tokyo'),
							'Australia/Sydney'	=> _('(GMT+10:00) Sydney')
						);

		print_r(json_encode($Timezones));
	break;
	
	############################
	# List Web Port
	############################
	case "WebPortList":
		$WebPortList = array(
							443		=> 443,
							18443	=> 18443
						);

		print_r(json_encode($WebPortList));
	break;
	
	############################
	# List Language
	############################
	case "LanguageList":
		$Language = array(
							'en-us' => 'English',
							'zh-cn' => '简体中文',
							'zh-tw' => '繁體中文'
						);
		print_r(json_encode($Language));
	break;
	
	############################
	# Option Level
	############################
	case "OptionLevel":
		$OptionLevel = array('USER' 	 => _('Default'),
							 'SUPERUSER' => _('Advanced'),
							 'DEBUG' 	 => _('Debug')
						);
		print_r(json_encode($OptionLevel));
	break;
	
	############################
	# Update TimeZone and Language
	############################
	case "UpdateTimezoneAndLanguage":
		$ACCT_UUID		= $_REQUEST['ACCT_UUID'];
		$ACCT_ZONE		= $_REQUEST['ACCT_ZONE'];
		$ACCT_PORT		= $_REQUEST['ACCT_PORT'];
		$ACCT_LANG		= $_REQUEST['ACCT_LANG'];
		$OPTION_LEVEL	= $_REQUEST['OPTION_LEVEL'];
		
		
		$UpdateInfo = $MyAdmin -> update_user_setting($ACCT_UUID,$ACCT_ZONE,$ACCT_PORT,$ACCT_LANG,$OPTION_LEVEL);
		
		if ($UpdateInfo == true)
		{			
			$Response = array('Code' => true, 'Msg' => _('User settings updated.'));
		}
		
		
		$REST_DATA = array(
						'Action' 	=> 'AcctTimeZoneAndLanguage',
						'AcctUUID' 	=> $ACCT_UUID,
						'AcctLang' 	=> $ACCT_LANG,
						'TimeZone'	=> $ACCT_ZONE
					);		
		CurlPostToRemote($REST_DATA);
		print_r(json_encode($Response));
	break;
	
	############################
	# Change Password
	############################
	case "ChangePassword":
		$ACCT_UUID 		 = $_REQUEST['ACCT_UUID'];
		$INPUT_PASSWORD  = $_REQUEST['INPUT_PASSWORD'];
		$REPEAT_PASSWORD = $_REQUEST['REPEAT_PASSWORD'];
		
		if ($INPUT_PASSWORD != '')
		{
			if ($INPUT_PASSWORD == $REPEAT_PASSWORD)
			{
				$ChangePassword = $MyAdmin -> change_password($ACCT_UUID,$INPUT_PASSWORD);
				
				if ($ChangePassword == true)
				{	
					#USER LOGS
					$MyAdmin -> add_user_logs('ChangePassword','Success');
		
					$Response = array('Code' => true, 'Msg' => _('Password changed.'));
				}			
			}
			else
			{
				#USER LOGS
				$PASSWORD_JSON = json_encode(array($INPUT_PASSWORD => $REPEAT_PASSWORD));
				$MyAdmin -> add_user_logs('ChangePassword',$PASSWORD_JSON);
		
				$Response = array('Code' => false, 'Msg' => _('Password does not match.'));
			}			
		}
		else
		{
			$Response = array('Code' => true, 'Msg' => _('User settings updated.'));
		}
		print_r(json_encode($Response));
	break;
	
	############################
	# Worker Thread Number
	############################
	case "WorkerThreadNumber":
		$ThreadNumber = array(1 => 1,
							  2 => 2,
							  3 => 3,
							  4 => 4,
							  5 => 5,
							  6 => 6,
							  7 => 7,
							  8 => 8);
		print_r(json_encode($ThreadNumber));	
	break;
	
	############################
	# Loader Thread Number
	############################
	case "LoaderThreadNumber":
		$ThreadNumber = array(1 => 1,
							  2 => 2,
							  3 => 3,
							  4 => 4,
							  5 => 5,
							  6 => 6,
							  7 => 7,
							  8 => 8);
		print_r(json_encode($ThreadNumber));	
	break;
	
	############################
	# Loader Trigger Number
	############################
	case "LoaderTriggerPercentage":
		$ThreadNumber = array(0   => _('Run at start'),
							  10  => '10%',
							  20  => '20%',
							  30  => '30%',
							  40  => '40%',
							  50  => '50%',
							  60  => '60%',
							  70  => '70%',
							  80  => '80%',
							  90  => '90%',
							  100 => '100%'
							  );
		print_r(json_encode($ThreadNumber));	
	break;
	
	############################
	# Query Buffer Size
	############################
	case "QueryBufferSize":
		$ThreadNumber = array(0    => _('No Limit'),
							  1    => '1 GB',
							  5    => '5 GB',
							  10   => '10 GB',
							  15   => '15 GB',
							  20   => '20 GB',
							  50   => '50 GB',
							  100  => '100 GB'
							  );
		print_r(json_encode($ThreadNumber));	
	break;
	
	############################
	# Image Type
	############################
	case "QueryImageType":
		$ImageType = array( 
							-1 =>  _('No'),
							0  => 'VHD',
							1  => 'VHDX'
						);
		print_r(json_encode($ImageType));
	break;
	
	############################
	# Convert Type
	############################
	case "QueryConvertType":
		$ConvertType = array( 
							_('Auto') 	=> -1,
							'Any To Any'=> 0,
							'OpenStack' => 1,
							'Xen' 		=> 2,
							'VMware' 	=> 3,
							'Hyper-V' 	=> 4
						);
		print_r(json_encode($ConvertType));
	break;
	
	############################
	# Update Web Port
	############################
	case "UpdateWebPort":
		$ACTION = 'UpdateWebPort';
		
		$REST_DATA = array(
						'Action' 	=> $ACTION,
						'WebPort' 	=> $_REQUEST['WEB_PORT']
					);
					
		Misc::renew_htaccess_port($_REQUEST['WEB_PORT']);
		CurlPostToRemote($REST_DATA);
	break;
	
	############################
	# Transform Account
	############################
	case "TransformAccount":
		$ACTION = 'TransformAccount';
		
		$REST_DATA = array(
						'Action' 	=> $ACTION,
						'AcctUUID' 	=> $_SESSION['admin']['ACCT_UUID'],
						'RegnUUID' 	=> $_SESSION['admin']['REGN_UUID'],
						'Username'	=> $_REQUEST['USER_NAME'],
						'Password'	=> $_REQUEST['PASS_WORD']
					);
		
		CurlPostToRemote($REST_DATA);
		print_r(json_encode(true));
	break;
	
	############################
	# Query Account Setting
	############################
	case "QueryAccountSetting":
		$ACTION = 'QueryAccountSetting';
		
		$REST_DATA = array(
						'Action' 	=> $ACTION,
						'AcctUUID' 	=> $_REQUEST['ACCT_UUID']
					);
		
		$output = CurlPostToRemote($REST_DATA);
		print_r($output);
	break;
		
	############################
	# UPDATE NOTIFICATION TYPE
	############################
	case "UpdateNotificationType":
		$ACTION = 'UpdateNotificationType';
		
		$REST_DATA = array(
						'Action' 		   => $ACTION,
						'AcctUUID' 		   => $_REQUEST['ACCT_UUID'],
						'NotificationType' => $_REQUEST['NOTIFICATION_TYPE'],
						'NotificationTime' => $_REQUEST['NOTIFICATION_TIME']
					);
		$output = CurlPostToRemote($REST_DATA);
		print_r($output);
	break;
	
	############################
	# DELETE SMTP SETTINGS
	############################
	case "DeleteSMTPSettings":
		$ACTION = 'DeleteSMTPSettings';
		
		$REST_DATA = array(
						'Action' 	=> $ACTION,
						'AcctUUID' 	=> $_REQUEST['ACCT_UUID']
					);
		
		$output = CurlPostToRemote($REST_DATA);
		print_r($output);
	break;
	
	############################
	# SAVE SMTP SETTING
	############################
	case "SaveSMTPSettings":
	
	$ACTION = 'SaveSMTPSettings';
	$REST_DATA = array('Action'   => $ACTION,
					   'AcctUUID' => $_REQUEST['ACCT_UUID'],
					   'SMTPHost' => $_REQUEST['SMTP_HOST'],	
					   'SMTPType' => $_REQUEST['SMTP_TYPE'],
					   'SMTPPort' => $_REQUEST['SMTP_PORT'],
					   'SMTPUser' => $_REQUEST['SMTP_USER'],
					   'SMTPPass' => $_REQUEST['SMTP_PASS'],
					   'SMTPFrom' => $_REQUEST['SMTP_FROM'],
					   'SMTPTo'   => $_REQUEST['SMTP_TO']
				);
	$output = CurlPostToRemote($REST_DATA);
	print_r($output);
	break;	
	
	############################
	# ReGen Host Access Code
	############################
	case "ReGenHostAccessCode":
		$ACTION = 'ReGenHostAccessCode';
		
		$REST_DATA = array(
						'Action'   => $ACTION,
						'AcctUUID' => $_REQUEST['ACCT_UUID']
					);
		$output = CurlPostToRemote($REST_DATA);
		print_r($output);	
	break;
	
	############################
	# Update Acct UUID
	############################
	case "UpdateAcctUUID":
		$ACTION = 'UpdateAcctUUID';
		
		$ACCT_UUID = $_REQUEST['ACCT_UUID'];
		$RENEW_DATA = $_REQUEST['RENEW_DATA'];
		
		$output = $MyAdmin -> renew_account_data($ACCT_UUID,$RENEW_DATA);
		print_r($output);		
	break;
	
	############################
	# Reset Database
	############################
	case "ResetDatabase":
		$ACTION    = 'ResetDatabase';

		$REST_DATA = array(
						'Action' 	=> $ACTION,
						'RestType' 	=> $_REQUEST['REST_TYPE']
					);
		$output = CurlPostToRemote($REST_DATA);
		print_r(json_encode($output));
	break;
}