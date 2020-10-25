<?php
require_once 'aws.phar';
###################################
#
#	AMAZON WEB SERVICES CLASS
#
###################################
class Aws_Action_Class
{
	###########################
	#CONSTRUCT FUNCTION
	###########################
	protected $AwsWebSevMgmt;
	
	public function __construct()
	{
		$this -> AwsWebSevMgmt 	= new Aws_Query_Class();
	}
	
	###########################
	#DEFAULT CREDENTIALS INFOMATION
	###########################
	protected $Ec2Client;
	//protected $PricingClient;
	public function get_credential_keys($CLOUD_UUID,$AWS_REGION=null)
	{		
		$AWS_CRED_INFO = $this -> AwsWebSevMgmt -> query_aws_connection_information($CLOUD_UUID);
	
		$ACCESSKEY = $AWS_CRED_INFO['ACCESS_KEY'];
		$SECRETKEY = $AWS_CRED_INFO['SECRET_KEY'];
		
		if ($AWS_REGION == null)
		{
			$AWS_REGION = $AWS_CRED_INFO['DEFAULT_ADDR'];
		}
	
		$LAST_CHAR = substr($AWS_REGION, -1);
		if (is_numeric($LAST_CHAR))
		{
			$REGION = $AWS_REGION;
		}
		else
		{
			$REGION = substr($AWS_REGION,0,-1);
		}	
		
		$AWS_CRED = new Aws\Credentials\Credentials($ACCESSKEY,$SECRETKEY);
		
		$CredentialsInfo = array(
								'version'     => 'latest',
								'region'      => $REGION,
								'credentials' => $AWS_CRED);
	
		#SET UP Ec2Client
		$this -> Ec2Client = new Aws\Ec2\Ec2Client($CredentialsInfo);
		//$this -> PricingClient = new Aws\Pricing\PricingClient($CredentialsInfo);
	}
		
