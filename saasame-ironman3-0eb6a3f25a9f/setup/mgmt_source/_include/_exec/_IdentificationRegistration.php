<?php
$GetPostData = file_get_contents("php://input"); 
$PostData = json_decode($GetPostData,true);

/* Secure Key Identification */
if (isset($PostData['EncryptKey']))
{
	$EncryptKey = $PostData['EncryptKey'];
	$KeyDecrypt = Misc_Class::encrypt_decrypt('decrypt', $EncryptKey);
	$DefineTime = time();
	$MinAllowed = $DefineTime - 15;
	$MaxAllowed = $DefineTime + 15;
	if ($MinAllowed < $KeyDecrypt AND $KeyDecrypt < $MaxAllowed)
	{	
		$SelectAction = $PostData['Action'];

		$AcctMgmt = new Account_Class();
		$ServMgmt = new Service_Class();
		$RestMgmt = new Reset_Class();
	}
	else
	{
		header($_SERVER["SERVER_PROTOCOL"]." 401 Unauthorized");
		print_r(json_encode(false));
		exit;	
	}
}
else
{
	header($_SERVER["SERVER_PROTOCOL"]." 401 Unauthorized");
	print_r(json_encode(false));
	exit;
}

if (isset($SelectAction))
{
	switch ($SelectAction)
	{
		############################
		# Transform Account
		############################
		case "TransformAccount":
			$AcctUUID = $PostData['AcctUUID'];
			$RegnUUID = $PostData['RegnUUID'];
			$Username = $PostData['Username'];
			$Password = $PostData['Password'];
					
			print_r($AcctMgmt -> initialize_account($AcctUUID,$RegnUUID,$Username,$Password));
		break;
		
		############################
		# Query Account Setting
		############################
		case "QueryAccountSetting":
			$AcctUUID = $PostData['AcctUUID'];
	
			$output = $AcctMgmt -> query_account($AcctUUID);
			
			if (isset($output['ACCT_DATA'] -> smtp_settings))
			{
				$output['ACCT_DATA'] -> smtp_settings -> SMTPPass = 'password_masking'; 
			}
			
			print_r(json_encode($output));
		break;
		
		############################
		# Update Timezone And Language
		############################
		case "AcctTimeZoneAndLanguage":
			print_r($AcctMgmt -> update_account_data($PostData));			
		break;

		############################
		# ReGen Host Access Code
		############################
		case "ReGenHostAccessCode":
			print_r($AcctMgmt -> update_account_data($PostData));			
		break;
				
		############################
		# Update Notification Type
		############################
		case "UpdateNotificationType":
			
			$ACCT_UUID = $PostData['AcctUUID'];
			
			#ACCOUNT INFORMATION
			$ACCT_INFO = $AcctMgmt -> query_account($ACCT_UUID);			
			
			#CONVERT USER SET TIME WITH TIMEZONE TO UTC TIME
			$USET_SET_ZONE = (isset($ACCT_INFO['ACCT_DATA'] -> account_timezone)) ? $ACCT_INFO['ACCT_DATA'] -> account_timezone : 'UTC';
		
			if (isset($ACCT_INFO['ACCT_DATA'] -> notification_type))
			{
				$NOTIFICATION_TYPE = $ACCT_INFO['ACCT_DATA'] -> notification_type;
				
				for($i=0; $i<count($NOTIFICATION_TYPE); $i++)
				{
					if ($NOTIFICATION_TYPE[$i] == 'DailyReport' OR $NOTIFICATION_TYPE[$i] == 'DailyBackup')
					{
						$ServMgmt -> remove_task($NOTIFICATION_TYPE[$i]);
					}
				}		
			}
			
			if (isset($PostData['NotificationType']))
			{
				if (in_array('DailyReport',$PostData['NotificationType']) OR in_array('DailyBackup',$PostData['NotificationType']))
				{
					foreach ($PostData['NotificationTime'] as $TASK_TYPE => $TASK_TIME)
					{
						$USER_SET_TIME = date("Y-m-d").' '.$TASK_TIME;
						$TASK_DATE = new DateTime($USER_SET_TIME, new DateTimeZone($USET_SET_ZONE));
						$TASK_DATE -> setTimezone(new DateTimeZone('UTC'));
						$TASK_TIME = $TASK_DATE -> format('Y-m-d H:i:s');					
						
						$TASK_TIME = (time() > strtotime($TASK_TIME)) ? date("Y-m-d H:i:s", strtotime($TASK_TIME) + 86400) : $TASK_TIME;						
						
						$TASK_PARARMETER = array(
												'id' 			 => $TASK_TYPE,
												'utc_start_time' => $TASK_TIME,
												'task_interval'	 => 24*60,
												'mgmt_addr'		 => '127.0.0.1',
												'mgmt_port'		 => 80,
												'is_ssl'		 => false,
												'parameters'	 => $ACCT_UUID
											);
						$ServMgmt -> create_task($TASK_PARARMETER);
					}				
				}
			}
			print_r($AcctMgmt -> update_account_data($PostData));
		break;
		
		############################
		# Delete Account SMTP Settings
		############################
		case "DeleteSMTPSettings":
			print_r($AcctMgmt -> update_account_data($PostData));
		break;
		
		############################
		# Save Account SMTP Settings
		############################
		case "SaveSMTPSettings":
			print_r($AcctMgmt -> update_account_data($PostData));
		break;
		
		############################
		# Update Web Port Configuration
		############################
		case "UpdateWebPort":
			$WebPort = $PostData['WebPort'];
			
			Misc_Class::update_port_number($WebPort);		
		break;
			
		############################
		# Reset Database
		############################
		case "ResetDatabase":
			$RestType = $PostData['RestType'];
			print_r($RestMgmt->ResetDatabase($RestType));
		break;		
	}	
}