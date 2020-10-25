<?php
require 'PHPMailer/PHPMailer.php';
require 'PHPMailer/SMTP.php';
require 'PHPMailer/Exception.php';

use PHPMailer\PHPMailer\PHPMailer;

class Mailer_Class
{
	protected $MailMgmt;
	
	###########################
	# Construct Function
	###########################
	public function __construct()
	{
		$this -> MailMgmt = new PHPMailer();
		
		#DEFAULT OPTIONS
		$this -> MailMgmt -> SMTPDebug = 0;	
		$this -> MailMgmt -> isSMTP();    		
		$this -> MailMgmt -> isHTML(true);  
		$this -> MailMgmt -> SMTPAuth = true;
		$this -> MailMgmt -> SMTPOptions = array(
											'ssl' => array(
													'verify_peer' => false,
													'verify_peer_name' => false,
													'allow_self_signed' => true
												)
											);
	}
	
	###########################
	# SEND MAIL
	###########################
	public function SendMail($CONTENTS)
	{
		$SMTP_HOST 			= $CONTENTS['SMTPHost'];
		$SMTP_USER 			= $CONTENTS['SMTPUser'];
		$SMTP_PASS 			= $CONTENTS['SMTPPass'];
		$SMTP_TYPE 			= $CONTENTS['SMTPType'];
		$SMTP_PORT 			= $CONTENTS['SMTPPort'];
		$SMTP_FROM 			= $CONTENTS['SMTPFrom'];
		$SMTP_TO   			= $CONTENTS['SMTPTo'];
		$MAIL_SUBJECT   	= $CONTENTS['MailSubject'];
		$PREPARE_MAIL_BODY	= $CONTENTS['MailBody'];
		$FILE_ATTACHMENT	= $CONTENTS['AddAttachment'];
	
		$this -> MailMgmt -> Host 		= $SMTP_HOST;
		$this -> MailMgmt -> Username 	= $SMTP_USER;
		$this -> MailMgmt -> Password 	= $SMTP_PASS;
		$this -> MailMgmt -> SMTPSecure = $SMTP_TYPE;
		$this -> MailMgmt -> Port 		= $SMTP_PORT;
		$this -> MailMgmt -> setFrom($SMTP_FROM, $SMTP_FROM);
		
		$MAIL_TO = explode(',',str_replace(';',',',$SMTP_TO));
		foreach ($MAIL_TO as $KEY => $ADDRESS)
		{
			$this -> MailMgmt -> AddAddress($ADDRESS, $ADDRESS);
			//$this -> MailMgmt -> AddBCC($ADDRESS, $ADDRESS);
		}
		
		$this -> MailMgmt -> Subject 	= $MAIL_SUBJECT;
		$this -> MailMgmt -> Body    	= $PREPARE_MAIL_BODY;
		
		if ($FILE_ATTACHMENT != null)
		{
			$this -> MailMgmt -> AddAttachment($FILE_ATTACHMENT,basename($FILE_ATTACHMENT));
		}
		
		if(!$this -> MailMgmt -> send())
		{
			Misc_Class::function_debug('_mgmt',__FUNCTION__,$this -> MailMgmt -> ErrorInfo);
			
			return array('status' => false, 'reason' => 'SMTP Error: ' . $this -> MailMgmt -> ErrorInfo);
		}
		else
		{
			return array('status' => true, 'reason' => '');
		}
	}
		
	###########################
	# EMAIL HTML BACK
	###########################
	private function email_html_backup($FOLDERNAME,$FILENAME,$DETAIL_INFO)
	{
		try
		{
			$BACKUP_PATH = $_SERVER['DOCUMENT_ROOT'].'/_include/_debug/'.$FOLDERNAME.'/notification';
			$BACKUP_FILE = $_SERVER['DOCUMENT_ROOT'].'/_include/_debug/'.$FOLDERNAME.'/notification/'.$FILENAME.'.html';
				
			#CHECK AND CREATE FOLDER EXISTS
			if(!file_exists($BACKUP_PATH))
			{
				mkdir($BACKUP_PATH);
			}				
				
			#CHECK AND CREATE FILE EXISTS
			if(!file_exists($BACKUP_FILE))
			{
				$fp = fopen($BACKUP_FILE,'w');
				if(!$fp)
				{
					throw new Exception('File open failed.');
				}
				else
				{
					fclose($fp);
				}
			}
			
			file_put_contents($BACKUP_FILE,$DETAIL_INFO);				
		}
		catch (Throwable $e)
		{
			return false;
		}		
	}
	
	