	#AWS DEBUG OUTPUT
	public function aws_debug($FUN_NAME,$RESPONSE)
	{
		$IS_ENABLE = true;
		
		if ($IS_ENABLE == TRUE)
		{
			$PATH = $_SERVER['DOCUMENT_ROOT'].'/_include/_debug/_mgmt/';
			$FILE = $_SERVER['DOCUMENT_ROOT'].'/_include/_debug/_mgmt/AwsDebug.txt';
			
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
				Misc_Class::smart_log_archive($FILE);
			}
			
			#CHECK AND CREATE FILE EXISTS
			if(!file_exists($FILE)){fopen($FILE,'w');}	
		
			$current = file_get_contents($FILE);
			$current .= 'NAME:'.$FUN_NAME."\n";
			$current .= 'RESPONSE:'."\n";
			$current .= $RESPONSE;
			$current .= "\n".'----------------------------------------------------------'."\n";
			file_put_contents($FILE, $current);
		}
	}
	
	
	###########################
	#GET USER INFORMATION
	###########################
	public function verify_aws_credential($AWS_REGION,$ACCESS_KEY,$SECRET_KEY)
	{
		$AWS_CRED = new Aws\Credentials\Credentials($ACCESS_KEY,$SECRET_KEY);		
		$CREDENTIALS_INFO = array(
								'version'     => 'latest',
								'region'      => $AWS_REGION,
								'credentials' => $AWS_CRED);
		try{
			$client = new Aws\Ec2\Ec2Client($CREDENTIALS_INFO);
			$result = $client -> describeAccountAttributes([]);
			
			#$client = new Aws\Iam\IamClient($CREDENTIALS_INFO);
			#$result = $client -> GetUser();
			return $result;
		}
		catch(Exception $e){
			$this -> aws_debug(__FUNCTION__,$e->getMessage());
			return false;
		}	
	}

	
	###########################
	#DESCRIBE AVAILABILITY REGIONS
	###########################
	public function describe_availability_regions($CLOUD_UUID)
	{
		try{
			$this -> get_credential_keys($CLOUD_UUID);
			
			$result = $this -> Ec2Client -> describeRegions(); 
			
			return $result['Regions'];
		}
		catch(Exception $e){
			$this -> aws_debug(__FUNCTION__,$e->getMessage());
			return false;
		}	
	}
	
	
	###########################
	#DESCRIBE INSTANCE
	###########################
	public function describe_instance($CLOUD_UUID,$ZONE_INFO,$INSTANCE_UUID=null)
	{
		if ($INSTANCE_UUID != 'TEMP_LOCK')
		{		
			try{
				$this -> get_credential_keys($CLOUD_UUID,$ZONE_INFO);
					
				if ($INSTANCE_UUID == null)
				{
					$result = $this -> Ec2Client -> describeInstances();
					$reservations = array($result['Reservations']);
				
					return $reservations;
				}
				else
				{
					$result = $this -> Ec2Client -> describeInstances(array('InstanceIds' => array($INSTANCE_UUID)));
					$reservations = array($result['Reservations'][0]);
				
					return $reservations;
				}
			}		
			catch(Exception $e){
				$this -> aws_debug(__FUNCTION__,$e->getMessage());
				return false;
			}
		}
		else
		{
			return false;
		}
	}
	
	###########################
	#	DESCRIBE ALL INSTANCES
	###########################
	public function describe_all_instances($CLOUD_UUID)
	{
		$REGIONS = $this -> describe_availability_regions($CLOUD_UUID);
	
		for ($i=0; $i<count($REGIONS); $i++)
		{
			$REGION_NAME = $REGIONS[$i]['RegionName'];
			
			$LIST_INSTANCE_INFO = $this -> describe_instance($CLOUD_UUID,$REGION_NAME);
		
			$FILTER_RESERVATIONS = $LIST_INSTANCE_INFO[0];
		
			for ($w=0; $w<count($FILTER_RESERVATIONS); $w++)
			{
				for ($x=0; $x<count($FILTER_RESERVATIONS[$w]['Instances']); $x++)
				{
					$INSTANCE[$REGION_NAME][] = $FILTER_RESERVATIONS[$w]['Instances'][$x];
				}
			}
		}
	
		return array_filter($INSTANCE);
	}	
	
	
	###########################
	#DESCRIBE AVAILABILITY ZONES
	###########################
	public function describe_availability_zones($CLOUD_UUID,$ZONE_INFO)
	{
		try{
			$this -> get_credential_keys($CLOUD_UUID,$ZONE_INFO);
			
			$result = $this -> Ec2Client ->DescribeAvailabilityZones();
	
			return $result['AvailabilityZones'];
		}
		catch(Exception $e){
			$this -> aws_debug(__FUNCTION__,$e->getMessage());
			return false;
		}
		
	}	
	
	
	###########################
	#	CREATE VOLUME
	###########################
	public function create_volume($CLOUD_UUID,$ZONE_INFO,$VOLUME_SIZE,$HOST_NAME,$IOPS)
	{
		try{
			switch($IOPS)
			{
				case is_numeric($IOPS):
				$VOLUME_INFO = array(
								'Size' => $VOLUME_SIZE,
								'AvailabilityZone' => $ZONE_INFO,
								'VolumeType' => 'io1',
								'Iops' => $IOPS);
				break;
				
				case 'ST':
				$VOLUME_INFO = array(
								'Size' => $VOLUME_SIZE,
								'AvailabilityZone' => $ZONE_INFO,
								'VolumeType' => 'standard');
				break;
				
				case 'GP2':
				$VOLUME_INFO = array(
								'Size' => $VOLUME_SIZE,
								'AvailabilityZone' => $ZONE_INFO,
								'VolumeType' => 'gp2');
				break;
			}
			$this -> get_credential_keys($CLOUD_UUID,$ZONE_INFO);
				
			$result = $this -> Ec2Client -> createVolume($VOLUME_INFO);

			$VOLUME_ID 		= $result['VolumeId'];
			$VOLUME_STATE 	= $result['State']; 	#FOR ERROR CONTROL
			
			#VOLUME META DATA
			$NOW_TIME = Misc_Class::current_utc_time();	
			$VOLUME_META_DATA = array
								(
									array(
										'Key' => 'Name',
										'Value' => $HOST_NAME.'@'.$NOW_TIME
									),
									array(
										'Key' => 'Description',
										'Value' => 'Volume Create By SaaSaMe Transport Service@'.$NOW_TIME.'',
									)
								);
			
			$this -> create_tags($CLOUD_UUID,$ZONE_INFO,$VOLUME_ID,$VOLUME_META_DATA);
						
			return $VOLUME_ID;
		}
		catch(Exception $e){
			$this -> aws_debug(__FUNCTION__,$e->getMessage());
			return false;
		}
	}
	
	
	###########################
	#	SET RESOURCE TAG
	###########################
	private function create_tags($CLOUD_UUID,$ZONE_INFO,$RESOURCE_ID,$RESOURCE_METADATA)
	{
		try{
			$this -> get_credential_keys($CLOUD_UUID,$ZONE_INFO);
			
			$result1 = $this -> Ec2Client -> createTags(array(
												'Resources' => array($RESOURCE_ID),
												'Tags' => $RESOURCE_METADATA
			));
			
			return true;
		}
		catch(Exception $e){
			$this -> aws_debug(__FUNCTION__,$e->getMessage());
			return false;
		}		
	}
	
	
	###########################
	#GET LAST BLOCK DEVICE MAPPING
	###########################
	private function get_last_block_device_mapping($CLOUD_UUID,$ZONE_INFO,$INSTANCE_ID,$OS_TYPE)
	{		
		try{
			$this -> get_credential_keys($CLOUD_UUID,$ZONE_INFO);
			
			$result = $this -> Ec2Client -> describeInstanceAttribute(array(
													'InstanceId' => $INSTANCE_ID,
													'Attribute'	=> 'blockDeviceMapping'
			));
		
			#GET BLOCK DEVICE MAPPING INFO
			$result = $result->get('BlockDeviceMappings');
		
			$CountDevice = count($result);
			if ($CountDevice == 0)
			{
				if ($OS_TYPE == 'MS')
				{
					return '/dev/sda1';
				}
				else
				{
					return '/dev/xvda';
				}
			}
			else
			{
				#FORMAT DEVICE NAME
				for ($i=0; $i<count($result); $i++)
				{
					$DeviceName = str_replace('/dev/', '',$result[$i]['DeviceName']);		
					$BlockDevice[] = $DeviceName;
				}		
			
				#SORT BLOCK DEVICE NAME
				sort($BlockDevice);
			
				#GET LAST BLOCK DEVICE NAME
				$LastBlockDevice = end($BlockDevice);		
				
				if ($LastBlockDevice == 'sda1')
				{
					$LastBlockDevice = 'xvdb';
				}
				else
				{
					$LastBlockDevice++; 
				}
				return $LastBlockDevice;
			}
		}
		catch (Exception $e){
			$this -> aws_debug(__FUNCTION__,$e->getMessage());
			return false;
		}
		
	}

	
	###########################
	# DELETE REPLICA JOB
	###########################
	public function delete_replica_job($CLOUD_UUID,$REPL_UUID,$INSTANCE_REGN,$DISK_UUID)
	{
		if ($DISK_UUID != false)
		{
			$ReplMgmt = new Replica_Class();
			
			for ($x=0; $x<count($DISK_UUID); $x++)
			{
				$VOLUME_UUID = $DISK_UUID[$x]['OPEN_DISK_UUID'];
				
				#DETACH DISK
				$this -> detach_volume($CLOUD_UUID,$INSTANCE_REGN,$VOLUME_UUID);
				$this -> wait_until($CLOUD_UUID,$INSTANCE_REGN,$VOLUME_UUID,'VolumeAvailable');
				
				#GET MATCH SNAPSHOT LIST
				$LIST_SNAPSHOT = $this -> describe_snapshots($CLOUD_UUID,$INSTANCE_REGN,$VOLUME_UUID);
				
				#BEGIN TO DELETE SNAPSHOT
				if ($LIST_SNAPSHOT != FALSE)
				{
					$SNPSHOT_COUNT = count($LIST_SNAPSHOT);
		
					if ($SNPSHOT_COUNT != 0)
					{
						$MESSAGE = $ReplMgmt -> job_msg('Removing the snapshots.');
						$ReplMgmt -> update_job_msg($REPL_UUID,$MESSAGE,'Replica');
						for ($SNAP=0; $SNAP<$SNPSHOT_COUNT; $SNAP++)
						{
							$SNAPSHOT_ID = $LIST_SNAPSHOT[$SNAP]['SnapshotId'];
							$this -> delete_snapshot($CLOUD_UUID,$INSTANCE_REGN,$SNAPSHOT_ID);
						}
					}
				}
				
				#DELETE VOLUME
				$this -> delete_volume($CLOUD_UUID,$INSTANCE_REGN,$VOLUME_UUID);
				
				#MESSAGE
				$MESSAGE = $ReplMgmt -> job_msg('The volume removed.');
				$ReplMgmt -> update_job_msg($REPL_UUID,$MESSAGE,'Replica');
			}
		}
	}
	
	
	###########################
	#	CREATE EBS SNAPSHOT
	###########################
	public function take_snapshot($CLOUD_UUID,$ZONE_INFO,$VOLUME_ID,$SNAP_NAME,$SNAP_TIME)
	{
		try{
			$this -> get_credential_keys($CLOUD_UUID,$ZONE_INFO);
		
			$result = $this -> Ec2Client -> createSnapshot(array(
												'VolumeId' 		=> $VOLUME_ID,
												'Description' 	=> 'Created By SaaSaMe Transport Service'
											));
			
			$SNAPSHOT_ID = $result['SnapshotId'];
			
			#SNAPSHOT METADATA
			$SNAPSHOT_METADATA = array(
									array(
										'Key' => 'Name',
										'Value' => $SNAP_NAME.'@'.$SNAP_TIME
									),
									array(
										'Key' => 'Description',
										'Value' => 'Snapshot Created By SaaSaMe Transport Service@'.$SNAP_TIME
									)
								);
			
			$this -> create_tags($CLOUD_UUID,$ZONE_INFO,$SNAPSHOT_ID,$SNAPSHOT_METADATA);
			
			$SANPSHOT_INFO = array($SNAPSHOT_ID,$result['VolumeId'],$result['VolumeSize'],$result['StartTime']);
			
			return $SANPSHOT_INFO;	
		}
		catch(Exception $e){
			$this -> aws_debug(__FUNCTION__,$e->getMessage());
			return false;
		}
	}
	
	
	###########################
	# SNAPSHOT CONTROL
	###########################
	public function snapshot_control($CLOUD_UUID,$ZONE_INFO,$VOLUME_ID,$NUMBER_SNAPSHOT)
	{
		$LIST_SNAPSHOTS = $this -> describe_snapshots($CLOUD_UUID,$ZONE_INFO,$VOLUME_ID);
		if ($LIST_SNAPSHOTS != FALSE)
		{
			$SLICE_SNAPSHOT = array_slice(array_reverse($LIST_SNAPSHOTS),$NUMBER_SNAPSHOT);
			for ($x=0; $x<count($SLICE_SNAPSHOT); $x++)
			{
				$REMOVE_SNAPSHOT_ID = $SLICE_SNAPSHOT[$x]['SnapshotId'];
				$this -> delete_snapshot($CLOUD_UUID,$ZONE_INFO,$REMOVE_SNAPSHOT_ID);
			}						
		}
	}
	
	
	###########################
	#DESCRIBE SNAPSHOTS BY VOLUME
	###########################
	public function describe_snapshots($CLOUD_UUID,$ZONE_INFO,$VOLUME_ID=null,$SNAPSHOT_ID=null)
	{
		if ($VOLUME_ID != null)
		{
			$SetFilter = array('Name' => 'volume-id','Values' => array($VOLUME_ID));
		}
		if ($SNAPSHOT_ID != null)
		{
			$SetFilter = array('Name' => 'snapshot-id','Values' => array($SNAPSHOT_ID));
		}
		
		try{
			$this -> get_credential_keys($CLOUD_UUID,$ZONE_INFO);
					
			$result = $this -> Ec2Client -> describeSnapshots(array(
												'Filters' => array($SetFilter)
			));
			
			$ListSnapshot = $result['Snapshots'];
		
			$COUNT_SNAPSHOT = count($ListSnapshot);
			
			if ($COUNT_SNAPSHOT != 0)
			{
				usort($ListSnapshot, function($a, $b) {return $a['StartTime']->format(\DateTime::ISO8601) <=> $b['StartTime'] -> format(\DateTime::ISO8601);});
			
				return $ListSnapshot;
			}
			else
			{
				return false;
			}
		}
		catch(Exception $e){
			$this -> aws_debug(__FUNCTION__,$e->getMessage());
			return false;
		}		
	}


	###########################
	#	DELETE SNAPSHOT
	###########################
	public function delete_snapshot($CLOUD_UUID,$ZONE_INFO,$SNAPSHOT_ID)
	{
		try{
			$this -> get_credential_keys($CLOUD_UUID,$ZONE_INFO);
			
			$result = $this -> Ec2Client -> deleteSnapshot(array(
												'SnapshotId' => $SNAPSHOT_ID		
			));
			
			return $result;
		}
		catch(Exception $e){
			$this -> aws_debug(__FUNCTION__,$e->getMessage());
			return false;
		}
	}
	
	
	###########################
	#LIST AWS GENERAL PURPOSE INSTANCES
	###########################
	public function describe_instance_types($EC2_TYPE=null)
	{
		#DEFINE DEFAULT REGION
		$AWS_EC2_TYPE = Misc_Class::define_mgmt_setting();
		
		if (isset($AWS_EC2_TYPE -> aws_ec2_type))
		{
			foreach ($AWS_EC2_TYPE -> aws_ec2_type as $Name => $Description)
			{
				$GeneralPurpose[$Name] = json_decode($Description,true);
			}
		}
		else
		{
			$GeneralPurpose = array(
									't2.nano'   => array('Name'=>'t2.nano',   'vCPU'=>1, 'ECU'=>'Variable', 'Memory'=>'0.5 GiB', 'InstanceStorage'=>'EBS Only'),
									't2.micro'  => array('Name'=>'t2.micro',  'vCPU'=>1, 'ECU'=>'Variable', 'Memory'=>'1 GiB',   'InstanceStorage'=>'EBS Only'),
									't2.small'  => array('Name'=>'t2.small',  'vCPU'=>1, 'ECU'=>'Variable', 'Memory'=>'2 GiB',   'InstanceStorage'=>'EBS Only'),
									't2.medium' => array('Name'=>'t2.medium', 'vCPU'=>2, 'ECU'=>'Variable', 'Memory'=>'4 GiB',   'InstanceStorage'=>'EBS Only'),
									't2.large'  => array('Name'=>'t2.large',  'vCPU'=>2, 'ECU'=>'Variable', 'Memory'=>'8 GiB',   'InstanceStorage'=>'EBS Only')
								);
		}		
		
		if ($EC2_TYPE == null)
		{
			return $GeneralPurpose;
		}
		else
		{
			return $GeneralPurpose[$EC2_TYPE];
		}	
	}
	
	###########################
	#LIST ON-LINE AWS INSTANCES (Not Use)
	###########################
	private function _describe_instance_types($CLOUD_UUID)
	{
		$this -> get_credential_keys($CLOUD_UUID,'us-east-1');
			
		$List_Instance = $this -> PricingClient -> GetAttributeValues([
																'AttributeName' => 'instanceType',
																'MaxResults' => 100,
																'ServiceCode' => 'AmazonEC2',
															])['AttributeValues'];
		
		for ($i=0; $i<count($List_Instance); $i++)
		{
			$Instance_List[] = $List_Instance[$i]['Value'];
		}
		
		for ($x=0; $x<count($Instance_List); $x++)
		{		
			$Instance_Result = $this -> PricingClient->getProducts([
												'Filters' => [
													[
														'Field' => 'instanceType',
														'Type' => 'TERM_MATCH',
														'Value' => $Instance_List[$x],
													],
												],
												'FormatVersion' => 'aws_v1',
												'MaxResults' => 1,
												'ServiceCode' => 'AmazonEC2',
											])['PriceList'][0];

			$Instance_type = json_decode($Instance_Result) -> product -> attributes -> instanceType;
			$Instance_vcpu= json_decode($Instance_Result) -> product -> attributes -> vcpu;
			$Instance_ecu = json_decode($Instance_Result) -> product -> attributes -> ecu;
			$Instance_Memory = json_decode($Instance_Result) -> product -> attributes -> memory;
			$Instance_Storage = json_decode($Instance_Result) -> product -> attributes -> storage;
			
			$GeneralPurpose[$Instance_type] = array('Name' => $Instance_type, 'vCPU' => $Instance_vcpu, 'ECU' => $Instance_ecu, 'Memory' => $Instance_Memory, 'InstanceStorage' => $Instance_Storage);
		}
		
		return $GeneralPurpose;
	}
	
	
	###########################
	#DESCRIBE AVAILABLE NETWORK
	###########################
	public function describe_available_network($CLOUD_UUID,$ZONE_INFO,$SUBNET_ID=null)
	{
		if ($SUBNET_ID == null)
		{
			$SetFilter = array('Name' => 'availabilityZone','Values' => array($ZONE_INFO));
		}
		else
		{
			$SetFilter = array('Name' => 'subnet-id','Values' => array($SUBNET_ID));
		}
		
		
		try{
			$this -> get_credential_keys($CLOUD_UUID,$ZONE_INFO);
			
			$result = $this -> Ec2Client ->describeSubnets(array(
												'Filters' => array($SetFilter)			
			));
			
			return $result['Subnets'];
		}
		catch(Exception $e){
			$this -> aws_debug(__FUNCTION__,$e->getMessage());
			return false;
		}		
	}

	
	###########################
	#LSIT SECURITY GROUPS
	###########################
	public function list_security_groups($CLOUD_UUID,$ZONE_INFO,$VPC_UUID)
	{
		$SetFilter = array('Name' => 'vpc-id','Values' => array($VPC_UUID));
		
		try{
			$this -> get_credential_keys($CLOUD_UUID,$ZONE_INFO);
	
			$result = $this -> Ec2Client -> describeSecurityGroups(array('Filters' => array($SetFilter)));
			
			return $result['SecurityGroups'];
		}
		catch(Exception $e){
			$this -> aws_debug(__FUNCTION__,$e->getMessage());
			return false;
		}
	}
	
	
	###########################
	#DESCRIBE SECURITY GROUPS
	###########################
	public function describe_security_group($CLOUD_UUID,$ZONE_INFO,$SGROUP_ID)
	{
		$SetFilter = array('GroupIds' => array($SGROUP_ID));
				
		try{
			$this -> get_credential_keys($CLOUD_UUID,$ZONE_INFO);
	
			$result = $this -> Ec2Client -> describeSecurityGroups($SetFilter);
			
			return $result['SecurityGroups'];
		}
		catch(Exception $e){
			$this -> aws_debug(__FUNCTION__,$e->getMessage());
			return false;
		}
	}

	
	###########################
	#	DESCRIBE KEY PAIR
	###########################
	public function describe_key_pairs($CLOUD_UUID,$ZONE_INFO,$KEY_NAME=null)
	{
		if ($KEY_NAME == null)
		{
			$SetFilter = array(null);
		}
		else
		{
			$SetFilter = array('KeyNames' => array($KEY_NAME));
		}
	
		try{
			$this -> get_credential_keys($CLOUD_UUID,$ZONE_INFO);
			
			$result = $this -> Ec2Client ->describeKeyPairs($SetFilter);
		
			return $result['KeyPairs'];
		}
		catch(Exception $e){
			$this -> aws_debug(__FUNCTION__,$e->getMessage());
			return false;
		}
	}
	
	
	###########################
	#	DESCRIBE BASE IMAGES
	###########################
	public function describe_base_images($CLOUD_UUID,$ZONE_INFO,$OS_TYPE)
	{
		if ($OS_TYPE == 'MS')
		{		
			$SetFilter = array(
							'Name' => 'owner-id',	'Values' => array(801119661308),
							'Name' => 'state',		'Values' => array('available'),
							'Name' => 'description','Values' => array('Microsoft Windows Server 2012 R2 RTM 64-bit Locale English AMI provided by Amazon')			
			);
		}
		elseif ($OS_TYPE == 'LX')
		{
			$SetFilter = array(
							'Name' => 'owner-id',	'Values' => array(801119661308),
							'Name' => 'state',		'Values' => array('available'),
							'Name' => 'description','Values' => array('Amazon Linux 2 AMI 2.0.20181008 x86_64 HVM gp2')
			);
		}
		else
		{
			return false;
		}
	
		try{
			$this -> get_credential_keys($CLOUD_UUID,$ZONE_INFO);

			$result = $this -> Ec2Client -> describeImages(array('Filters' => array($SetFilter)));
		
			$ImageInfo  = $result['Images'];
			$CountImage = count($ImageInfo);
			if ($CountImage != 0)
			{
				return (object)$ImageInfo[0];
			}
			else
			{
				return false;
			}		
		}
		catch(Exception $e){
			$this -> aws_debug(__FUNCTION__,$e->getMessage());
			return false;
		}
		
	}
	
	
	###########################
	#CREATE VOLUME FROM SNAPSHOT
	###########################
	public function create_volume_from_snapshot($CLOUD_UUID,$ZONE_INFO,$SNAPSHOT_ID,$HOST_NAME)
	{
		try{
			$this -> get_credential_keys($CLOUD_UUID,$ZONE_INFO);
			
			$result = $this -> Ec2Client -> createVolume(array(
												'SnapshotId' => $SNAPSHOT_ID,
												'AvailabilityZone' => $ZONE_INFO,
												'VolumeType' => 'gp2',
			));
			
			$VOLUME_ID = $result['VolumeId'];	
			
			#SNAPSHOT METADATA
			$NOW_TIME = Misc_Class::current_utc_time();	
			$VOLUME_METADATA = array(
									array(
										'Key' => 'Name',
										'Value' => 'snap-'.$HOST_NAME.'@'.$NOW_TIME
									),
									array(
										'Key' => 'Description',
										'Value' => 'Snapshot Volume Created By SaaSaMe Transport Service@'.$NOW_TIME.'',
									)
								);
			
			$this -> create_tags($CLOUD_UUID,$ZONE_INFO,$VOLUME_ID,$VOLUME_METADATA);		
			
			$this -> wait_until($CLOUD_UUID,$ZONE_INFO,$VOLUME_ID,'VolumeAvailable');
			
			return $VOLUME_ID;
		}
		catch(Exception $e){
			$this -> aws_debug(__FUNCTION__,$e->getMessage());
			return false;
		}		
	}
		
	
	###########################
	#	RUN INSTANCE
	###########################
	public function run_instance($CLOUD_UUID,$ZONE_INFO,$INSTANCE_CONFIG,$OS_TYPE)
	{
		$ImageId = $this -> describe_base_images($CLOUD_UUID,$ZONE_INFO,$OS_TYPE);
		
		if ($ImageId == false)
		{
			return false;
		}
		else
		{
			$IMAGE_ID 		   	= $ImageId -> ImageId;
			$INSTANCE_TYPE  	= $INSTANCE_CONFIG -> InstanceType;
			$SECURITYGROUP_ID  	= $INSTANCE_CONFIG -> SecurityGroup;
			$SUBNET_ID  		= $INSTANCE_CONFIG -> SubnetId;
			$HOST_NAME			= $INSTANCE_CONFIG -> Hostname;
			$PRIVATE_ADDR		= $INSTANCE_CONFIG -> PrivateIpAddress;
			
			$InstanceConfig = array(
								'Monitoring'		=> array('Enabled' => false),
								'ImageId'		 	=> $IMAGE_ID,
								'MinCount'       	=> 1,
								'MaxCount'       	=> 1,
								'InstanceType'   	=> $INSTANCE_TYPE,
								'SecurityGroupIds' 	=> array($SECURITYGROUP_ID),
								'Placement'			=> array('AvailabilityZone' => $ZONE_INFO),
								'SubnetId'			=> $SUBNET_ID
								);
								
			if ($PRIVATE_ADDR != 'DynamicAssign')
			{
				$InstanceConfig['PrivateIpAddress'] = $PRIVATE_ADDR;
			}
		}
		
		try{
			$this -> get_credential_keys($CLOUD_UUID,$ZONE_INFO);
					
			$result = $this -> Ec2Client -> runInstances($InstanceConfig);
			
			$INSTANCE_ID = $result['Instances'][0]['InstanceId'];
			
			#INSTANCE TAG METADATA
			$NOW_TIME = Misc_Class::current_utc_time();	
			$INSTANCE_METADATA = array
								(
									array(
										'Key' => 'Name',
										'Value' => $HOST_NAME.'@'.$NOW_TIME
									),
									array(
										'Key' => 'Description',
										'Value' => 'Instance Create By SaaSaMe Transport Service@'.$NOW_TIME.'',
									)
								);
			
			
			$this -> create_tags($CLOUD_UUID,$ZONE_INFO,$INSTANCE_ID,$INSTANCE_METADATA);
			
			return $INSTANCE_ID;			
		}
		catch(Exception $e){
			$this -> aws_debug(__FUNCTION__,$e->getMessage());
			return false;
		}
	}

	
	###########################
    #	WAIT UNTIL METHOD
    ###########################
	public function wait_until($CLOUD_UUID,$ZONE_INFO,$RESOURCE_ID,$WAITER_TYPE)
    {
        switch ($WAITER_TYPE)
        {
            case "InstanceRunning":
                $RESOURCE_TYPE = 'InstanceIds';
            break;
            
            case "InstanceStopped":
                $RESOURCE_TYPE = 'InstanceIds';
            break;
			
			case "InstanceTerminated":
				$RESOURCE_TYPE = 'InstanceIds';
			break;
                
            case "VolumeAvailable":
                $RESOURCE_TYPE = 'VolumeIds';
            break;
            
            case "VolumeInUse":
                $RESOURCE_TYPE = 'VolumeIds';
            break;
            
            case "SnapshotCompleted":
                $RESOURCE_TYPE = 'SnapshotIds';
            break;
        }
        
        try{
			$this -> get_credential_keys($CLOUD_UUID,$ZONE_INFO);
			
			$this -> Ec2Client -> waitUntil($WAITER_TYPE, [$RESOURCE_TYPE => array($RESOURCE_ID)]); 
        }
        catch(Exception $e){
			$this -> aws_debug(__FUNCTION__,$e->getMessage());
			return false;
		}
    }
    
	###########################
	#	POWER SWITCH INSTANCE
	###########################
	public function power_instance($CLOUD_UUID,$ZONE_INFO,$INSTANCE_ID,$ACTION)
	{
		try{
			$this -> get_credential_keys($CLOUD_UUID,$ZONE_INFO);

			if ($ACTION == 'PowerOn')
			{
				$result = $this -> Ec2Client -> startInstances(array(
													'InstanceIds' => array($INSTANCE_ID)
				));
				return $result['StartingInstances'][0];
			}
			else
			{
				$result = $this -> Ec2Client -> stopInstances(array(
													'InstanceIds' => array($INSTANCE_ID),
													'Force'		  => true
				));
				return $result['StoppingInstances'][0];
			}
		}
		catch(Exception $e){
			$this -> aws_debug(__FUNCTION__,$e->getMessage());
			return false;
		}
	}
	
	###########################
	#	DESCRIBE VOLUME
	###########################
	public function describe_volumes($CLOUD_UUID,$ZONE_INFO,$VOLUME_ID)
	{
		try{
			$this -> get_credential_keys($CLOUD_UUID,$ZONE_INFO);
			
			$result = $this -> Ec2Client -> describeVolumes(array('VolumeIds' => array($VOLUME_ID)));
			
			return $result['Volumes'];
		}
		catch(Exception $e){
			$this -> aws_debug(__FUNCTION__,$e->getMessage());
			return false;
		}		
	}
	
	###########################
	#	ATTACH VOLUME
	###########################
	public function attach_volume($CLOUD_UUID,$ZONE_INFO,$INSTANCE_ID,$VOLUME_ID,$OS_TYPE='MS')
	{
		$this -> detach_volume($CLOUD_UUID,$ZONE_INFO,$VOLUME_ID);
		
		$this -> wait_until($CLOUD_UUID,$ZONE_INFO,$VOLUME_ID,'VolumeAvailable');
			
		$MOUNT_POINT = $this -> get_last_block_device_mapping($CLOUD_UUID,$ZONE_INFO,$INSTANCE_ID,$OS_TYPE);
		
		try{
			$this -> get_credential_keys($CLOUD_UUID,$ZONE_INFO);
				
			$result = $this -> Ec2Client -> attachVolume(array(
												'InstanceId' => $INSTANCE_ID,
												'VolumeId' => $VOLUME_ID,									
												'Device' => $MOUNT_POINT
			));
			
			$this -> wait_until($CLOUD_UUID,$ZONE_INFO,$VOLUME_ID,'VolumeInUse');
			
			return $result;			
		}
		catch(Exception $e){
			$this -> aws_debug(__FUNCTION__,$e->getMessage());
			return $e;
		}		
	}
	
	###########################
	#	DETACH VOLUME
	###########################
	public function detach_volume($CLOUD_UUID,$ZONE_INFO,$VOLUME_ID)
	{
		$VOL_ATTACH_STATUS = $this -> describe_volumes($CLOUD_UUID,$ZONE_INFO,$VOLUME_ID)[0]['Attachments'];
		
		if (count($VOL_ATTACH_STATUS) == 1)
		{	
			$INSTANCE_ID = $VOL_ATTACH_STATUS[0]['InstanceId'];

			try{
				$this -> get_credential_keys($CLOUD_UUID,$ZONE_INFO);
				
				$result = $this -> Ec2Client -> detachVolume(array(
													'InstanceId' => $INSTANCE_ID,
													'VolumeId' 	 => $VOLUME_ID
				));
				return $result;			
			}
			catch(Exception $e){
				$this -> aws_debug(__FUNCTION__,$e->getMessage());
				return false;
			}
		}
	}
	
	
	###########################
	#	DELETE VOLUME
	###########################
	public function delete_volume($CLOUD_UUID,$ZONE_INFO,$VOLUME_ID)
	{
		$this -> detach_volume($CLOUD_UUID,$ZONE_INFO,$VOLUME_ID);
		
		sleep(5);
		
		$this -> wait_until($CLOUD_UUID,$ZONE_INFO,$VOLUME_ID,'VolumeAvailable');
			
		try{
			$this -> get_credential_keys($CLOUD_UUID,$ZONE_INFO);
				
			$result = $this -> Ec2Client -> deleteVolume(array('VolumeId' => $VOLUME_ID));
			return $result;			
		}
		catch(Exception $e){
			$this -> aws_debug(__FUNCTION__,$e->getMessage());
			return false;
		}
	}
	
	
	###########################
	#	TERMINATE INSTANCES
	###########################
	public function terminate_instances($CLOUD_UUID,$ZONE_INFO,$INSTANCE_ID,$DATAMODE_BOOTABLE)
	{
		if ($DATAMODE_BOOTABLE == TRUE)
		{
			#POWER OFF INSTANCE
			$this -> power_instance($CLOUD_UUID,$ZONE_INFO,$INSTANCE_ID,'PowerOff');
			sleep(30);
			
			#WAIT INSTANCE POWER OFF
			$this -> wait_until($CLOUD_UUID,$ZONE_INFO,$INSTANCE_ID,'InstanceStopped');
		}
		else
		{		
			try{
				$this -> get_credential_keys($CLOUD_UUID,$ZONE_INFO);
					
				$result = $this -> Ec2Client -> terminateInstances(array('InstanceIds' => array($INSTANCE_ID)));
				
				$this -> wait_until($CLOUD_UUID,$ZONE_INFO,$INSTANCE_ID,'InstanceTerminated');
				
				return $result;			
			}
			catch(Exception $e){
				$this -> aws_debug(__FUNCTION__,$e->getMessage());
				return false;
			}
		}
	}
	
	
	###########################
	#	DESCRIBE ELASTIC ADDRESS
	###########################
	public function describe_addresses($CLOUD_UUID,$ZONE_INFO,$ALLOCATION_ID)
	{
		try{
			$this -> get_credential_keys($CLOUD_UUID,$ZONE_INFO);
				
			if ($ALLOCATION_ID != null)
			{
				$ALLOCATION_FILTER = array('Filters' => array(array('Name' => 'allocation-id','Values' => array($ALLOCATION_ID))));				
			}
			else
			{
				$ALLOCATION_FILTER = array();
			}
				
				
			$result = $this -> Ec2Client -> describeAddresses($ALLOCATION_FILTER);
			
			return $result['Addresses'];			
		}
		catch(Exception $e){
			$this -> aws_debug(__FUNCTION__,$e->getMessage());
			return false;
		}
	}
	
	
	###########################
	#	DESCRIBE NETWORK INTERFACES
	###########################
	public function describe_network_interfaces($CLOUD_UUID,$ZONE_INFO,$VPC_ID)
	{
		try{
			$this -> get_credential_keys($CLOUD_UUID,$ZONE_INFO);
				
			$ALLOCATION_FILTER = array('Filters' => array(array('Name' => 'vpc-id','Values' => array($VPC_ID))));

			$result = $this -> Ec2Client -> describeNetworkInterfaces($ALLOCATION_FILTER);
			
			return $result['NetworkInterfaces'];			
		}
		catch(Exception $e){
			$this -> aws_debug(__FUNCTION__,$e->getMessage());
			return false;
		}
	}
	
	
	###########################
	#	ASSOCIATE PRIVATE ADDRESS
	###########################
	public function associate_private_addresses($CLOUD_UUID,$ZONE_INFO,$INSTANCE_ID,$PRIVATE_ADDR)
	{
		$NIC_ID = $this -> describe_instance($CLOUD_UUID,$ZONE_INFO,$INSTANCE_ID)[0]['Instances'][0]['NetworkInterfaces'][0]['NetworkInterfaceId'];
		
		try{
			$this -> get_credential_keys($CLOUD_UUID,$ZONE_INFO);
			
			$ASSOCIATE_CONFIG = array('NetworkInterfaceId' => $NIC_ID, 'PrivateIpAddresses' => array($PRIVATE_ADDR));
					
			$result = $this -> Ec2Client -> assignPrivateIpAddresses($ASSOCIATE_CONFIG);
			
			return $ASSOCIATE_CONFIG;		
		}
		catch(Exception $e){
			$this -> aws_debug(__FUNCTION__,$e->getMessage());
			return false;
		}
	}		
	
	
	###########################
	#	ASSOCIATE ELASTIC ADDRESS
	###########################
	public function associate_address($CLOUD_UUID,$ZONE_INFO,$INSTANCE_ID,$ALLOCATION_ID)
	{
		try{
			$this -> get_credential_keys($CLOUD_UUID,$ZONE_INFO);
			
			$ASSOCIATE_CONFIG = array('AllocationId' => $ALLOCATION_ID, 'InstanceId' => $INSTANCE_ID);
		
			$result = $this -> Ec2Client -> associateAddress($ASSOCIATE_CONFIG);
			
			return true;		
		}
		catch(Exception $e){
			$this -> aws_debug(__FUNCTION__,$e->getMessage());
			return false;
		}
	}
	
	###########################	
	#BEGIN TO CREATE AWS VOLUME
	###########################
	public function begin_volume_for_loader($CLOUD_UUID,$ZONE_INFO,$INSTANCE_ID,$VOLUME_SIZE,$HOST_NAME)
	{
		#CREATE EBS DISK
		$VOLUME_ID = $this -> create_volume($CLOUD_UUID,$ZONE_INFO,$VOLUME_SIZE,$HOST_NAME,'GP2');
		
		#SET WAIT FOR VOLUME STATUS IS AVAILABLE
		#$this -> wait_until($CLOUD_UUID,$ZONE_INFO,$VOLUME_ID,'VolumeAvailable');
				
		#ATTACH EBS TO TRANSPORT SERVER		
		$ATTACH_VOLUME = $this -> attach_volume($CLOUD_UUID,$ZONE_INFO,$INSTANCE_ID,$VOLUME_ID,'MS');
				
		#SET WAIT FOR VOLUME STATUS IS IN-USE
		$this -> wait_until($CLOUD_UUID,$ZONE_INFO,$VOLUME_ID,'VolumeInUse');
		
		$VOLUMD_INFO = (object)array(
								'serverId' 	 => $INSTANCE_ID.'|'.$ZONE_INFO,							
								'volumeId'	 => $VOLUME_ID,
								'volumeSize' => $VOLUME_SIZE,
								'volumePath' => $ATTACH_VOLUME['Device']
						);
		
		return $VOLUMD_INFO;
	}
	
	###########################
	#   BEGIN TO RUN AWS INSTANCE
	###########################   	
	public function begin_to_create_instance($SERV_UUID,$ZONE_INFO,$VOLUME_INFO,$HOST_NAME)
	{
		$ReplMgmt = new Replica_Class();
		$ServiceMgmt = new Service_Class();
		
		$SERV_QUERY = $ServiceMgmt -> query_service($SERV_UUID);
	
		$CLOUD_UUID = $SERV_QUERY['CLUSTER_UUID'];
		
		$OS_TYPE = $SERV_QUERY['OS_TYPE'];
		
		$DATAMODE_INSTANCE = json_decode($SERV_QUERY['JOBS_JSON']) -> datamode_instance;
		$DATAMODE_BOOTABLE = json_decode($SERV_QUERY['JOBS_JSON']) -> is_datamode_boot;
		
		if ($DATAMODE_INSTANCE != 'NoAssociatedDataModeInstance' AND $DATAMODE_BOOTABLE == TRUE)
		{
			#ATTACH CONVERT VOLUME
			$MESSAGE = $ReplMgmt -> job_msg('Attach recovery volumes to instance.'); #MESSAGE
			$ReplMgmt -> update_job_msg($SERV_UUID,$MESSAGE,'Service');
			for($i=0; $i<count($VOLUME_INFO); $i++)
			{
				$this -> attach_volume($CLOUD_UUID,$ZONE_INFO,$DATAMODE_INSTANCE,$VOLUME_INFO[$i],$OS_TYPE);
				sleep(10);
			}
			
			#POWER ON INSTANCE
			$MESSAGE = $ReplMgmt -> job_msg('Powering on the instance.'); #MESSAGE
			$ReplMgmt -> update_job_msg($SERV_UUID,$MESSAGE,'Service');
			$this -> power_instance($CLOUD_UUID,$ZONE_INFO,$DATAMODE_INSTANCE,'PowerOn');
	
			return $DATAMODE_INSTANCE;
		}
		
		$ELASTIC_ADDR = json_decode($SERV_QUERY['JOBS_JSON']) -> elastic_address_id;
		$PRIVATE_ADDR = json_decode($SERV_QUERY['JOBS_JSON']) -> private_address_id;

		$INSTANCE_CONFIG = (object)array(
									'InstanceType' 		=> $SERV_QUERY['FLAVOR_ID'],
									'SecurityGroup' 	=> $SERV_QUERY['SGROUP_UUID'],
									'SubnetId'			=> $SERV_QUERY['NETWORK_UUID'],
									'Hostname'			=> $HOST_NAME,
									'PrivateIpAddress'	=> $PRIVATE_ADDR
							);
		$INSTANCE_ID = $this -> run_instance($CLOUD_UUID,$ZONE_INFO,$INSTANCE_CONFIG,$OS_TYPE);
		
		if ($INSTANCE_ID != FALSE)
		{
			#GET TEMP ROOT VOLUME
			sleep(10);
			$INSTANCE_INFO = $this -> describe_instance($CLOUD_UUID,$ZONE_INFO,$INSTANCE_ID);
			$ROOT_DEVICE = $INSTANCE_INFO[0]['Instances'][0]['BlockDeviceMappings'][0]['Ebs']['VolumeId'];
			
			$MESSAGE = $ReplMgmt -> job_msg('Getting the instance root device.'); #MESSAGE
			$ReplMgmt -> update_job_msg($SERV_UUID,$MESSAGE,'Service');	
				
			#WAIT INSTANCE POWER ON
			$this -> wait_until($CLOUD_UUID,$ZONE_INFO,$INSTANCE_ID,'InstanceRunning');
						
			#POWER OFF INSTANCE
			$MESSAGE = $ReplMgmt -> job_msg('Powering off the instance.'); #MESSAGE
			$ReplMgmt -> update_job_msg($SERV_UUID,$MESSAGE,'Service');
			
			$this -> power_instance($CLOUD_UUID,$ZONE_INFO,$INSTANCE_ID,'PowerOff');
			sleep(30);
			
			#WAIT INSTANCE POWER OFF
			$this -> wait_until($CLOUD_UUID,$ZONE_INFO,$INSTANCE_ID,'InstanceStopped');
			
			$MESSAGE = $ReplMgmt -> job_msg('The instance powered off.'); #MESSAGE
			$ReplMgmt -> update_job_msg($SERV_UUID,$MESSAGE,'Service');
			
			#DETACH TEMP ROOT VOLUME
			$MESSAGE = $ReplMgmt -> job_msg('Detach temp root device from the instance.'); #MESSAGE
			$ReplMgmt -> update_job_msg($SERV_UUID,$MESSAGE,'Service');
			$this -> detach_volume($CLOUD_UUID,$ZONE_INFO,$ROOT_DEVICE);
			sleep(10);
			
			#ATTACH CONVERT VOLUME
			$MESSAGE = $ReplMgmt -> job_msg('Attach recovery volumes to instance.'); #MESSAGE
			$ReplMgmt -> update_job_msg($SERV_UUID,$MESSAGE,'Service');
			for($i=0; $i<count($VOLUME_INFO); $i++)
			{
				$this -> attach_volume($CLOUD_UUID,$ZONE_INFO,$INSTANCE_ID,$VOLUME_INFO[$i],$OS_TYPE);
				sleep(10);
			}
			
			#DELETE TEMP ROOT VOLUME
			$MESSAGE = $ReplMgmt -> job_msg('Delete the temp root device.'); #MESSAGE
			$ReplMgmt -> update_job_msg($SERV_UUID,$MESSAGE,'Service');
			$this -> delete_volume($CLOUD_UUID,$ZONE_INFO,$ROOT_DEVICE);
			
			
			#ASSOCIATE ELASTIC ADDRESS
			if ($ELASTIC_ADDR != 'DynamicAssign')
			{
				$MESSAGE = $ReplMgmt -> job_msg('Associate elastic address to instance.'); #MESSAGE
				$ReplMgmt -> update_job_msg($SERV_UUID,$MESSAGE,'Service');
				$this -> associate_address($CLOUD_UUID,$ZONE_INFO,$INSTANCE_ID,$ELASTIC_ADDR);
			}	
			
			#POWER ON INSTANCE
			$MESSAGE = $ReplMgmt -> job_msg('Powering on the instance.'); #MESSAGE
			$ReplMgmt -> update_job_msg($SERV_UUID,$MESSAGE,'Service');
			$this -> power_instance($CLOUD_UUID,$ZONE_INFO,$INSTANCE_ID,'PowerOn');
			
			return $INSTANCE_ID;
		}
		else
		{
			return false;
		}	
	}
	
	###########################
	# DESCRIBE ALL INSTANCES
	###########################
	public function describe_region_instances($CLOUD_UUID,$ZONE_INFO,$TRANSPORT_ID)
	{
		$LIST_REGION_INSTANCE = $this -> describe_instance($CLOUD_UUID,$ZONE_INFO)[0];
		
		$REGION_INSTANCE = array();
		for ($i=0; $i<count($LIST_REGION_INSTANCE); $i++)
		{
			if ($LIST_REGION_INSTANCE[$i]['Instances'][0]['Placement']['AvailabilityZone'] == $ZONE_INFO AND $LIST_REGION_INSTANCE[$i]['Instances'][0]['InstanceId'] != $TRANSPORT_ID AND $LIST_REGION_INSTANCE[$i]['Instances'][0]['State']['Code'] != 48)
			{
				$REGION_INSTANCE[] = $LIST_REGION_INSTANCE[$i]['Instances'][0];
			}
		}
		
		return $REGION_INSTANCE;
	}
	
		
	public function start_vm($CLOUD_UUID,$ZONE_INFO,$INSTANCE_ID)
	{
		$POWER_STATUS = $this -> describe_instance($CLOUD_UUID,$ZONE_INFO,$INSTANCE_ID)[0]['Instances'][0]['State']['Code'];
		if ($POWER_STATUS == 80)
		{
			$this -> power_instance($CLOUD_UUID,$ZONE_INFO,$INSTANCE_ID,'PowerOn');
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
#	AMAZON WEB SERVICES QUERY CLASS
#
###################################
class Aws_Query_Class extends Db_Connection
{
	###########################
	#CREATE NEW AWS CONNECTION
	###########################
	public function create_aws_connection($ACCT_UUID,$REGN_UUID,$AWS_REGION,$ACCESS_KEY,$SECRET_KEY)
	{
		$CLOUD_UUID  = Misc_Class::guid_v4();
		
		$AWS_TYPE = ($AWS_REGION == 'us-east-1')?"AWS":"AWS China";
		
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
								_CLOUD_TYPE,
								_STATUS)
							VALUE(
								'',
								
								'".$ACCT_UUID."',
								'".$REGN_UUID."',
								'".$CLOUD_UUID."',
								
								'".$AWS_TYPE."',								
								'".Misc_Class::encrypt_decrypt('encrypt',$ACCESS_KEY)."',
								'".Misc_Class::encrypt_decrypt('encrypt',$SECRET_KEY)."',
								'".$AWS_REGION."',
								
								'".$ACCESS_KEY."',
								
								'".Misc_Class::current_utc_time()."',
								'2',
								'Y')";
								
		$this -> DBCON -> prepare($INSERT_EXEC) -> execute();		
		return $AWS_TYPE;
	}
	
	
	###########################
	#UPDATE AWS CONNECTION
	###########################
	public function update_aws_connection($CLUSTER_UUID,$AWS_REGION,$ACCESS_KEY,$SECRET_KEY)
	{
		$AWS_TYPE = ($AWS_REGION == 'us-east-1')?"AWS":"AWS China";
		
		$UPDATE_EXEC = "UPDATE _CLOUD_MGMT
						SET
							_PROJECT_NAME	= '".$AWS_TYPE."',
							_CLUSTER_USER 	= '".Misc_Class::encrypt_decrypt('encrypt',$ACCESS_KEY)."',
							_CLUSTER_PASS	= '".Misc_Class::encrypt_decrypt('encrypt',$SECRET_KEY)."',
							_CLUSTER_ADDR	= '".$AWS_REGION."',
							_AUTH_TOKEN		= '".$ACCESS_KEY."',
							_TIMESTAMP		= '".Misc_Class::current_utc_time()."'
						WHERE
							_CLUSTER_UUID 	= '".$CLUSTER_UUID."'";
		
		$QUERY = $this -> DBCON -> prepare($UPDATE_EXEC) -> execute();
		return $AWS_TYPE;
	}
	
	
	###########################
	#QUERY AWS KEY INFORMATION
	###########################
	public function query_aws_connection_information($CLUSTER_UUID)
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
}