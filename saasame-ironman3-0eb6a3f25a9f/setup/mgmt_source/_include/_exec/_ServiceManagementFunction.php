<?php 

chdir(__DIR__.'\..\..');

require_once '_include/_inc/_class_aws.php';
require_once '_include/_inc/_class_openstack.php';
require_once '_include/_inc/Azure.php';
require_once '_include/_inc/Aliyun_Model.php';
require_once '_include/_inc/Aliyun.php';
require_once '_include/_inc/_class_mailer.php';
require_once '_include/_inc/_class_Tencent_Model.php';
require_once '_include/_inc/_class_Tencent.php';
require_once '_include/_inc/_class_azure_blob.php';
require_once '_include/_inc/_class_ctyun.php';
require_once '_include\_inc\_class_server.php';
require_once '_include\_inc\_class_service.php';
require_once '_include\_inc\_class_replica.php';

chdir('_include\_inc');
require_once '_class_VMWare.php';
function addHost( $AcctUUID, $RegnUUID, $OpenUUID, $ServUUID, $HostADDR, $HostUSER, $HostPASS, $ServTYPE, $SystTYPE, $PriorityAddr, $SelectVMHost ){
	
	$ServerMgmt  	= new Server_Class();
	$ServiceMgmt 	= new Service_Class();
	
	$SERV_INFO = $ServerMgmt -> query_server_info($ServUUID);
	if ($SERV_INFO['SERV_INFO']['direct_mode'] == true)
	{
		$ServADDR = $SERV_INFO['SERV_ADDR'];
	}
	else
	{
		$ServADDR = array($SERV_INFO['SERV_INFO']['machine_id']);
	}
				
	$HostADDR = explode(',', $HostADDR);

	switch ($ServTYPE)
	{
		case 'Virtual Packer':
			for ($s=0; $s<count($ServADDR); $s++)
			{
				try{
					$ServINFO = $ServiceMgmt -> get_connection($ServADDR[$s],$ServTYPE);
				
					if ($ServINFO != '')
					{
						for ($i=0; $i<count($HostADDR); $i++)
						{
							$VmsINFO = $ServiceMgmt -> virtual_host_info($ServADDR[$s],$HostADDR[$i],$HostUSER,$HostPASS);
							$PackerAddress = $HostADDR[$i];	
						
							if (isset($SelectVMHost))
							{
								$SelectVMS = $SelectVMHost;
							}
							else
							{
								$SelectVMS = false;
							}								
						}
						break;
					}
				}
				catch (Throwable $e){
			
				}
			}						
		break;
		
		case 'Physical Packer':
			for ($s=0; $s<count($ServADDR); $s++)
			{
				for ($i=0; $i<count($HostADDR); $i++)
				{
					$VmsINFO = $ServiceMgmt -> physical_host_info($ServADDR[$s],$HostADDR[$i]);
					if ($VmsINFO != '')
					{
						$PackerAddress = $HostADDR[$i];
						break 2;
					}
				}
			}
			
			#REPLACE MANUFACTURER INFORMATION
			if ($HostPASS == '')
			{
				$VmsINFO -> manufacturer = 'Generic Physical';
			}
			
			#SET CARRIER PRIORITY ADDRESS
			$VmsINFO -> priority_addr = $PriorityAddr;
			
			#MARK AS DIRECT MODE
			$VmsINFO -> direct_mode = true;
			
			$ServINFO = $VmsINFO;
			$SelectVMS = false;
		break;
		
		case 'Offline Packer':
			for ($s=0; $s<count($ServADDR); $s++)
			{
				for ($i=0; $i<count($HostADDR); $i++)
				{
					$VmsINFO  = $ServiceMgmt -> physical_host_info($ServADDR[$s],$HostADDR[$i]);
					if ($VmsINFO != '')
					{
						$PackerAddress = $HostADDR[$i];
						break 2;
					}							
				}
			}
			
			#SET CARRIER PRIORITY ADDRESS
			$VmsINFO -> priority_addr = $PriorityAddr;
			
			#MARK AS DIRECT MODE
			$VmsINFO -> direct_mode = true;
			
			$ServINFO = $VmsINFO;
			$SelectVMS = false;
		break;
		
		default:
			$ServINFO = $ServiceMgmt -> get_connection($ServADDR,$ServTYPE);
			
			#SET CARRIER PRIORITY ADDRESS
			$VmsINFO -> priority_addr = $PriorityAddr;
			
			#MARK AS DIRECT MODE
			$ServINFO -> direct_mode = true;
			
			$VmsINFO  = "";
			$SelectVMS = false;
		break;
	}
	
	print_r($ServerMgmt -> initialize_server($AcctUUID,$RegnUUID,$OpenUUID,$ServUUID,$ServADDR,$PackerAddress,$HostUSER,$HostPASS,$ServTYPE,$ServINFO,$VmsINFO,$SystTYPE,$SelectVMS,true));
}

?>