	###########################
	# HTML HEADER
	###########################
	private function html_header($WIDTH)
	{
		$HTML_HEADER = '<!DOCTYPE html PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN"
						"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
						<html><head><meta http-equiv="Content-Type" content="text/html; charset=utf-8"/>
						<title>SaaSaMe Email Notification</title>
						</head>
						<STYLE TYPE="text/css">
							TD{font-family: Arial; font-size: 10pt; color: #60504F;}
							.lineheight{line-height:1.8;}
							u { text-decoration: none; border-bottom:1 solid; }
							.strike_line{
								text-decoration: line-through;
								text-decoration-color: #e60000;	
							}
						</STYLE>
						<body>
						<table border="0" cellpadding="0" cellspacing="0" width="'.$WIDTH.'"><tr>
						<table border="0" cellpadding="0" cellspacing="0" width="'.$WIDTH.'"><tr>
						<td bgcolor="#CCCCCC" valign="TOP">';
		
		return $HTML_HEADER;
	}
	
	###########################
	# HTML FOOTER
	###########################
	private function html_footer()
	{
		$HTML_FOOTER = '</td></tr></table></tr></table></body></html>';
		
		return $HTML_FOOTER;
	}
		
	###########################
	# SEND VERIFY EMAIL
	###########################
	public function send_verify_email($POST_DATA)
	{
		#DEFINE SENDMAIL INFORMATION
		$SENDMAIL_INFO = (array)$POST_DATA;
		
		$ACCT_LANG = $SENDMAIL_INFO['Language'];
		$MailSubject = $this -> translate('SMTP Setting Verification',$ACCT_LANG);
		$MailMessage = $this -> translate('This is sending test message from SaaSaMe Transport Server.',$ACCT_LANG);
			
		#Load HTML Header
		$VERIFY_MAIL_BODY = $this -> html_header(800);
		
		$VERIFY_MAIL_BODY .= '<table border="0" cellpadding="5" cellspacing="0" width="800">';
		$VERIFY_MAIL_BODY .= '<tr>';
		$VERIFY_MAIL_BODY .= '<td bgcolor="#FFFFFF" valign="TOP" width="800">'.$MailMessage.'</td>';
		$VERIFY_MAIL_BODY .= '</tr>';
		$VERIFY_MAIL_BODY .= '</table>';
		
		#Load HTML Footer
		$VERIFY_MAIL_BODY .= $this -> html_footer();
	
		#DEFINE SENDMAIL INFORMATION
		$SENDMAIL_INFO['MailSubject'] = $MailSubject;
		$SENDMAIL_INFO['MailBody'] = $VERIFY_MAIL_BODY;
		$SENDMAIL_INFO['AddAttachment'] = null;

		//return $SENDMAIL_INFO; #FOR DEBUG
		return $this -> SendMail($SENDMAIL_INFO);	
	}
		
	###########################
	# REPLICATION NOTIFICATION
	###########################
	public function gen_replication_notification($ACCT_UUID,$REPL_UUID)
	{
		$AcctMgmt = new Account_Class();
				
		$QUERY_SMTP_DATA = $AcctMgmt -> query_account($ACCT_UUID)['ACCT_DATA'];
		$ACCT_LANG = $QUERY_SMTP_DATA -> account_language;
			
		if (isset($QUERY_SMTP_DATA -> notification_type) AND in_array('Replication', $QUERY_SMTP_DATA -> notification_type) AND isset($QUERY_SMTP_DATA -> smtp_settings) AND $QUERY_SMTP_DATA -> smtp_settings != '')
		{
			$ServiceMgmt = new Service_Class();
			$REPLICA_INFO = $ServiceMgmt -> query_prepare_workload($REPL_UUID);

			#Get User Define TimeZone
			if (isset($REPLICA_INFO -> JobConfig -> TimeZone))
			{
				$TimeZone = $REPLICA_INFO -> JobConfig -> TimeZone;
			}
			elseif (isset($QUERY_SMTP_DATA -> account_timezone))
			{
				$TimeZone = $QUERY_SMTP_DATA -> account_timezone;
			}
			else
			{
				$TimeZone = 'UTC';
			}
					
			#Load HTML Header
			$PREPARE_MAIL_BODY = $this -> html_header(800);
			
			#Transport Information
			$PREPARE_MAIL_BODY .= '<table border="0" cellpadding="5" cellspacing="1" width="800">';
			$PREPARE_MAIL_BODY .= '<tr>';
			$PREPARE_MAIL_BODY .= '<td bgcolor="#908d81" valign="TOP" width="800" colspan="3"><font color="#f4f3ee">'.$this -> translate('Transport Information',$ACCT_LANG).'</font></td>';
			$PREPARE_MAIL_BODY .= '</tr>';
			
			$source_hostname = $REPLICA_INFO -> PairTransport['source_hostname'].' ['.$this -> translate('Source',$ACCT_LANG).']';
			$target_hostname = $REPLICA_INFO -> PairTransport['target_hostname'].' ['.$this -> translate('Target',$ACCT_LANG).']';
			$source_address  = implode(',',$REPLICA_INFO -> PairTransport['source_address']);
			$target_address  = implode(',',$REPLICA_INFO -> PairTransport['target_address']);
			$source_type     = $REPLICA_INFO -> PairTransport['source_type'];
			$target_type     = $REPLICA_INFO -> PairTransport['target_type'];
					
			$PREPARE_MAIL_BODY .= '<tr>';
			$PREPARE_MAIL_BODY .= '<td bgcolor="#F5F5F5" valign="TOP" width="180">'.$this -> translate('Hostname',$ACCT_LANG).'</td>';
			$PREPARE_MAIL_BODY .= '<td bgcolor="#F5F5F5" valign="TOP" width="310">'.$source_hostname.'</td>';
			$PREPARE_MAIL_BODY .= '<td bgcolor="#F5F5F5" valign="TOP" width="310">'.$target_hostname.'</td>';
			$PREPARE_MAIL_BODY .= '</tr>';
			
			$PREPARE_MAIL_BODY .= '<tr>';
			$PREPARE_MAIL_BODY .= '<td bgcolor="#FFFFFF" valign="TOP" width="180">'.$this -> translate('Address',$ACCT_LANG).'</td>';
			$PREPARE_MAIL_BODY .= '<td bgcolor="#FFFFFF" valign="TOP" width="310">'.$source_address.'</td>';
			$PREPARE_MAIL_BODY .= '<td bgcolor="#FFFFFF" valign="TOP" width="310">'.$target_address.'</td>';
			$PREPARE_MAIL_BODY .= '</tr>';
			
			$PREPARE_MAIL_BODY .= '<tr>';
			$PREPARE_MAIL_BODY .= '<td bgcolor="#F5F5F5" valign="TOP" width="180">'.$this -> translate('Type',$ACCT_LANG).'</td>';
			$PREPARE_MAIL_BODY .= '<td bgcolor="#F5F5F5" valign="TOP" width="310">'.$source_type.'</td>';
			$PREPARE_MAIL_BODY .= '<td bgcolor="#F5F5F5" valign="TOP" width="310">'.$target_type.'</td>';
			$PREPARE_MAIL_BODY .= '</tr>';
			
			$PREPARE_MAIL_BODY .= '</table>';
			#Transport Information
			
			
			#Host Information
			$PREPARE_MAIL_BODY .= '<div><div style="display:block; background-color:#FFFFFF;">&nbsp;</div></div>';
			$PREPARE_MAIL_BODY .= '<table border="0" cellpadding="5" cellspacing="1" width="800">';
			$PREPARE_MAIL_BODY .= '<tr>';
			$PREPARE_MAIL_BODY .= '<td bgcolor="#79addc" valign="TOP" width="800" colspan="2"><font color="#f4f3ee">'.$this -> translate('Host Information',$ACCT_LANG).'</font></td>';
			$PREPARE_MAIL_BODY .= '</tr>';
			
			$PREPARE_MAIL_BODY .= '<tr>';
			$PREPARE_MAIL_BODY .= '<td bgcolor="#F5F5F5" valign="TOP" width="180">'.$this -> translate('Hostname',$ACCT_LANG).'</td>';
			$PREPARE_MAIL_BODY .= '<td bgcolor="#F5F5F5" valign="TOP" width="620">'.$REPLICA_INFO -> Hostname.' ('.$REPLICA_INFO -> manufacturer.')</td>';
			$PREPARE_MAIL_BODY .= '</tr>';
			
			$PREPARE_MAIL_BODY .= '<tr>';
			$PREPARE_MAIL_BODY .= '<td bgcolor="#FFFFFF" valign="TOP" width="180">'.$this -> translate('Packer Type',$ACCT_LANG).'</td>';
			$PREPARE_MAIL_BODY .= '<td bgcolor="#FFFFFF" valign="TOP" width="620">'.$REPLICA_INFO -> HostType.'</td>';
			$PREPARE_MAIL_BODY .= '</tr>';	
			
			$PREPARE_MAIL_BODY .= '<tr>';
			$PREPARE_MAIL_BODY .= '<td bgcolor="#F5F5F5" valign="TOP" width="180">'.$this -> translate('OS Type',$ACCT_LANG).'</td>';
			$PREPARE_MAIL_BODY .= '<td bgcolor="#F5F5F5" valign="TOP" width="620">'.$REPLICA_INFO -> OSName.'</td>';
			$PREPARE_MAIL_BODY .= '</tr>';
			
			$PREPARE_MAIL_BODY .= '<tr>';
			$PREPARE_MAIL_BODY .= '<td bgcolor="#FFFFFF" valign="TOP" width="180">'.$this -> translate('Host Address',$ACCT_LANG).'</td>';
			$PREPARE_MAIL_BODY .= '<td bgcolor="#FFFFFF" valign="TOP" width="620">'.$REPLICA_INFO -> Address.'</td>';
			$PREPARE_MAIL_BODY .= '</tr>';
			
			$DISK_INFO = $REPLICA_INFO -> Disk;

			for ($i=0; $i<count($DISK_INFO); $i++)
			{
				if ($i % 2 == 0)
				{
					$BG_COLOR = 'bgcolor="#F5F5F5"';
				}
				else
				{
					$BG_COLOR = 'bgcolor="#FFFFFF"';
				}
				
				if ($DISK_INFO[$i]['is_boot'] == TRUE)
				{
					$IS_BOOT = '<i class="fa fa-check-circle-o" aria-hidden="true"></i> ';
				}
				else
				{
					$IS_BOOT = '<i class="fa fa-circle-o" aria-hidden="true"></i> ';
				}
				
				if ($DISK_INFO[$i]['is_skip'] == TRUE)
				{
					$STRIKE = 'strike_line';
				}
				else
				{
					$STRIKE = '';
				}
				
				$DISK_NAME = $DISK_INFO[$i]['disk_name'];
				
				$DISK_SIZE = ($DISK_INFO[$i]['size']) / 1024 / 1024 / 1024;
				
				$PREPARE_MAIL_BODY .= '<tr>';
				$PREPARE_MAIL_BODY .= '<td '.$BG_COLOR.' valign="TOP" width="180">'.$this -> translate('Disk',$ACCT_LANG).' '.$i.'</td>';
				$PREPARE_MAIL_BODY .= '<td '.$BG_COLOR.' valign="TOP" width="620" class="'.$STRIKE.'">'.$IS_BOOT.$DISK_NAME.' ('.round($DISK_SIZE).'GB)</td>';
				$PREPARE_MAIL_BODY .= '</tr>';
			}
			$PREPARE_MAIL_BODY .= '</table>';
			#Host Information
			
			#Cloud Volume
			$PREPARE_MAIL_BODY .= '<div><div style="display:block; background-color:#FFFFFF;">&nbsp;</div></div>';
			$PREPARE_MAIL_BODY .= '<table border="0" cellpadding="5" cellspacing="1" width="800">';
			$PREPARE_MAIL_BODY .= '<tr>';
			$PREPARE_MAIL_BODY .= '<td bgcolor="#f97171" valign="TOP" width="800" colspan="2"><font color="#f4f3ee">'.$this -> translate('Cloud Disk Information',$ACCT_LANG).'</font></td>';
			$PREPARE_MAIL_BODY .= '</tr>';
			
			$x=0;
			foreach ($REPLICA_INFO -> CloudVolume as $CLOUD_DISK_NAME => $CLOUD_DISK_SIZE)
			{
				if ($x % 2 == 0)
				{
					$CBG_COLOR = 'bgcolor="#F5F5F5"';
				}
				else
				{
					$CBG_COLOR = 'bgcolor="#FFFFFF"';
				}
							
				$PREPARE_MAIL_BODY .= '<tr>';
				$PREPARE_MAIL_BODY .= '<td '.$CBG_COLOR.' valign="TOP" width="180">'.$this -> translate('Disk',$ACCT_LANG).' '.$x.'</td>';
				$PREPARE_MAIL_BODY .= '<td '.$CBG_COLOR.' valign="TOP" width="620">'.$CLOUD_DISK_NAME.' ('.$CLOUD_DISK_SIZE.'GB)</td>';
				$PREPARE_MAIL_BODY .= '</tr>';
				$x++;
			}
			
			$PREPARE_MAIL_BODY .= '</table>';
			#Cloud Volume
			
			#Job Configuration
			$PREPARE_MAIL_BODY .= '<div><div style="display:block; background-color:#FFFFFF;">&nbsp;</div></div>';
			$PREPARE_MAIL_BODY .= '<table border="0" cellpadding="5" cellspacing="1" width="800">';
			$PREPARE_MAIL_BODY .= '<tr>';
			$PREPARE_MAIL_BODY .= '<td bgcolor="#66beb2" valign="TOP" width="800" colspan="2"><font color="#f4f3ee">'.$this -> translate('Replication Workload Configuration',$ACCT_LANG).'</font></td>';
			$PREPARE_MAIL_BODY .= '</tr>';
			
			if ($REPLICA_INFO -> JobConfig -> triggers[0] -> start == '')
			{
				$START_TIME = 'Run at begin.';
			}
			else
			{
				$START_TIME = $REPLICA_INFO -> JobConfig -> triggers[0] -> start;
			}
			
			$PREPARE_MAIL_BODY .= '<tr>';
			$PREPARE_MAIL_BODY .= '<td bgcolor="#F5F5F5" valign="TOP" width="180">'.$this -> translate('Job ID',$ACCT_LANG).'</td>';
			$PREPARE_MAIL_BODY .= '<td bgcolor="#F5F5F5" valign="TOP" width="620">'.$REPLICA_INFO -> JobUUID.'</td>';
			$PREPARE_MAIL_BODY .= '</tr>';
			
			$PREPARE_MAIL_BODY .= '<tr>';
			$PREPARE_MAIL_BODY .= '<td bgcolor="#FFFFFF" valign="TOP" width="180">'.$this -> translate('Job Start at',$ACCT_LANG).'</td>';
			$PREPARE_MAIL_BODY .= '<td bgcolor="#FFFFFF" valign="TOP" width="620">'.$START_TIME.'</td>';
			$PREPARE_MAIL_BODY .= '</tr>';
			
			$PREPARE_MAIL_BODY .= '<tr>';
			$PREPARE_MAIL_BODY .= '<td bgcolor="#F5F5F5" valign="TOP" width="180">'.$this -> translate('Snapshot Number',$ACCT_LANG).'</td>';
			$PREPARE_MAIL_BODY .= '<td bgcolor="#F5F5F5" valign="TOP" width="620">'.$REPLICA_INFO -> JobConfig -> snapshot_rotation.'</td>';
			$PREPARE_MAIL_BODY .= '</tr>';
			
			$PREPARE_MAIL_BODY .= '<tr>';
			$PREPARE_MAIL_BODY .= '<td bgcolor="#FFFFFF" valign="TOP" width="180">'.$this -> translate('Packer Thread',$ACCT_LANG).'</td>';
			$PREPARE_MAIL_BODY .= '<td bgcolor="#FFFFFF" valign="TOP" width="620">'.$REPLICA_INFO -> JobConfig -> worker_thread_number.'</td>';
			$PREPARE_MAIL_BODY .= '</tr>';
			
			$PREPARE_MAIL_BODY .= '<tr>';
			$PREPARE_MAIL_BODY .= '<td bgcolor="#F5F5F5" valign="TOP"  width="180">'.$this -> translate('Loader Thread',$ACCT_LANG).'</td>';
			$PREPARE_MAIL_BODY .= '<td bgcolor="#F5F5F5" valign="TOP" width="620">'.$REPLICA_INFO -> JobConfig -> loader_thread_number.'</td>';
			$PREPARE_MAIL_BODY .= '</tr>';
			
			$PREPARE_MAIL_BODY .= '<tr>';
			$PREPARE_MAIL_BODY .= '<td bgcolor="#FFFFFF" valign="TOP" width="180">'.$this -> translate('Loader Trigger',$ACCT_LANG).' %</td>';
			$PREPARE_MAIL_BODY .= '<td bgcolor="#FFFFFF" valign="TOP" width="620">'.$REPLICA_INFO -> JobConfig -> loader_trigger_percentage.'</td>';
			$PREPARE_MAIL_BODY .= '</tr>';	
			
			if ($REPLICA_INFO -> JobConfig -> export_path != '')
			{
				$PREPARE_MAIL_BODY .= '<tr>';
				$PREPARE_MAIL_BODY .= '<td bgcolor="#F5F5F5" valign="TOP" width="180">'.$this -> translate('Export Path',$ACCT_LANG).'</td>';
				$PREPARE_MAIL_BODY .= '<td bgcolor="#F5F5F5" valign="TOP" width="620">'.$REPLICA_INFO -> JobConfig -> export_path.'</td>';
				$PREPARE_MAIL_BODY .= '</tr>';
				
				switch ($REPLICA_INFO -> JobConfig -> export_type)
				{
					case 0:
						$EXPORT_TYPE = 'VHD';
					break;
					
					case 1:
						$EXPORT_TYPE = 'VHDX';
					break;
				}			
				$PREPARE_MAIL_BODY .= '<tr>';
				$PREPARE_MAIL_BODY .= '<td bgcolor="#FFFFFF" valign="TOP" width="180">'.$this -> translate('Export Type',$ACCT_LANG).'</td>';
				$PREPARE_MAIL_BODY .= '<td bgcolor="#FFFFFF" valign="TOP" width="620">'.$EXPORT_TYPE.'</td>';
				$PREPARE_MAIL_BODY .= '</tr>';			
			}
						
			$PREPARE_MAIL_BODY .= '</table>';		
			#Job Configuration		
		
			#Load HTML Footer
			$PREPARE_MAIL_BODY .= $this -> html_footer();
	
			#HTML Backup						
			$LOG_LOCATION = $REPLICA_INFO -> LogLocation;
			$HTML_FILE_NAME = 'Replication-'.$REPLICA_INFO -> Hostname.'-'.time();			
			$this -> email_html_backup($LOG_LOCATION,$HTML_FILE_NAME,$PREPARE_MAIL_BODY);
				
			#GEN EMAIL SUBJECT
			date_default_timezone_set($TimeZone);
			$DateTime = new DateTime('now', new DateTimeZone($REPLICA_INFO -> JobConfig -> timezone));
			$ABBREVIATION = $DateTime -> format('T');			
			$NOTIFICATION_TIME = date('Y-m-d H:i:s', time());			
			$PREPARE_MAIL_SUBJECT = $this -> translate('New Replication Created',$ACCT_LANG).' ('.$NOTIFICATION_TIME.' '.$ABBREVIATION.')';			
			
			#DEFINT SENDMAIL INFORMATION
			$SENDMAIL_INFO = (array)$QUERY_SMTP_DATA -> smtp_settings;
			$SENDMAIL_INFO['MailSubject'] = $PREPARE_MAIL_SUBJECT;
			$SENDMAIL_INFO['MailBody'] = $PREPARE_MAIL_BODY;
			$SENDMAIL_INFO['AddAttachment'] = null;

			//return $SENDMAIL_INFO; #FOR DEBUG
			$this -> SendMail($SENDMAIL_INFO);
		}
	}
	
	###########################
	# RECOVERY NOTIFICATION
	###########################
	public function gen_recovery_notification($ACCT_UUID,$SERV_UUID)
	{
		$AcctMgmt = new Account_Class();

		$QUERY_SMTP_DATA = $AcctMgmt -> query_account($ACCT_UUID)['ACCT_DATA'];
		$ACCT_LANG = $QUERY_SMTP_DATA -> account_language;
	
		if (isset($QUERY_SMTP_DATA -> notification_type) AND in_array('Recovery', $QUERY_SMTP_DATA -> notification_type) AND isset($QUERY_SMTP_DATA -> smtp_settings) AND $QUERY_SMTP_DATA -> smtp_settings != '')
		{
			$ServiceMgmt = new Service_Class();			
			$RECOVERY_INFO = $ServiceMgmt -> query_recovery_workload($SERV_UUID);
		
			#Get User Define TimeZone
			if (isset($RECOVERY_INFO -> TimeZone))
			{
				$TimeZone = $RECOVERY_INFO -> TimeZone;
			}
			elseif (isset($QUERY_SMTP_DATA -> account_timezone))
			{
				$TimeZone = $QUERY_SMTP_DATA -> account_timezone;
			}
			else
			{
				$TimeZone = 'UTC';
			}
						
			#Load HTML Header
			$RECOVERY_MAIL_BODY = $this -> html_header(800);
						
			#Host Information
			$RECOVERY_MAIL_BODY .= '<table border="0" cellpadding="5" cellspacing="1" width="800">';
			$RECOVERY_MAIL_BODY .= '<tr>';
			$RECOVERY_MAIL_BODY .= '<td bgcolor="#87b6a7" valign="TOP" width="800" colspan="2"><font color="#f4f3ee">'.$this -> translate('Host Information',$ACCT_LANG).'</font></td>';
			$RECOVERY_MAIL_BODY .= '</tr>';
						
			$RECOVERY_MAIL_BODY .= '<tr>';
			$RECOVERY_MAIL_BODY .= '<td bgcolor="#F5F5F5" valign="TOP" width="170">'.$this -> translate('Hostname',$ACCT_LANG).'</td>';
			$RECOVERY_MAIL_BODY .= '<td bgcolor="#F5F5F5" valign="TOP" width="630">'.$RECOVERY_INFO -> Hostname.'</td>';
			$RECOVERY_MAIL_BODY .= '</tr>';
				
			$RECOVERY_MAIL_BODY .= '<tr>';
			$RECOVERY_MAIL_BODY .= '<td bgcolor="#FFFFFF" valign="TOP" width="170">'.$this -> translate('Recover Type',$ACCT_LANG).'</td>';
			$RECOVERY_MAIL_BODY .= '<td bgcolor="#FFFFFF" valign="TOP" width="630">'.$RECOVERY_INFO -> RecoveryType.'</td>';
			$RECOVERY_MAIL_BODY .= '</tr>';
			
			$RECOVERY_MAIL_BODY .= '<tr>';
			$RECOVERY_MAIL_BODY .= '<td bgcolor="#F5F5F5" valign="TOP" width="170">'.$this -> translate('OS Name',$ACCT_LANG).'</td>';
			$RECOVERY_MAIL_BODY .= '<td bgcolor="#F5F5F5" valign="TOP" width="630">'.$RECOVERY_INFO -> OSName.'</td>';
			$RECOVERY_MAIL_BODY .= '</tr>';		
			
			$RECOVERY_MAIL_BODY .= '<tr>';
			$RECOVERY_MAIL_BODY .= '<td bgcolor="#FFFFFF" valign="TOP" width="170">'.$this -> translate('OS Type',$ACCT_LANG).'</td>';
			$RECOVERY_MAIL_BODY .= '<td bgcolor="#FFFFFF" valign="TOP" width="630">'.$RECOVERY_INFO -> OSType.'</td>';
			$RECOVERY_MAIL_BODY .= '</tr>';
			
			$DISK_INFO = $RECOVERY_INFO -> Disk;
			for ($i=0; $i<count($DISK_INFO); $i++)
			{
				if ($i % 2 == 0)
				{
					$IBG_COLOR = 'bgcolor="#F5F5F5"';
				}
				else
				{
					$IBG_COLOR = 'bgcolor="#FFFFFF"';
				}	
				
				$DISK_SIZE = ($DISK_INFO[$i] / 1024 /1024);
				
				$RECOVERY_MAIL_BODY .= '<tr>';
				$RECOVERY_MAIL_BODY .= '<td '.$IBG_COLOR.' valign="TOP" width="170">'.$this -> translate('Disk',$ACCT_LANG).' '.$i.'</td>';
				$RECOVERY_MAIL_BODY .= '<td '.$IBG_COLOR.' valign="TOP" width="630">'.round($DISK_SIZE).'GB</td>';
				$RECOVERY_MAIL_BODY .= '</tr>';
			}
			
			$RECOVERY_MAIL_BODY .= '</table>';
			
			if ($RECOVERY_INFO -> JobTypeX == FALSE)
			{
				$RECOVERY_MAIL_BODY .= '<div><div style="display:block; background-color:#FFFFFF;">&nbsp;</div></div>';
				$RECOVERY_MAIL_BODY .= '<table border="0" cellpadding="5" cellspacing="1" width="800">';
				$RECOVERY_MAIL_BODY .= '<tr>';
				$RECOVERY_MAIL_BODY .= '<td bgcolor="#f79f79" valign="TOP" width="800" colspan="2"><font color="#f4f3ee">'.$this -> translate('Instance Information',$ACCT_LANG).'</font></td>';
				$RECOVERY_MAIL_BODY .= '</tr>';
				
				$RECOVERY_MAIL_BODY .= '<tr>';
				$RECOVERY_MAIL_BODY .= '<td bgcolor="#F5F5F5" valign="TOP" width="170">'.$this -> translate('Instance Hostname',$ACCT_LANG).'</td>';
				$RECOVERY_MAIL_BODY .= '<td bgcolor="#F5F5F5" valign="TOP" width="630">'.$RECOVERY_INFO -> InstanceHostname.'</td>';
				$RECOVERY_MAIL_BODY .= '</tr>';
					
				$RECOVERY_MAIL_BODY .= '<tr>';
				$RECOVERY_MAIL_BODY .= '<td bgcolor="#FFFFFF" valign="TOP" width="170">'.$this -> translate('Flavor',$ACCT_LANG).'</td>';
				if ($RECOVERY_INFO -> Flavor['name'] == 'processing..')
				{
					$RECOVERY_MAIL_BODY .= '<td bgcolor="#FFFFFF" valign="TOP" width="630">'.$RECOVERY_INFO -> Flavor['name'].'</td>';
				}
				else
				{
					$RECOVERY_MAIL_BODY .= '<td bgcolor="#FFFFFF" valign="TOP" width="630">'.$RECOVERY_INFO -> Flavor['name'].' / '.$RECOVERY_INFO -> Flavor['vcpus'].'Cores / '.$RECOVERY_INFO -> Flavor['ram'].'MB</td>';	
				}
				$RECOVERY_MAIL_BODY .= '</tr>';
				
				$RECOVERY_MAIL_BODY .= '<tr>';
				$RECOVERY_MAIL_BODY .= '<td bgcolor="#F5F5F5" valign="TOP" width="170">'.$this -> translate('Security Groups',$ACCT_LANG).'</td>';
				$RECOVERY_MAIL_BODY .= '<td bgcolor="#F5F5F5" valign="TOP" width="630">'.$RECOVERY_INFO -> SecurityGroups.'</td>';
				$RECOVERY_MAIL_BODY .= '</tr>';
				
				$RECOVERY_MAIL_BODY .= '<tr>';
				$RECOVERY_MAIL_BODY .= '<td bgcolor="#FFFFFF" valign="TOP" width="170">'.$this -> translate('Hypervisor / Region',$ACCT_LANG).'</td>';
				$RECOVERY_MAIL_BODY .= '<td bgcolor="#FFFFFF" valign="TOP" width="630">'.$RECOVERY_INFO -> HypervisorHostname.'</td>';
				$RECOVERY_MAIL_BODY .= '</tr>';
				
				$NIC_INFO = $RECOVERY_INFO -> NicInfo;
				for ($x=0; $x<count($NIC_INFO); $x++)
				{
					if ($i % 2 == 0)
					{
						$XBG_COLOR = 'bgcolor="#FFFFFF"';
					}
					else
					{
						$XBG_COLOR = 'bgcolor="#F5F5F5"';
					}	
					
					$NIC_ADDR = $NIC_INFO[$x]['addr'];
					$NIC_TYPE = $NIC_INFO[$x]['type'];
					$NIC_MAC  = $NIC_INFO[$x]['mac'];		
					
					$RECOVERY_MAIL_BODY .= '<tr>';
					$RECOVERY_MAIL_BODY .= '<td '.$XBG_COLOR.' valign="TOP" width="170">'.$this -> translate('NIC',$ACCT_LANG).' '.$x.'</td>';
					if ($NIC_ADDR == 'processing..')
					{
						$RECOVERY_MAIL_BODY .= '<td '.$XBG_COLOR.' valign="TOP" width="630">'.$NIC_ADDR.'</td>';
					}
					else
					{
						$RECOVERY_MAIL_BODY .= '<td '.$XBG_COLOR.' valign="TOP" width="630">'.$NIC_ADDR.' / '.$NIC_MAC.' / '.$NIC_TYPE.'</td>';
					}
					$RECOVERY_MAIL_BODY .= '</tr>';
				}
				
				$RECOVERY_MAIL_BODY .= '<tr>';
				$RECOVERY_MAIL_BODY .= '<td bgcolor="#FFFFFF" valign="TOP" width="170">'.$this -> translate('Job ID',$ACCT_LANG).'</td>';
				$RECOVERY_MAIL_BODY .= '<td bgcolor="#FFFFFF" valign="TOP" width="630">'.$RECOVERY_INFO -> JobUUID.'</td>';
				$RECOVERY_MAIL_BODY .= '</tr>';
				
				$RECOVERY_MAIL_BODY .= '<tr>';
				$RECOVERY_MAIL_BODY .= '<td bgcolor="#F5F5F5" valign="TOP" width="170">'.$this -> translate('Recover Cloud Type',$ACCT_LANG).'</td>';
				$RECOVERY_MAIL_BODY .= '<td bgcolor="#F5F5F5" valign="TOP" width="630">'.$RECOVERY_INFO -> CloudType.'</td>';
				$RECOVERY_MAIL_BODY .= '</tr>';
				
				if ($RECOVERY_INFO -> CloudType == 'Aliyun')
				{
					$RECOVERY_MAIL_BODY .= '<tr>';
					$RECOVERY_MAIL_BODY .= '<td bgcolor="#FFFFFF" valign="TOP" width="170">'.$this -> translate('Administrator Password',$ACCT_LANG).'</td>';
					$RECOVERY_MAIL_BODY .= '<td bgcolor="#FFFFFF" valign="TOP" width="630">'.$RECOVERY_INFO -> AdminPassword.'</td>';
					$RECOVERY_MAIL_BODY .= '</tr>';
				}
				
				$RECOVERY_MAIL_BODY .= '</table>';
			}
			
			#Load HTML Footer
			$RECOVERY_MAIL_BODY .= $this -> html_footer();

			#HTML Backup
			$LOG_LOCATION = $RECOVERY_INFO -> LogLocation;
			$HTML_FILE_NAME = 'Recovery-'.$RECOVERY_INFO -> Hostname.'-'.time();
			$this -> email_html_backup($LOG_LOCATION,$HTML_FILE_NAME,$RECOVERY_MAIL_BODY);
			
			#GEN EMAIL SUBJECT
			date_default_timezone_set($TimeZone);
			$DateTime = new DateTime('now', new DateTimeZone($RECOVERY_INFO -> TimeZone));
			$ABBREVIATION = $DateTime -> format('T');			
			$NOTIFICATION_TIME = date('Y-m-d H:i:s', time());			
			$RECOVERY_MAIL_SUBJECT = $this -> translate('The Recovery Instance is Ready',$ACCT_LANG).' ('.$NOTIFICATION_TIME.' '.$ABBREVIATION.')';	
			
			#DEFINT SENDMAIL INFORMATION
			$SENDMAIL_INFO = (array)$QUERY_SMTP_DATA -> smtp_settings;
			$SENDMAIL_INFO['MailSubject'] = $RECOVERY_MAIL_SUBJECT;
			$SENDMAIL_INFO['MailBody'] = $RECOVERY_MAIL_BODY;
			$SENDMAIL_INFO['AddAttachment'] = null;

			//return $SENDMAIL_INFO; #FOR DEBUG
			$this -> SendMail($SENDMAIL_INFO);		
		}
	}
	
	###########################
	# CHANGE TIME FROM UTC
	###########################
	private function change_time_from_UTC($TIME,$TIMEZONE)
	{
		$changetime = new DateTime($TIME, new DateTimeZone('UTC'));
		$changetime->setTimezone(new DateTimeZone($TIMEZONE));
		return $changetime->format('Y-m-d H:i:s');
	}
	
	###########################
	# DAILY REPORT
	###########################
	public function get_daily_report($ACCT_UUID)
	{
		$AcctMgmt = new Account_Class();
		
		$QUERY_SMTP_DATA = $AcctMgmt -> query_account($ACCT_UUID)['ACCT_DATA'];
		$ACCT_LANG = $QUERY_SMTP_DATA -> account_language;	
	
		$ReplMgmt = new Replica_Class();		
		$REPORT_REPLICA = $ReplMgmt -> query_replica_report($ACCT_UUID,24);

		#Get User Define TimeZone
		if (isset($QUERY_SMTP_DATA -> account_timezone))
		{
			$TimeZone = $QUERY_SMTP_DATA -> account_timezone;
		}
		elseif (isset(json_decode($REPORT_REPLICA[0]['JOBS_JSON']) -> timezone))
		{
			$TimeZone = json_decode($REPORT_REPLICA[0]['JOBS_JSON']) -> timezone;
		}
		else
		{
			$TimeZone = 'UTC';
		}

		if (isset($QUERY_SMTP_DATA -> notification_type) AND in_array('DailyReport', $QUERY_SMTP_DATA -> notification_type) AND isset($QUERY_SMTP_DATA -> smtp_settings) AND $QUERY_SMTP_DATA -> smtp_settings != '')
		{
			#Load HTML Header
			$DAILYREPORT_MAIL_BODY = $this -> html_header(1350);
			
			#######################
			#BEGIN REPLICA REPORT
			#######################
			$DAILYREPORT_MAIL_BODY .= '<div style="background-color:#FFFFFF;">&nbsp;&nbsp;&nbsp;&nbsp;- '.$this -> translate('Daily Replication Information',$ACCT_LANG).' -</div>';
			$DAILYREPORT_MAIL_BODY .= '<table border="0" cellpadding="5" cellspacing="1" width="100%">';
			$DAILYREPORT_MAIL_BODY .= '<tr>';
			$DAILYREPORT_MAIL_BODY .= '<td bgcolor="#009688" valign="TOP" width="33" align="center"><font color="#f4f3ee">-</font></td>';
			$DAILYREPORT_MAIL_BODY .= '<td bgcolor="#009688" valign="TOP" width="167"><font color="#f4f3ee">'.$this -> translate('Hostname',$ACCT_LANG).'</font></td>';
			$DAILYREPORT_MAIL_BODY .= '<td bgcolor="#009688" valign="TOP" width="165"><font color="#f4f3ee">'.$this -> translate('Cloud Type',$ACCT_LANG).'</font></td>';
			$DAILYREPORT_MAIL_BODY .= '<td bgcolor="#009688" valign="TOP" width="253"><font color="#f4f3ee">'.$this -> translate('Source Server',$ACCT_LANG).'</font></td>';
			$DAILYREPORT_MAIL_BODY .= '<td bgcolor="#009688" valign="TOP" width="253"><font color="#f4f3ee">'.$this -> translate('Target Server',$ACCT_LANG).'</font></td>';
			$DAILYREPORT_MAIL_BODY .= '<td bgcolor="#009688" valign="TOP" width="130"><font color="#f4f3ee">'.$this -> translate('Snapshot Count',$ACCT_LANG).'</font></td>';
			$DAILYREPORT_MAIL_BODY .= '<td bgcolor="#009688" valign="TOP" width="130"><font color="#f4f3ee">'.$this -> translate('Create Time',$ACCT_LANG).'</font></td>';
			$DAILYREPORT_MAIL_BODY .= '<td bgcolor="#009688" valign="TOP" width="130"><font color="#f4f3ee">'.$this -> translate('Delete Time',$ACCT_LANG).'</font></td>';
			$DAILYREPORT_MAIL_BODY .= '<td bgcolor="#009688" valign="TOP" width="89"><font color="#f4f3ee">'.$this -> translate('Status',$ACCT_LANG).'</font></td>';
			$DAILYREPORT_MAIL_BODY .= '</tr>';
				
			if ($REPORT_REPLICA != false)
			{
				for ($i=0; $i<count($REPORT_REPLICA); $i++)
				{
					if ($i % 2 == 0)
					{
						$IBG_COLOR = 'bgcolor="#F5F5F5"';
					}
					else
					{
						$IBG_COLOR = 'bgcolor="#FFFFFF"';
					}					
					
					#RE-FORMAT CLOUD TYPE NAME
					$DEFINE_NAME = ["OPENSTACK", "Aliyun"];
					$CHANGE_NAME = ["OpenStack", "Alibaba cloud"];
					$CLOUD_TYPE = str_replace($DEFINE_NAME, $CHANGE_NAME, $REPORT_REPLICA[$i]['CLOUD_TYPE']);
					
					if ($REPORT_REPLICA[$i]['SNAP_COUNT'] == 0)
					{
						$SNAPSHOT_COLOR = 'red';
					}
					else
					{
						$SNAPSHOT_COLOR = '#60504F';
					}					
					
					#CONVERT TIME TO USER DEFINE TIMEZONE
					$REPLICA_CREATE_TIME = $this -> change_time_from_UTC($REPORT_REPLICA[$i]['REPLICA_CREATE_TIME'],$TimeZone);
					if ($REPORT_REPLICA[$i]['REPLICA_DELETE_TIME'] != '0000-00-00 00:00:00')
					{
						$REPLICA_DELETE_TIME = $this -> change_time_from_UTC($REPORT_REPLICA[$i]['REPLICA_DELETE_TIME'],$TimeZone);
					}
					else
					{
						$REPLICA_DELETE_TIME = '-';
					}
					
					#RE-FORMAT STATUS NAME
					$STATUS_FLAG = ["Y", "X"];
					$STATUS_NAME = ["Active", "Deleted"];
					$REPLICA_STATUS = str_replace($STATUS_FLAG, $STATUS_NAME, $REPORT_REPLICA[$i]['REPLICA_STATUS']);
					
					$DAILYREPORT_MAIL_BODY .= '<tr>';
					$DAILYREPORT_MAIL_BODY .= '<td nowrap '.$IBG_COLOR.' valign="TOP" align="center">'.($i+1).'</td>';
					$DAILYREPORT_MAIL_BODY .= '<td nowrap '.$IBG_COLOR.' valign="TOP">'.$REPORT_REPLICA[$i]['PACKER_NAME'].'</td>';
					$DAILYREPORT_MAIL_BODY .= '<td nowrap '.$IBG_COLOR.' valign="TOP">'.$CLOUD_TYPE.'</td>';
					$DAILYREPORT_MAIL_BODY .= '<td nowrap '.$IBG_COLOR.' valign="TOP">'.$REPORT_REPLICA[$i]['SOURCE_SERVER'].' ('.json_decode($REPORT_REPLICA[$i]['SOURCE_SERVER_ADDR'])[0].')</td>';
					$DAILYREPORT_MAIL_BODY .= '<td nowrap '.$IBG_COLOR.' valign="TOP">'.$REPORT_REPLICA[$i]['TARGET_SERVER'].' ('.json_decode($REPORT_REPLICA[$i]['TARGET_SERVER_ADDR'])[0].')</td>';
					$DAILYREPORT_MAIL_BODY .= '<td nowrap '.$IBG_COLOR.' valign="TOP"><font color="'.$SNAPSHOT_COLOR.'">'.$REPORT_REPLICA[$i]['SNAP_COUNT'].'</font></td>';
					$DAILYREPORT_MAIL_BODY .= '<td nowrap '.$IBG_COLOR.' valign="TOP">'.$REPLICA_CREATE_TIME.'</td>';
					$DAILYREPORT_MAIL_BODY .= '<td nowrap '.$IBG_COLOR.' valign="TOP">'.$REPLICA_DELETE_TIME.'</td>';
					$DAILYREPORT_MAIL_BODY .= '<td nowrap '.$IBG_COLOR.' valign="TOP">'.$REPLICA_STATUS.'</td>';
					$DAILYREPORT_MAIL_BODY .= '</tr>';
				}
			}
			else
			{
				$DAILYREPORT_MAIL_BODY .= '<tr>';
				$DAILYREPORT_MAIL_BODY .= '<td bgcolor="#F5F5F5" colspan="11">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;'.$this -> translate('No Replication Information',$ACCT_LANG).'</td>';
				$DAILYREPORT_MAIL_BODY .= '</tr>';
			}
			$DAILYREPORT_MAIL_BODY .= '</table>';
			
			
			#######################
			#WHITE SPACE TR
			#######################
			$DAILYREPORT_MAIL_BODY .= '<table border="0" cellpadding="0" cellspacing="0" width="100%">';
			$DAILYREPORT_MAIL_BODY .= '<tr>';
			$DAILYREPORT_MAIL_BODY .= '<td bgcolor="#FFFFFF" colspan="9">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</td>';
			$DAILYREPORT_MAIL_BODY .= '</tr>';
			$DAILYREPORT_MAIL_BODY .= '</table>';
						
			#######################
			#BEGIN SERVICE REPORT
			#######################
			$REPORT_SERVICE = $ReplMgmt -> query_service_report($ACCT_UUID,24);
			
			$DAILYREPORT_MAIL_BODY .= '<div style="background-color:#FFFFFF;">&nbsp;&nbsp;&nbsp;&nbsp;- '.$this -> translate('Daily Recovery Information',$ACCT_LANG).' -</div>';
			$DAILYREPORT_MAIL_BODY .= '<table border="0" cellpadding="5" cellspacing="1" width="100%">';
			$DAILYREPORT_MAIL_BODY .= '<tr>';
			$DAILYREPORT_MAIL_BODY .= '<td bgcolor="#3F51B5" valign="TOP" width="33" align="center"><font color="#f4f3ee">-</font></td>';
			$DAILYREPORT_MAIL_BODY .= '<td bgcolor="#3F51B5" valign="TOP" width="167"><font color="#f4f3ee">'.$this -> translate('Hostname',$ACCT_LANG).'</font></td>';
			$DAILYREPORT_MAIL_BODY .= '<td bgcolor="#3F51B5" valign="TOP" width="165"><font color="#f4f3ee">'.$this -> translate('Cloud Type',$ACCT_LANG).'</font></td>';
			$DAILYREPORT_MAIL_BODY .= '<td bgcolor="#3F51B5" valign="TOP" width="252"><font color="#f4f3ee">'.$this -> translate('Recovery Type',$ACCT_LANG).'</font></td>';
			$DAILYREPORT_MAIL_BODY .= '<td bgcolor="#3F51B5" valign="TOP" width="252"><font color="#f4f3ee">'.$this -> translate('Job Status',$ACCT_LANG).'</font></td>';
			$DAILYREPORT_MAIL_BODY .= '<td bgcolor="#3F51B5" valign="TOP" width="130"><font color="#f4f3ee">'.$this -> translate('OS Type',$ACCT_LANG).'</font></td>';
			$DAILYREPORT_MAIL_BODY .= '<td bgcolor="#3F51B5" valign="TOP" width="130"><font color="#f4f3ee">'.$this -> translate('Disk Size',$ACCT_LANG).'</font></td>';			
			$DAILYREPORT_MAIL_BODY .= '<td bgcolor="#3F51B5" valign="TOP" width="130"><font color="#f4f3ee">'.$this -> translate('Time',$ACCT_LANG).'</font></td>';			
			$DAILYREPORT_MAIL_BODY .= '<td bgcolor="#3F51B5" valign="TOP" width="87"><font color="#f4f3ee">'.$this -> translate('Status',$ACCT_LANG).'</font></td>';
			$DAILYREPORT_MAIL_BODY .= '</tr>';
			
			if ($REPORT_SERVICE != false)
			{
				for ($x=0; $x<count($REPORT_SERVICE); $x++)
				{
					#TD BACKGROUND COLOR
					if ($x % 2 == 0)
					{
						$XBG_COLOR = 'bgcolor="#F5F5F5"';
					}
					else
					{
						$XBG_COLOR = 'bgcolor="#FFFFFF"';
					}					
					
					#RE-FORMAT OS NAME
					$OS_FLAG = ["MS", "LX"];
					$OS_NAME = ["Windows", "Linux"];
					$OS_TYPE = str_replace($OS_FLAG, $OS_NAME, $REPORT_SERVICE[$x]['OS_TYPE']);
					
					#RE-FORMAT CLOUD TYPE NAME
					$DEFINE_NAME = ["OPENSTACK", "Aliyun"];
					$CHANGE_NAME = ["OpenStack", "Alibaba cloud"];
					$CLOUD_TYPE = str_replace($DEFINE_NAME, $CHANGE_NAME, $REPORT_SERVICE[$x]['CLOUD_TYPE']);
					
					#CONVERT TIME TO USER DEFINE TIMEZONE
					$SERVICE_TIMESTAMP = $this -> change_time_from_UTC($REPORT_SERVICE[$x]['SERVICE_TIME'],$TimeZone);
					
					#RE-FORMAT STATUS NAME
					$STATUS_FLAG = ["Y", "X"];
					$STATUS_NAME = ["Active", "Deleted"];
					$SERVOCE_STATUS = str_replace($STATUS_FLAG, $STATUS_NAME, $REPORT_SERVICE[$x]['STATUS']);

					$DAILYREPORT_MAIL_BODY .= '<tr>';
					$DAILYREPORT_MAIL_BODY .= '<td nowrap '.$XBG_COLOR.' valign="TOP" align="center">'.($x+1).'</td>';
					$DAILYREPORT_MAIL_BODY .= '<td nowrap '.$XBG_COLOR.' valign="TOP">'.$REPORT_SERVICE[$x]['PACKER_NAME'].'</td>';
					$DAILYREPORT_MAIL_BODY .= '<td nowrap '.$XBG_COLOR.' valign="TOP">'.$CLOUD_TYPE.'</td>';
					$DAILYREPORT_MAIL_BODY .= '<td nowrap '.$XBG_COLOR.' valign="TOP">'.preg_replace('/(?<!\ )[A-Z]/', ' $0', $REPORT_SERVICE[$x]['SERVICE_TYPE']).'</td>';
					$DAILYREPORT_MAIL_BODY .= '<td nowrap '.$XBG_COLOR.' valign="TOP">'.preg_replace('/(?<!\ )[A-Z]/', ' $0', $REPORT_SERVICE[$x]['JOB_STATUS']).'</td>';
					$DAILYREPORT_MAIL_BODY .= '<td nowrap '.$XBG_COLOR.' valign="TOP">'.$OS_TYPE.'</td>';
					$DAILYREPORT_MAIL_BODY .= '<td nowrap '.$XBG_COLOR.' valign="TOP">'.($REPORT_SERVICE[$x]['DISK_SIZE']/1024/1024).' GB</td>';
					$DAILYREPORT_MAIL_BODY .= '<td nowrap '.$XBG_COLOR.' valign="TOP">'.$SERVICE_TIMESTAMP.'</td>';
					$DAILYREPORT_MAIL_BODY .= '<td nowrap '.$XBG_COLOR.' valign="TOP">'.$SERVOCE_STATUS.'</td>';				
					$DAILYREPORT_MAIL_BODY .= '</tr>';
					
				}
			}
			else
			{
				$DAILYREPORT_MAIL_BODY .= '<tr>';
				$DAILYREPORT_MAIL_BODY .= '<td bgcolor="#F5F5F5" colspan="9">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;'.$this -> translate('No Recovery Information',$ACCT_LANG).'</td>';
				$DAILYREPORT_MAIL_BODY .= '</tr>';
			}
						
			$DAILYREPORT_MAIL_BODY .= '</table>';			
			$DAILYREPORT_MAIL_BODY .= $this -> html_footer();
			
			#HTML Backup
			$HTML_FILE_NAME = 'DailyReport-'.time();
			$this -> email_html_backup('_mgmt',$HTML_FILE_NAME,$DAILYREPORT_MAIL_BODY);
			
			#GET EMAIL SUBJECT
			date_default_timezone_set($TimeZone);
			$DateTime = new DateTime('now', new DateTimeZone($TimeZone));
			$ABBREVIATION = $DateTime -> format('T');			
			$NOTIFICATION_TIME = date('Y-m-d H:i:s', time());			
			$DAILYREPORT_MAIL_SUBJECT = $this -> translate('SaaSaMe Replication&Recovery Daily Report',$ACCT_LANG).' ('.$NOTIFICATION_TIME.' '.$ABBREVIATION.')';
					
			#DEFINE SENDMAIL INFORMATION
			$SENDMAIL_INFO = (array)$QUERY_SMTP_DATA -> smtp_settings;
			$SENDMAIL_INFO['MailSubject'] = $DAILYREPORT_MAIL_SUBJECT;
			$SENDMAIL_INFO['MailBody'] = $DAILYREPORT_MAIL_BODY;
			$SENDMAIL_INFO['AddAttachment'] = null;

			//return $SENDMAIL_INFO; #FOR DEBUG
			$this -> SendMail($SENDMAIL_INFO);
		}	
	}

	###########################
	# DAILY BACKUP
	###########################
	public function get_daily_backup($ACCT_UUID)
	{
		$AcctMgmt = new Account_Class();
		
		$QUERY_SMTP_DATA = $AcctMgmt -> query_account($ACCT_UUID)['ACCT_DATA'];
		$ACCT_LANG = $QUERY_SMTP_DATA -> account_language;
	
		#Get User Define TimeZone
		$TimeZone = isset($QUERY_SMTP_DATA -> account_timezone)? $QUERY_SMTP_DATA -> account_timezone : 'UTC'; 
			
		if (isset($QUERY_SMTP_DATA -> notification_type) AND in_array('DailyBackup', $QUERY_SMTP_DATA -> notification_type) AND isset($QUERY_SMTP_DATA -> smtp_settings) AND $QUERY_SMTP_DATA -> smtp_settings != '')
		{
			#USER TIME OBJECT
			date_default_timezone_set($TimeZone);
			$DateTime = new DateTime('now', new DateTimeZone($TimeZone));
			
			$SQL_FILE_NAME = Misc_Class::backup_database('NOW');
			
			#Load HTML Header
			$DAILYBACKUP_MAIL_BODY = $this -> html_header(600);
			
			#SQL File Information
			$DAILYBACKUP_MAIL_BODY .= '<table border="0" cellpadding="5" cellspacing="1" width="600">';
			$DAILYBACKUP_MAIL_BODY .= '<tr>';
			$DAILYBACKUP_MAIL_BODY .= '<td bgcolor="#104472" valign="TOP" width="600" colspan="2"><div style="color:#f4f3ee">'.$this -> translate('Daily Backup File Information',$ACCT_LANG).'</div></td>';
			$DAILYBACKUP_MAIL_BODY .= '</tr>';

			$DAILYBACKUP_MAIL_BODY .= '<tr>';
			$DAILYBACKUP_MAIL_BODY .= '<td bgcolor="#F5F5F5" valign="TOP" width="150">'.$this -> translate('Name',$ACCT_LANG).'</td>';
			$DAILYBACKUP_MAIL_BODY .= '<td bgcolor="#F5F5F5" valign="TOP" width="450">'.basename($SQL_FILE_NAME).'</td>';
			$DAILYBACKUP_MAIL_BODY .= '</tr>';
			
			$DAILYBACKUP_MAIL_BODY .= '<tr>';
			$DAILYBACKUP_MAIL_BODY .= '<td bgcolor="#FFFFFF" valign="TOP" width="150">MD5</td>';
			$DAILYBACKUP_MAIL_BODY .= '<td bgcolor="#FFFFFF" valign="TOP" width="450">'.md5_file($SQL_FILE_NAME).'</td>';
			$DAILYBACKUP_MAIL_BODY .= '</tr>';
			
			$DAILYBACKUP_MAIL_BODY .= '<tr>';
			$DAILYBACKUP_MAIL_BODY .= '<td bgcolor="#F5F5F5" valign="TOP" width="150">'.$this -> translate('Size',$ACCT_LANG).'</td>';
			$DAILYBACKUP_MAIL_BODY .= '<td bgcolor="#F5F5F5" valign="TOP" width="450">'.round((filesize($SQL_FILE_NAME)/1024/1024),2).' MB</td>';
			$DAILYBACKUP_MAIL_BODY .= '</tr>';
			
			$DAILYBACKUP_MAIL_BODY .= '<tr>';
			$DAILYBACKUP_MAIL_BODY .= '<td bgcolor="#FFFFFF" valign="TOP" width="150">'.$this -> translate('Type',$ACCT_LANG).'</td>';
			$DAILYBACKUP_MAIL_BODY .= '<td bgcolor="#FFFFFF" valign="TOP" width="450">'.mime_content_type($SQL_FILE_NAME).'</td>';
			$DAILYBACKUP_MAIL_BODY .= '</tr>';
			
			$DAILYBACKUP_MAIL_BODY .= '<tr>';
			$DAILYBACKUP_MAIL_BODY .= '<td bgcolor="#F5F5F5" valign="TOP" width="150">'.$this -> translate('Last modified',$ACCT_LANG).'</td>';
			$DAILYBACKUP_MAIL_BODY .= '<td bgcolor="#F5F5F5" valign="TOP" width="450">'.date("Y-m-d H:i:s.",filemtime($SQL_FILE_NAME)).'</td>';
			$DAILYBACKUP_MAIL_BODY .= '</tr>';
			
			$DAILYBACKUP_MAIL_BODY .= '</table>';
			
			#Load HTML Footer
			$DAILYBACKUP_MAIL_BODY .= $this -> html_footer();
			
			#HTML Backup
			$HTML_FILE_NAME = 'DailyBackup-'.time();
			$this -> email_html_backup('_mgmt',$HTML_FILE_NAME,$DAILYBACKUP_MAIL_BODY);
			
			#GET EMAIL SUBJECT
			$ABBREVIATION = $DateTime -> format('T');			
			$NOTIFICATION_TIME = date('Y-m-d H:i:s', time());			
			$DAILYREPORT_MAIL_SUBJECT = $this -> translate('SaaSaMe Daily Backup Report',$ACCT_LANG).' ('.$NOTIFICATION_TIME.' '.$ABBREVIATION.')';
			
			#DEFINE SENDMAIL INFORMATION
			$SENDMAIL_INFO = (array)$QUERY_SMTP_DATA -> smtp_settings;
			$SENDMAIL_INFO['MailSubject'] = $DAILYREPORT_MAIL_SUBJECT;
			$SENDMAIL_INFO['MailBody'] = $DAILYBACKUP_MAIL_BODY;
			$SENDMAIL_INFO['AddAttachment'] = $this -> create_archive($SQL_FILE_NAME);
			
			//return $SENDMAIL_INFO; #FOR DEBUG
			$this -> SendMail($SENDMAIL_INFO);
		}
	}
	
	###########################
	# CREATE ARCHIVE
	###########################
	private function create_archive($ADD_FILE)
	{
		$ZIP_FILE = $_SERVER['DOCUMENT_ROOT'].'/_include/_debug/_mgmt/notification/'.explode(".",basename($ADD_FILE))[0].'.zip';
		
		$PASSWORD = $this -> archive_password(pathinfo($ZIP_FILE)['filename']);
		
		$zip = new ZipArchive;
		if ($zip -> open($ZIP_FILE, ZipArchive::CREATE) === TRUE)
		{
			#Add files to the zip file
			$zip -> addFile($ADD_FILE,basename($ADD_FILE));
			
			#SET ARCHIVE PASSWORD
			$zip -> setEncryptionName(basename($ADD_FILE), ZipArchive::EM_AES_256, $PASSWORD);
			
			#Add irm_transport files
			$KEY = '.bak';
			$MATCHES_BAK = array_values(array_filter(scandir(getenv('WEBROOT').'logs'), function($var) use ($KEY) { return preg_match("/\b$KEY\b/i", $var); }));
			if (count($MATCHES_BAK) != 0)
			{
				for ($i=0; $i<count($MATCHES_BAK); $i++)
				{
					$BAK_FILE = getenv('WEBROOT').'logs\\'.$MATCHES_BAK[$i];
					$zip -> addFile($BAK_FILE,basename($BAK_FILE));
					
					#SET ARCHIVE PASSWORD
					$zip -> setEncryptionName(basename($BAK_FILE), ZipArchive::EM_AES_256, $PASSWORD);
				}
			}
		
			#All files are added, so close the zip file.
			$zip -> close();
		}		
		return $ZIP_FILE;
	}
	
	###########################
	# EXTRACT ARCHIVE
	###########################
	public function extract_archive($ARCHIVE_FILE,$PASSWORD)
	{
		$ZIP_FILE = $_SERVER['DOCUMENT_ROOT'].'/_include/_debug/_mgmt/_restoreDB/'.$ARCHIVE_FILE;
		
		$zip = new ZipArchive;
		
		$res = $zip->open($ZIP_FILE);
		
		if ($res === TRUE)
		{
			for ($i=0; $i<$zip->numFiles; $i++ )
			{
				if (strpos($zip->getNameIndex($i),'.sql') !== false) {$SQL_FILE_NAME = $zip->getNameIndex($i); break;}
			}
			
			$zip -> setPassword($PASSWORD);
			
			if ($zip -> extractTo($_SERVER['DOCUMENT_ROOT'].'/_include/_debug/_mgmt/_restoreDB/',array($SQL_FILE_NAME)) == TRUE)
			{
				return array('Status' => true, 'Msg' => 'Successful extract archive','SqlFile' => $SQL_FILE_NAME);
			}
			else
			{
				return array('Status' => false, 'Msg' => 'Faled to extract archive.');
			}
			$zip -> close();
		}	
		else 
		{
			return array('Status' => false, 'Msg' => 'Failed opening archive.');
		}
	}
	
	###########################
	# ARCHIVE PASSWORD
	###########################
	public function archive_password($STRING)
	{
		$HMAC_PASSWORD = substr(hash_hmac("ripemd160", $STRING, 'SaaSaMe'), 13, 8);
		return $HMAC_PASSWORD;
	}

	###########################
	# TRANSLATE
	###########################
	private function translate($STRING,$LANG)
	{
		#DEFINE DEFAULT REGION
		$DEFINE_LANG_FILE = __DIR__ .'\_transport\smtp_translation.txt';
		$CSV_File = file($DEFINE_LANG_FILE);
		$LANG_DATA = [];
		foreach ($CSV_File as $LINE) {$LANG_DATA[] = str_getcsv($LINE);}
	
		#GET LANGUAGE INDEX KEY
		switch($LANG)
		{
			case 'zh-tw':
				$LANG_ID = 1;
			break;
			
			case 'zh-cn':
				$LANG_ID = 2;
			break;
			
			default:
				$LANG_ID = 0;
		}
		
		for($i=0; $i<count($LANG_DATA); $i++)
		{
			if ($LANG_DATA[$i][0] == $STRING)
			{
				$STRING = $LANG_DATA[$i][$LANG_ID];
				break;
			}
		}
		
		return $STRING;
		
		/*
		switch($LANG)
		{
			case 'zh-tw':
				switch ($STRING)
				{
					#Testing Translation
					case 'SMTP Setting Verification': return 'SMTP 設置驗證'; break;
					case 'This is sending test message from SaaSaMe Transport Server.': return '這是從芝麻開雲 Transport 伺服器發送測試消息。'; break;
					
					#Replica Translation
					case 'Transport Information': return 'Transport 伺服器資訊'; break;
					case 'Hostname': return '來源主機名稱'; break;
					case 'Source': return '來源'; break;
					case 'Target': return '目標'; break;
					case 'Address': return '地址'; break;
					case 'Type': return '類型'; break;
					case 'Host Information': return '來源主機資訊'; break;
					case 'Packer Type': return 'Packer 類型'; break;
					case 'OS Name': return '作業系統'; break;
					case 'Host Address': return '來源主機地址'; break;
					case 'Disk': return '磁碟'; break;
					case 'Cloud Disk Information': return '複製磁碟資訊'; break;
					case 'Replication Workload Configuration': return '複製設定'; break;
					case 'Job ID': return '工作 ID'; break;
					case 'Job Start at': return '序開始時間'; break;
					case 'Snapshot Number': return '保留快照數量'; break;
					case 'Packer Thread': return 'Packer 讀取命令'; break;
					case 'Loader Thread': return '複製寫入命令'; break;
					case 'Loader Trigger': return '開始複製寫入'; break;
					case 'Export Path': return '磁碟複製檔案儲存路徑'; break;
					case 'Export Type': return '磁碟複製檔案類型'; break;
					case 'New Replication Created': return '成功建立複製程序'; break;
					
					#Recover Translation
					case 'Recover Type': return '還原執行類型'; break;
					case 'OS Type': return '作業系統'; break;
					case 'Instance Information': return '還原執行資訊'; break;
					case 'Instance Hostname': return '還原主機名稱'; break;
					case 'Flavor': return '虛擬硬體規格'; break;
					case 'Security Groups': return '安全群組'; break;
					case 'Hypervisor / Region': return '虛擬伺服器/地區'; break;
					case 'NIC': return '網路卡'; break;
					case 'Recover Cloud Type': return '目標還原雲平台類型'; break;
					case 'Administrator Password': return '管理員密碼'; break;
					case 'The Recovery Instance is Ready': return '目標還原主機回報已成功完成系統啟動'; break;
					
					#DailyReport Translation
					case 'Daily Replication Information': return '每日複製程序資訊'; break;
					case 'Daily Recovery Information': return '每日還原程序資訊'; break;
					case 'Source Server': return '來源伺服器'; break;
					case 'Target Server': return '目標伺服器'; break;
					case 'Cloud Type': return '雲平台類型'; break;
					case 'Snapshot Count': return '快照數量'; break;
					case 'Create Time': return '創建時間'; break;
					case 'Delete Time': return '刪除時間'; break;
					case 'Status': return '狀態'; break;						
					case 'Disk Size': return '磁碟大小'; break;
					case 'Recovery Type': return '還原類型'; break;
					case 'Job Status': return '工作狀態'; break;
					case 'Time': return '時間'; break;
					case 'SaaSaMe Replication&Recovery Daily Report': return '芝麻開雲複製程序與還原程序每日報告'; break;
					case 'No Replication Information': return '沒有複製資訊'; break;
					case 'No Recovery Information': return '沒有還原資訊'; break;
					
					#DailyBackup Translation					
					case 'Daily Backup File Information': return '每日備份文件信息'; break;
					case 'Name': return '文件名'; break;
					case 'Size': return '文件大小'; break;
					case 'Type': return '文件類型'; break;
					case 'Last modified': return '最後修改'; break;
					case 'SaaSaMe Daily Backup Report': return '芝麻開雲每日備份報告'; break;

					#DEFAULT
					default: return $STRING;
				}
			break;
			
			case 'zh-cn':
				switch ($STRING)
				{
					#Testing Translation
					case 'SMTP Setting Verification': return 'SMTP 设置验证'; break;
					case 'This is sending test message from SaaSaMe Transport Server.': return '这是从芝麻开云 Transport 服务器发送测试消息。'; break;
					
					#Replica Translation
					case 'Transport Information': return 'Transport 服务器信息'; break;
					case 'Hostname': return '来源主机名'; break;
					case 'Source': return '来源'; break;
					case 'Target': return '目标'; break;
					case 'Address': return '地址'; break;
					case 'Type': return '类型'; break;
					case 'Host Information': return '来源主机名'; break;
					case 'Packer Type': return 'Packer 类型'; break;
					case 'OS Name': return '操作系统'; break;
					case 'Host Address': return '来源主机地址'; break;
					case 'Disk': return '磁盘'; break;
					case 'Cloud Disk Information': return '复制磁盘信息'; break;
					case 'Replication Workload Configuration': return '复制设定'; break;
					case 'Job ID': return '执行程序 ID'; break;
					case 'Job Start at': return '程序开始时间'; break;
					case 'Snapshot Number': return '保留快照数量'; break;
					case 'Packer Thread': return 'Packer 读取命令'; break;
					case 'Loader Thread': return '复制写入命令'; break;
					case 'Loader Trigger': return '开始复制写入'; break;
					case 'Export Path': return '磁盘复制文件保存路径'; break;
					case 'Export Type': return '磁盘复制文件类型'; break;
					case 'New Replication Created': return '成功建立复制程序'; break;
					
					#Recover Translation
					case 'Recover Type': return '恢复执行类型'; break;
					case 'OS Type': return '操作系统'; break;
					case 'Instance Information': return '恢复执行信息'; break;
					case 'Instance Hostname': return '恢复主机名'; break;
					case 'Flavor': return '虚拟硬件规格'; break;
					case 'Security Groups': return '安全组'; break;
					case 'Hypervisor / Region': return '虚拟服务器/地区'; break;
					case 'NIC': return '网络卡'; break;
					case 'Recover Cloud Type': return '目标恢复云平台类型'; break;
					case 'Administrator Password': return '管理员密码'; break;
					case 'The Recovery Instance is Ready': return '目标恢复主机系统启动'; break;
					
					#DailyReport Translation
					case 'Daily Replication Information': return '每日复制程序信息'; break;
					case 'Daily Recovery Information': return '每日恢复程序信息'; break;
					case 'Source Server': return '来源服务器'; break;
					case 'Target Server': return '目标服务器'; break;
					case 'Cloud Type': return '云平台类型'; break;
					case 'Snapshot Count': return '快照数量'; break;
					case 'Create Time': return '创造时间'; break;
					case 'Delete Time': return '删除时间'; break;
					case 'Status': return '状态'; break;
					case 'Disk Size': return '磁盘大小'; break;
					case 'Recovery Type': return '恢复类型'; break;
					case 'Job Status': return '执行状态'; break;
					case 'Time': return '时间'; break;
					case 'SaaSaMe Replication&Recovery Daily Report': return '芝麻开云复制程序與還原類型每日报告'; break;
					case 'No Replication Information': return '没有复制信息'; break;
					case 'No Recovery Information': return '沒有恢复信息'; break;
					
					#DailyBackup Translation					
					case 'Daily Backup File Information': return '每日备份文件信息'; break;
					case 'Name': return '文件名'; break;
					case 'Size': return '文件大小'; break;
					case 'Type': return '文件类型'; break;
					case 'Last modified': return '最后修改'; break;
					case 'SaaSaMe Daily Backup Report': return '芝麻开云每日备份报告'; break;
					
					#DEFAULT
					default: return $STRING;
				}
			break;
				
			default:
				return $STRING;
		}
		*/
	}	
}