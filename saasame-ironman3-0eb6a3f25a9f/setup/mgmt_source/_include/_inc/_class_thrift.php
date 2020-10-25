<?php
set_time_limit(0);
require_once '_class_aws.php';
require_once '_class_identification.php';
require_once '_class_replica.php';
require_once '_class_server.php';
require_once '_class_service.php';
require_once '_class_mailer.php';
require_once '_class_openstack.php';
require_once 'Azure.php';
require_once 'Common_Model.php';
require_once 'Aliyun.php';
require_once 'Aliyun_Model.php';
require_once '_class_Tencent_Model.php';
require_once '_class_Tencent.php';
require_once '_class_azure_blob.php';
require_once '_class_ctyun.php';
require_once '_class_VMWare.php';

###################################
#
#	THRIFT COMMUNICATION CLASS
#
###################################

class management_communication extends Db_Connection
#class management_communication extends Db_Connection implements saasame\transport\management_serviceIf
{
	###########################
	# Construct Function
	###########################
	protected $AliMgmt;
	protected $AzureMgmt;
	protected $AzureBlobMgmt;
	protected $TencentMgmt;
	protected $AwsMgmt;
	protected $ReplMgmt;
	protected $ServerMgmt;
	protected $ServiceMgmt;
	protected $MailerMgmt;
	protected $OpenStackMgmt;	
	protected $CtyunMgmt;
	protected $VMWareMgmt;
	
	protected $MgmtConfig;
	
	public function __construct()
	{
		parent::__construct();
		$this -> AliMgmt  	 	= new Aliyun_Controller();	
		$this -> AzureMgmt   	= new Azure_Controller();
		$this -> AzureBlobMgmt 	= new Azure_Blob_Action_Class();
		$this -> TencentMgmt 	= new Tencent_Controller();
		$this -> AwsMgmt     	= new Aws_Action_Class();
		$this -> ReplMgmt    	= new Replica_Class();	
		$this -> ServerMgmt  	= new Server_Class();
		$this -> ServiceMgmt 	= new Service_Class();
		$this -> MailerMgmt	 	= new Mailer_Class();
		$this -> OpenStackMgmt 	= new OpenStack_Action_Class();
		$this -> CtyunMgmt		= new Ctyun_Action_Class();
		$this -> VMWareMgmt		= new VM_Ware();
		
		$this -> MgmtConfig = Misc_Class::define_mgmt_setting();
	}
		
	###########################
	#	INITIALIZE NEW SNAPSHOT
	###########################
	private function initialize_snapshot($HOST_UUID,$SNAPSHOT_MAPPING,$DISK_MAPS,$BACKUP_MAPS,$PROGRESS_MAPS,$OFFSET_MAPS,$SNAP_TIME,$SNAP_UUID)
	{	
		#$TIMESTAMP = date("Y-m-d H:i:s", strtotime($SNAP_TIME ));
		
		$SNAPSHOT_COUNT = "SELECT * FROM _REPLICA_SNAP WHERE _SNAP_UUID = '".$SNAP_UUID."' AND _STATUS = 'Y'";
		$QUERY = $this -> DBCON -> prepare($SNAPSHOT_COUNT);
		$QUERY -> execute();
		$COUNT_ROWS = $QUERY -> rowCount();
		
		if ($COUNT_ROWS == 0 AND $SNAP_UUID != '')
		{
			$CASCADING_INFO = $this -> ReplMgmt -> cascading_connection($HOST_UUID)['transport'];
			
			foreach ($CASCADING_INFO as $JOB_UUID => $MACHINE_ID)
			{		
				foreach($SNAPSHOT_MAPPING as $DISK_UUID => $SNAP_NAME)
				{
					#GET REPLICA DISK UUID
					$REPL_DISK_UUID = substr($DISK_UUID, 0, 36);

					$DISK_SIZE 	   = $DISK_MAPS[$DISK_UUID];
					$BACKUP_SIZE   = $BACKUP_MAPS[$DISK_UUID];
					$PROGRESS_SIZE = $PROGRESS_MAPS[$DISK_UUID];
					$OFFSET_SIZE   = $OFFSET_MAPS[$DISK_UUID];
					
					$ADD_NEW_SNAPSHOT = "INSERT
											INTO _REPLICA_SNAP(
												_ID,
											
												_REPL_UUID,
												_DISK_UUID,
												_SNAP_UUID,
												_SNAP_NAME,
												
												_ORIGINAL_SIZE,
												_BACKUP_SIZE,
												_PROGRESS_SIZE,
												_OFFSET_SIZE,
												_SNAP_OPTIONS,
								
												_SNAP_TIME,
												_TIMESTAMP,
												_STATUS)
											VALUE(
												'',
												
												'".$JOB_UUID."',
												'".$REPL_DISK_UUID."',
												'".$SNAP_UUID."',
												'".$SNAP_NAME."',
												
												'".$DISK_SIZE."',
												'".$BACKUP_SIZE."',
												'".$PROGRESS_SIZE."',
												'".$OFFSET_SIZE."',
												'".json_encode(array('machine_id' => $MACHINE_ID['target_machine_id']))."',
												
												'".$SNAP_TIME."',
												'".Misc_Class::current_utc_time()."',
												'Y')";
						
					$this -> DBCON -> prepare($ADD_NEW_SNAPSHOT) -> execute();
				}
			}
		}
	}
	
	###########################
	#	UPDATE DISK CBT INFORMATION
	###########################
	private function update_disk_cbt_info($REPL_UUID,$DISK_MAPS,$CBT_INFO)
	{
		if ($CBT_INFO != '')
		{
			foreach($DISK_MAPS as $DISK_UUID => $DISK_SIZE)
			{
				$DISK_UUID = substr($DISK_UUID, 0, 36);
								
				$UPDATE_CBT_INFO = "UPDATE
										_REPLICA_DISK
									SET
										_CBT_INFO	= '".$CBT_INFO."'
									WHERE
										_DISK_UUID 	= '".$DISK_UUID."' AND 
										_REPL_UUID 	= '".$REPL_UUID."'";
							
				$this -> DBCON -> prepare($UPDATE_CBT_INFO) -> execute();
			}
		}
	}
		
	###########################
	#	UPDATE SNAPSHOT BACKUP PROGRESS
	###########################
	private function update_snapshot_progress($REPL_UUID,$SNAP_MAPS,$BACKUP_MAPS,$PROGRESS_MAP,$OFFSET_MAPS,$CBT_INFO)
	{	
		foreach($SNAP_MAPS as $DISK_UUID => $SNAP_NAME)
		{
			$BACKUP_SIZE    = $BACKUP_MAPS[$DISK_UUID];
			$PROGRESS_SIZE  = $PROGRESS_MAP[$DISK_UUID];
			$OFFSET_SIZE 	= $OFFSET_MAPS[$DISK_UUID];
			
			$UPDATE_SNAP = "UPDATE
								_REPLICA_SNAP
							SET
								_BACKUP_SIZE    = '".$BACKUP_SIZE."',
								_PROGRESS_SIZE	= '".$PROGRESS_SIZE."',
								_OFFSET_SIZE	= '".$OFFSET_SIZE."',
								_CBT_INFO		= '".$CBT_INFO."',
								_TIMESTAMP		= '".Misc_Class::current_utc_time()."'
							WHERE
								_SNAP_NAME 		= '".$SNAP_NAME."'";
							
			$this -> DBCON -> prepare($UPDATE_SNAP) -> execute();
		}
	}
		
	###########################
	#	CHECK AND TRIGGER LOADER JOB
	###########################
	private function check_and_trigger_loader_job($REPL_UUID,$SNAP_UUID,$SNAP_MAPS,$BACKUP_MAP,$PROGRESS_MAP)
	{
		#$CHECK_LOADER_JOB = $this -> ServiceMgmt -> get_service_job_status($REPL_UUID,'LOADER');
		
		#QUERY REPLICA INFORMATION
		$REPL_QUERY = $this -> ReplMgmt -> query_replica($REPL_UUID);
		$HOST_UUID	= $REPL_QUERY['PACK_UUID'];
		$JOB_UUID 	= $this -> ReplMgmt -> cascading_connection($HOST_UUID)['transport'];
		
		foreach ($JOB_UUID as $REPLICA_ID => $MACHINE_ID)
		{
			#QUERY REPLICA INFORMATION
			$REPL_INFO = $this -> ReplMgmt -> query_replica($REPLICA_ID);
			
			#GET JOB INFORMATION
			$JOB_INFO = $REPL_INFO['JOBS_JSON'];
			$TRIGGER_PERCENTAGE = $JOB_INFO -> loader_trigger_percentage;
			$BUFFER_SIZE		= $JOB_INFO -> buffer_size;
			$INIT_LOADER		= $JOB_INFO -> init_loader;
			$IS_RESUME			= $JOB_INFO -> is_resume;
			$KEEP_ALIVE			= $JOB_INFO -> loader_keep_alive;			
			
			$CHECK_TRIGGER = "SELECT * FROM _REPLICA_SNAP WHERE _REPL_UUID = '".$REPLICA_ID."' AND _SNAP_UUID = '".$SNAP_UUID."' AND _LOADER_TRIG = 'N' AND _SNAP_OPEN = 'N'";
			$QUERY = $this -> DBCON -> prepare($CHECK_TRIGGER);
			$QUERY -> execute();
			$COUNT_ROWS = $QUERY -> rowCount();	
			
			$TOTAL_BACKUP = 0;
			$TOTAL_PROGRESS = 0;
			
			if ($COUNT_ROWS != 0)
			{		
				foreach($SNAP_MAPS as $DISK_UUID => $SNAP_NAME)
				{
					$TOTAL_BACKUP 	= $TOTAL_BACKUP   + $BACKUP_MAP[$DISK_UUID];
					$TOTAL_PROGRESS = $TOTAL_PROGRESS + $PROGRESS_MAP[$DISK_UUID];
					
					if ($TOTAL_BACKUP != 0)
					{				
						$PERCENTAGE = round(($TOTAL_PROGRESS / $TOTAL_BACKUP) * 100);
					}
					else
					{
						$PERCENTAGE = 0;
					}
				}
				# TWEAK SOON
				/*
					PROGRAMMING LOGIC

					IF BUFFER_SIZE NOT SET THE LOADER WILL TRIIGER BY PROGRESS PRECENTAGE				
					IF BUFFER_SIZE IS SET IT WILL FIRST CHECK BY BUFFER_SIZE LIMIT; 
					IF BUFFER_SIZE DOES NOT REACH THE LIMIT IT WILL CHECK THE PROGRESS PRECENTAGE
				*/			
				if ($BUFFER_SIZE == 0)
				{	
					#LOADER TRIGGER PERCENTAGE
					if ($PERCENTAGE >= $TRIGGER_PERCENTAGE)
					{
						if ($INIT_LOADER == FALSE)
						{
							#SUBMIT LOADER JOB		
							$this -> ServiceMgmt -> create_loader_job($REPLICA_ID);
						
							#MESSAGE
							$MESSAGE = $this -> ReplMgmt -> job_msg('Created Loader job for the specified data size.');
							$this -> ReplMgmt -> update_job_msg($REPLICA_ID,$MESSAGE,'Replica');
						
							#UPDATE TRIGGER LOADER FLAG
							$UPDATE_TRIGGER = "UPDATE _REPLICA_SNAP SET _LOADER_TRIG = 'Y' WHERE _REPL_UUID = '".$REPLICA_ID."' AND _SNAP_UUID = '".$SNAP_UUID."'";
							$this -> DBCON -> prepare($UPDATE_TRIGGER) -> execute();	
						}
						else
						{
							if ($IS_RESUME == FALSE AND $KEEP_ALIVE == FALSE)
							{
								#CHANGE LOADER RESUME STATUS
								$JOB_INFO -> is_resume = true;					
								$this -> ServiceMgmt -> update_trigger_info($REPLICA_ID,$JOB_INFO,'REPLICA');
								
								#RESUME LOADER JOB
								$this -> ServiceMgmt -> resume_service_job($REPLICA_ID,"LOADER");
						
								#MESSAGE
								$MESSAGE = $this -> ReplMgmt -> job_msg('Resume Loader job for the specified data size.');
								$this -> ReplMgmt -> update_job_msg($REPLICA_ID,$MESSAGE,'Replica');
							}
						}
					}
				}
				else
				{
					$BUFFER_BYTE = $BUFFER_SIZE * 1024 * 1024 * 1024;
					$PERCENTAGE_SIZE = $BUFFER_BYTE * 0.5;
					
					if ($TOTAL_PROGRESS >= $PERCENTAGE_SIZE)
					{
						if ($INIT_LOADER == FALSE)
						{							
							#SUBMIT LOADER JOB		
							$this -> ServiceMgmt -> create_loader_job($REPLICA_ID);
							
							#MESSAGE						
							$MESSAGE = $this -> ReplMgmt -> job_msg('Created Loader job for the specified buffer size.');
							$this -> ReplMgmt -> update_job_msg($REPLICA_ID,$MESSAGE,'Replica');
							
							#UPDATE TRIGGER LOADER FLAG
							$UPDATE_TRIGGER = "UPDATE _REPLICA_SNAP SET _LOADER_TRIG = 'Y' WHERE _REPL_UUID = '".$REPLICA_ID."' AND _SNAP_UUID = '".$SNAP_UUID."'";
							$this -> DBCON -> prepare($UPDATE_TRIGGER) -> execute();
						}
						else
						{
							if ($IS_RESUME == FALSE AND $KEEP_ALIVE == FALSE)
							{
								#CHANGE LOADER RESUME STATUS
								$JOB_INFO -> is_resume = true;
								$this -> ServiceMgmt -> update_trigger_info($REPLICA_ID,$JOB_INFO,'REPLICA');
								
								#RESUME LOADER JOB
								$this -> ServiceMgmt -> resume_service_job($REPLICA_ID,"LOADER");
								
								#MESSAGE
								$MESSAGE = $this -> ReplMgmt -> job_msg('Resume Loader job for specified buffer size.');
								$this -> ReplMgmt -> update_job_msg($REPLICA_ID,$MESSAGE,'Replica');
								
								#UPDATE TRIGGER LOADER FLAG
								$UPDATE_TRIGGER = "UPDATE _REPLICA_SNAP SET _LOADER_TRIG = 'Y' WHERE _REPL_UUID = '".$REPLICA_ID."' AND _SNAP_UUID = '".$SNAP_UUID."'";
								$this -> DBCON -> prepare($UPDATE_TRIGGER) -> execute();
							}
						}
					}
					else
					{
						#LOADER TRIGGER PERCENTAGE
						if ($PERCENTAGE >= $TRIGGER_PERCENTAGE)
						{
							if ($INIT_LOADER == FALSE)
							{
								#SUBMIT LOADER JOB		
								$this -> ServiceMgmt -> create_loader_job($REPLICA_ID);
								
								#MESSAGE
								$MESSAGE = $this -> ReplMgmt -> job_msg('Created Loader job for the specified data size.');
								$this -> ReplMgmt -> update_job_msg($REPLICA_ID,$MESSAGE,'Replica');

								#UPDATE TRIGGER LOADER FLAG
								$UPDATE_TRIGGER = "UPDATE _REPLICA_SNAP SET _LOADER_TRIG = 'Y' WHERE _REPL_UUID = '".$REPLICA_ID."' AND _SNAP_UUID = '".$SNAP_UUID."'";
								$this -> DBCON -> prepare($UPDATE_TRIGGER) -> execute();
							}
							else
							{
								if ($IS_RESUME == FALSE AND $KEEP_ALIVE == FALSE)
								{
									#CHANGE LOADER RESUME STATUS
									$JOB_INFO -> is_resume = true;
									$this -> ServiceMgmt -> update_trigger_info($REPLICA_ID,$JOB_INFO,'REPLICA');
								
									#RESUME LOADER JOB
									$this -> ServiceMgmt -> resume_service_job($REPLICA_ID,"LOADER");
								
									#MESSAGE
									$MESSAGE = $this -> ReplMgmt -> job_msg('Resume Loader job for the specified data size.');
									$this -> ReplMgmt -> update_job_msg($REPLICA_ID,$MESSAGE,'Replica');
								
									#UPDATE TRIGGER LOADER FLAG
									$UPDATE_TRIGGER = "UPDATE _REPLICA_SNAP SET _LOADER_TRIG = 'Y' WHERE _REPL_UUID = '".$REPLICA_ID."' AND _SNAP_UUID = '".$SNAP_UUID."'";
									$this -> DBCON -> prepare($UPDATE_TRIGGER) -> execute();
								}
							}
						}
					}
				}
			}				
		}
	}
	
	###########################
	#	RENEW DISK CUSTOMIZED UUID
	###########################
	private function renew_disk_customized_uuid($REPL_UUID)
	{
		#QUERY REPLICA JOB INFO
		$REPL_INFO = (object)$this -> ReplMgmt -> query_replica($REPL_UUID);
				
		#DEBUG LOCATION
		$LOG_LOCATION = $REPL_INFO -> LOG_LOCATION;
		
		#GET TARGET CONNECTION
		$TARGET_CONNECTION = json_decode($REPL_INFO -> CONN_UUID) -> TARGET;
		
		#QUERY CONNECTION INFORMATION
		$QUERY_CONNECTION = (object)$this -> ServerMgmt -> query_connection_info($TARGET_CONNECTION);
		
		#GET LOADER ADDRESS
		$LOADER_ADDR = $QUERY_CONNECTION -> LOAD_ADDR;
	
		#QUERY DISK INFORMATION
		$DISK_INFO = $this -> ReplMgmt -> get_replica_disk_info($REPL_UUID);
		
		#MGMT ADDRESS
		$MGMT_ADDR = $REPL_INFO -> MGMT_ADDR;
		
		#MUTEX MSG
		$MUTEX_MSG = $REPL_UUID.'-'.$MGMT_ADDR.'-'.__FUNCTION__;
		
		#MUTEX LOCK
		$this -> ServiceMgmt -> disk_mutex_action($REPL_UUID,'Replica',$LOADER_ADDR,$MUTEX_MSG,'LOCK_EX','WINDOWS');

		#LOOP TO SET DISK COSTOMIZED ID
		for ($i=0; $i<count($DISK_INFO); $i++)
		{
			$DISK_UUID = $DISK_INFO[$i]['DISK_UUID'];
			$OPEN_DISK = $DISK_INFO[$i]['OPEN_DISK'];
			$SCSI_ADDR = $DISK_INFO[$i]['SCSI_ADDR'];
			
			#ONLY RENEW DISK CUSTOMIZED UUID WHEN THE LENGH IS 36
			if (strlen($SCSI_ADDR) == 36)
			{
				$DISK_ADDR = Misc_Class::guid_v4();
				
				$SET_DISK_ID = $this -> ServiceMgmt -> set_disk_customized_id($LOADER_ADDR,$SCSI_ADDR,$DISK_ADDR);
				
				if ($SET_DISK_ID == TRUE)
				{
					#UPDATE REPLICA DISK SCSI INFO
					$this -> ServiceMgmt -> update_replica_disk_scsi_info($REPL_UUID,$DISK_UUID,$OPEN_DISK,$DISK_ADDR);
					
					Misc_Class::function_debug($LOG_LOCATION,__FUNCTION__,array('OldDiskId' => $SCSI_ADDR, 'NewDiskId' => $DISK_ADDR));
				}
				else
				{
					Misc_Class::function_debug($LOG_LOCATION,__FUNCTION__,array('OldDiskId' => $SCSI_ADDR, 'NewDiskId' => $DISK_ADDR, 'FailReason' => 'Failed to set customized disk id.'));
				}
			}
		}
		
		#MUTEX UNLOCK
		$this -> ServiceMgmt -> disk_mutex_action($REPL_UUID,'Replica',$LOADER_ADDR,$MUTEX_MSG,'LOCK_UN','WINDOWS');
	}	
	
	###########################
	#	UPDATE REPL SNAPSHOT STATUS
	###########################
	private function update_repl_snapshot_status($REPL_UUID,$SNAP_UUID,$STATUS)
	{
		$UPDATE_STATUS = 'UPDATE _REPLICA_SNAP
							SET 
								_SNAP_OPEN = "'.$STATUS.'",
								_TIMESTAMP = "'.Misc_Class::current_utc_time().'"
							WHERE
								_REPL_UUID = "'.$REPL_UUID.'" AND
								_SNAP_UUID = "'.$SNAP_UUID.'"';
								
		$this -> DBCON -> prepare($UPDATE_STATUS) -> execute();
	}
		
	###########################
	#	CHECK REPL SNAPSHOT
	###########################
	private function check_repl_snapshot($MACHINE_ID,$SNAP_UUID)
	{
		//Misc_Class::function_debug('_mgmt',__FUNCTION__,func_get_args());
		
		if ($MACHINE_ID != '')
		{
			$CHECK_TRIGGER = 'SELECT * FROM _REPLICA_SNAP WHERE _SNAP_UUID = "'.$SNAP_UUID.'" AND JSON_CONTAINS(_SNAP_OPTIONS, \'"'.$MACHINE_ID.'"\', \'$.machine_id\') AND (_SNAP_OPEN = "Y" OR _SNAP_OPEN = "X")';
		}
		else
		{
			$CHECK_TRIGGER = "SELECT * FROM _REPLICA_SNAP WHERE _SNAP_UUID = '".$SNAP_UUID."' AND (_SNAP_OPEN = 'Y' OR _SNAP_OPEN = 'X')";
		}
		
		$QUERY = $this -> DBCON -> prepare($CHECK_TRIGGER);		
		$QUERY -> execute();		
		$COUNT_ROWS = $QUERY -> rowCount();
		if ($COUNT_ROWS == 0)
		{
			$SNAPSHOT_STATUS = "SELECT * FROM _REPLICA_SNAP WHERE _SNAP_UUID = '".$SNAP_UUID."' AND (_SNAP_OPEN = 'Y' OR _SNAP_OPEN = 'X')";
			$SNAPSHOT_QUERY = $this -> DBCON -> prepare($SNAPSHOT_STATUS);		
			$SNAPSHOT_QUERY -> execute();		
			$ROWS_COUNT = $SNAPSHOT_QUERY -> rowCount();
			if ($ROWS_COUNT != 0)
			{	
				$SNAPSHOT_INFO = $SNAPSHOT_QUERY -> fetchAll(PDO::FETCH_ASSOC);
				$PACKER_UUID = $this -> ReplMgmt -> query_replica($SNAPSHOT_INFO[0]['_REPL_UUID'])['PACK_UUID'];
				$CASCADE_INFO = $this -> ReplMgmt -> cascading_connection($PACKER_UUID);
				$SOURCE_MACHINE_ID = $CASCADE_INFO['transport'][$CASCADE_INFO['replica']]['source_machine_id'];				
				return ($SOURCE_MACHINE_ID == $MACHINE_ID)?true:false;
			}
			else
			{
				return false;
			}
		}
		else
		{
			return true;
		}
	}
	
	###########################
	#	TIME SYNC WITH MGMT
	###########################
	private function sync_time_with_mgmt($HISTORY_TIME,$DIFF_SECONDS)
	{
		#CONVERT HISTORY TIME
		$HIST_UNIX_TIME = strtotime($HISTORY_TIME);
		$HIST_SQL_TIME  = gmdate("Y-m-d H:i:s", $HIST_UNIX_TIME);
			
		#SYNC SQL TIME
		$SYNC_SQL_TIME = gmdate("Y-m-d H:i:s", ($HIST_UNIX_TIME - $DIFF_SECONDS));
		
		#NEW TIME ARRAY
		$TIME_ARRAY = (object)array('time' 		  => $SYNC_SQL_TIME,
									'unsync_time' => $HIST_SQL_TIME);
		
		return $TIME_ARRAY;
	}	
	
	###########################
	#	JOB HISTORY PARSER
	###########################
	private function parser_job_history($REPL_UUID,$JOB_HIST,$UPDATE_TIME,$TYPE)
	{
		#TIME DIFFERENT
		$DIFF_SECONDS = strtotime($UPDATE_TIME) - time();
		
		foreach ($JOB_HIST as $JOB_HIST)
		{
			if (sizeof($JOB_HIST)!= 0)
			{
				$JOB_HIST_TIME = $JOB_HIST -> time;
				$NEW_HIST_TIME = $this -> sync_time_with_mgmt($JOB_HIST_TIME,$DIFF_SECONDS);
	
				$SYNC_TIME = $NEW_HIST_TIME -> time;
				
				#unset($JOB_HIST -> time);
				#$JOB_HIST = (object)array_merge((array)$NEW_HIST_TIME,(array)$JOB_HIST);
			
				$this -> ReplMgmt -> update_job_msg($REPL_UUID,$JOB_HIST,$TYPE,$SYNC_TIME);
			}
		}
	}
	
	###########################
	#	LOADER PROGRESS DATA
	###########################
	private function update_loader_progress_data($REPL_UUID,$SNAP_UUID,$DISK_DATA,$TRANSPORT_DATA)
	{
		foreach ($DISK_DATA as $DISK_UUID => $PROG_VALUE)
		{
			$TRAN_VALUE = $TRANSPORT_DATA[$DISK_UUID];
			
			$UPDATE_EXEC = "UPDATE 
								_REPLICA_SNAP
							SET 
								_LOADER_DATA 	= '".$PROG_VALUE."',
								_TRANSPORT_DATA = '".$TRAN_VALUE."'
							WHERE
								_REPL_UUID		= '".$REPL_UUID."' AND
								_SNAP_UUID	 	= '".$SNAP_UUID."' AND
								_DISK_UUID 	 	= '".substr($DISK_UUID, 0, 36)."'";
								
			$this -> DBCON -> prepare($UPDATE_EXEC) -> execute();
		}	
	}
	
	###########################
	# FORMAT HOSTNAME
	###########################
	private function format_hostname($HOST_NAME,$OS_TYPE)
	{
		if(is_numeric($HOST_NAME))
		{
			$HOST_NAME = 'CANT-B-NUM-ONLY';
		}
		else
		{
			if(substr($HOST_NAME,0,1) === '.') #REPLACE DOT AT BEGIN
			{
				$HOST_NAME = str_replace(substr($HOST_NAME,0,1),'s',$HOST_NAME);
			}
			
			if(substr($HOST_NAME,0,1) === '-') #REPLACE DASH AT BEGIN
			{
				$HOST_NAME = str_replace(substr($HOST_NAME,0,1),'s',$HOST_NAME);
			}
			
			if(is_numeric(substr($HOST_NAME,0,1))) #REPLACE NUMBER AT BEGIN
			{
				$HOST_NAME = str_replace(substr($HOST_NAME,0,1),'s',$HOST_NAME);
			}
		}
		
		if ($OS_TYPE == 'WINDOWS' ? $ALLOW_LENGTH = 15 : $ALLOW_LENGTH = 128)
		
		$HOST_NAME = str_replace(' ','',$HOST_NAME);
		if (preg_match("/\p{Han}+/u", $HOST_NAME) == false)
		{
			$HOST_NAME = preg_replace("/[^a-zA-Z0-9-]/", "-", $HOST_NAME);
		}	
		$HOST_NAME = substr($HOST_NAME,0,$ALLOW_LENGTH);
		
		return $HOST_NAME;
	}
	
	###########################
	#	GET CLOUD AUTH ITEM
	###########################
	private function get_cloud_auth_item($REPL_UUID,$HOST_NAME)
	{
		#GET ACCOUNT UUID AND CONNECTION UUID AND HOSTNAME
		$REPL_QUERY = $this -> ReplMgmt -> query_replica($REPL_UUID);
		
		$CLUSTER_UUID   = $REPL_QUERY['CLUSTER_UUID'];

		$HOST_INFO = $this -> ServerMgmt -> query_host_info($REPL_QUERY['PACK_UUID']);
		if ($HOST_NAME == '')
		{
			$HOST_NAME = $HOST_INFO['HOST_NAME'];
		}
		$OS_TYPE   = $HOST_INFO['OS_TYPE'];
		$ACCT_UUID = $REPL_QUERY['ACCT_UUID'];
		$CONN_UUID = json_decode($REPL_QUERY['CONN_UUID'],false) -> TARGET;
			
		#GET ACTION
		$SERV_INFO 	= $this -> ServerMgmt -> query_connection_info($CONN_UUID);
		$LOAD_OPEN 	= $SERV_INFO['LOAD_OPEN'];	#IMPORTANT REQUIRE VALUE
		$LAUN_OPEN 	= $SERV_INFO['LAUN_OPEN'];	#IMPORTANT REQUIRE VALUE
		$CLOUD_TYPE = $SERV_INFO['CLOUD_TYPE'];
	
		#GET LAUNCHER INFORMATION
		$LAUN_UUID 	= $SERV_INFO['LAUN_UUID'];
		$LAUN_ADDR  = $SERV_INFO['LAUN_ADDR'];
		$LAUN_SYST  = $SERV_INFO['LAUN_SYST'];
		
		#MGMT ADDRESS
		$MGMT_ADDR  = $SERV_INFO['CONN_DATA']['MGMT_ADDR'];
		
		return (object)array('CLUSTER_UUID'	=> $CLUSTER_UUID,
							 'LOAD_OPEN' 	=> $LOAD_OPEN,
							 'LAUN_OPEN'	=> $LAUN_OPEN,
							 'HOST_NAME' 	=> $this -> format_hostname($HOST_NAME,$OS_TYPE),
							 'CLOUD_TYPE'	=> $CLOUD_TYPE,
							 'LAUN_UUID'	=> $LAUN_UUID,
							 'LAUN_ADDR'	=> $LAUN_ADDR,
							 'LAUN_SYST'	=> $LAUN_SYST,
							 'MGMT_ADDR'	=> $MGMT_ADDR);
	}
		
	###########################
	#	UPDATE SNAPSHOT OPTIONS
	###########################
	private function update_snapshot_options($SNAP_UUID,$SNAP_OPTIONS,$MACHINE_ID)
	{
		//Misc_Class::function_debug('_mgmt',__FUNCTION__,func_get_args());
				
		$SNAPSHOTS = $this -> ReplMgmt -> query_replica_snapshot_with_machine_id($MACHINE_ID,$SNAP_UUID);
	
		if ($SNAPSHOTS != FALSE)
		{
			for ($i=0; $i<count($SNAPSHOTS); $i++)
			{
				$SNAP_OPTION = $SNAPSHOTS[$i]['SNAP_OPTIONS'];
				foreach ($SNAP_OPTIONS as $OPTIONS => $VALUE)
				{
					$SNAP_OPTION -> $OPTIONS = $VALUE;
				}
				
				$JSON_OPTION = str_replace("\\","\\\\",json_encode($SNAP_OPTION));
				
				$UPDATE_EXEC = 'UPDATE _REPLICA_SNAP
								SET
									_SNAP_OPTIONS = \''.$JSON_OPTION.'\'
								WHERE
									JSON_CONTAINS(_SNAP_OPTIONS, \'"'.$MACHINE_ID.'"\', \'$.machine_id\') AND
									_SNAP_UUID = "'.$SNAP_UUID.'"';
				
				$this -> DBCON -> prepare($UPDATE_EXEC) -> execute();
			}
		}
	}
	
	###########################
	#	SNAPSHOT CBT INFORMATION
	###########################
	public function last_snapshot_cbt_information($REPL_UUID)
	{
		$QUERY_EXEC = "SELECT DISTINCT 
							_REPL_UUID, _SNAP_UUID, _SNAP_TIME, _CBT_INFO
						FROM 
							_REPLICA_SNAP
						WHERE 
							_REPL_UUID = '".$REPL_UUID."' AND
							_SNAP_OPEN = 'Y'
						ORDER BY 
							_SNAP_TIME DESC LIMIT 1 ";
	
		$QUERY = $this -> DBCON -> prepare($QUERY_EXEC);
		$QUERY -> execute();
		$COUNT_ROWS = $QUERY -> rowCount();

		if ($COUNT_ROWS != 0)
		{
			return $QUERY -> fetch(PDO::FETCH_ASSOC)['_CBT_INFO'];
		}
		else
		{
			return;
		}
	}
	
	###########################
	#	PREVIOUS EXCLUDED PATHS
	###########################
	private function previous_excluded_paths($REPL_UUID)
	{
		$QUERY_EXEC = "SELECT DISTINCT 
							_REPL_UUID, _SNAP_UUID, _SNAP_TIME, _SNAP_OPTIONS
						FROM 
							_REPLICA_SNAP
						WHERE 
							_REPL_UUID = '".$REPL_UUID."' AND
							_SNAP_OPEN = 'Y'
						ORDER BY 
							_SNAP_TIME DESC LIMIT 1 ";
	
		$QUERY = $this -> DBCON -> prepare($QUERY_EXEC);
		$QUERY -> execute();
		$COUNT_ROWS = $QUERY -> rowCount();

		if ($COUNT_ROWS != 0)
		{
			$SNAP_OPTIONS = json_decode($QUERY -> fetch(PDO::FETCH_ASSOC)['_SNAP_OPTIONS'],true);
			return (isset($SNAP_OPTIONS['excluded_paths']))?$SNAP_OPTIONS['excluded_paths']:array('excluded_paths' => '');
		}
		else
		{
			return array('excluded_paths' => '');
		}
	}
   	
	######################################################
	#
	#	GET REPLICA JOB CREATE DETAIL
	#
	######################################################
	public function get_replica_job_create_detail($SESSION_ID,$JOB_UUID)
	{
		#BACKUP DATABASE
		Misc_Class::backup_database('SYSTEM');

		$REPL_QUERY = $this -> ReplMgmt -> query_replica($JOB_UUID);
		
		if ($REPL_QUERY != FALSE)
		{
			#LOG LOCATION
			$LOG_LOCATION = $REPL_QUERY['LOG_LOCATION'];

			#GET JOB INFORMATION
			$IS_ENCRYPTED 	 			= false;
			$ACCT_UUID					= $REPL_QUERY['ACCT_UUID'];
			$JOB_INFO 		 			= $REPL_QUERY['JOBS_JSON'];
			$HOST_UUID					= $REPL_QUERY['PACK_UUID'];
			$BLOCK_MODE 	 			= $JOB_INFO -> block_mode_enable;
			$THREAD_NUMBER 	 			= $JOB_INFO -> worker_thread_number;
			$CHECKSUM_VERIFY 			= $JOB_INFO -> checksum_verify;
			$BUFFER_SIZE	 			= $JOB_INFO -> buffer_size;
			$SCHEDULE_PAUSE	 			= $JOB_INFO -> schedule_pause;
			$DATA_COMPRESSED 			= $JOB_INFO -> is_compressed;
			$DATA_CHECKSUM	 			= $JOB_INFO -> is_checksum;
			$FILE_SYSTEM_FILTER 		= $JOB_INFO -> file_system_filter;
			$ENABLE_CDR					= $JOB_INFO -> is_continuous_data_replication;
			$PRE_SNAPSHOT_SCRIPT		= $JOB_INFO -> pre_snapshot_script;
			$POST_SNAPSHOT_SCRIPT		= $JOB_INFO -> post_snapshot_script;
			$PACKER_DATA_COMPRESSED		= $JOB_INFO -> is_packer_data_compressed;
			$REPLICATION_RETRY			= $JOB_INFO -> always_retry;
			$PACKER_ENCRYPTION			= $JOB_INFO -> is_encrypted;
			$EXCLUDED_PATHS				= $JOB_INFO -> excluded_paths;
			$PREVIOUS_EXCLUDED_PATHS	= $this -> previous_excluded_paths($JOB_UUID)['excluded_paths'];
			$REPAIR_SYNC_MODE			= $JOB_INFO -> repair_sync_mode;
			$FULL_REPLICA 				= $JOB_INFO -> is_full_replica;
	
			$CASCADE_INFO = $this -> ReplMgmt -> cascading_connection($HOST_UUID);
	
			$CASCADE_TRANSPORT = $CASCADE_INFO['transport'];
			$CASCADE_TYPE	   = $CASCADE_INFO['cascading_type'];
		
			foreach ($CASCADE_TRANSPORT as $CASCADE_JOB_ID => $CASCADE_TRANSPORT_INFO)
			{
				$CONN_UUID = $CASCADE_TRANSPORT_INFO['souce_conn_uuid'];
				$CONN_INFO = $this -> ServerMgmt -> query_connection_info($CONN_UUID);
				
				$CARRIERS_MAP[$CONN_UUID] = $CONN_INFO['LIST_CARR'];

				$PRIORITY_ADDR = $this -> ReplMgmt -> query_replica($CASCADE_JOB_ID)['JOBS_JSON'] -> priority_addr;
				$PRIORITY_MAP[$CONN_UUID] = ($PRIORITY_ADDR == '0.0.0.0')?array():$PRIORITY_ADDR;
				
				if ($CASCADE_TRANSPORT_INFO['souce_direct'] == TRUE)
				{
					$CHECK_CONN_STATUS = $this -> ServiceMgmt -> get_connection_status($CONN_UUID,'CARRIER');
					if ($CHECK_CONN_STATUS != FALSE)
					{
						if ($CHECK_CONN_STATUS -> id == '')
						{
							#RE CREATE CONNECTION
							$RE_CREATE_CONN = $this -> ServiceMgmt -> re_create_connection($CONN_UUID,'CARRIER');
							if ($RE_CREATE_CONN == TRUE)
							{
								$MESSAGE = $this -> ReplMgmt -> job_msg('Success to recreate the Carrier connection.');
								$this -> ReplMgmt -> update_job_msg($JOB_UUID,$MESSAGE,'Replica');
							}
							else
							{
								$MESSAGE = $this -> ReplMgmt -> job_msg('Cannot recreate the Carrier connection.');
								$this -> ReplMgmt -> update_job_msg($JOB_UUID,$MESSAGE,'Replica');
								return false;
							}
						}
					}
					else
					{
						$MESSAGE = $this -> ReplMgmt -> job_msg('Failed to connect to transport server. Cannot recreate the Carrier connection.');
						$this -> ReplMgmt -> update_job_msg($JOB_UUID,$MESSAGE,'Replica');
						return false;
					}
					break;
				}				
			}
	
			#GET HOST INFORMATION
			$HOST_INFO = $this -> ServerMgmt -> query_host_info($REPL_QUERY['PACK_UUID']);

			if ($HOST_INFO != FALSE)
			{
				$PACK_INFO = $this -> ServerMgmt -> query_server_info($HOST_INFO['HOST_SERV']['SERV_UUID']);

				if ($HOST_INFO['HOST_TYPE'] == 'Physical')
				{
					if ($HOST_INFO['HOST_INFO']['direct_mode'] == FALSE)
					{
						$HOST_ADDR = $HOST_INFO['HOST_INFO']['machine_id'];
					}
					else
					{
						$HOST_ADDR = $HOST_INFO['HOST_ADDR'];						
					}
					
					$PACK_ADDR = $HOST_INFO['HOST_NAME'];
					$HOST_ADDR = array($HOST_ADDR => 0);
					$HOST_USER = '';
					$HOST_PASS = '';
					$HOST_UUID = '';
					$HOST_TYPE = \saasame\transport\job_type::physical_transport_type;
					
				}
				elseif ($HOST_INFO['HOST_TYPE'] == 'Virtual')
				{
					$PACK_ADDR = $PACK_INFO['SERV_MISC']['ADDR'];
					$HOST_ADDR = array('127.0.0.1' => 0);
					$HOST_USER = $PACK_INFO['SERV_MISC']['USER'];
					$HOST_PASS = $PACK_INFO['SERV_MISC']['PASS'];
					$HOST_UUID = $HOST_INFO['HOST_INFO']['uuid'];
					$HOST_TYPE = \saasame\transport\job_type::virtual_transport_type;
				}
				elseif ($HOST_INFO['HOST_TYPE'] == 'Offline')
				{
					if ($HOST_INFO['HOST_INFO']['direct_mode'] == FALSE)
					{
						$HOST_ADDR = $HOST_INFO['HOST_INFO']['machine_id'];						
					}
					else
					{	
						$HOST_ADDR = $HOST_INFO['HOST_ADDR'];
					}
					
					$PACK_ADDR = $HOST_INFO['HOST_NAME'];
					$HOST_ADDR = array($HOST_ADDR => 0);
					$HOST_USER = '';
					$HOST_PASS = '';
					$HOST_UUID = '';
					$HOST_TYPE = \saasame\transport\job_type::winpe_transport_job_type;
				}
				else
				{
					return false;
				}
				
				$DISK_INFO = $this -> ReplMgmt -> get_replica_disk_info($REPL_QUERY['REPL_UUID']);
				for ($i=0; $i<count($DISK_INFO); $i++)
				{
					$INFO_DISK[$DISK_INFO[$i]['PACK_URI']] = $i;
					$DISK_IDS[$DISK_INFO[$i]['PACK_URI']]  = $DISK_INFO[$i]['DISK_UUID'].'-'.$DISK_INFO[$i]['ID'];
				}
			}
			else
			{
				return false;
			}
		
			$REPL_JOB_DETAIL = array(
									'host' 				 			 => $PACK_ADDR,
									'addr' 				 			 => $HOST_ADDR,
									'username' 			 			 => $HOST_USER,
									'password' 			 			 => $HOST_PASS,
									'type' 		 		 			 => $HOST_TYPE,
									'virtual_machine_id' 			 => $HOST_UUID,
									'disks'				 			 => $INFO_DISK,									
									'targets'	 		 			 => array($CONN_UUID => $REPL_QUERY['REPL_UUID']),
									'carriers'	 		 			 => $CARRIERS_MAP,
									'full_replicas' 	 			 => array('full_replicas' => 1),
									'disk_ids'			 			 => $DISK_IDS,
									'buffer_size'					 => $BUFFER_SIZE,									
									'checksum_verify'				 => $CHECKSUM_VERIFY,
									'is_full_replica'				 => $FULL_REPLICA,
									'is_paused'						 => $SCHEDULE_PAUSE,
									'is_compressed'					 => $DATA_COMPRESSED,
									'is_checksum'					 => $DATA_CHECKSUM,
									'priority_carrier'				 => $PRIORITY_MAP,
									'cbt_info'			 			 => $REPAIR_SYNC_MODE == 'delta' ? $this -> last_snapshot_cbt_information($JOB_UUID) : '',
									'snapshot_info'		 			 => '',
									'timeout'			 			 => 300,
									'is_encrypted'		 			 => $IS_ENCRYPTED,
									'block_mode_enable'				 => $BLOCK_MODE,
									'file_system_filter_enable'		 => $FILE_SYSTEM_FILTER,
									'is_continuous_data_replication' => $ENABLE_CDR,
									'worker_thread_number'			 => $THREAD_NUMBER,
									'pre_snapshot_script'			 =>	$PRE_SNAPSHOT_SCRIPT,
									'post_snapshot_script'			 => $POST_SNAPSHOT_SCRIPT,
									'is_compressed_by_packer'		 => $PACKER_DATA_COMPRESSED,
									'always_retry'					 => $REPLICATION_RETRY,
									'is_encrypted'					 => $PACKER_ENCRYPTION,
									'excluded_paths'				 => $EXCLUDED_PATHS != '' ? array_flip(explode(",",$EXCLUDED_PATHS)): array(),
									'previous_excluded_paths'		 => $PREVIOUS_EXCLUDED_PATHS != '' ? $PREVIOUS_EXCLUDED_PATHS : array()
								);
		
			#SET BACK FULL SYNC TO DELTA
			#$JOB_INFO -> is_full_replica = false;
			
			#SET INITIALIZATION EMAIL
			#$JOB_INFO -> initialization_email = false;
			
			#SET JOB ON EXECUTING
			#$JOB_INFO -> is_executing = true;
				
			if ($JOB_INFO -> init_carrier != TRUE)
			{
				#MESSAGE
				$MESSAGE = $this -> ReplMgmt -> job_msg('Submitted replication process to Transport Server.');
				$this -> ReplMgmt -> update_job_msg($JOB_UUID,$MESSAGE,'Replica');
			
				#SET CARRIER TO INIT
				$JOB_INFO -> init_carrier = true;
				
				#UPDATE REPLICA JOB CONFIGURATION
				$this -> ServiceMgmt -> update_trigger_info($JOB_UUID,$JOB_INFO,'REPLICA');
			}
		
			#FOR DEBUG
			Misc_Class::function_debug($LOG_LOCATION,__FUNCTION__,$REPL_JOB_DETAIL);
			
			return new saasame\transport\replica_job_create_detail($REPL_JOB_DETAIL);
		}
		else
		{	
			throw new \saasame\transport\invalid_operation(array('what_op'=>0x00003002, 'why'=>'Job '.$JOB_UUID.' is not found.'));
		}		
	}
	
	
	######################################################
	#
	#	CHECK REPLICA JOB ALIVE
	#
	######################################################
	public function is_replica_job_alive($SESSION_ID,$JOB_UUID)
	{
		$REPL_QUERY = $this -> ReplMgmt -> query_replica($JOB_UUID);

		if ($REPL_QUERY != FALSE)
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	
	
	######################################################
	#
	#	UPDATE REPLICA JOB STATE
	#
	######################################################
	public function update_replica_job_state($SESSION_ID, \saasame\transport\replica_job_detail $REPL_STATUS)
	{
		#REPL_UUID
		$REPL_UUID = $REPL_STATUS -> replica_id;
		
		#REPL JOB TYPE
		$REPL_JOB_TYPE = $REPL_STATUS -> type;
		
		#GET REPLICA INFORMATION
		$REPL_QUERY = $this -> ReplMgmt -> query_replica($REPL_UUID);
	
		#ACCT UUID
		$ACCT_UUID	  = $REPL_QUERY['ACCT_UUID'];
	
		#HOST UUID
		$HOST_UUID	  = $REPL_QUERY['PACK_UUID'];
		
		#LOG LOCATION
		$LOG_LOCATION = $REPL_QUERY['LOG_LOCATION'];
	
		#FOR DEBUG
		Misc_Class::function_debug($LOG_LOCATION,__FUNCTION__,$REPL_STATUS);
	
		#UPDATE TIME
		$UPDATE_TIME = $REPL_STATUS -> updated_time;
	
		#SNAPSHOT_MAPPING
		$SNAPSHOT_MAPPING = $REPL_STATUS -> snapshot_mapping;
	
		#ORIGINAL_SIZE
		$DISK_MAPS = $REPL_STATUS -> original_size;
		
		#BACKUP_SIZE
		$BACKUP_MAPS = $REPL_STATUS -> backup_size;
		
		#BACKUP_PROGRESS
		$PROGRESS_MAPS = $REPL_STATUS -> backup_progress;
		
		#OFFSET_SIZE
		$OFFSET_MAPS = $REPL_STATUS -> backup_image_offset;
				
		#SNAPSHOT_TIME
		$SNAP_TIME = $REPL_STATUS -> snapshot_time;
		
		#SNAPSHOT_UUID
		$SNAP_UUID = $REPL_STATUS -> snapshot_info;
		
		#CBT_INFO
		$CBT_INFO = $REPL_STATUS -> cbt_info;
		
		#HISTORIES
		$REPL_HIST = $REPL_STATUS -> histories;
		
		#INIT NEW SNAPSHOT
		$this -> initialize_snapshot($HOST_UUID,$SNAPSHOT_MAPPING,$DISK_MAPS,$BACKUP_MAPS,$PROGRESS_MAPS,$OFFSET_MAPS,$SNAP_TIME,$SNAP_UUID);
		
		#UPDATE SNAPSHOT PROGRESS
		$this -> update_snapshot_progress($REPL_UUID,$SNAPSHOT_MAPPING,$BACKUP_MAPS,$PROGRESS_MAPS,$OFFSET_MAPS,$CBT_INFO);
		
		#UPDATE CBT INFORMATION
		$this -> update_disk_cbt_info($REPL_UUID,$DISK_MAPS,$CBT_INFO);
		
		#UPDATE REPL HISTORY
		$this -> parser_job_history($REPL_UUID,$REPL_HIST,$UPDATE_TIME,'Replica');
			
		#CHECK AND TRIGGER LOADER JOB
		if ($REPL_STATUS -> is_error == '')
		{
			$this -> check_and_trigger_loader_job($REPL_UUID,$SNAP_UUID,$SNAPSHOT_MAPPING,$BACKUP_MAPS,$PROGRESS_MAPS);
		}
		
		#UPDATE VIRTUAL PACKER PARTITION INFORMATION
		if (isset($REPL_STATUS -> virtual_disk_infos))
		{
			$VIRTUAL_DISK_INFO = $REPL_STATUS -> virtual_disk_infos;
			$this -> ServerMgmt -> update_virtual_packer_disk_partition_info($HOST_UUID,$VIRTUAL_DISK_INFO);
		}

		#TO SUPPORT CASCADE JOB UPDATE
		$CASCADE_INFO = $this -> ReplMgmt -> cascading_connection($HOST_UUID);
		foreach ($CASCADE_INFO['transport'] as $JOB_UUID => $MACHINE_ID)
		{
			#QUERY JOB INFORMATION
			$JOB_INFO = $this -> ReplMgmt -> query_replica($JOB_UUID)['JOBS_JSON'];
			
			#GET BOOT DISK
			$BOOT_DISK = $REPL_STATUS -> boot_disk;
			
			#UPDATE REPLICA BOOT DISK
			if ($JOB_INFO -> boot_disk == '')
			{
				if ($REPL_JOB_TYPE != 4) #NONE VIRTUAL JOB TYPE
				{
					$BOOT_DISK = json_decode($BOOT_DISK);
				}
				else
				{
					$SYSTEM_DISKS = $REPL_STATUS -> system_disks;				
					$this -> ServerMgmt -> update_virtual_packer_boot_disk_info($HOST_UUID,$BOOT_DISK,$SYSTEM_DISKS);
				}				
				$JOB_INFO -> boot_disk = $BOOT_DISK;
			}
			
			#EXECUTE PATH
			$EXCLUDED_PATHS = array('excluded_paths' => $REPL_STATUS -> excluded_paths);
			$this -> update_snapshot_options($SNAP_UUID,$EXCLUDED_PATHS,$MACHINE_ID['target_machine_id']);
			
			$JOB_INFO -> is_full_replica = false;
			$JOB_INFO -> is_executing = true;
			$this -> ServiceMgmt -> update_trigger_info($JOB_UUID,$JOB_INFO,'REPLICA');
		}
	}
	
	######################################################
	#
	#	GET LOADER JOB DETAIL
	#
	######################################################
	public function get_loader_job_create_detail($SESSION_ID,$REPL_UUID)
	{	
		$REPL_QUERY = $this -> ReplMgmt -> query_replica($REPL_UUID);
		
		$Common_Model = new Common_Model();
		
		$ReplicaInfo = $Common_Model->getReplicatInfo( $REPL_UUID );

		if ($REPL_QUERY != FALSE)
		{
			#ACCT UUID
			$ACCT_UUID = $REPL_QUERY['ACCT_UUID'];
			
			#HOSTNAME 
			$HOSTNAME = $REPL_QUERY['HOST_NAME'];
			
			#LOG LOCATION
			$LOG_LOCATION = $REPL_QUERY['LOG_LOCATION'];
		
			#GET CONNECTION UUID
			$SOURCE_CONNE_UUID = json_decode($REPL_QUERY['CONN_UUID'],false) -> SOURCE;
			$TARGET_CONNE_UUID = json_decode($REPL_QUERY['CONN_UUID'],false) -> TARGET;

			#GET CONNECTION INFORMATON
			$SOURCE_CONNECTION = $this -> ServiceMgmt -> get_connection_status($SOURCE_CONNE_UUID,'CARRIER');
			$TARGET_CONNECTION = $this -> ServiceMgmt -> get_connection_status($TARGET_CONNE_UUID,'LOADER');
	
			if ($TARGET_CONNECTION != FALSE)
			{
				if ($TARGET_CONNECTION -> id == '')
				{				
					#RE CREATE CONNECTION
					$RE_CREATE_CONN = $this -> ServiceMgmt -> re_create_connection($TARGET_CONNE_UUID,'LOADER');
					if ($RE_CREATE_CONN == TRUE)
					{
						$MESSAGE = $this -> ReplMgmt -> job_msg('Successfully rebuild Loader connection.');
						$this -> ReplMgmt -> update_job_msg($REPL_UUID,$MESSAGE,'Replica');
					}
					else
					{
						$MESSAGE = $this -> ReplMgmt -> job_msg('Cannot rebuild Loader connection.');
						$this -> ReplMgmt -> update_job_msg($REPL_UUID,$MESSAGE,'Replica');
						return false;
					}
				}
			}
			else
			{
				//$MESSAGE = $this -> ReplMgmt -> job_msg('Failed to connect to transport server. Cannot recreate the Loader connection.');
				//$this -> ReplMgmt -> update_job_msg($REPL_UUID,$MESSAGE,'Replica');
				return false;
			}
			
			#GET JOB INFORMATION
			$JOB_INFO = $REPL_QUERY['JOBS_JSON'];
			$LOADER_THREAD_NUMBER   = $JOB_INFO -> loader_thread_number;
			$EXPORT_PATH 		    = $JOB_INFO -> export_path;
			$EXPORT_TYPE 		    = $JOB_INFO -> export_type;
			$KEEP_ALIVE 		    = $JOB_INFO -> loader_keep_alive;
			$ENABLE_CDR 		    = $JOB_INFO -> is_continuous_data_replication;
			$IS_AZURE_BLOB_MODE     = $JOB_INFO -> is_azure_blob_mode;
			$IS_PAUSED			    = isset($JOB_INFO -> is_paused)? $JOB_INFO -> is_paused:false;
			$POST_LOADER_SCRIPT     = $JOB_INFO -> post_loader_script;
			
			#DEFINE DEFAULT ARRAY VALUE
			$SNAP_LIST = array();
			$SNAP_MAPS = array();
			$DISK_ORDER = array();
			
			//$REPLICA_UUID = ($CASCADE_INFO['replica'] == FALSE)?$REPL_UUID:$CASCADE_INFO['replica'];

			#DEFIND UPDATE
			$DISK_INFO = $this -> ReplMgmt -> get_replica_disk_info($REPL_UUID);			
			for ($i=0; $i<count($DISK_INFO); $i++)
			{
				$DISK_UUID = $DISK_INFO[$i]['DISK_UUID'].'-'.$DISK_INFO[$i]['ID'];
				$REPL_DISK_UUID = substr($DISK_UUID, 0, 36);
				
				$DISK_SIZE = $DISK_INFO[$i]['DISK_SIZE'];
				$DISK_ADDR = $DISK_INFO[$i]['SCSI_ADDR'];
				$OPEN_UUID = $DISK_INFO[$i]['OPEN_DISK'];
				
				#NEW VOL ID FOR AWS
				if (strpos($OPEN_UUID, 'vol-') === 0){$OPEN_UUID = str_replace("vol-","vol",$OPEN_UUID);}

				#NEW ID FOR ALIYUN
				if (strpos($OPEN_UUID, 'd-') === 0){$OPEN_UUID = substr($OPEN_UUID,strlen('d-'));}
				
				#DETECT MAP TYPE
				if ($EXPORT_PATH == '')
				{
					if ($IS_AZURE_BLOB_MODE == TRUE) # AZURE BLOG
					{					
						$DETECT_TYPE = \saasame\transport\disk_detect_type::AZURE_BLOB;
						$LUNS_MAPS[$DISK_UUID] = $DISK_UUID.'.vhd';
						$SIZE_MAPS[$DISK_UUID] = $DISK_SIZE;						
					}
					else #CLOUD AND RCD
					{
						if (substr_count($DISK_ADDR, ':') == 3)
						{
							$DETECT_TYPE = 0;
							$LUNS_MAPS[$DISK_UUID] = $DISK_ADDR;						
						}						
						elseif ($OPEN_UUID == "000000000-RECOV-0000-MEDIA-000000000")
						{
							$DETECT_TYPE = (substr_count($DISK_ADDR, '-') == 4)?4:1;
							$LUNS_MAPS[$DISK_UUID] = $DISK_ADDR;
						}
						elseif (strlen($DISK_ADDR) == 32)
						{
							$DETECT_TYPE = 3;
							$LUNS_MAPS[$DISK_UUID] = $DISK_ADDR;
						}
						elseif (strlen($DISK_ADDR) == 36)
						{
							$DETECT_TYPE = 4;
							$LUNS_MAPS[$DISK_UUID] = $DISK_ADDR;
						}
						else
						{
							$DETECT_TYPE = 1;
							$LUNS_MAPS[$DISK_UUID] = $OPEN_UUID;
						}
					}
				}
				else #IMAGE EXPORT
				{
					$DETECT_TYPE = 2;
					$LUNS_MAPS[$DISK_UUID] = $DISK_ADDR;
					$SIZE_MAPS[$DISK_UUID] = $DISK_SIZE;
				}
				
				if( $ReplicaInfo[0]["CloudType"] == "VMWare" ){
					$LUNS_MAPS[$DISK_UUID] = $DISK_UUID.'.vmdk';
					$SIZE_MAPS[$DISK_UUID] = $DISK_SIZE;
				}
				
				#GIVE SNAPSHOT INFORMATION
				//$SNAP_INFO = $this -> ReplMgmt -> query_replica_snapshot_by_machine_id($REPLICA_UUID,$REPL_DISK_UUID,$SESSION_ID);
				$SNAP_INFO = $this -> ReplMgmt -> query_replica_snapshot($REPL_UUID,$REPL_DISK_UUID);
				if ($SNAP_INFO != FALSE)
				{
					for ($SNAP=0; $SNAP<count($SNAP_INFO); $SNAP++)
					{					
						if ($SNAP_INFO[$SNAP]['SNAP_OPEN'] == 'N')
						{
							$SNAP_NAME = $SNAP_INFO[$SNAP]['SNAP_NAME'];
							$SNAP_UUID = $SNAP_INFO[$SNAP]['SNAP_UUID'];
							if ($SNAP_UUID != '')
							{							
								$SNAP_LIST[] = $SNAP_UUID;
								$SNAP_MAPS[$SNAP_UUID][$DISK_UUID] = $SNAP_NAME;
							}
						}
					}
				}
				else
				{
					$SNAP_MAPS = '';
				}
				
				#SET PAGE DATA
				$DATA_PURGE = $DISK_INFO[$i]['PURGE_DATA'];
				if ($DATA_PURGE == 'Y')
				{
					$PURGE_DATA = true;
				}
				else
				{
					$PURGE_DATA = false;
				}
				
				#ADD DISK ORDER
				array_push($DISK_ORDER,$DISK_UUID);
			}
			
			$LOADER_JOB_DETAIL = array(
									'replica_id' 					 => $REPL_UUID,
									'disks_lun_mapping'				 => $LUNS_MAPS,
									'snapshots'						 => array_unique($SNAP_LIST),
									'disks_snapshot_mapping'		 => $SNAP_MAPS,										
									'connection_id'					 => $TARGET_CONNE_UUID,
									'purge_data'					 => $PURGE_DATA,
									'detect_type'					 => $DETECT_TYPE,
									'worker_thread_number'			 => $LOADER_THREAD_NUMBER,
									'host_name'						 => $HOSTNAME,
									'is_continuous_data_replication' => $ENABLE_CDR,
									'is_paused'						 => $IS_PAUSED,
									'keep_alive'					 => $KEEP_ALIVE,
									'post_snapshot_script'			 => $POST_LOADER_SCRIPT,
									'disks_order'					 => $DISK_ORDER
								);
			
			#ADD EXPORT INFORMATION TO JOB DETAIL
			if ($EXPORT_PATH != '')
			{
				$LOADER_JOB_DETAIL['export_disk_type'] = $EXPORT_TYPE;
				$LOADER_JOB_DETAIL['export_path'] = $EXPORT_PATH;
				$LOADER_JOB_DETAIL['disks_size_mapping'] = $SIZE_MAPS;
			}		
			
			#ADD AZURE BLOB INFORMATION TO JOB DETAIL
			if($IS_AZURE_BLOB_MODE == true)
			{
				$LOADER_JOB_DETAIL['export_path'] = $REPL_UUID;
				$LOADER_JOB_DETAIL['disks_size_mapping'] = $SIZE_MAPS;
				$LOADER_JOB_DETAIL['azure_storage_connection_string'] = $this -> AzureBlobMgmt -> get_blob_connection_string($REPL_UUID) -> ConnectionString;
			}
	
			if( $ReplicaInfo[0]["CloudType"] == "VMWare" && $EXPORT_PATH == ''){
				$this->VMWareMgmt->procPreCreateLoaderJobDetail( $REPL_UUID, $HOSTNAME, $LUNS_MAPS, $SIZE_MAPS, $LOADER_JOB_DETAIL);
			}
			
			#FOR CASCADE SETUP
			$PACKER_UUID = $REPL_QUERY['PACK_UUID'];
			$CASCADE_INFO = $this -> ReplMgmt -> cascading_connection($PACKER_UUID);
			
			if ($CASCADE_INFO['is_cascading_ready'] == TRUE)
			{
				$CASCADE_TRANSPORT 		 = $CASCADE_INFO['transport'];
				$CASCADE_SOUCE_DIRECTION = $CASCADE_INFO['is_souce_https'];
				$CASCADE_TYPE	   		 = $CASCADE_INFO['cascading_type'];
				
				#FOR PARALLEL
				if ($CASCADE_TYPE == 'parallel' AND $CASCADE_SOUCE_DIRECTION == TRUE)
				{				
					foreach ($CASCADE_TRANSPORT as $CASCADE_JOB_ID => $CASCADE_TRANSPORT_INFO)
					{
						$CASCADE[$CASCADE_JOB_ID] = new saasame\transport\cascading(array('level' => 1, 'machine_id' => $CASCADE_TRANSPORT_INFO['target_machine_id'], 'connection_info' => $TARGET_CONNECTION));
					}				
					$LOADER_JOB_DETAIL['cascadings'] = new saasame\transport\cascading(array('level' => 0, 'branches' => $CASCADE));
				}
				
				#FOR CASCADE
				if ($CASCADE_TYPE == 'cascaded')
				{				
					$REPLICA_LEVEL = $CASCADE_TRANSPORT[$CASCADE_INFO['replica']];
					$LOADER_JOB_DETAIL['cascadings'] = new saasame\transport\cascading(array('level' => $REPLICA_LEVEL['job_order'] - 1, 'machine_id' => $REPLICA_LEVEL['target_machine_id'], 'connection_info' => $SOURCE_CONNECTION));
					
					#UNSET SOURCE REPLICA JOB
					unset($CASCADE_TRANSPORT[$CASCADE_INFO['replica']]);
					
					foreach ($CASCADE_TRANSPORT as $CASCADE_JOB_ID => $CASCADE_TRANSPORT_INFO)
					{
						$CASCADE_SUB_DETAIL[$CASCADE_JOB_ID] = new saasame\transport\cascading(array('level' => $CASCADE_TRANSPORT_INFO['job_order'] - 1, 'machine_id' => $CASCADE_TRANSPORT_INFO['target_machine_id'], 'connection_info' => $SOURCE_CONNECTION));
						$LOADER_JOB_DETAIL['cascadings'] -> branches = $CASCADE_SUB_DETAIL;
					}
				}
			}
		
			#FOR DEBUG
			Misc_Class::function_debug($LOG_LOCATION,__FUNCTION__,$LOADER_JOB_DETAIL);

			return new saasame\transport\loader_job_create_detail($LOADER_JOB_DETAIL);			
		}
		else
		{
			throw new \saasame\transport\invalid_operation(array('what_op'=>0x00003002, 'why'=>'Job '.$REPL_UUID.' does not found.'));
		}
	}

	
	######################################################
	#
	#	UPDATE LOADER JOB STATE
	#
	######################################################
	public function update_loader_job_state($SESSION_ID, \saasame\transport\loader_job_detail $REPL_STATUS)
	{
		#QUERY REPL UUID
		$REPL_UUID = $REPL_STATUS -> replica_id;
		
		#UPDATE TIME
		$UPDATE_TIME = $REPL_STATUS -> updated_time;
		
		#CHECK REPLICA
		$REPL_QUERY = $this -> ReplMgmt -> query_replica($REPL_UUID);
		
		#GET REPLICA JOB INFORMATION
		$JOB_INFO = $REPL_QUERY['JOBS_JSON'];
			
		if ($REPL_QUERY != FALSE)
		{		
			#LOG LOCATION
			$LOG_LOCATION = $REPL_QUERY['LOG_LOCATION'];
			
			#QUERY AND UPDATE HISTORY ARRAY
			$REPL_HIST = $REPL_STATUS -> histories;
			$this -> parser_job_history($REPL_UUID,$REPL_HIST,$UPDATE_TIME,'Replica');
			
			#QUERY AND UPDATE LOADER PROGRESS
			$SNAP_UUID 		= $REPL_STATUS -> snapshot_id;
			$DISK_DATA 		= $REPL_STATUS -> data;
			$TRANSPORT_DATA = $REPL_STATUS -> transport_data;
			$this -> update_loader_progress_data($REPL_UUID,$SNAP_UUID,$DISK_DATA,$TRANSPORT_DATA);
			
			#FOR DEBUG
			Misc_Class::function_debug($LOG_LOCATION,__FUNCTION__,$REPL_STATUS);
		}
		else
		{
			return false;
		}
	}
		
	######################################################
	#
	#	CHECK SNAPSHOTS
	#
	######################################################
	public function check_snapshots($SESSION_ID, $SNAPSHOT_UUID)
	{
		$SNAP_CHECK = $this -> check_repl_snapshot($SESSION_ID, $SNAPSHOT_UUID);

		return $SNAP_CHECK;
	}
	
	
	######################################################
	#
	#	IS LOADER JOB DEVICES READY
	#
	######################################################	
	public function is_loader_job_devices_ready($SESSION_ID, $JOB_ID)
	{
		return true;
	}
	
	######################################################
	#
	#	MOUNT LOADER JOB DEVICES
	#
	######################################################	
	public function mount_loader_job_devices($SESSION_ID, $JOB_ID)
	{
		return true;
	}
	
	######################################################
	#
	#	DISMOUNT LOADER JOB DEVICES
	#
	######################################################	
	public function dismount_loader_job_devices($SESSION_ID, $JOB_ID)
	{
		return true;
	}
	
	######################################################
	#
	#	DISCARD SNAPSHOTS
	#
	######################################################	
	public function discard_snapshots($SESSION_ID, $SNAPSHOTS_ID)
	{
		#LOG DISCARD SNAPSHOTS
		$SNAPSHOT_INFO = $this -> ReplMgmt -> query_replica_snapshot_with_machine_id($SESSION_ID, $SNAPSHOTS_ID);
		if ($SNAPSHOT_INFO != FALSE)
		{	
			$REPL_UUID   = $SNAPSHOT_INFO[0]['REPL_UUID'];
			$REPL_INFO = $this -> ReplMgmt -> query_replica($REPL_UUID);
			$LOG_LOCATION = $REPL_INFO['LOG_LOCATION'];
			Misc_Class::function_debug($LOG_LOCATION,__FUNCTION__,$SNAPSHOTS_ID);
		}
		
		$DISCARD_SNAPSHOTS = 'UPDATE _REPLICA_SNAP
							  SET
								_SNAP_OPEN = "X"
							  WHERE
								JSON_CONTAINS(_SNAP_OPTIONS, \'"'.$SESSION_ID.'"\', \'$.machine_id\') AND
								_SNAP_UUID = "'.$SNAPSHOTS_ID.'"';
							
		$this -> DBCON -> prepare($DISCARD_SNAPSHOTS) -> execute();
		return true;
	}

	######################################################
	#
	#	TAKE SNAPSHOTS
	#
	######################################################
	public function take_snapshots($SESSION_ID, $SNAPSHOT_UUID)
	{
		//Misc_Class::function_debug('_mgmt',__FUNCTION__,func_get_args());
		
		if ($SESSION_ID != '')
		{
			$SNAPSHOT_INFO = $this -> ReplMgmt -> query_replica_snapshot_with_machine_id($SESSION_ID,$SNAPSHOT_UUID);
		}
		else
		{
			$SNAPSHOT_INFO = $this -> ReplMgmt -> query_replica_snapshot_by_uuid($SNAPSHOT_UUID);
		}
	
		if ($SNAPSHOT_INFO != FALSE)
		{
			#GET REPL INFO
			$REPL_UUID   = $SNAPSHOT_INFO[0]['REPL_UUID'];
			$REPL_INFO = $this -> ReplMgmt -> query_replica($REPL_UUID);
			
			#LOG LOCATION
			$LOG_LOCATION = $REPL_INFO['LOG_LOCATION'];
			
			#GET JOB INFORMATION
			$JOB_INFO = $REPL_INFO['JOBS_JSON'];
			$NUMBER_SNAPSHOT = $JOB_INFO -> snapshot_rotation;
			
			if ($REPL_INFO["WINPE_JOB"] == 'N' AND $NUMBER_SNAPSHOT != -1)
			{
				$CLOUD_MGMT  = $this -> get_cloud_auth_item($REPL_UUID,'');
				$CLOUD_UUID  = $CLOUD_MGMT -> CLUSTER_UUID;
				$HOST_NAME   = $CLOUD_MGMT -> HOST_NAME;
				$BACKUP_SIZE = 0;
				$LAUN_UUID  = $CLOUD_MGMT -> LAUN_UUID;
				
				$SERV_INFO = explode("|",$CLOUD_MGMT -> LOAD_OPEN);
				if (isset($SERV_INFO[1]))
				{
					$ZONE_INFO = $SERV_INFO[1];
				}
				else
				{
					$ZONE_INFO = 'Jupiter';
				}
				
				#SWITCH AZURE TO AZUREBLOB ON CLOUD TYPE
				if ($JOB_INFO -> is_azure_blob_mode == TRUE)
				{
					$CLOUD_MGMT -> CLOUD_TYPE = 'AzureBlob';
				}				
				
				for ($i=0; $i<count($SNAPSHOT_INFO); $i++)
				{
					$SNAP_OPEN = $SNAPSHOT_INFO[$i]['SNAP_OPEN'];

					if ($SNAP_OPEN == 'N')
					{
						$DISK_UUID   	= $SNAPSHOT_INFO[$i]['DISK_UUID'];
						$BACKUP_SIZE 	= $SNAPSHOT_INFO[$i]['BACKUP_SIZE'] + $BACKUP_SIZE;
						$DISK_INFO   	= $this -> ReplMgmt -> query_replica_disk_by_uuid($REPL_UUID,$DISK_UUID);
						$VOLUME_ID   	= $DISK_INFO['OPEN_DISK'];
						$SNAP_NAME 		= $HOST_NAME.'_'.$i;
						$SNAP_TIME   	= $SNAPSHOT_INFO[$i]['SNAP_TIME'];

						###############################
						#TAKE SNAPSHOT
						###############################
						switch ($CLOUD_MGMT -> CLOUD_TYPE)
						{
							case 'OPENSTACK':
								$TAKE_SNAPSHOT[$REPL_UUID] = $this -> OpenStackMgmt -> take_snapshot($CLOUD_UUID,$REPL_UUID,$VOLUME_ID,$SNAP_NAME,$SNAP_TIME);
							break;
							
							case 'AWS':
								$TAKE_SNAPSHOT[$REPL_UUID] = $this -> AwsMgmt -> take_snapshot($CLOUD_UUID,$ZONE_INFO,$VOLUME_ID,$SNAP_NAME,$SNAP_TIME);
							break;
							
							case 'Azure':
								$this->AzureMgmt->SetReplicaId( $REPL_UUID );
								$this->AzureMgmt->getVMByServUUID($LAUN_UUID,$CLOUD_UUID);
								$TAKE_SNAPSHOT[$REPL_UUID] = $this -> AzureMgmt -> create_disk_snapshot($CLOUD_UUID,$ZONE_INFO,$VOLUME_ID,$SNAP_NAME,$SNAP_TIME);
							break;
							
							case 'AzureBlob':
								#SKIP NO USE
							break;
							
							case 'Aliyun':
								$TAKE_SNAPSHOT[$REPL_UUID] = $this -> AliMgmt -> create_disk_snapshot($CLOUD_UUID,$ZONE_INFO,$VOLUME_ID,$SNAP_NAME,$SNAP_TIME);
							break;

							case 'Tencent':
								$TAKE_SNAPSHOT[$REPL_UUID] = $this -> TencentMgmt -> create_disk_snapshot($CLOUD_UUID,$ZONE_INFO,$VOLUME_ID,$SNAP_NAME,$SNAP_TIME);
							break;
							
							case 'Ctyun':
								$TAKE_SNAPSHOT[$REPL_UUID] = $this -> CtyunMgmt -> take_snapshot($CLOUD_UUID,$VOLUME_ID,$SNAP_NAME,$SNAP_TIME);
							break;
						}
						
						###############################
						#SNAPSHOT ROTATION CONTROL
						###############################
						if ($NUMBER_SNAPSHOT != 0)
						{
							switch ($CLOUD_MGMT -> CLOUD_TYPE)
							{
								case 'OPENSTACK':
									$this -> OpenStackMgmt -> snapshot_control($CLOUD_UUID,$VOLUME_ID,$NUMBER_SNAPSHOT);
								break;
								
								case 'AWS':
									$this -> AwsMgmt -> snapshot_control($CLOUD_UUID,$ZONE_INFO,$VOLUME_ID,$NUMBER_SNAPSHOT);
								break;
								
								case 'Azure':
									$this -> AzureMgmt -> getVMByServUUID($LAUN_UUID,$CLOUD_UUID);
									$this -> AzureMgmt -> snapshot_control($CLOUD_UUID,$ZONE_INFO,$VOLUME_ID, $NUMBER_SNAPSHOT);
								break;
								
								case 'AzureBlob':
									$this -> AzureBlobMgmt -> snapshot_control($REPL_UUID,$VOLUME_ID,$NUMBER_SNAPSHOT);
								break;
								
								case 'Aliyun':
									$this -> AliMgmt->snapshot_control($CLOUD_UUID,$ZONE_INFO,$VOLUME_ID, $NUMBER_SNAPSHOT);
								break;								

								case 'Tencent':
									$this -> TencentMgmt->snapshot_control($CLOUD_UUID,$ZONE_INFO,$VOLUME_ID, $NUMBER_SNAPSHOT);
								break;

								case 'Ctyun':
									$this -> CtyunMgmt -> snapshot_control($CLOUD_UUID,$VOLUME_ID,$NUMBER_SNAPSHOT);
								break;
								
								case 'VMWare':
									$this -> VMWareMgmt -> snapshot_control($REPL_UUID,$NUMBER_SNAPSHOT);
								break;
							}
						}
					}
				}		
				
				#RENEW DISK CUSTOMIZED UUID
				$this -> renew_disk_customized_uuid($REPL_UUID);
				
				if( isset($TAKE_SNAPSHOT) ){
					#FOR DEBUG
					Misc_Class::function_debug($LOG_LOCATION,__FUNCTION__,$TAKE_SNAPSHOT);
				}
				
				#REPORT DATA SIZE
				$REPORT_DATA = $this -> ServiceMgmt -> replica_data_transfer('DataTransfer',$REPL_UUID,$BACKUP_SIZE);
				$REPORT_RESPONSE = Misc_Class::transport_report($REPORT_DATA);
			}

			#SET REPAIRED REPLCIA FLAG
			$JOB_INFO -> repair_sync_mode = 'full';

			#SET JOB OFF EXECUTING AND ENABLE RESUME
			$JOB_INFO -> is_resume = false;
			$JOB_INFO -> is_executing = false;
		
			$this -> ServiceMgmt -> update_trigger_info($REPL_UUID,$JOB_INFO,'REPLICA');
						
			$this -> update_repl_snapshot_status($REPL_UUID,$SNAPSHOT_UUID,'Y');
		
			return true;
		}
		else
		{
			return false;
		}
	}
	
	######################################################
	#
	#	GET LAUNCHER JOB CREATE DETAIL
	#
	######################################################
	public function get_launcher_job_create_detail($SESSION_ID,$SERVICE_UUID)
	{
		$SERV_DISK   = $this -> ServiceMgmt -> query_service_disk($SERVICE_UUID);

		if ($SERV_DISK != FALSE)
		{
			#QUERY SERVICE INFORMATION
			$SERV_INFO = $this -> ServiceMgmt -> query_service($SERVICE_UUID);
	
			#GET OS TYPE ENUM
			$OS_TYPE = $this -> os_type_transfer_to_enum($SERV_INFO["OS_TYPE"]);

			#HOST NAME
			$HOST_NAME = $SERV_INFO['HOST_NAME'];
			
			#LOG LOCATION
			$LOG_LOCATION = $SERV_INFO['LOG_LOCATION'];
			
			#SERVICE JOB JSON
			$JOB_INFO = json_decode($SERV_INFO['JOBS_JSON'],false);
			
			#SET CALLBACK URL
			$ACCT_UUID = $SERV_INFO['ACCT_UUID'];
			$REPL_UUID = $SERV_INFO['REPL_UUID'];			
			
			#DEFINE CALLBACK ADDRESS
			$URL_INFO = $this -> ServiceMgmt -> query_multiple_mgmt_address($ACCT_UUID,$REPL_UUID);
			
			#DEFINE CALLBACK PORT
			$VERIFY_MGMT_COMM = Misc_Class::mgmt_comm_type('verify');
			$CALLBACK_PORT = $VERIFY_MGMT_COMM['mgmt_port'];
			
			#DEFINE CALLBACK AND SCRIPT URLs
			$PRE_SCRIPT_URI = '';
			$POST_SCRIPT_URI = '';
			
			#DEFINE PRE/POST FILE URL
			$PRE_FILE_URL = array();
			$POST_FILE_URL = array();
			
			foreach ($URL_INFO as $Key => $Value)
			{
				$URL = 'https://'.$Key.':'.$CALLBACK_PORT.'/restful/ServiceManagement?ServiceCallback='.$SERVICE_UUID;
				$CALLBACK_URL[$URL] = $Value;
			
				if (isset($JOB_INFO -> rcvy_pre_script))
				{
					if ($JOB_INFO -> rcvy_pre_script != '')
					{
						$PRE_SCRIPT_URL = 'https://'.$Key.':'.$CALLBACK_PORT.'/restful/ServiceManagement?DownloadFile='.$JOB_INFO -> rcvy_pre_script;
						$PRE_FILE_URL[$PRE_SCRIPT_URL] = $Value;				
					}
				
					if ($JOB_INFO -> rcvy_post_script != '')
					{
						$POST_SCRIPT_URL = 'https://'.$Key.':'.$CALLBACK_PORT.'/restful/ServiceManagement?DownloadFile='.$JOB_INFO -> rcvy_post_script;
						$POST_FILE_URL[$POST_SCRIPT_URL] = $Value;								
					}
				}
			}

			#GET REPLICA INFO
			$REPL_QUERY = $this -> ReplMgmt -> query_replica($REPL_UUID);
			
			#GET REPLICA JOB INFORMATION
			$REPL_JOB_INFO     = $REPL_QUERY['JOBS_JSON'];
			$EXPORT_TYPE       = $REPL_JOB_INFO -> export_type;
			$EXPORT_PATH       = $REPL_JOB_INFO -> export_path;
			$TARGET_TYPE       = $JOB_INFO -> convert_type;
			$AZURE_CONN_STRING = '';

			#DISK ARRAY SLICE FOR MS TYPE
			/*if ($SERV_INFO["OS_TYPE"] == 'MS')
			{
				$BOOT_DISK_ID = $JOB_INFO -> boot_disk_id;
				$SERV_DISK = array_slice($SERV_DISK, $BOOT_DISK_ID, $BOOT_DISK_ID+1); 
			}*/
			
			$d_array = array();
			
			for ($i=0; $i<count($SERV_DISK); $i++)
			{
				array_push( $d_array, $SERV_DISK[$i]['DISK_UUID'] );
				
				$DISK_UUID = $SERV_DISK[$i]['DISK_UUID'];
				$SCSI_ADDR = $SERV_DISK[$i]['SCSI_ADDR'];
				$OPEN_DISK = $SERV_DISK[$i]['OPEN_DISK'];
				
				#NEW VOL ID FOR AWS
				if (strpos($OPEN_DISK, 'vol-') === 0){$OPEN_DISK = str_replace("vol-","vol",$OPEN_DISK);}

				#NEW ID FOR ALIYUN
				if (strpos($OPEN_DISK, 'd-') === 0){$OPEN_DISK = substr($OPEN_DISK,strlen('d-'));}
					
				#DETECT MAP TYPE
				if ($EXPORT_PATH == '')
				{
					if ($SCSI_ADDR != 'L:O:C:K')
					{
						#DETECT MAP TYPE
						if($SERV_INFO['CLOUD_TYPE'] == 'VMWare')
						{
							$DETECT_TYPE = 6;
							$LUN_MAPS[$SCSI_ADDR] = $SCSI_ADDR;
						}
						elseif (substr_count($SCSI_ADDR, ':') == 3 or substr_count($SCSI_ADDR, '/') == 2)
						{
							$DETECT_TYPE = 0;
							$LUN_MAPS[$DISK_UUID] = $SCSI_ADDR;
						}
						elseif ($SERV_INFO['FLAVOR_ID'] == "00000000-RECOVERY-000-KIT-0000000000")
						{
							$DETECT_TYPE = (substr_count($SCSI_ADDR, '-') == 4)?4:1;
							$LUN_MAPS[$DISK_UUID] = $SCSI_ADDR;
						}
						elseif (strlen($SCSI_ADDR) == 36)
						{
							$DETECT_TYPE = 4;
							$LUN_MAPS[$DISK_UUID] = $SCSI_ADDR;
						}
						elseif (strpos($OPEN_DISK, '.vhd') != 0)
						{
							$DETECT_TYPE = 5;
							$LUN_MAPS[$DISK_UUID] = $OPEN_DISK;
							
							#OVERWRITE DEFAULT OPTIONS
							$EXPORT_PATH = $REPL_UUID;
							$AZURE_CONN_STRING = $this -> AzureBlobMgmt -> get_blob_connection_string($REPL_UUID) -> ConnectionString;
						}
						else
						{
							$DETECT_TYPE = 1;
							$LUN_MAPS[$DISK_UUID] = $OPEN_DISK;
						}
					}
				}
				else
				{
					$DETECT_TYPE = 2;
					$LUN_MAPS[$DISK_UUID] = $SCSI_ADDR;
				}
			}

			
			$LUN_MAPS_MERGE = array_merge($LUN_MAPS);
			$LAUNCHER_JOB_DETAIL = array(
										'replica_id'						 => $REPL_UUID,
										'disks_lun_mapping'					 => $LUN_MAPS_MERGE,
										'is_sysvol_authoritative_restore' 	 => false,
										'is_enable_debug'					 => false,
										'is_disable_machine_password_change' => false,
										'is_force_normal_boot'				 => false,
										'gpt_to_mbr'						 => true,
										'reboot_winpe'						 => $JOB_INFO -> auto_reboot,
										'detect_type'						 => $DETECT_TYPE,
										'callbacks'						 	 => $CALLBACK_URL,
										'host_name'							 => $HOST_NAME,
										'callback_timeout'					 => 60,
										'os_type'							 => $OS_TYPE,
										'pre_scripts'						 => $PRE_FILE_URL,
										'post_scripts'						 => $POST_FILE_URL,
										'export_disk_type'					 => $EXPORT_TYPE,
										'export_path'						 => $EXPORT_PATH,
										'target_type'						 => $TARGET_TYPE,
										'azure_storage_connection_string'	 => $AZURE_CONN_STRING
									);
			
			if( $JOB_INFO->recovery_type == "DevelopmentTesting" )
				$LAUNCHER_JOB_DETAIL["mode"] = \saasame\transport\recovery_type::TEST_RECOVERY;
			else if( $JOB_INFO->recovery_type == "DisasterRecovery" )
				$LAUNCHER_JOB_DETAIL["mode"] = \saasame\transport\recovery_type::DISASTER_RECOVERY;
			else if( $JOB_INFO->recovery_type == "PlannedMigration" )
				$LAUNCHER_JOB_DETAIL["mode"] = \saasame\transport\recovery_type::MIGRATION_RECOVERY;
			
			#FOR ALI YUN IMAGE UPLOAD
			$ServiceInfo = $this -> ServiceMgmt -> getCloudTypeByServiceId($SERVICE_UUID);
			if (count($ServiceInfo) != 0)
			{
				if($ServiceInfo[0]["CLOUD"] == 'Aliyun' && $this->MgmtConfig->{"alibaba_cloud"}->{"recover_mode"} == 1 ||
				($ServiceInfo[0]["CLOUD"] == 'Tencent') )
				{
					$user = Misc_Class::encrypt_decrypt('decrypt',$ServiceInfo[0]["_CLUSTER_USER"]);
					$pass = Misc_Class::encrypt_decrypt('decrypt',$ServiceInfo[0]["_CLUSTER_PASS"]);
			
					if(($ServiceInfo[0]["CLOUD"] == 'Aliyun' && $this->MgmtConfig->{"alibaba_cloud"}->{"recover_mode"} == 1) ||	($ServiceInfo[0]["CLOUD"] == 'Tencent'))
					{
						$CONN_INFO = $this->ServerMgmt->query_connection_info( $ServiceInfo[0]["_CONN_UUID"] );
						$LAUN_INFO = explode('|',$CONN_INFO['LAUN_OPEN']);
						$SERVER_REGION = $LAUN_INFO[1];
				
						$AliModel = new Aliyun_Model();
						$cloud_info = $AliModel->query_cloud_connection_information( $CONN_INFO["CLUSTER_UUID"] );
	
						//$bucketName = "saasame-".$SERVER_REGION.'-'.$CONN_INFO["CLUSTER_UUID"];
						//$this->AliMgmt->VerifyOss( $CONN_INFO["CLUSTER_UUID"], $SERVER_REGION, $bucketName, false );
				
						if( $ServiceInfo[0]["CLOUD"] == 'Aliyun' )
						{
							$bucketName = "saasame-".$SERVER_REGION.'-'.md5( $cloud_info['ACCESS_KEY'] );
							$uploadInfo =  array(
												"access_key" 	=> $user,
												"secret_key" 	=> $pass,
												"objectname" 	=> preg_replace( "/[^a-zA-Z0-9_.-]/", "", $ServiceInfo[0]["_HOST_NAME"].$SERVICE_UUID ),
												"bucketname" 	=> $bucketName,
												"region" 		=> $this->AliMgmt->endpointCheck( $SERVER_REGION ),
												"number_of_upload_threads" => 5
							);
							$LAUNCHER_JOB_DETAIL['options_type'] = 1;
					
							$cloud_array = array("aliyun" => new saasame\transport\aliyun_options($uploadInfo));
						} 
						else if($ServiceInfo[0]["CLOUD"] == 'Tencent') 
						{					
							$bucketName = "saasame-".$SERVER_REGION;
							$uploadInfo =  array(
												"access_key" 	=> $user,
												"secret_key" 	=> $pass,
												"objectname" 	=> preg_replace( "/[^a-zA-Z0-9_.-]/", "", $ServiceInfo[0]["_HOST_NAME"].$SERVICE_UUID ),
												"bucketname" 	=> $bucketName.'-'.$cloud_info['USER_UUID'],
												//"bucketname" 	=> $bucketName,
												"region" 		=> $this->TencentMgmt->getCurrectRegion( $SERVER_REGION ),
												"number_of_upload_threads" => 5
											);
							$LAUNCHER_JOB_DETAIL['options_type'] = 2;

							$cloud_array = array("tencent" => new saasame\transport\tencent_options($uploadInfo));
						}
					}
					
					$LAUNCHER_JOB_DETAIL['options'] = new saasame\transport\extra_options($cloud_array);
				}
			}
						
			#UPDATE JOB STATUS
			$JOB_INFO -> job_status = 'TransportJobReceived';
			$this -> ServiceMgmt -> update_trigger_info($SERVICE_UUID,$JOB_INFO,'SERVICE');
			
			if (count($ServiceInfo) != 0 AND $ServiceInfo[0]["CLOUD"] == 'VMWare')
			{
				$this->VMWareLauncherConfig( $d_array, $SERVICE_UUID, $REPL_UUID, $LAUNCHER_JOB_DETAIL );
			}
			
			Misc_Class::function_debug($LOG_LOCATION,__FUNCTION__,$LAUNCHER_JOB_DETAIL);
			
			return new saasame\transport\launcher_job_create_detail($LAUNCHER_JOB_DETAIL);
		}
		else
		{
			throw new \saasame\transport\invalid_operation(array('what_op'=>0x00003002, 'why'=>'Job '.$SERVICE_UUID.' is not found.'));
		}
	}
	
	private function VMWareLauncherConfig( $d_array, $SERVICE_UUID, $REPL_UUID, &$LAUNCHER_JOB_DETAIL ){
			
		$VMWare = new VM_Ware();
				
		$RecoveryInfo = $VMWare->getRecoveryInfo( $SERVICE_UUID );
 
		if( $RecoveryInfo["OSTYPE"] == "-1" )
			$osinfo = array("guestId"=>$RecoveryInfo["guest_id"]);
		else
			$osinfo = $VMWare->getOSFromHostInfo( $RecoveryInfo["OSTYPE_DISPLAY"] );
		
		$mac 		= array();
		
		if( $RecoveryInfo["networkSetting"] != null ){
			end( $RecoveryInfo["networkSetting"] );
			$lastkey = key( $RecoveryInfo["networkSetting"] );
		}
		else
			$lastkey = 0;
		
		$networkConfigs = array();
		
		for( $i = 0; $i <= $lastkey ; $i++ ){
			if( $RecoveryInfo["Convert"] == "true" && isset($RecoveryInfo["networkSetting"][$i]))
			{
				array_push($mac, $RecoveryInfo["networkSetting"][$i]["mac"]);
				
				$network = array(
					"ip_addresses"	=> array( $RecoveryInfo["networkSetting"][$i]["ip"] ),
					"subnet_masks"	=> array( $RecoveryInfo["networkSetting"][$i]["subnet"] ),
					"gateways"		=> array( $RecoveryInfo["networkSetting"][$i]["gateway"] ),
					"dnss"			=> array( $RecoveryInfo["networkSetting"][$i]["dns"] ),
					"is_dhcp_v4"	=> false,
					"mac_address"	=> $RecoveryInfo["networkSetting"][$i]["mac"]
				);
				
				$networkConfig = new saasame\transport\network_info( $network );
				
				array_push($networkConfigs, $networkConfig);
		
			}
			else{
				array_push($mac, "");
			}
		}
		
		$vm_ware_info = array(
			"host" => $RecoveryInfo["EsxInfo"]["EsxIp"],
			"username" => $RecoveryInfo["EsxInfo"]["Username"],
			"password" => $RecoveryInfo["EsxInfo"]["Password"],
			"esx" => $RecoveryInfo["EsxName"],
			"datastore" => $RecoveryInfo["datastore"],
			"folder_path" => $RecoveryInfo["folder_path"]
		);

		$vm_connetion = new saasame\transport\vmware_connection_info( $vm_ware_info );	
		
		if( $RecoveryInfo["Snapshot"] == "UseLastSnapshot" ){
			$snapshotList = $this->VMWareMgmt->getSnapshotList( $REPL_UUID );
			
			$RecoveryInfo["Snapshot"] = $snapshotList[0]["name"];
		}
		
		$recovery = array(
			"connection" 				=> $vm_connetion,
			"virtual_machine_id"		=> $RecoveryInfo["MachineId"],
			"virtual_machine_snapshot"	=> $RecoveryInfo["Snapshot"],
			"number_of_cpus"			=> $RecoveryInfo["CPU"],
			"number_of_memory_in_mb"	=> $RecoveryInfo["Memory"],
			"network_connections"		=> $RecoveryInfo["network"],
			"vm_name"					=> $RecoveryInfo["vmName"],
			"network_adapters"			=> $RecoveryInfo["networkAdapter"],
			"scsi_adapters"				=> array($RecoveryInfo["SCSI_CONTROLLER"] => $d_array ),
			"guest_id"					=> $osinfo["guestId"],
			"firmware"					=> $RecoveryInfo["firmware"],
			"install_vm_tools"			=> $RecoveryInfo["VMTool"] == "true"? true:false,
			"mac_addresses"				=> $mac
		);

		$vmware = new saasame\transport\vmware_options( $recovery );
		
		$LAUNCHER_JOB_DETAIL["vmware"] = $vmware;
		
		$LAUNCHER_JOB_DETAIL["skip_system_injection"] = $RecoveryInfo["Convert"] == "true"? false:true;
		
		/*$network = array(
			"is_dhcp_v4"	=> $ip,
			"subnet_masks"	=> $subnet,
			"gateways"		=> $gateway,
			"dnss"			=> $dns,
			"mac_address"	=> $mac
		);
		
		$networkConfig = new saasame\transport\network_info( $network );*/
		
		$LAUNCHER_JOB_DETAIL["network_infos"] = $networkConfigs;
		
		$LAUNCHER_JOB_DETAIL["detect_type"] = \saasame\transport\disk_detect_type::VMWARE_VADP;
	}
	
	##########################
	# OS type transfer to enum
	##########################
	private function os_type_transfer_to_enum($os) 
	{
		switch($os)
		{
			case "MS":
				$type = 1;
				break;
		
			case "LX":
				$type = 2;
				break;
			
			default:
				$type = 0;
				break;
		}		
		return $type;
	}	
	
	######################################################
	#
	#	UPDATE LAUNCHER JOB STATE
	#
	######################################################
	public function update_launcher_job_state($SESSION_ID, \saasame\transport\launcher_job_detail $SERV_STATUS)
	{
		#GET SERVICE UUID
		$JOBS_UUID = $SERV_STATUS -> replica_id;
		$SERV_UUID = $SERV_STATUS -> id;
		
		#UPDATE TIME
		$UPDATE_TIME = $SERV_STATUS -> updated_time;
		
		#GET SERVICE INFORMATION
		$SERV_QUERY  = $this -> ServiceMgmt -> query_service($SERV_UUID);
		$SERV_DISK   = $this -> ServiceMgmt -> query_service_disk($SERV_UUID);
	
		#LOG LOCATION
		$LOG_LOCATION = $SERV_QUERY['LOG_LOCATION'];
		
		#FOR DEBUG
		Misc_Class::function_debug($LOG_LOCATION,__FUNCTION__,$SERV_STATUS);		
			
		#UUID INFORMATION
		$REPL_UUID 	  = $SERV_QUERY['REPL_UUID'];
		$NOVA_VM_UUID = $SERV_QUERY['NOVA_VM_UUID'];
		$OS_TYPE	  = $SERV_QUERY['OS_TYPE'];
		
		#GET HISTORY ARRAY
		$SERV_HIST = $SERV_STATUS -> histories;
		
		#UPDATE SERVICE HISTORY		
		$this -> parser_job_history($SERV_UUID,$SERV_HIST,$UPDATE_TIME,'Service');
		
		#GET SERVICE JOB JSON
		$JOB_INFO = json_decode($SERV_QUERY['JOBS_JSON'],false);
		
		#IMAGE ID
		$IMAGE_ID = $SERV_QUERY['IMAGE_ID'];
		
		#HOST NAME
		$HOST_NAME = $SERV_STATUS -> host_name;
		
		#GET CLOUD INFOMATION
		$CLOUD_MGMT = $this -> get_cloud_auth_item($REPL_UUID,$HOST_NAME);
		
		#ENUM RECOVERY JOB TYPE	
		$RECOVERY_TYPE = $JOB_INFO -> recovery_type;
	
		#GET STATUS CODE
		$STATUS_CODE = $SERV_STATUS -> state;
	
		if( $CLOUD_MGMT -> CLOUD_TYPE == "VMWare" && $JOB_INFO -> virtual_machine_id == "" ){
			$Common_Model = new Common_Model();
			
			$Common_Model->updateMachineIdInServiceTable( $SERV_STATUS->virtual_machine_id, $SERV_UUID);
			
		}
		
		#BEGIN TO RUN RECOVERY JOB
		switch ($STATUS_CODE)
		{
			case \saasame\transport\job_state::job_state_uploading:
				$this -> recovery_image_upload($SERV_STATUS,$JOB_INFO,$SERV_UUID);
			break;
			
			case \saasame\transport\job_state::job_state_upload_completed:
				$this -> recovery_image_process($CLOUD_MGMT,$SERV_UUID,$IMAGE_ID,$SERV_STATUS);
			break;
			
			case \saasame\transport\job_state::job_state_finished:

				$this->hostname = $SERV_STATUS -> host_name;

				if( $CLOUD_MGMT -> CLOUD_TYPE == "VMWare"){
					$JOB_INFO -> job_status = 'InstanceCreated';
					$this -> ServiceMgmt -> update_trigger_info($SERV_UUID,$JOB_INFO,'SERVICE');	
					break;
				}
				
				if ($NOVA_VM_UUID == '')
				{	
					#UPDATE TEMP LOCK
					$this -> ServiceMgmt -> update_service_vm_info($SERV_UUID,'TEMP_LOCK','LOCK_TEMP');
			
					switch ($RECOVERY_TYPE)
					{
						case 'PlannedMigration':
						case 'DisasterRecovery':
						case 'DevelopmentTesting':
							#MESSAGE
							$MESSAGE = $this -> ReplMgmt -> job_msg('Preparing instances.');
							$this -> ReplMgmt -> update_job_msg($SERV_UUID,$MESSAGE,'Service');	
							sleep(1);
							
							$this -> standard_cloud_recovery($SERV_UUID,$CLOUD_MGMT,$OS_TYPE,$RECOVERY_TYPE,$SERV_DISK,$JOB_INFO);
						break;
						
						case 'RecoveryKit':
							$this -> recovery_kit_recovery($SERV_UUID,$SERV_QUERY['WINPE_JOB']);
						break;
						
						case 'ExportImage':
							$this -> export_image_recovery($SERV_UUID);
						break;
						
						default:
							#UPDATE TEMP LOCK FOR ERROR
							$this -> ServiceMgmt -> update_service_vm_info($SERV_UUID,'ERROR_LOCK','LOCK_ERROR');
						break;
					}
				}
			break;
		}
	
		#UPDATE LAUNCHER CONVERTED FAILED STATUS
		if ($SERV_STATUS -> is_error != '')
		{
			$JOB_INFO -> job_status = 'LauncherConvertFailed';
			$this -> ServiceMgmt -> update_trigger_info($SERV_UUID,$JOB_INFO,'SERVICE');
		}
	}

	
	###########################
	# RECOVERY IMAGE UPLOAD
	###########################
	private function recovery_image_upload($SERV_STATUS,$JOB_INFO,$SERV_UUID)
	{
		$persatage = $SERV_STATUS -> progress / $SERV_STATUS -> vhd_size * 100;
			
		if(!isset($JOB_INFO -> upload_persatage ))
		{			
			$JOB_INFO -> upload_persatage = 0;
		}
		
		$gap = 20;
		if($JOB_INFO -> upload_persatage <= $persatage)
		{
			$MESSAGE = $this -> ReplMgmt -> job_msg('Uploading...%1%',array(round($persatage,1).'%.'));
			$this -> ReplMgmt -> update_job_msg($SERV_UUID,$MESSAGE,'Service');
			
			$JOB_INFO -> upload_persatage = ( floor( $persatage / $gap ) + 1 ) * $gap;
			$this -> ServiceMgmt -> update_trigger_info($SERV_UUID,$JOB_INFO,'SERVICE');
		}
	}
	
	
	###########################
	# RECOVERY IMAGE PROCESS
	###########################
	private function recovery_image_process($CLOUD_MGMT,$SERV_UUID,$IMAGE_ID,$SERV_STATUS)
	{
		if (!isset($IMAGE_ID))
		{
			$this -> ServiceMgmt -> update_service_image_id($SERV_UUID,'image_id','');
	
			#GET CLOUD MGMT
			$CLUSTER_UUID = $CLOUD_MGMT -> CLUSTER_UUID;
			$SERV_INFO    = explode("|",$CLOUD_MGMT -> LAUN_OPEN);
			$SERVER_ZONE  = $SERV_INFO[1];
	
			#GET SERVICE INFO
			$ServiceInfo = $this -> ServiceMgmt -> getCloudTypeByServiceId($SERV_UUID);
			
			$AliModel = new Aliyun_Model();
			
			$cloud_info = $AliModel -> query_cloud_connection_information($CLUSTER_UUID);
		
			$objectname = preg_replace( "/[^a-zA-Z0-9_.-]/", "", $ServiceInfo[0]["_HOST_NAME"].$SERV_UUID);

			switch ($CLOUD_MGMT -> CLOUD_TYPE)
			{
				case "Aliyun":
					$bucketName = "saasame-".$SERVER_ZONE.'-'.md5( $cloud_info['ACCESS_KEY'] );
					$imageInfo = $this -> AliMgmt -> ImportImageFromOss( $CLUSTER_UUID, $SERVER_ZONE, $bucketName, $objectname, $SERV_STATUS);
				break;

				case "Tencent":
					$bucketName = "saasame-".$SERVER_ZONE ;
					$imageInfo = $this -> TencentMgmt -> ImportImageFromOss( $CLUSTER_UUID, $SERVER_ZONE, $bucketName, $objectname, $SERV_STATUS);
				
					if( $imageInfo == false )
					{
						$MESSAGE = $this -> ReplMgmt -> job_msg("Import image fail.");
						$this -> ReplMgmt -> update_job_msg($SERV_UUID,$MESSAGE,'Service');
						return false;
					}
				break;
			}
			
			$MESSAGE = $this -> ReplMgmt -> job_msg('Creating image from VHD.');
			$this -> ReplMgmt -> update_job_msg($SERV_UUID,$MESSAGE,'Service');
			$this -> ServiceMgmt-> update_service_image_id($SERV_UUID, $imageInfo -> {"ImageId"}, $imageInfo -> {"TaskId"});
		}
	}
	
	###########################
	# STANDARD CLOUD RECOVERY
	###########################
	private function standard_cloud_recovery($SERV_UUID,$CLOUD_MGMT,$OS_TYPE,$RECOVERY_TYPE,$SERV_DISK,$JOB_INFO)
	{
		#VOLUME MGMT
		$VOLUME_INFO = $this -> detach_cloud_disk($SERV_UUID,$CLOUD_MGMT,$OS_TYPE,$RECOVERY_TYPE,$SERV_DISK,$JOB_INFO);
		
		#INSTANCE MGMT
		$this -> create_instance($SERV_UUID,$VOLUME_INFO,$CLOUD_MGMT);
			
		$JOB_INFO -> job_status = 'InstanceCreated';
		$this -> ServiceMgmt -> update_trigger_info($SERV_UUID,$JOB_INFO,'SERVICE');
	}
	
	###########################
	# DETACH CLOUD DISK
	###########################	
	private function detach_cloud_disk($SERV_UUID,$CLOUD_MGMT,$OS_TYPE,$RECOVERY_TYPE,$SERV_DISK,$JOB_INFO)
	{
		//Misc_Class::function_debug('_mgmt',__FUNCTION__,func_get_args());
	
		#RESET DISK ORDER
		$BOOT_DISK_ID = 0;
		if (isset($JOB_INFO -> boot_disk_id))
		{		
			$BOOT_DISK_ID = $JOB_INFO -> boot_disk_id;
		}
		$SERV_DISK = array_values(array($BOOT_DISK_ID => $SERV_DISK[$BOOT_DISK_ID]) + $SERV_DISK);		
		
		#CLOUD INFORMATION
		$CLUSTER_UUID  = $CLOUD_MGMT -> CLUSTER_UUID;
		$LAUNCHER_UUID = $CLOUD_MGMT -> LAUN_UUID;
		$LAUNCHER_ADDR = $CLOUD_MGMT -> LAUN_ADDR;
		$LAUNCHER_SYST = $CLOUD_MGMT -> LAUN_SYST;
		$MGMT_ADDR	   = $CLOUD_MGMT -> MGMT_ADDR;
		
		#AZURE MGMT DISK
		$IS_AZURE_MGMT_DISK = $JOB_INFO -> is_azure_mgmt_disk;
		
		$SERVER_INFO   = explode("|",$CLOUD_MGMT -> LAUN_OPEN);
		if (isset($SERVER_INFO[1]))
		{
			$SERVER_UUID = $SERVER_INFO[0];
			$SERVER_ZONE = $SERVER_INFO[1];
		}
		else
		{
			$SERVER_UUID = $CLOUD_MGMT -> LAUN_OPEN;
			$SERVER_ZONE = 'Mercury';
		}

		#DETACH DISK FROM TRANSPORT SERVER
		$MESSAGE = $this -> ReplMgmt -> job_msg('Detaching the volume from the Transport server.');
		$this -> ReplMgmt -> update_job_msg($SERV_UUID,$MESSAGE,'Service');
		
		#MUTEX MSG
		$MUTEX_MSG = $SERV_UUID.'-'.$MGMT_ADDR.'-'.__FUNCTION__;
			
		#LOCK WITH MUTEX CONTROL
		$this -> ServiceMgmt -> disk_mutex_action($SERV_UUID,'Service',$LAUNCHER_ADDR,$MUTEX_MSG,'LOCK_EX',$LAUNCHER_SYST);
	
		$DETACH_DISK_ARRAY = array();
		for ($DISK=0; $DISK<count($SERV_DISK); $DISK++)
		{
			$VOLUME_ID = $SERV_DISK[$DISK]['OPEN_DISK'];
			$SCSI_ADDR = $SERV_DISK[$DISK]['SCSI_ADDR'];
			$DISK_SIZE = $SERV_DISK[$DISK]['DISK_SIZE'];
			
			if ($VOLUME_ID == $SCSI_ADDR){$CLOUD_MGMT -> CLOUD_TYPE = 'AzureBlobDirect';} #OVERWRITE AZURE BLOB DIRECT MODE
		
			switch ($CLOUD_MGMT -> CLOUD_TYPE)
			{
				case 'OPENSTACK':
					$this -> OpenStackMgmt -> detach_volume_from_server($CLUSTER_UUID,$SERVER_UUID,$VOLUME_ID);
					$VOLUME_INFO[] = array('uuid' => $VOLUME_ID,'source_type' => 'volume', 'destination_type' => 'volume', 'boot_index' => $DISK);
				break;
					
				case 'AWS':
					$this -> AwsMgmt -> detach_volume($CLUSTER_UUID,$SERVER_ZONE,$VOLUME_ID);
					$VOLUME_INFO[] = $VOLUME_ID;
				break;
					
				case 'Azure':
					array_push($DETACH_DISK_ARRAY,$VOLUME_ID);
					$VOLUME_INFO[] = $VOLUME_ID;
				break;
				
				case 'AzureBlobDirect':
					$VOLUME_INFO[] = $this -> AzureBlobMgmt -> detach_volume($CLUSTER_UUID,$LAUNCHER_UUID,$SERVER_ZONE,$SERV_UUID,$SCSI_ADDR,$DISK,$DISK_SIZE);
				break;
					
				case 'Aliyun':
					$this -> AliMgmt -> detach_volume($CLUSTER_UUID,$SERVER_ZONE,$SERVER_UUID,$VOLUME_ID);					
					$VOLUME_INFO[] = $VOLUME_ID;
				break;
					
				case 'Tencent':
					if ($RECOVERY_TYPE == 'Planned_Migration')
					{
						$this -> TencentMgmt -> detach_volume($CLUSTER_UUID,$SERVER_ZONE,$SERVER_UUID,$VOLUME_ID);
					}
					$VOLUME_INFO[] = $VOLUME_ID;			
				break;
				
				case 'Ctyun':
					$this -> CtyunMgmt -> detach_volume($CLUSTER_UUID,$VOLUME_ID);
					$VOLUME_INFO[] = $VOLUME_ID;
				break;
			}
		}

		if ($CLOUD_MGMT -> CLOUD_TYPE == 'Azure') 
		{
			$this -> AzureMgmt -> getVMByServUUID($LAUNCHER_UUID,$CLUSTER_UUID);
			if($IS_AZURE_MGMT_DISK == TRUE)
			{				
				$this -> AzureMgmt -> detach_volume($CLUSTER_UUID,$SERVER_ZONE,$SERVER_UUID,$DETACH_DISK_ARRAY);
			}
			else
			{
				$this -> AzureMgmt -> detach_vhd($CLUSTER_UUID,$SERVER_ZONE,$SERVER_UUID,$DETACH_DISK_ARRAY);
			}
		}
		
		#REWRITE BACK TO AZURE TYPE
		if ($CLOUD_MGMT -> CLOUD_TYPE == 'AzureBlobDirect'){$CLOUD_MGMT -> CLOUD_TYPE = 'Azure';}
		
		sleep(10);
		
		#LOCK WITH MUTEX CONTROL
		$this -> ServiceMgmt -> disk_mutex_action($SERV_UUID,'Service',$LAUNCHER_ADDR,$MUTEX_MSG,'LOCK_UN',$LAUNCHER_SYST);
		
		#UPDATE JOB STATUS
		$JOB_INFO -> job_status = 'ConvertedDiskDetached';
		$this -> ServiceMgmt -> update_trigger_info($SERV_UUID,$JOB_INFO,'SERVICE');
		
		$MESSAGE = $this -> ReplMgmt -> job_msg('The Volumes detached from the Transport server.');
		$this -> ReplMgmt -> update_job_msg($SERV_UUID,$MESSAGE,'Service');
			
		return $VOLUME_INFO;
	}
	
	###########################
	# CREATE INSTANCE
	###########################	
	private function create_instance($SERV_UUID,$VOLUME_INFO,$CLOUD_MGMT)
	{
		#QUERY SERVICE INFORMATION
		$SERV_INFO = $this -> ServiceMgmt -> query_service($SERV_UUID);
		
		#IMAGE ID
		$IMAGE_ID = $SERV_INFO['IMAGE_ID'];
		
		#LOG LOCATION
		$LOG_LOCATION = $SERV_INFO['LOG_LOCATION'];
		
		#SERVICE JOB
		$SERVICE_JOB = json_decode($SERV_INFO['JOBS_JSON']);
		
		#IS PROMOTE
		$IS_PROMOTE = $SERVICE_JOB -> is_promote;
		
		#IS AZURE MGMT DISK
		$IS_AZURE_MGMT_DISK = $SERVICE_JOB -> is_azure_mgmt_disk;
		
		#API REFERER ADDR
		$API_REFERER_FROM = $SERVICE_JOB -> api_referer_from;
		
		$MESSAGE = $this -> ReplMgmt -> job_msg('Creating an instance.');
		$this -> ReplMgmt -> update_job_msg($SERV_UUID,$MESSAGE,'Service');
		
		#CLOUD INFORMATION
		$CLUSTER_UUID  = $CLOUD_MGMT -> CLUSTER_UUID;
		if (isset($SERVICE_JOB -> hostname_tag)){$HOST_NAME = $SERVICE_JOB -> hostname_tag;}else{$HOST_NAME = $CLOUD_MGMT -> HOST_NAME;}
		$LAUNCHER_UUID = $CLOUD_MGMT -> LAUN_UUID;
		$SERVER_INFO   = explode("|",$CLOUD_MGMT -> LAUN_OPEN);
		if (isset($SERVER_INFO[1]))
		{
			$SERVER_UUID = $SERVER_INFO[0];
			$SERVER_ZONE = $SERVER_INFO[1];
		}
		else
		{
			$SERVER_UUID = $CLOUD_MGMT -> LAUN_OPEN;
			$SERVER_ZONE = 'Mercury';
		}
		
		#RANDOM PASSWORD
		$RANDOM_PASSWORD = Misc_Class::password_generator(12);
		
		switch ($CLOUD_MGMT -> CLOUD_TYPE)
		{
			case 'OPENSTACK':
				$INSTANCE_INFO = $this -> OpenStackMgmt -> create_instance_from_volume($SERV_UUID,$CLUSTER_UUID,$HOST_NAME,json_encode($VOLUME_INFO),$API_REFERER_FROM);						
			break;
			
			case 'AWS':
				$INSTANCE_INFO = $this -> AwsMgmt -> begin_to_create_instance($SERV_UUID,$SERVER_ZONE,$VOLUME_INFO,$HOST_NAME);
			break;
			
			case 'Azure':
				$this -> AzureMgmt -> getVMByServUUID($LAUNCHER_UUID,$CLUSTER_UUID);
			
				$azureModel = new Common_Model();
				/*if ($this -> AzureMgmt -> IsAzureStack($CLUSTER_UUID) == TRUE)
				{
					$StorageInfo = array(
						"container"  => $SERV_INFO["REPL_UUID"],
						"connectionString" => $azureModel->getConnectionString( $SERV_INFO["REPL_UUID"] )
					);
				}
				*/
				if ($IS_AZURE_MGMT_DISK == FALSE AND $IS_PROMOTE == FALSE)
				{
					$StorageInfo = array(
						"container"  => $SERV_INFO["REPL_UUID"],
						"connectionString" => $azureModel->getConnectionString( $SERV_INFO["REPL_UUID"] )
					);
				}
				else
				{
					$StorageInfo = null;
				}			

				try{
					$INSTANCE_INFO = $this -> AzureMgmt -> run_instance_from_disk($SERV_INFO,$SERVER_ZONE,$VOLUME_INFO,$HOST_NAME, $StorageInfo );
				}
				catch( Exception $e ){
					$INSTANCE_INFO = false;
					$err = json_decode( $e->getMessage(), true );
					$MESSAGE = $this -> ReplMgmt -> job_msg(json_decode($err["output"],true)["error"]["message"]);
					$this -> ReplMgmt -> update_job_msg($SERV_UUID,$MESSAGE,'Service');
				}
			break;
			
			case 'Aliyun':
				if( $this -> MgmtConfig -> {"alibaba_cloud"} -> {"recover_mode"} == 0 )
				{
					$INSTANCE_INFO = $this -> AliMgmt -> run_instance_from_disk($SERV_INFO,$SERVER_ZONE,$VOLUME_INFO,$HOST_NAME);
				}
				else
				{
					$ServiceInfo = $this->ServiceMgmt->getCloudTypeByServiceId($SERV_UUID);

					$AliModel = new Aliyun_Model();

					$cloud_info = $AliModel -> query_cloud_connection_information($CLUSTER_UUID);
		
					$bucketName = "saasame-".$SERVER_ZONE.'-'.md5($cloud_info['ACCESS_KEY']);

					$objectname = $ServiceInfo[0]["_HOST_NAME"].$SERV_UUID;
	
					$this -> AliMgmt -> DeleteObjectFromOSS($CLUSTER_UUID, $SERVER_ZONE,$bucketName,$objectname,false);
					
					$SERV_INFO["Password"] = $RANDOM_PASSWORD;
					
					#$SERV_INFO["hostName"] = $SERV_STATUS -> host_name;
					$SERV_INFO["hostName"] = $HOST_NAME;
					
					$INSTANCE_INFO = $this -> AliMgmt -> run_instance_from_image($SERV_INFO,$SERVER_ZONE,$VOLUME_INFO,$IMAGE_ID, $this->hostname);
				}
			break;
			
			case "Tencent":
				$ServiceInfo = $this -> ServiceMgmt -> getCloudTypeByServiceId($SERV_UUID);
				
				$AliModel = new Aliyun_Model();

				$cloud_info = $AliModel -> query_cloud_connection_information($CLUSTER_UUID);
		
				$bucketName = "saasame-".$SERVER_ZONE;

				$objectname = $ServiceInfo[0]["_HOST_NAME"].$SERV_UUID;
	
				$this->TencentMgmt -> DeleteObjectFromStorage($CLUSTER_UUID,$SERVER_ZONE,$bucketName,$objectname,false);
				
				$SERV_INFO["Password"] = $RANDOM_PASSWORD;
				
				$INSTANCE_INFO = $this -> TencentMgmt -> run_instance_from_image($SERV_INFO,$SERVER_ZONE,$VOLUME_INFO,$IMAGE_ID);							
			break;
			
			case "Ctyun":
				$INSTANCE_INFO = $this -> CtyunMgmt -> begin_to_run_recovery_instance($SERV_UUID,$CLUSTER_UUID,$SERVER_UUID,$HOST_NAME,$VOLUME_INFO);
			break;
		}		
		
		#FOR DEBUG
		Misc_Class::function_debug($LOG_LOCATION,__FUNCTION__,$INSTANCE_INFO);
		
		#UPDATE INSTANCE INFORMATION
		if ($INSTANCE_INFO != FALSE OR $INSTANCE_INFO != '')
		{
			if (is_object($INSTANCE_INFO))
			{
				#INSTANCE_INFO
				$INSTANCE_ID = $INSTANCE_INFO -> instance_id;
				$ADMIN_PASS = $INSTANCE_INFO -> adminPass;
				
				$this -> ServiceMgmt -> update_service_vm_info($SERV_UUID,$INSTANCE_ID,$ADMIN_PASS);
				$MESSAGE = $this -> ReplMgmt -> job_msg($INSTANCE_INFO -> message);
				$this -> ReplMgmt -> update_job_msg($SERV_UUID,$MESSAGE,'Service');	
			}
			else
			{			
				#INSTANCE_INFO
				$INSTANCE_ID = $INSTANCE_INFO;
				$ADMIN_PASS = $RANDOM_PASSWORD;
			
				$this -> ServiceMgmt -> update_service_vm_info($SERV_UUID,$INSTANCE_ID,$ADMIN_PASS);
				$MESSAGE = $this -> ReplMgmt -> job_msg('The instance created.');
				$this -> ReplMgmt -> update_job_msg($SERV_UUID,$MESSAGE,'Service');	
			}
			
			#GENERATE REPORT DATA
			$REPORT_DATA = $this -> ServiceMgmt -> recovery_report('create',$SERV_UUID,'Success');
		}
		else
		{
			$MESSAGE = $this -> ReplMgmt -> job_msg('Cannot create an instance.');
			$this -> ReplMgmt -> update_job_msg($SERV_UUID,$MESSAGE,'Service');

			#GENERATE REPORT DATA
			$REPORT_DATA = $this -> ServiceMgmt -> recovery_report('create',$SERV_UUID,'Failed');
		}
		
		#UPDATE TO REPORTING SERVER
		$REPORT_RESPONSE = Misc_Class::transport_report($REPORT_DATA);
	}
	
	
	###########################
	# EXPORT IMAGE RECOVERY
	###########################	
	private function export_image_recovery($SERV_UUID)
	{
		#EXPORT IMAGE MESSAGE
		$MESSAGE = $this -> ReplMgmt -> job_msg('Export image is ready to use.');
		$this -> ReplMgmt -> update_job_msg($SERV_UUID,$MESSAGE,'Service');
					
		#UPDATE TEMP LOCK FOR IMAGE EXPORT
		$this -> ServiceMgmt -> update_service_vm_info($SERV_UUID,'EXPORT_LOCK','LOCK_EXPORT');
		
		#GENERATE REPORT DATA
		$REPORT_DATA = $this -> ServiceMgmt -> recovery_report('create',$SERV_UUID,'Success');
		
		#UPDATE TO REPORTING SERVER
		$REPORT_RESPONSE = Misc_Class::transport_report($REPORT_DATA);		
	}
	
	
	###########################
	# RECOVERY KIT RECOVERY
	###########################	
	private function recovery_kit_recovery($SERV_UUID,$IS_RCD)
	{
		#RCD MESSAGE
		$RCD_JOB_MSG = ($IS_RCD == 'Y')?"Recovery kit media is ready to use.":"System converted via Transport Server. Please detach system and data disks.";
		$MESSAGE = $this -> ReplMgmt -> job_msg($RCD_JOB_MSG);
		$this -> ReplMgmt -> update_job_msg($SERV_UUID,$MESSAGE,'Service');
					
		#UPDATE TEMP LOCK FOR WINPE
		$this -> ServiceMgmt -> update_service_vm_info($SERV_UUID,'WINPE_LOCK','LOCK_WINPE');
		
		#GENERATE REPORT DATA
		$REPORT_DATA = $this -> ServiceMgmt -> recovery_report('create',$SERV_UUID,'Success');
		
		#UPDATE TO REPORTING SERVER
		$REPORT_RESPONSE = Misc_Class::transport_report($REPORT_DATA);
	}
	
	
	######################################################
	#
	#	CHECK ALI CLOUD IMAGE IS READY
	#
	######################################################
	public function is_launcher_job_image_ready( $session_id, $service_id  )
	{
		$serviceInfo = $this -> ServiceMgmt -> query_service($service_id);

		$CLOUD_MGMT  = $this -> get_cloud_auth_item($serviceInfo['REPL_UUID'],'');
		
		$CLUSTER_UUID = $CLOUD_MGMT -> CLUSTER_UUID;

		$SERV_INFO = explode("|",$CLOUD_MGMT -> LAUN_OPEN);
		
		if( isset( $SERV_INFO[1] ) )
		{
			$SERVER_UUID = $SERV_INFO[0];
			$SERVER_ZONE = $SERV_INFO[1];
		}
		
		if( $CLOUD_MGMT->CLOUD_TYPE == "Aliyun" ) {
			
			$taskInfo = $this->AliMgmt->describe_task_detail( $CLUSTER_UUID, $SERVER_ZONE, $serviceInfo['TASK_ID'] );
			
			$persatage = (int)$taskInfo->{"TaskProcess"};
		}
		else if( $CLOUD_MGMT->CLOUD_TYPE == "Tencent" ) {
			
			$image_info = $this->TencentMgmt->describe_image_detail( $CLUSTER_UUID, $SERVER_ZONE, $serviceInfo['TASK_ID'] );
			
			$persatage = (int)$image_info;

			if( $persatage == "100%" ){
				
				$MESSAGE = $this -> ReplMgmt -> job_msg('Creating image 100%.');
		
				$this -> ReplMgmt -> update_job_msg($service_id,$MESSAGE,'Service');
				
				$MESSAGE = $this -> ReplMgmt -> job_msg('Image create completed.');
				$this -> ReplMgmt -> update_job_msg($service_id,$MESSAGE,'Service');

				return true;
			}
			
		}
		
		$JOB_INFO = json_decode($serviceInfo['JOBS_JSON'],false);

		if(!isset( $JOB_INFO -> image_convert_persatage ))
		{			
			$JOB_INFO -> image_convert_persatage = 0;
		}

		if($JOB_INFO->image_convert_persatage <= $persatage)
		{
			$MESSAGE = $this -> ReplMgmt -> job_msg('Creating image %1%', array(round($persatage,1)."%."));
		
			$this -> ReplMgmt -> update_job_msg($service_id,$MESSAGE,'Service');
			
			$JOB_INFO -> image_convert_persatage = ( floor( $persatage / 20 ) + 1 ) * 20;
			
			$this -> ServiceMgmt -> update_trigger_info($service_id,$JOB_INFO,'SERVICE');
		}
		
		if($persatage != "100%")
		{
			return false;
		}
		
		if( $CLOUD_MGMT->CLOUD_TYPE == "Aliyun" )
			$imageInfo = $this->AliMgmt->describe_image_detail( $CLUSTER_UUID, $SERVER_ZONE, $serviceInfo['IMAGE_ID'] );
		
		if( $imageInfo != null && $imageInfo->{"Progress"} == "100%" && $imageInfo->{"Status"} == "Available" )
		{
			$MESSAGE = $this -> ReplMgmt -> job_msg('Image create completed.');
			$this -> ReplMgmt -> update_job_msg($service_id,$MESSAGE,'Service');
			
			return true;
		}		
		return false;
	}
	
		
	###########################
	#	GET CLOUD AUTH ITEM
	###########################
	private function async_job_op_type($SYNC_TYPE)
	{
		switch ($SYNC_TYPE)
		{
			case "JOB_OP_UNKNOWN":
				$TASK_OPERATION_TYPE = \saasame\transport\async_job_op_type::JOB_OP_UNKNOWN;
			break;
			
			case "JOB_OP_CREATE":
				$TASK_OPERATION_TYPE = \saasame\transport\async_job_op_type::JOB_OP_CREATE;
			break;
			
			case "JOB_OP_SUSPEND":
				$TASK_OPERATION_TYPE = \saasame\transport\async_job_op_type::JOB_OP_SUSPEND;
			break;
			
			case "JOB_OP_REMOVE":
				$TASK_OPERATION_TYPE = \saasame\transport\async_job_op_type::JOB_OP_REMOVE;
			break;
			
			case "JOB_OP_RESUME":
				$TASK_OPERATION_TYPE = \saasame\transport\async_job_op_type::JOB_OP_RESUME;
			break;
			
			case "JOB_OP_MODIFY":
				$TASK_OPERATION_TYPE = \saasame\transport\async_job_op_type::JOB_OP_MODIFY;
			break;
			
			case "JOB_OP_VERIFY":
				$TASK_OPERATION_TYPE = \saasame\transport\async_job_op_type::JOB_OP_VERIFY;
			break;
			
			default:
				$TASK_OPERATION_TYPE = 0;
			break;
		}

		return $TASK_OPERATION_TYPE;
	}
	
	
	###########################
	#	GET CLOUD AUTH ITEM
	###########################
	private function async_job_op_result($RESULT_CODE)
	{
		switch ($RESULT_CODE)
		{
			case 0:
				$TASK_OPERATION_RESULT = 'JOB_OP_RESULT_UNKNOWN';
			break;
			
			case 1:
				$TASK_OPERATION_RESULT = 'JOB_OP_RESULT_SUCCESS';
			break;
			
			case 2:
				$TASK_OPERATION_RESULT = 'JOB_OP_RESULT_FAILED';
			break;
			
			case 3:
				$TASK_OPERATION_RESULT = 'JOB_OP_RESULT_NOT_FOUND';
			break;
			
			default:
				$TASK_OPERATION_RESULT = 'JOB_OP_RESULT_UNKNOWN';
			break;
		}
		
		return $TASK_OPERATION_RESULT;
	}
	
	
	######################################################
	#
	#	PASSIVE REGISTER TRANSPORT SERVICE
	#
	######################################################
	public function register_service($SESSION_ID,$REGISTER_INFO,$MACHINE_INFO)
	{
		$HTTPS_RESPONSE_MSG_1 = 'Recovery Kit added.';
		$HTTPS_RESPONSE_MSG_2 = 'Transport Server added.';
		$HTTPS_RESPONSE_MSG_3 = 'Transport Server updated.';
		$HTTPS_RESPONSE_MSG_4 = 'Invalid Security code.';

		if (isset($MACHINE_INFO -> system_default_ui_language))
		{
			switch ($MACHINE_INFO -> system_default_ui_language)
			{
				case 1028:
					$HTTPS_RESPONSE_MSG_1 = '成功新增 Recovery Kit';
					$HTTPS_RESPONSE_MSG_2 = '成功新增 Transport 伺服器';
					$HTTPS_RESPONSE_MSG_3 = '成功更新 Transport 伺服器';
					$HTTPS_RESPONSE_MSG_4 = '不正確的安全驗證碼';
				break;
				
				case 2052:
					$HTTPS_RESPONSE_MSG_1 = '成功新增 Recovery Kit';
					$HTTPS_RESPONSE_MSG_2 = '成功新增 Transport 服务器';
					$HTTPS_RESPONSE_MSG_3 = '成功更新 Transport 服务器';
					$HTTPS_RESPONSE_MSG_4 = '不正确的安全验证码';
				break;
			}
		}		
		
		//Misc_Class::function_debug('_mgmt',__FUNCTION__,$REGISTER_INFO);
		//Misc_Class::function_debug('_mgmt',__FUNCTION__,$MACHINE_INFO);
		
		$IDENTITY_STRING = $REGISTER_INFO -> username;
		$ACCT_INFO = $this -> ServiceMgmt -> check_security_code($IDENTITY_STRING);
		
		if ($ACCT_INFO != FALSE)
		{
			#ACCOUNT INFORMATION
			$ACCT_UUID = $ACCT_INFO -> ACCT_UUID;
			$REGN_UUID = $ACCT_INFO -> REGN_UUID;
			
			#DEFINE AS ON-PREMISE TRNASPORT SERVER
			$OPEN_UUID = 'ONPREMISE-00000-LOCK-00000-PREMISEON';
			$HOST_UUID = Misc_Class::guid_v4();
			
			#MACHINE UUID AS UNIQUE CHECK KEY
			$CHECK_KEY = $MACHINE_INFO -> machine_id;	
			
			#IP ADDRESS INFORMATION
			$NETWORK_INFO = $MACHINE_INFO -> network_infos;
			for($i=0; $i<count($NETWORK_INFO); $i++)
			{
				$SERV_ADDR[] = $NETWORK_INFO[$i] -> ip_addresses;
			}					
			$SERV_ADDR = array_reverse(call_user_func_array('array_merge', $SERV_ADDR));
			
			#ADD MACHINE UUID INTO ADDRESS
			#array_unshift($SERVER_ADDRESS, $CHECK_KEY);
			
			#TRANSPORT SET TO WINDOW TYPE
			$SYST_TYPE = 'WINDOWS';
			
			#SET AS IN-DIRECT MODE
			$MACHINE_INFO -> direct_mode = false;

			#IS WINPE
			$IS_WINPE = $MACHINE_INFO -> is_winpe;
	
			#CHECK SERVER EXIST
			$CHECK_SERVER_EXIST = $this -> ServerMgmt -> check_server_exist($ACCT_UUID,$CHECK_KEY,'Launcher',null);
			
			if ($CHECK_SERVER_EXIST === TRUE)
			{
				foreach($REGISTER_INFO -> service_types as $Service_id => $Service_value)
				{
					$MACHINE_INFO -> id = $Service_id;
					$MACHINE_INFO -> version = $REGISTER_INFO -> version;
					$MACHINE_INFO -> path = $REGISTER_INFO -> path;
					
					switch ($Service_id)
					{
						case \saasame\transport\Constant::get('SCHEDULER_SERVICE'):
							$SERV_TYPE = 'Scheduler';						
						break;
						
						case \saasame\transport\Constant::get('CARRIER_SERVICE'):
							$SERV_TYPE = 'Carrier';						
						break;						
						
						case \saasame\transport\Constant::get('LOADER_SERVICE'):
							$SERV_TYPE = 'Loader';						
						break;
						
						case \saasame\transport\Constant::get('LAUNCHER_SERVICE'):
							$SERV_TYPE = 'Launcher';						
						break;
					}
					
					/* NO NEED ARGUMENT FOR SERVER */
					$HOST_ADDR = null;
					$HOST_USER = null;
					$HOST_PASS = null;
					$VMS_INFO = null;
					$SELECTED_VMS = null;
					
					$this -> ServerMgmt -> initialize_server($ACCT_UUID,$REGN_UUID,$OPEN_UUID,$HOST_UUID,$SERV_ADDR,$HOST_ADDR,$HOST_USER,$HOST_PASS,$SERV_TYPE,$MACHINE_INFO,$VMS_INFO,$SYST_TYPE,$SELECTED_VMS,false);
					
				}
				
				if ($IS_WINPE == TRUE)
				{
					$RETURN_MSG = $HTTPS_RESPONSE_MSG_1;
				}
				else
				{
					$RETURN_MSG = $HTTPS_RESPONSE_MSG_2;
				}
				
				$MGMT_STRING = $this -> ServiceMgmt -> mgmt_generate_session($CHECK_KEY);
				
				return new \saasame\transport\register_return(array('message'=>$RETURN_MSG, 'session'=>$MGMT_STRING));
			}
			else
			{
				foreach($REGISTER_INFO -> service_types as $Service_id => $Service_value)
				{
					$MACHINE_INFO -> id = $Service_id;
					$MACHINE_INFO -> version = $REGISTER_INFO -> version;
					$MACHINE_INFO -> path = $REGISTER_INFO -> path;
					
					switch ($Service_id)
					{
						case \saasame\transport\Constant::get('SCHEDULER_SERVICE'):
							$SERV_TYPE = 'Scheduler';						
						break;
						
						case \saasame\transport\Constant::get('CARRIER_SERVICE'):
							$SERV_TYPE = 'Carrier';						
						break;		
						
						case \saasame\transport\Constant::get('LOADER_SERVICE'):
							$SERV_TYPE = 'Loader';						
						break;
						
						case \saasame\transport\Constant::get('LAUNCHER_SERVICE'):
							$SERV_TYPE = 'Launcher';						
						break;
					}
					
					$SERV_UUID = $this -> ServerMgmt -> list_match_service_id($MACHINE_INFO -> machine_id)['ServiceId'][$SERV_TYPE];
					
					$this -> ServerMgmt -> update_server_metadata($SERV_UUID,$MACHINE_INFO);
				}

				$MGMT_STRING = $this -> ServiceMgmt -> mgmt_generate_session($CHECK_KEY);
				
				return new \saasame\transport\register_return(array('message'=>$HTTPS_RESPONSE_MSG_3, 'session'=>$MGMT_STRING));
			}
		}
		else
		{
			throw new \saasame\transport\invalid_operation(array('what_op'=>0x5, 'why'=>$HTTPS_RESPONSE_MSG_4));
		}
	}
	
	
	######################################################
	#
	#	PASSIVE REGISTER PACKER SERVICE
	#
	######################################################
	public function register_physical_packer($SESSION_ID,$REGISTER_INFO,$MACHINE_INFO)
	{
		$HTTPS_RESPONSE_MSG_1 = 'Please register the cloud transport server.';
		$HTTPS_RESPONSE_MSG_2 = 'Host added. Please verify settings on management portal.';
		$HTTPS_RESPONSE_MSG_3 = 'Host updated. Please verify settings on management portal.';
		$HTTPS_RESPONSE_MSG_4 = 'Invalid Security code.';
				
		if (isset($MACHINE_INFO -> system_default_ui_language))
		{
			switch ($MACHINE_INFO -> system_default_ui_language)
			{
				case 1028:
					$HTTPS_RESPONSE_MSG_1 = '請先新增目標Transport伺服器';
					$HTTPS_RESPONSE_MSG_2 = '成功新增來源主機，請由主控台執行驗證和管理';
					$HTTPS_RESPONSE_MSG_3 = '成功更新來源主機，請由主控台執行驗證和管理';
					$HTTPS_RESPONSE_MSG_4 = '不正確的安全驗證碼';
				break;
				
				case 2052:
					$HTTPS_RESPONSE_MSG_1 = '请先新增目标Transport服务器';
					$HTTPS_RESPONSE_MSG_2 = '成功新增来源主机，请由控制台执行验证和管理';
					$HTTPS_RESPONSE_MSG_3 = '成功更新来源主机，请由控制台执行验证和管理';
					$HTTPS_RESPONSE_MSG_4 = '不正确的安全验证码';
				break;
			}
		}
		
		#Misc_Class::function_debug('_mgmt',__FUNCTION__,$REGISTER_INFO);
		#Misc_Class::function_debug('_mgmt',__FUNCTION__,$MACHINE_INFO);
	
		$ACCT_STRING = $REGISTER_INFO -> username;
		$ACCT_INFO = $this -> ServiceMgmt -> check_security_code($ACCT_STRING);
		
		if ($ACCT_INFO != FALSE)
		{
			#ACCOUNT INFORMATION
			$ACCT_UUID = $ACCT_INFO -> ACCT_UUID;
			$REGN_UUID = $ACCT_INFO -> REGN_UUID;
			
			#MACHINE UUID AS UNIQUE CHECK KEY
			$CHECK_KEY = $MACHINE_INFO -> machine_id;	
			
			$CHECK_PACKER_EXIST = $this -> ServerMgmt -> check_host_exist($ACCT_UUID,$CHECK_KEY);
		
			if ($CHECK_PACKER_EXIST == TRUE)
			{
				#SET AS IN-DIRECT MODE
				$MACHINE_INFO -> direct_mode = false;
				
				#REGISTRATION PAIRED WITH THE MANAGEMENT SERVER
				$HOST_UUID = $this -> ServiceMgmt -> get_mgmt_transport_uuid($ACCT_UUID,'127.0.0.1');
				if ($HOST_UUID == FALSE)
				{
					throw new \saasame\transport\invalid_operation(array('what_op'=>0x5, 'why'=>$HTTPS_RESPONSE_MSG_1));
				}
				
				#IP ADDRESS INFORMATION
				$NETWORK_INFO = $MACHINE_INFO -> network_infos;
				for($i=0; $i<count($NETWORK_INFO); $i++)
				{
					$HOST_ADDR[] = $NETWORK_INFO[$i] -> ip_addresses;
				}					
				$HOST_ADDR = call_user_func_array('array_merge', $HOST_ADDR);
				
				#ADD MACHINE UUID INTO ADDRESS
				#array_unshift($PACKER_ADDRESS, $CHECK_KEY);
				
				#FILTER OUT MS DEFAULT IP ADDRESS
				foreach ($HOST_ADDR as $Key => $Addr)
				{
					if (strpos($Addr, '169.254') !== false)
					{
						unset($HOST_ADDR[$Key]);
					}
				}
				
				$HOST_ADDR = implode(',',$HOST_ADDR);
				
				#DEFINE PHYSICAL PACKER TYPE
				if ($MACHINE_INFO -> is_winpe == FALSE)
				{
					$SERV_TYPE = 'Physical Packer';
				}
				else
				{
					$SERV_TYPE = 'Offline Packer';
				}
				
				#APPEND DOT AT MANUFACTURER METADATA FOR HTTPS MODE
				$MACHINE_INFO -> manufacturer = $MACHINE_INFO -> manufacturer.'.';
				
				#DEFINE PACKER METADATA
				$SERV_INFO = $MACHINE_INFO;
				$VMS_INFO = $MACHINE_INFO;
				
				/* NO NEED ARGUMENT FOR PACKER */
				$SERV_ADDR = null;
				$OPEN_UUID = null;
				$HOST_USER = null;
				$HOST_PASS = null;
				$SYST_TYPE = null;
				$SELECTED_VMS = null;
				
				$this -> ServerMgmt -> initialize_server($ACCT_UUID,$REGN_UUID,$OPEN_UUID,$HOST_UUID,$SERV_ADDR,$HOST_ADDR,$HOST_USER,$HOST_PASS,$SERV_TYPE,$SERV_INFO,$VMS_INFO,$SYST_TYPE,$SELECTED_VMS,false);

				$MGMT_STRING = $this -> ServiceMgmt -> mgmt_generate_session($CHECK_KEY);
				
				return new \saasame\transport\register_return(array('message'=>$HTTPS_RESPONSE_MSG_2, 'session'=>$MGMT_STRING));
			}
			else
			{
				$MGMT_STRING = $this -> ServiceMgmt -> mgmt_generate_session($CHECK_KEY);
				
				return new \saasame\transport\register_return(array('message'=>$HTTPS_RESPONSE_MSG_3, 'session'=>$MGMT_STRING));
			}
		}
		else
		{
			throw new \saasame\transport\invalid_operation(array('what_op'=>0x5, 'why'=>$HTTPS_RESPONSE_MSG_4));
		}
		
	}	
	
	######################################################
	#
	#	CHECK RUNNING TASK
	#
	######################################################
	public function ping()
	{
		#SERVICE INFO ARRAY
		$SERVICE_INFO = array('id' 			=> null,
							  'version' 	=> 'xyzzy',
							  'path' 		=> getenv('WEBROOT').'\apache24');
			
			
		#DEFINE DETAIL ARRAY
		return new saasame\transport\service_info($SERVICE_INFO);		
	}
	
	######################################################
	#
	#	CHECK RUNNING TASK
	#
	######################################################
	public function check_running_task($JOB_ID,$PARAMETERS)
	{
		switch ($JOB_ID)
		{
			case "DailyReport":
				$this -> MailerMgmt -> get_daily_report($PARAMETERS);
				return false;
			break;
			
			case "DailyBackup":
				$this -> MailerMgmt -> get_daily_backup($PARAMETERS);
				return false;
			break;
			
			default:
				return true;
		}
	}
}