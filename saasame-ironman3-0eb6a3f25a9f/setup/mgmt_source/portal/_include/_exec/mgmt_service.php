<?php
set_time_limit(0);
session_start();
include '../_inc/_class_main.php';
New Misc();

define('ROOT_PATH', __DIR__);
require_once(ROOT_PATH . "\..\_inc\languages\setlang.php");
	
function CurlPostToRemote($REST_DATA,$TIME_OUT = 0)
{
	$URL = 'http://127.0.0.1:8080/restful/ServiceManagement';
	
	$REST_DATA['EncryptKey'] = Misc::encrypt_decrypt('encrypt',time());
	$ENCODE_DATA = json_encode($REST_DATA);
	
	$ch = curl_init($URL);
	curl_setopt($ch, CURLOPT_CUSTOMREQUEST, "POST");
	curl_setopt($ch, CURLOPT_HEADER, false);
	curl_setopt($ch, CURLOPT_POSTFIELDS, $ENCODE_DATA);
	curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
	curl_setopt($ch, CURLOPT_HTTPHEADER, array('Content-Type: application/json','Content-Length: ' . strlen($ENCODE_DATA)));
	curl_setopt($ch, CURLOPT_TIMEOUT_MS, $TIME_OUT);
	$output = curl_exec($ch);	

	if (is_string($output) && is_array(json_decode($output, true)) == TRUE)
	{
		$output = json_decode($output);
		if (isset($output -> Msg) AND !is_object($output -> Msg))
		{
			$output -> Msg = _($output -> Msg);
		}	
		return json_encode($output);
	}
	else
	{
		return $output;
	}
}

$SelectAction = $_REQUEST["ACTION"];
switch ($SelectAction)
{
	############################
	# UNSET SESSION
	############################
	case "UnsetSession":
	
	$ACTION = 'ReplicaUnsetSession';
	
	unset($_SESSION['WINPE_JOB']);
	unset($_SESSION['CLUSTER_UUID']);
	unset($_SESSION['CLOUD_TYPE']);
	unset($_SESSION['VENDOR_NAME']);
	
	unset($_SESSION['REPL_UUID']);
	unset($_SESSION['HOST_UUID']);
	unset($_SESSION['HOST_OS']);
	unset($_SESSION['HOST_TYPE']);
	unset($_SESSION['PRIORITY_ADDR']);	
	unset($_SESSION['LOAD_UUID']);
	unset($_SESSION['LAUN_UUID']);
	unset($_SESSION['SERV_TYPE']);	
	unset($_SESSION['HOST_NAME']);
	unset($_SESSION['PLAN_UUID']);
	unset($_SESSION['PLAN_JSON']);
	unset($_SESSION['EDIT_PLAN']);
	unset($_SESSION['REPL_OS_TYPE']);
	unset($_SESSION['IsAzureBlobMode']);
	
	/* WORKLOAD CONFIGURATION */
	//unset($_SESSION['WORKLOAD_SETTINGS']);
	
	/* REPLICA CONFIGURATION */
	unset($_SESSION['REPLICA_SETTINGS']);
	unset($_SESSION['EDIT_REPLICA_UUID']);

	unset($_SESSION['SERVICE_SETTINGS']);
	
	unset($_SESSION['SERV_UUID']);
	unset($_SESSION['SERV_REGN']);		
	unset($_SESSION['RECY_TYPE']);
	unset($_SESSION['SNAP_UUID']);
	unset($_SESSION['FLAVOR_ID']);
	unset($_SESSION['ZONE_UUID']);
	unset($_SESSION['NETWORK_UUID']);
	unset($_SESSION['SGROUP_UUID']);
	unset($_SESSION['PRIVATE_ADDR_ID']);								 
	unset($_SESSION['PUBLIC_ADDR_ID']);
	unset($_SESSION['RG']);
	unset($_SESSION['RCVY_PRE_SCRIPT']);
	unset($_SESSION['RCVY_POST_SCRIPT']);
	unset($_SESSION['SWITCH_UUID']);
	unset($_SESSION['SUBNET_UUID']);
	unset($_SESSION['SERVER_UUID']);
	unset($_SESSION['VOLUME_UUID']);

	break;
	
	############################
	# LIST CLOUD
	############################
	case "ListCloud":
	
	$ACTION    = 'ListCloud';
	$REST_DATA = array(
					'Action' 			=> $ACTION,
					'AcctUUID'			=> $_REQUEST['ACCT_UUID']				
				);
	
	$output = CurlPostToRemote($REST_DATA);
	
	$JSON_DECODE = json_decode($output);
	
	if ($JSON_DECODE != FALSE)
	{
		for ($i=0; $i<count($JSON_DECODE); $i++)
		{
			$LIST_CLOUD[] = array(
							'CLUSTER_UUID' 	=> $JSON_DECODE[$i] -> CLUSTER_UUID,
							'PROJECT_NAME' 	=> $JSON_DECODE[$i] -> PROJECT_NAME,
							'CLUSTER_ADDR' 	=> $JSON_DECODE[$i] -> CLUSTER_ADDR,
							'CLUSTER_USER' 	=> $JSON_DECODE[$i] -> CLUSTER_USER,
							'TIMESTAMP' 	=> Misc::time_convert_with_zone($JSON_DECODE[$i] -> TIMESTAMP),
							'CLOUD_TYPE'	=> $JSON_DECODE[$i] -> CLOUD_TYPE,
							'VENDOR_TYPE'	=> $JSON_DECODE[$i] -> VENDOR,
							'REGION_NAME'	=> $JSON_DECODE[$i] -> REGION,
							'TransportName'	=> $JSON_DECODE[$i] -> TransportName
							);	
		}
		$Response = $LIST_CLOUD;
	}
	else
	{
		$Response = false;
	}
	print_r(json_encode($Response));
	break;
	
	############################
	# Edit Select Cloud
	############################
	case "EditSelectCloud":
	
	$ACTION = 'EditSelectCloud';	
	if (isset($_REQUEST['CLUSTER_UUID']))
	{	
		$REST_DATA = array(
						'Action' 			=> $ACTION,
						'ClusterUUID'		=> $_REQUEST['CLUSTER_UUID']									
					);
	
		$output = CurlPostToRemote($REST_DATA);		
		if ($output == false)
		{
			$Response = array('Code' => false, 'Msg' => _('Cannot get connection information.'));
		}
		else
		{
			$_SESSION['CLUSTER_UUID'] = $_REQUEST['CLUSTER_UUID'];
			
			$CLUSTER_ADDR = json_decode($output, false) -> CLUSTER_ADDR;
			if (strpos($CLUSTER_ADDR, 'myhuaweicloud') !== false)
			{
				$REDIRECT_TYPE = 'HuaweiCloud';
			}
			else
			{			
				$REDIRECT_TYPE = json_decode($output, false) -> CLOUD_TYPE;
			}
			$Response = array('Code' => true, 'Msg' => $REDIRECT_TYPE);
		}
	}
	else
	{
		$Response = array('Code' => false, 'Msg' => _('Please select a cloud connection.'));
	}
	echo json_encode($Response);	
	break;
	
	############################
	# Delete Select Cloud
	############################
	case "DeleteSelectCloud":
	
	$ACTION    = 'DeleteSelectCloud';	
	if (isset($_REQUEST['CLUSTER_UUID']))
	{		
	
		$REST_DATA = array(
						'Action' 			=> $ACTION,
						'ClusterUUID'		=> $_REQUEST['CLUSTER_UUID']									
					);
	
		$output = json_decode(CurlPostToRemote($REST_DATA));
		
		$Response = array('Code' => $output -> Code, 'Msg' => $output -> Msg);
	}
	else
	{
		$Response = array('Code' => false, 'Msg' => _('Please select a cloud connection.'));
	}
	echo json_encode($Response);	
	break;
	
	############################
	# LIST CLOUD DISK
	############################
	case "ListCloudDisk":
	
	$ACTION    = 'ListCloudDisk';
	$REST_DATA = array('Action'   => $ACTION,
					   'ReplUUID' => $_REQUEST['REPL_UUID']
				);
	
	
	$output = CurlPostToRemote($REST_DATA);
	
	print_r($output);	
	break;
	
	############################
	# SELECT CLUSTER UUID
	############################
	case "SelectOpenStackClusterUUID":
	
	$ACTION    = 'SelectOpenStackClusterUUID';
	if (isset($_REQUEST['CLOUD_INFO']))
	{
		$CLOUD_INFO = explode('|', $_REQUEST['CLOUD_INFO']);
		$CLUSTER_UUID = $CLOUD_INFO[0];
		$TYPE_CLOUD = $CLOUD_INFO[1];
		$CLOUD_TYPE = $CLOUD_INFO[2];
		
		$_SESSION['CLUSTER_UUID'] = $CLUSTER_UUID;
		$_SESSION['CLOUD_TYPE']   = $CLOUD_TYPE;
		print_r(json_encode(array('code' => true, 'RDIR' => $CLOUD_TYPE)));
	}
	else
	{
		print_r(json_encode(array('code' => false, 'RDIR' => _('Please select a cloud connection.'))));
	}
	break;
	
	
	############################
	# SELECT INSTANCE UUID
	############################
	case "SelectedInstance":
	
	$ACTION = 'SelectedInstance';
	if (isset($_REQUEST['HOST_UUID']))
	{
		$_SESSION['HOST_UUID'] = $_REQUEST['HOST_UUID'];
		
		$data = explode( '|', $_SESSION['HOST_UUID'] );
		
		if (isset($data[2]))
		{
			$_SESSION['RG'] = $data[2];
		}
		
		if (isset($data[3]))
		{
			$_SESSION['AzureManageTransport'] = $data[3];
		}
		
		print_r(json_encode(array('code' => true, 'msg' => '')));
	}
	else
	{
		
		print_r(json_encode(array('code' => false, 'msg' => _('Please select an instance.'))));
	}	
	break;
	
		
	############################
	# VERIFY FOLDER CONNECTION
	############################
	case "VerifyFolderConnection":
	
	$ACTION    = 'VerifyFolderConnection';
	$REST_DATA = array('Action'   => $ACTION,
					   'ServADDR' => $_REQUEST['SERV_ADDR'],
					   'ConnDEST' => $_REQUEST['CONN_DEST']
				);
	$output = CurlPostToRemote($REST_DATA);
	
	if ($output != FALSE)
	{
		echo json_encode(true);
	}
	else
	{
		echo json_encode('Cannot access '.$output);
	}
	break;
	
		
	############################
	# QUERY AVAILABLE SERVICE
	############################
	case "QueryAvailableService":

	$ACTION    = 'QueryAvailableService';	
	$REST_DATA = array(
					'Action' 	=> $ACTION,
					'AcctUUID' 	=> $_REQUEST['ACCT_UUID'],
					'ServTYPE'	=> $_REQUEST['SERV_TYPE'],
					'SystTYPE'	=> $_REQUEST['SYST_TYPE']
				);
	$output = CurlPostToRemote($REST_DATA);
	
	print_r($output);
	break;
	
	
	############################
	# QUERY SELECT TRANSPORT INFO
	############################
	case "QuerySelectTransportInfo":
	
	$ACTION    = 'QuerySelectTransportInfo';	
	$REST_DATA = array(
					'Action' 	=> $ACTION,
					'ServUUID'	=> $_REQUEST['SERV_UUID']
				);
	$output = CurlPostToRemote($REST_DATA);
	
	$SERVER_INFO = json_decode($output);

	$TS_Hostname 		= $SERVER_INFO -> HOST_NAME;
	$TS_Path			= chop($SERVER_INFO -> SERV_INFO -> path, '\Launcher');
	$TS_Version  		= $SERVER_INFO -> SERV_INFO -> version;
	$TS_Manufacturer  	= $SERVER_INFO -> SERV_INFO -> manufacturer;
	$TS_CPUCore  		= $SERVER_INFO -> SERV_INFO -> logical_processors;
	$TS_MemorySize		= $SERVER_INFO -> SERV_INFO -> physical_memory;
	$TS_OSType			= $SERVER_INFO -> SERV_INFO -> os_name;
	$TS_NetworkInfo 	= $SERVER_INFO -> SERV_INFO -> network_infos;
	$TS_DiskInfo		= $SERVER_INFO -> SERV_INFO -> disk_infos;
	$TS_Address			= $SERVER_INFO -> SERV_ADDR;
	$IS_RCD				= $SERVER_INFO -> SERV_INFO -> is_winpe;
	
	$SERV_INFO_ARRAY = array(
							_('Host Name') 				=> $TS_Hostname,
							_('Installation Path') 		=> $TS_Path.' ('.$TS_Version.')',
							_('OS Type')				=> $TS_OSType,
							_('CPU / Memory')			=> $TS_CPUCore.'Cores / '.$TS_MemorySize.'MB',
							'Network Info'				=> $TS_NetworkInfo,
							_('Access address')			=> implode(", ", $TS_Address)
						);
	
	if ($IS_RCD == TRUE)
	{
		$SERV_INFO_ARRAY = array_slice($SERV_INFO_ARRAY, 0, 4, true) + array('Disk Info' => $TS_DiskInfo) + array_slice($SERV_INFO_ARRAY, 4, NULL, true);
	}
	
	$TRANSPORT_INFO_TABLE  = '';
	$TRANSPORT_INFO_TABLE .= '<table class="TransportTable">';
	$TRANSPORT_INFO_TABLE .= '<thead><tr>';
	$TRANSPORT_INFO_TABLE .= '<th width="800px" colspan="2" style="line-height:30px;">'._('Server').' <button id="TakeXray" class="btn btn-success pull-right btn-sm"><i id="UpdateSensorSpin" class="fa fa-cog fa-lg fa-fw"></i> X-Ray</button></th>';
			
	foreach($SERV_INFO_ARRAY as $Name => $Value)
	{
		switch ($Name)
		{
			case 'Disk Info':
				for ($i=0; $i<count($Value); $i++)
				{
					$DISK_SIZE = ($Value[$i] -> size)/1024/1024/1024;						
					
					$TRANSPORT_INFO_TABLE .= '<tr>';
					$TRANSPORT_INFO_TABLE .= '<td width="150px">'._('Disk').' '.$i.'</td>';
					$TRANSPORT_INFO_TABLE .= '<td width="650px">'.$Value[$i] -> friendly_name.' ('.$DISK_SIZE.'GB)</div></td>';
					$TRANSPORT_INFO_TABLE .= '</tr>';						
				}				
			break;
				
			case 'Network Info':
				for ($i=0; $i<count($Value); $i++)
				{
					$TRANSPORT_INFO_TABLE .= '<tr>';
					$TRANSPORT_INFO_TABLE .= '<td width="150px">'._('Adapter').' '.$i.'</td>';
					
					if( isset( $Value[$i] -> ip_addresses[0] ) )
						$TRANSPORT_INFO_TABLE .= '<td width="650px">'.$Value[$i] -> description.' ('.$Value[$i] -> ip_addresses[0].')</div></td>';
					else
						$TRANSPORT_INFO_TABLE .= '<td width="650px">'.$Value[$i] -> description.' (None)</div></td>';
					$TRANSPORT_INFO_TABLE .= '</tr>';
				}				
			break;
		
			default:			
				$TRANSPORT_INFO_TABLE .= '<tr>';
				$TRANSPORT_INFO_TABLE .= '<td width="150px">'.$Name.'</td>';
				$TRANSPORT_INFO_TABLE .= '<td width="650px">'.$Value.'</div></td>';
				$TRANSPORT_INFO_TABLE .= '</tr>';
		}
	}
	
	$TRANSPORT_INFO_TABLE .= '</tbody></table>';
	
	$TRANSPORT_INFO_TABLE .= '<script>';
	$TRANSPORT_INFO_TABLE .= '$("#TakeXray").click(function TakeXray(){$("#UpdateSensorSpin").addClass("fa-spin");$("#LoadingOverLay").addClass("TransparentOverlay SpinnerLoading");$.ajax({type:"POST",dataType:"JSON",url:"_include/_exec/mgmt_service.php",';
	$TRANSPORT_INFO_TABLE .= 'data:{"ACTION":"TakeXray","ACCT_UUID":"'.$_SESSION['admin']['ACCT_UUID'].'","XRAY_PATH":"'.str_replace("\\","/",str_replace("_exec/mgmt_service.php","_inc/_xray/",$_SERVER['SCRIPT_FILENAME'])).'","XRAY_TYPE":"server","SERV_UUID":"'.$_REQUEST["SERV_UUID"].'"},';
	$TRANSPORT_INFO_TABLE .= 'success:function(xray){$("#UpdateSensorSpin").removeClass("fa-spin");$("#LoadingOverLay").removeClass("TransparentOverlay SpinnerLoading");if (xray != false){window.location.href = "./_include/_inc/_xray/"+xray}}});})';
	$TRANSPORT_INFO_TABLE .= '</script>';
	
	print_r($TRANSPORT_INFO_TABLE);
	break;
	
		
	############################
	# QUERY AVAILABLE HOST
	############################
	case "QueryAvailableHost":
	
	unset($_SESSION['HOST_UUID']);	
	$ACTION    = 'QueryAvailableHost';	
	$REST_DATA = array(
					'Action' 	=> $ACTION,
					'AcctUUID' 	=> $_REQUEST['ACCT_UUID'],
					'ServTYPE'	=> $_REQUEST['SERV_TYPE']
				);
	$output = CurlPostToRemote($REST_DATA);
		
	print_r($output);	
	break;
	
	############################
	# TEST SERVICE CONNECTION
	############################
	case "TestServiceConnection":
	
	if ($_REQUEST['SERV_TYPE'] == 'Physical Packer' or $_REQUEST['SERV_TYPE'] == 'Offline Packer')
	{
		if ($_REQUEST['HOST_ADDR'] != '')
		{
			$ACTION    = 'TestServiceConnection';	
			$REST_DATA = array('Action'   => $ACTION,
							   'AcctUUID' => $_REQUEST['ACCT_UUID'],
							   'ServUUID' => $_REQUEST['SERV_UUID'],
						       'HostADDR' => $_REQUEST['HOST_ADDR'],
						       'HostUSER' => $_REQUEST['HOST_USER'],
						       'HostPASS' => $_REQUEST['HOST_PASS'],
							   'HostUUID' => $_REQUEST['HOST_UUID'],
						       'ServTYPE' => $_REQUEST['SERV_TYPE']
							);
			$PhysicalPackerOutput = json_decode(CurlPostToRemote($REST_DATA));

			$Response = json_encode(array('Code' => $PhysicalPackerOutput -> Code, 'Msg' => $PhysicalPackerOutput -> Msg, 'OS_Type' => $PhysicalPackerOutput -> OS_Type, 'Server_Addr' => $PhysicalPackerOutput -> Server_Addr));
		}
		else
		{
			$Response = json_encode(array('Code' => false, 'Msg' => _('Please enter the IP address of the host.'), 'OS_Type' => '', 'Server_Addr' => ''));
		}
	}
	elseif ($_REQUEST['SERV_TYPE'] == 'Virtual Packer')
	{
		if ($_REQUEST['HOST_ADDR'] != '' and $_REQUEST['HOST_USER'] != '' and $_REQUEST['HOST_PASS'] != '')
		{
			$ACTION    = 'TestServiceConnection';	
			$REST_DATA = array('Action'   => $ACTION,
			                   'AcctUUID' => $_REQUEST['ACCT_UUID'],
							   'ServUUID' => $_REQUEST['SERV_UUID'],
						       'HostADDR' => $_REQUEST['HOST_ADDR'],
						       'HostUSER' => $_REQUEST['HOST_USER'],
						       'HostPASS' => $_REQUEST['HOST_PASS'],
							   'HostUUID' => $_REQUEST['HOST_UUID'],
						       'ServTYPE' => $_REQUEST['SERV_TYPE']
							);
			$VirtualPackerOutput = json_decode(CurlPostToRemote($REST_DATA));

			$Response = json_encode(array('Code' => $VirtualPackerOutput -> Code, 'Msg'=> $VirtualPackerOutput -> Msg, 'OS_Type' => ''));
		}
		else
		{
			$Response = json_encode(array('Code' => false, 'Msg'=> _('Please fill in all the required fields'), 'OS_Type' => ''));
		}
	}
	else
	{
		$Response = json_encode(array('Code' => false, 'Msg'=> _('Wrong packer type please contact support.'), 'OS_Type' => ''));
	}
	print_r($Response);	
	break;
	
	############################
	# TEST ALL IN ONE SERVICE CONNECTION
	############################
	case "TestServicesConnection":
	$ACTION = 'TestServicesConnection';

	if ($_REQUEST['SERV_ADDR'] != '')
	{
		$REST_DATA = array('Action'   => $ACTION,
						   'ServADDR' => $_REQUEST['SERV_ADDR'],
						   'SeltSERV' => $_REQUEST['SELT_SERV'],
						   'MgmtADDR' => $_REQUEST['MGMT_ADDR']
					);
		$output = CurlPostToRemote($REST_DATA);
	}
	else
	{
		$MissingInput = array('Code' => false, 'Msg' => _('Please enter the IP address of the Transport server.'));
		$output = json_encode($MissingInput);
	}
	
	print_r($output);
	break;

	############################
	# INITIALIZE NEW SERVICE (ALL-IN-ONE)
	############################
	case "InitializeNewServices":
	
	$ACTION = 'InitializeNewServices';
	
	if (isset($_REQUEST['MGMT_DISK']))
	{
		$MGMT_DISK = $_REQUEST['MGMT_DISK'];
	}
	else
	{
		$MGMT_DISK = false;
	}
	
	$REST_DATA = array(
					'Action'   		=> $ACTION,
					'AcctUUID' 		=> $_REQUEST['ACCT_UUID'],
					'RegnUUID' 		=> $_REQUEST['REGN_UUID'],
					'OpenUUID' 		=> $_REQUEST['OPEN_UUID'],
					'HostUUID' 		=> $_REQUEST['HOST_UUID'],
					'ServADDR' 		=> $_REQUEST['SERV_ADDR'],
					'SeltSERV' 		=> $_REQUEST['SELT_SERV'],
					'SystTYPE' 		=> $_REQUEST['SYST_TYPE'],
					'MgmtADDR' 		=> $_REQUEST['MGMT_ADDR'],
					'ConnDEST' 		=> $_REQUEST['CONN_DEST'],
					'MgmtDisk'		=> $MGMT_DISK
				);
	$output = CurlPostToRemote($REST_DATA);

	$NewService = json_decode($output,TRUE);
	
	if ($NewService == false)
	{
		$Response = array('Code' => false, 'Msg' => _('Cannot add services.'));
	}
	else
	{		
		$Response = array('Code' => true, 'Msg' => _('Transport server added.'));
	}
		
	print_r(json_encode($Response));
	break;

	############################
	# QUERY VIRTUAL HOST LIST
	############################
	case "QueryVirtualHostList":
		$ACTION    = 'QueryVirtualHostList';
		$REST_DATA = array(
						'Action'   => $ACTION,
						'AcctUUID' => $_REQUEST['ACCT_UUID'],
						'ServUUID' => $_REQUEST['SERV_UUID'],
						'HostADDR' => $_REQUEST['HOST_ADDR'],
						'HostUSER' => $_REQUEST['HOST_USER'],						
						'HostPASS' => $_REQUEST['HOST_PASS']
					);
		$output = CurlPostToRemote($REST_DATA);

		$VirtualHostList = json_decode($output,FALSE);

		$VIRTUAL_HOST_TABLE = '';		
		$VIRTUAL_HOST_TABLE .= '<table id="VmHostTable">';
		$VIRTUAL_HOST_TABLE .= '<thead><tr>';
		$VIRTUAL_HOST_TABLE .= '<th width="60px" class="TextCenter"><input type="checkbox" id="checkAll" /></th>';
		$VIRTUAL_HOST_TABLE .= '<th width="730px">'._('Virtual Machine Name').'</th>';
		$VIRTUAL_HOST_TABLE .= '<th width="60px" class="TextCenter">'._('Details').'</th>';
		$VIRTUAL_HOST_TABLE .= '</tr></thead><tbody>';
	
		for($i=0; $i<count($VirtualHostList); $i++)
		{
			$IN_DB_LIST = $VirtualHostList[$i] -> in_db_list;
			if ($IN_DB_LIST == TRUE)
			{
				$DISABLE = ' disabled';
			}
			else
			{
				$DISABLE = '';
			}
			
			#CHECKED SELECT VM
			if ($VirtualHostList[$i] -> vm_uuid == $_REQUEST['SELECT_VM'])
			{
				$CHECKED = ' checked';
			}
			else
			{
				$CHECKED = '';
			}
			
			$VIRTUAL_HOST_TABLE .= '<tr>';
			$VIRTUAL_HOST_TABLE .= '<td width="60px" class="TextCenter"><input type="checkbox" name="select_vm" value="'.$VirtualHostList[$i] -> vm_uuid.'" '.$DISABLE.''.$CHECKED.'></td>';
			$VIRTUAL_HOST_TABLE .= '<td width="730px"><div class="HostListOverFlow">'.$VirtualHostList[$i] -> name.'</div></td>';
			$VIRTUAL_HOST_TABLE .= '<td width="60px" class="TextCenter"><div id="VmHostDetail" data-repl="'.$VirtualHostList[$i] -> vm_uuid.'" style="cursor:pointer;"><i class="fa fa-indent fa-lg"></i></td>';
			$VIRTUAL_HOST_TABLE .= '</tr>';
		}
		
		$VIRTUAL_HOST_TABLE .= '</tbody></table>';
		
		#BEGIN JAVASCRIPT
		$VIRTUAL_HOST_TABLE .= '<script>';	
		
		#DATATABLE Select VM Host Information
		if(count($VirtualHostList) > 10)
		{
			$VIRTUAL_HOST_TABLE .= '$(document).ready(function(){';
			$VIRTUAL_HOST_TABLE .= '$("#VmHostTable").DataTable({';
			$VIRTUAL_HOST_TABLE .= 'paging		: true,';
			$VIRTUAL_HOST_TABLE .= 'Info		: false,';
			$VIRTUAL_HOST_TABLE .= 'bFilter		: true,';
			$VIRTUAL_HOST_TABLE .= 'lengthChange: false,';
			$VIRTUAL_HOST_TABLE .= 'pageLength	: 10,';
			$VIRTUAL_HOST_TABLE .= 'pagingType	: "simple_numbers",';
			$VIRTUAL_HOST_TABLE .= 'ordering	: false,';					
			$VIRTUAL_HOST_TABLE .= 'order		: [],';
			$VIRTUAL_HOST_TABLE .= 'aoColumns	: [null,null,null],';
			$VIRTUAL_HOST_TABLE .= 'language	: {search: "_INPUT_"}';
			$VIRTUAL_HOST_TABLE .= '});';
		
			$VIRTUAL_HOST_TABLE .= '$("div.dataTables_filter input").attr("placeholder",$.parseHTML("&#xF002; Search")[0].data);';
			$VIRTUAL_HOST_TABLE .= '$("div.dataTables_filter input").addClass("form-control input-sm");';
			$VIRTUAL_HOST_TABLE .= '$("div.dataTables_filter input").css("max-width", "180px");';
			$VIRTUAL_HOST_TABLE .= '$("div.dataTables_filter input").css("padding-right", "40px");';
			$VIRTUAL_HOST_TABLE .= '});';
		}
	
		#AJAX Select VM Host Information
		$VIRTUAL_HOST_TABLE .= '$("#VmHostTable").on("click", "#VmHostDetail", function(e){';
		$VIRTUAL_HOST_TABLE .= '$("#LoadingOverLay").addClass("TransparentOverlay SpinnerLoading");';
		$VIRTUAL_HOST_TABLE .= '$("#VmHostDetail").prop("disabled", true);';
		$VIRTUAL_HOST_TABLE .= 'var HOST_UUID = $(this).attr("data-repl");';		
		$VIRTUAL_HOST_TABLE .= 'QuerySelectVmHostInfo(HOST_UUID);';
		$VIRTUAL_HOST_TABLE .= '});';
		
		$VIRTUAL_HOST_TABLE .= 'function QuerySelectVmHostInfo(HOST_UUID){';
		$VIRTUAL_HOST_TABLE .= '$.ajax({';
		$VIRTUAL_HOST_TABLE .= 'type:"POST",';
		$VIRTUAL_HOST_TABLE .= 'dataType:"TEXT",';
		$VIRTUAL_HOST_TABLE .= 'url:"_include/_exec/mgmt_service.php",';
		$VIRTUAL_HOST_TABLE .= 'data:{"ACTION":"QuerySelectVmHostInfo","SERV_UUID":"'.$_REQUEST['SERV_UUID'].'","HOST_ADDR":"'.$_REQUEST['HOST_ADDR'].'","HOST_USER":"'.$_REQUEST['HOST_USER'].'","HOST_PASS":"'.$_REQUEST['HOST_PASS'].'","SELECT_VM":HOST_UUID},';
		$VIRTUAL_HOST_TABLE .= 'success:function(jso){';
		$VIRTUAL_HOST_TABLE .= '$("#LoadingOverLay").removeClass("TransparentOverlay SpinnerLoading");';
		$VIRTUAL_HOST_TABLE .= '$("#VmHostDetail").prop("disabled", false);';
		$VIRTUAL_HOST_TABLE .= 'window.setTimeout(function(){BootstrapDialog.show({title: "'._('Host Information').'",cssClass: "workload-dialog",message: jso,type: BootstrapDialog.TYPE_PRIMARY,draggable: true});}, 0);';
		$VIRTUAL_HOST_TABLE .= '},';
		$VIRTUAL_HOST_TABLE .= 'error: function(xhr){';
		$VIRTUAL_HOST_TABLE .= '$("#LoadingOverLay").removeClass("TransparentOverlay SpinnerLoading");';	
		$VIRTUAL_HOST_TABLE .= '}';
		$VIRTUAL_HOST_TABLE .= '});}';	
				
		#SELECT ALL CHECK
		$VIRTUAL_HOST_TABLE .= '$("#checkAll").click(function() {$("#VmHostTable input:checkbox:enabled").prop("checked", this.checked);});';		
		$VIRTUAL_HOST_TABLE .= '</script>';		
		
		print_r($VIRTUAL_HOST_TABLE);
	break;
	
	############################
	# QUERY SELECT VM HOST INFO
	############################
	case "QuerySelectVmHostInfo":
		$ACTION    = 'QuerySelectVmHostInfo';
		$REST_DATA = array(
						'Action'   => $ACTION,
						'ServUUID' => $_REQUEST['SERV_UUID'],
						'HostADDR' => $_REQUEST['HOST_ADDR'],
						'HostUSER' => $_REQUEST['HOST_USER'],						
						'HostPASS' => $_REQUEST['HOST_PASS'],
						'SelectVM' => $_REQUEST['SELECT_VM']
					);
		$output = CurlPostToRemote($REST_DATA);
		$SELECT_VM = json_decode($output,FALSE);
		
		$VIRTUAL_HOST_DETAIL = '';
		$VIRTUAL_HOST_DETAIL .= '<table id="VmHostDetail">';
		#$VIRTUAL_HOST_DETAIL .= '<thead><tr><th colspan="2">Host</th></tr></thrad>';
		$VIRTUAL_HOST_DETAIL .= '<tbody>';
		$VIRTUAL_HOST_DETAIL .= '<tr><td width="155px">'._('Virtual Machine Name​').'</td><td>'.$SELECT_VM -> name.'</td></tr>';
		$VIRTUAL_HOST_DETAIL .= '<tr><td width="155px">'._('Number Of CPU​').'</td><td>'.$SELECT_VM -> number_of_cpu.'</td></tr>';
		$VIRTUAL_HOST_DETAIL .= '<tr><td width="155px">'._('Memory').'</td><td>'.$SELECT_VM -> memory_mb.' MB</td></tr>';
		$VIRTUAL_HOST_DETAIL .= '<tr><td width="155px">'._('Guest OS​').'</td><td>'.$SELECT_VM -> guest_os_name.'</td></tr>';
		
		$VM_DISK = $SELECT_VM -> disks;
		for ($i=0; $i<count($VM_DISK); $i++)
		{
			$DISK_SIZE = ($VM_DISK[$i] -> size_kb /1024/1024).'GB';
			
			$VIRTUAL_HOST_DETAIL .= '<tr><td width="155px">'._('Disk').' '.$i.'</td><td>'.$VM_DISK[$i] -> name.' ('.$DISK_SIZE.')</td></tr>';
		}
		
		$VM_ADAPTER = $SELECT_VM -> network_adapters;
		for ($n=0; $n<count($VM_ADAPTER); $n++)
		{
			//$INFO = count($VM_ADAPTER[$n] -> ip_addresses);
					
			if (isset($VM_ADAPTER[$n] -> ip_addresses))
			{
				$NETWORK_ADDRESS = implode(",", $VM_ADAPTER[$n] -> ip_addresses);
			}
			else
			{
				$NETWORK_ADDRESS = _('No Information');
			}
			
			$VIRTUAL_HOST_DETAIL .= '<tr><td width="155px">'._('Adapter').' '.$n.'</td><td>'.$VM_ADAPTER[$n] -> name.' ('.$NETWORK_ADDRESS.')</td></tr>';
		}
		
		if ($SELECT_VM -> guest_ip == '')
		{
			$GUEST_IP = _('No Information');
		}
		else
		{
			$GUEST_IP = $SELECT_VM -> guest_ip;
		}
		$VIRTUAL_HOST_DETAIL .= '<tr><td width="155px">'._('Guest IP​').'</td><td>'.$GUEST_IP.'</td></tr>';		
		
		$VIRTUAL_HOST_DETAIL .= '</tbody></table>';
		print_r($VIRTUAL_HOST_DETAIL);
	break;

	############################
	# SELECT EDIT SERVICE BY UUID
	############################
	case "SelectEditServiceByUUID":
	if (isset($_REQUEST['SERV_UUID']))
	{	
		$ACTION    = 'SelectEditServiceByUUID';
		$REST_DATA = array(
						'Action'   => $ACTION,
						'ServUUID' => $_REQUEST['SERV_UUID'],
					);
		$output = CurlPostToRemote($REST_DATA);
		
		if ($output == false)
		{
			$Response = array('Code' => false, 'ServInfo' => _('Cannot get Transport server information.'));
		}
		else
		{
			$SERV_INFO = json_decode($output);
			$_SESSION['CLUSTER_UUID'] = $SERV_INFO -> OPEN_UUID;
			$_SESSION['SERV_UUID'] 	  = $SERV_INFO -> SERV_UUID;
			$_SESSION['HOST_UUID'] 	  = $SERV_INFO -> OPEN_HOST;
				
			$Response = array('Code' => true, 'ServInfo' => $SERV_INFO);
		}
	}
	else
	{
		$Response = array('Code' => false, 'ServInfo' => _('Please select a Transport server.'));
	}
	print_r(json_encode($Response));
	break;
	
	
	############################
	# QUERY TRANSPORT SERVER INFORMATION
	############################
	case "QueryTransportServerInformation":
		
		$ACTION    = 'QueryTransportServerInformation';
		$REST_DATA = array(
						'Action'   => $ACTION,
						'ServUUID' => $_REQUEST['SERV_UUID'],
						'OnTheFly' => (isset($_REQUEST['ON_THE_FLY']))?$_REQUEST['ON_THE_FLY']:false					
					);
		$output = CurlPostToRemote($REST_DATA);
		print_r($output);
	break;
	
	
	############################
	# Check Running Connection
	############################
	case "CheckRunningConnection":
		
		$ACTION    = 'CheckRunningConnection';
		$REST_DATA = array(
						'Action'   => $ACTION,
						'ServUUID' => $_REQUEST['SERV_UUID'],
					);
		$output = CurlPostToRemote($REST_DATA);
		$SERV_INFO = json_decode($output);
		
		if ($SERV_INFO == FALSE)
		{
			$Response = array('code' => false, 'msg' => _('Transport service verify successfully, however Transport server still links to running workload.'));
		}
		else
		{
			$Response = array('msg' => true, 'msg' => '');
		}		
		print_r(json_encode($Response));
	break;
	
	
	############################
	# UPDATE TRNASPORT SERVICES
	############################
	case "UpdateTrnasportServices":
		$ACTION    = 'UpdateTrnasportServices';
		$REST_DATA = array(
						'Action'    => $ACTION,
						'ServUUID'  => $_REQUEST['SERV_UUID'],
						'ServADDR'  => $_REQUEST['SERV_ADDR'],
						'MgmtADDR'  => $_REQUEST['MGMT_ADDR']
					);
		$output = CurlPostToRemote($REST_DATA);
		$Response = array('Code' => $output, 'Msg' => _('Transport server information updated.'));
		print_r(json_encode($Response));	
	break;
	
	
	############################
	# DELETE SERVICE BY UUID
	############################
	case "DeleteServiceByUUID":
	
	if (isset($_REQUEST['SERV_UUID']))
	{	
		$ACTION    = 'DeleteServiceByUUID';
		$REST_DATA = array(
						'Action'   => $ACTION,
						'ServUUID' => $_REQUEST['SERV_UUID'],
					);
		$output = CurlPostToRemote($REST_DATA);
		
		if ($output == TRUE)
		{		
			$Response = array('Code' => true, 'Msg' => _('Transport server deleted.'));
		}
		else
		{
			$Response = array('Code' => false, 'Msg' => _('Some associated items still registered with select Transport server.'));
		}
	}
	else
	{
		$Response = array('Code' => false, 'Msg' => _('Please select a Transport server.'));
	}
	print_r(json_encode($Response));
	#print_r($output);
	break;
	
	
	############################
	# INITIALIZE NEW SERVICE
	############################
	case "InitializeNewService":
	
	$ACTION = 'InitializeNewService';
	if (isset($_REQUEST['PRIORITY_ADDR']))
	{
		$PRIORITY_ADDR = $_REQUEST['PRIORITY_ADDR'];
	}
	else
	{
		$PRIORITY_ADDR = null;
	}
	
	if (isset($_REQUEST['SELECT_VM_HOST']))
	{
		$SELECT_VM_HOST = $_REQUEST['SELECT_VM_HOST'];
	}
	else
	{
		$SELECT_VM_HOST = null;
	}
	
	$REST_DATA = array(
					'Action'   		=> $ACTION,
					'AcctUUID' 		=> $_REQUEST['ACCT_UUID'],
					'RegnUUID' 		=> $_REQUEST['REGN_UUID'],
					'OpenUUID' 		=> $_REQUEST['OPEN_UUID'],
					'ServUUID'		=> $_REQUEST['SERV_UUID'],
					'HostADDR' 		=> $_REQUEST['HOST_ADDR'],
					'HostUSER' 		=> $_REQUEST['HOST_USER'],
					'HostPASS' 		=> $_REQUEST['HOST_PASS'],
					'ServTYPE' 	 	=> $_REQUEST['SERV_TYPE'],
					'SystTYPE'	 	=> $_REQUEST['SYST_TYPE'],
					'PriorityAddr'	=> $PRIORITY_ADDR,
					'SelectVMHost'	=> $SELECT_VM_HOST
				);

	switch ($_REQUEST['SERV_TYPE'])
	{
		case "Physical Packer":
			$RESPONSE_MSG = 'New Packer for '.$_REQUEST['OS_TYPE'].' added.';
		break;
		
		case "Virtual Packer":
			$RESPONSE_MSG = 'New Packer for VMware added.';
			if ($SELECT_VM_HOST == '')
			{
				$Response = array('Code' => false, 'Msg' => _('Please select a VM host.'));
			}
		break;
		
		case "Offline Packer":
			$RESPONSE_MSG = 'New Packer for Offline added.';
		break;
		
		default:
			$RESPONSE_MSG = $_REQUEST['SERV_TYPE'];
		break;
	}
	
	if (!isset($Response))
	{
		$output = CurlPostToRemote($REST_DATA);
		
		if ($output == true)
		{
			$Response = array('Code' => true, 'Msg' => _($RESPONSE_MSG));
		}
		else
		{
			$Response = array('Code' => false, 'Msg' => _('Failed to add host due to duplicated machine ID or registration detail.'));
		}
	}
	
	print_r(json_encode($Response));
	break;
	
	
	############################
	# UPDATE HOST INFORMATION
	############################
	case "UpdateHostInformation":
	
	$ACTION = 'UpdateHostInformation';
	if (isset($_REQUEST['PRIORITY_ADDR']))
	{
		$PRIORITY_ADDR = $_REQUEST['PRIORITY_ADDR'];
	}
	else
	{
		$PRIORITY_ADDR = null;
	}
	
	if (isset($_REQUEST['SELECT_VM_HOST']))
	{
		$SELECT_VM_HOST = $_REQUEST['SELECT_VM_HOST'];
	}
	else
	{
		$SELECT_VM_HOST = null;
	}
	
	$REST_DATA = array(
					'Action'   		=> $ACTION,
					'AcctUUID' 		=> $_REQUEST['ACCT_UUID'],
					'RegnUUID' 		=> $_REQUEST['REGN_UUID'],
					'OpenUUID' 		=> $_REQUEST['OPEN_UUID'],
					'ServUUID' 		=> $_REQUEST['SERV_UUID'],
					'HostUUID' 		=> $_REQUEST['HOST_UUID'],
					'HostADDR' 		=> $_REQUEST['HOST_ADDR'],
					'HostUSER'	 	=> $_REQUEST['HOST_USER'],
					'HostPASS' 		=> $_REQUEST['HOST_PASS'],
					'ServTYPE' 		=> $_REQUEST['SERV_TYPE'],
					'PriorityAddr'	=> $PRIORITY_ADDR,
					'SelectVMHost'	=> $SELECT_VM_HOST
				);
	$output = CurlPostToRemote($REST_DATA);

	switch ($_REQUEST['SERV_TYPE'])
	{
		case "Physical Packer":
			$UPDATE_MSG = 'Packer for '.$_REQUEST['OS_TYPE'].' information updated.';
			$ERRROR_MSG = 'Cannot update '.$_REQUEST['OS_TYPE'].' information.';
		break;
		
		case "Virtual Packer":
			$UPDATE_MSG = 'Packer for VMware information updated.';
			$ERRROR_MSG = 'Cannot update Packer for VMware information.';
		break;
		
		case "Offline Packer":
			$UPDATE_MSG = 'Packer for Offline information updated.';
			$ERRROR_MSG = 'Cannot update Packer for Offline information.';
		break;
		
		default:
			$UPDATE_MSG = $_REQUEST['SERV_TYPE'];
			$ERRROR_MSG = $_REQUEST['SERV_TYPE'];
		break;
	}
		
	if ($output == true)
	{
		$Response = array('Code' => true, 'Msg' => _($UPDATE_MSG));
	}
	else
	{
		$Response = array('Code' => false, 'Msg' => _($ERRROR_MSG));
	}
	print_r(json_encode($Response));
	break;
	

	############################
	# PASSVIA SERVICE UUID
	############################
	case "PassviaServiceUUID":
		
	$ACTION    = 'PassviaServiceUUID';	
	$JSON_DATA = array('Action'   => $ACTION,
					   'ServUUID' => $_REQUEST['SERV_UUID'],
					   'ServTYPE' => $_REQUEST['SERV_TYPE']
				);
	
	$_SESSION['data'] = json_decode(json_encode($JSON_DATA), FALSE);;
	
	print_r(json_encode(array('Code' => true)));
	break;
	
	/*
	############################
	# UPDATE SERVICE INFORMATION
	############################
	case "UpdateServiceInfo":
	
	$ACTION    = 'UpdateServiceInfo';	
	$REST_DATA = array('Action'   => $ACTION,
					   'ServUUID' => $_REQUEST['SERV_UUID'],
					   'ServADDR' => $_REQUEST['SERV_ADDR'],
					   'ServTYPE' => $_REQUEST['SERV_TYPE']
				);
	$output = CurlPostToRemote($REST_DATA);
	
	print_r(json_encode(array('Code' => true)));
	break;
	*/
	
	############################
	# LIST AVAILABLE REPLICA
	############################
	case "ListAvailableReplica":
	
	$ACTION = 'ListAvailableReplica';
	$REST_DATA = array('Action'   => $ACTION,
					   'AcctUUID' => $_REQUEST['ACCT_UUID']
				);
	$ListReplica = CurlPostToRemote($REST_DATA);
	
	$ListReplicaArray = json_decode($ListReplica,true);
	
	if ($ListReplicaArray != FALSE)
	{		
		$COUNT_REPLICA = count($ListReplicaArray);
	
		for ($i=0; $i<$COUNT_REPLICA; $i++)
		{
			array_unshift( $ListReplicaArray[$i]['REPL_ARGUMENTS'], $ListReplicaArray[$i]['REPL_FORMAT']);
			
			$msg = call_user_func_array('translate', $ListReplicaArray[$i]['REPL_ARGUMENTS']);
			
			$LIST_REPL[$i] = array(
								'REPL_UUID' 				=> $ListReplicaArray[$i]['REPL_UUID'],
								'HOST_NAME' 				=> $ListReplicaArray[$i]['HOST_NAME'],
								'REPL_MESG' 				=> $msg,
								'REPL_FORMAT'				=> $ListReplicaArray[$i]['REPL_FORMAT'],
								'REPL_ARGUMENTS'			=> $ListReplicaArray[$i]['REPL_ARGUMENTS'],
								'PROGRESS_PRECENT' 			=> $ListReplicaArray[$i]['PROGRESS_PRECENT'],
								'LOADER_PROGRESS_PRECENT' 	=> $ListReplicaArray[$i]['LOADER_PROGRESS_PRECENT'],
								'REPL_TIME' 				=> Misc::time_convert_with_zone($ListReplicaArray[$i]['SYNC_TIME']),
								'EXPORT_JOB'				=> $ListReplicaArray[$i]['EXPORT_JOB'],
								'WINPE_JOB'					=> $ListReplicaArray[$i]['WINPE_JOB'],
								'MULTI_LAYER'				=> $ListReplicaArray[$i]['MULTI_LAYER']
							);
		}
		print_r(json_encode($LIST_REPL));
	}
	else
	{
		print_r($ListReplica);
	}
	break;
	
	############################
	# LIST AVAILABLE REPLICA WITH PLAN
	############################
	case "ListAvailableReplicaWithPlan":
	
	$ACTION = 'ListAvailableReplicaWithPlan';
	$REST_DATA = array('Action'   => $ACTION,
					   'AcctUUID' => $_REQUEST['ACCT_UUID']
				);
	$ListReplica = CurlPostToRemote($REST_DATA);
	
	$ListReplicaArray = json_decode($ListReplica,true);
	
	if ($ListReplicaArray != FALSE)
	{		
		$COUNT_REPLICA = count($ListReplicaArray);
	
		for ($i=0; $i<$COUNT_REPLICA; $i++)
		{
			if( isset($ListReplicaArray[$i]["MSG_FORMAT"]) ) {
				array_unshift( $ListReplicaArray[$i]["MSG_ARGUMENTS"], $ListReplicaArray[$i]["MSG_FORMAT"]);
				$description = call_user_func_array('translate', $ListReplicaArray[$i]["MSG_ARGUMENTS"]);
			}
			else
				$description = $ListReplicaArray[$i]['REPL_MESG'];
		
			$LIST_REPL[$i] = array(
								'HOST_NAME'     		=> $ListReplicaArray[$i]['HOST_NAME'],
								'REPL_UUID'     		=> $ListReplicaArray[$i]['REPL_UUID'],								
								'PLAN_UUID'	    		=> $ListReplicaArray[$i]['PLAN_UUID'],
								'MSG_FORMAT'    		=> $ListReplicaArray[$i]['MSG_FORMAT'],
								'MSG_ARGUMENTS' 		=> $ListReplicaArray[$i]['MSG_ARGUMENTS'],
								//'REPL_MESG'     		=> $ListReplicaArray[$i]['REPL_MESG'],
								'REPL_MESG'     		=> $description,
								'REPL_TIME'     		=> Misc::time_convert_with_zone($ListReplicaArray[$i]['SYNC_TIME']),
								'IS_EXPORT_JOB' 		=> $ListReplicaArray[$i]['IS_EXPORT_JOB'],
								'IS_RCD_JOB'    		=> $ListReplicaArray[$i]['IS_RCD_JOB'],								
								'IS_TEMPLATE'   		=> $ListReplicaArray[$i]['IS_TEMPLATE'],
								'HAS_RECOVERY_RUNNING'	=> $ListReplicaArray[$i]['HAS_RECOVERY_RUNNING'],
								'CLOUD_TYPE'			=> $ListReplicaArray[$i]['CLOUD_TYPE'],
								'MULTI_LAYER'			=> $ListReplicaArray[$i]['MULTI_LAYER']
							);
		}
		print_r(json_encode($LIST_REPL));
	}
	else
	{
		print_r($ListReplica);
	}
	break;	
	
	############################
	# LIST JOB HISTORY
	############################
	case "DisplayJobHistory":
	
	$ACTION = 'DisplayJobHistory';
	$REST_DATA = array('Action'   => $ACTION,
					   'JobsUUID' => $_REQUEST['JOBS_UUID'],
					   'ItemLimit' => $_REQUEST['ITEM_LIMIT']
				);
	$History = CurlPostToRemote($REST_DATA);
			
	$HistoryJSON = json_decode($History,true);
	$HISTORY_COUNT = count($HistoryJSON);
	
	$HISTORY_TABLE = '';
	$HISTORY_TABLE .= '<script>$(document).ready(function(){';
	$HISTORY_TABLE .= '$(".HistoryTable").DataTable({';
	$HISTORY_TABLE .= 'paging		: true,';
	$HISTORY_TABLE .= 'Info			: false,';
	$HISTORY_TABLE .= 'bFilter		: true,';
	$HISTORY_TABLE .= 'lengthChange	: false,';
	$HISTORY_TABLE .= 'pageLength	: 16,';
	$HISTORY_TABLE .= 'pagingType	: "numbers",';
	$HISTORY_TABLE .= 'ordering		: true,';
	$HISTORY_TABLE .= 'destroy		: true,';
	$HISTORY_TABLE .= 'order		: [],';
	$HISTORY_TABLE .= 'columnDefs	: [{"targets":"NoOrder", "orderable": false}],';
	$HISTORY_TABLE .= 'language		: {search: "_INPUT_"},';
	$HISTORY_TABLE .= 'dom			: "lBftip",';
		
	$HISTORY_TABLE .= 'buttons: [';
	$HISTORY_TABLE .= '{extend:"excelHtml5",text:"<i class=\'fa fa-file-excel-o\'></i>",titleAttr:"Excel"},';
	$HISTORY_TABLE .= '{extend:"csvHtml5",text:"<i class=\'fa fa-file-text-o\'></i>",titleAttr:"CSV",bom:true},';
	#$HISTORY_TABLE .= '{extend:"pdfHtml5",text:"<i class=\'fa fa-file-pdf-o\'></i>",titleAttr:"PDF",pageSize:"A4",customize:function(doc){doc.pageMargins=[50,20,20,20];doc.styles.tableHeader.bold=false;doc.styles.tableHeader.fillColor="#00739A";doc.styles.tableBodyEven.fillColor="#F8F8F8";doc.styles.tableBodyOdd.fillColor="#dcdcdc";doc.styles.tableHeader.fontSize=12;doc.defaultStyle.fontSize=12;}}';
	$HISTORY_TABLE .= '],';
			
	$HISTORY_TABLE .= '});';
	
	$HISTORY_TABLE .= '$("div.dataTables_filter input").attr("placeholder",$.parseHTML("&#xF002; '._('Search').'")[0].data);';
	$HISTORY_TABLE .= '$("div.dataTables_filter input").addClass("form-control input-sm");';
	$HISTORY_TABLE .= '$("div.dataTables_filter input").css("max-width", "180px");';
	$HISTORY_TABLE .= '$("div.dataTables_filter input").css("padding-right", "40px");';
		
	$HISTORY_TABLE .= '});</script>';

	$HISTORY_TABLE .= '<table class="HistoryTable">';
	$HISTORY_TABLE .= '<thead><tr>';
	$HISTORY_TABLE .= '<th width="40px"  class="TextCenter NoOrder">#</th>';
	$HISTORY_TABLE .= '<th width="660px" class="TextCenter NoOrder">'._('Messages').'</th>';
	$HISTORY_TABLE .= '<th width="150px" class="TextCenter">'._('Time').'</th>';	
	$HISTORY_TABLE .= '</tr></thead><tbody>';	

	for ($i=0; $i<count($HistoryJSON); $i++)
	{

		if(isset($HistoryJSON[$i]["format"]))
		{
			array_unshift( $HistoryJSON[$i]["arguments"], $HistoryJSON[$i]["format"]);
			$description = call_user_func_array('translate', $HistoryJSON[$i]["arguments"]);
		}
		else
		{
			$description = $HistoryJSON[$i]['description'];
		}
	
		$time 		 = $HistoryJSON[$i]['time'];
		#$status 	 = $DecodeJSON[$i]['status'];

		if (mb_strwidth($description) > 93)
		{		
			$HISTORY_TABLE .= '<tr>';
			$HISTORY_TABLE .= '<td width="40px" class="TextCenter NoOrder">'.$HISTORY_COUNT.'</td>';
			$HISTORY_TABLE .= '<td width="660px"><div class="ReplicaHistoryOverFlow">'.$description.'<span class="tooltiptext">'.$description.'</span></div></td>';
			$HISTORY_TABLE .= '<td width="150px" class="TextCenter">'.Misc::time_convert_with_zone($time).'</td>';
			$HISTORY_TABLE .= '</tr>';
		}
		else
		{
			$HISTORY_TABLE .= '<tr>';
			$HISTORY_TABLE .= '<td width="40px" class="TextCenter NoOrder">'.$HISTORY_COUNT.'</td>';
			$HISTORY_TABLE .= '<td width="660px"><div class="ReplicaHistoryOverFlow">'.$description.'</div></td>';
			$HISTORY_TABLE .= '<td width="150px" class="TextCenter">'.Misc::time_convert_with_zone($time).'</td>';
			$HISTORY_TABLE .= '</tr>';
		}
		$HISTORY_COUNT--;
	}
	$HISTORY_TABLE .= '</tbody></table>';
	
	print_r($HISTORY_TABLE);	
	break;
	
	############################
	# DISPLAY REPLICA JOB INFORMATION
	############################
	case "DisplayPrepareJobInfo":
		$ACTION = 'DisplayPrepareJobInfo';
		$REST_DATA = array('Action'   => $ACTION,
						   'JobsUUID' => $_REQUEST['JOBS_UUID']
					);
		$PrepareJobInfo = CurlPostToRemote($REST_DATA);

		$PrepareInfo = json_decode($PrepareJobInfo,false);

		/* Transport Information */
		$PREPARE_HOST_TABLE  = '';
		$PREPARE_HOST_TABLE .= '<table class="ParpareHostTable">';
		$PREPARE_HOST_TABLE .= '<thead><tr>';
		#$PREPARE_HOST_TABLE .= '<th width="800px" colspan="3"><span style="position: absolute; padding-top:5px;">'._('Transport Server Information').'</span><button id="NetworkTopology" data-repl="'.$PrepareInfo -> JobUUID.'" class="btn btn-success pull-right btn-sm"><i id="UpdateSensorSpin" class="fa fa-map fa-lg fa-fw"></i>'._('Network Topology').'</button></th>';
		$PREPARE_HOST_TABLE .= '<th width="800px" colspan="3">'._('Transport Server Information').'</th>';
		$PREPARE_HOST_TABLE .= '</tr></thead><tbody>';
		
		if (isset($PrepareInfo -> PairTransport -> source_hostname))
		{
			$source_hostname = $PrepareInfo -> PairTransport -> source_hostname.' ['._('Source').']';
			$target_hostname = $PrepareInfo -> PairTransport -> target_hostname.' ['._('Target').']';
			$source_address  = implode(',',$PrepareInfo -> PairTransport -> source_address);
			$target_address  = implode(',',$PrepareInfo -> PairTransport -> target_address);
			$source_type     = $PrepareInfo -> PairTransport -> source_type;
			$target_type     = $PrepareInfo -> PairTransport -> target_type;			
		}
		else
		{			
			$source_hostname = _('processing..');
			$target_hostname = _('processing..');
			$source_address  = _('processing..');
			$target_address  = _('processing..');
			$source_type     = _('processing..');
			$target_type     = _('processing..');
		}		
		
		$PREPARE_HOST_TABLE .= '<tr>';
		$PREPARE_HOST_TABLE .= '<td width="155px">'._('Host Name').'</td>';
		$PREPARE_HOST_TABLE .= '<td width="322px">'.$source_hostname.'</td>';
		$PREPARE_HOST_TABLE .= '<td width="323px">'.$target_hostname.'</td>';
		$PREPARE_HOST_TABLE .= '</tr>';
		
		$PREPARE_HOST_TABLE .= '<tr>';
		$PREPARE_HOST_TABLE .= '<td width="155px">'._('Address').'</td>';
		$PREPARE_HOST_TABLE .= '<td width="322px">'.$source_address.'</td>';
		$PREPARE_HOST_TABLE .= '<td width="323px">'.$target_address.'</td>';
		$PREPARE_HOST_TABLE .= '</tr>';
		
		$PREPARE_HOST_TABLE .= '<tr>';
		$PREPARE_HOST_TABLE .= '<td width="155px">'._('Type').'</td>';
		$PREPARE_HOST_TABLE .= '<td width="322px">'.$source_type.'</td>';
		$PREPARE_HOST_TABLE .= '<td width="323px">'.$target_type.'</td>';
		$PREPARE_HOST_TABLE .= '</tr>';	
		$PREPARE_HOST_TABLE .= '</tbody></table>';
		/* Transport Information */

		
		/* Host Information */		
		$PREPARE_HOST_TABLE .= '<div style=" margin-top: 5px; display: block;"></div>';
		$PREPARE_HOST_TABLE .= '<table class="ParpareHostTable">';
		$PREPARE_HOST_TABLE .= '<thead><tr>';
		$PREPARE_HOST_TABLE .= '<th width="800px" colspan="2">'._('Host Information').'</th>';
		$PREPARE_HOST_TABLE .= '</tr></thead><tbody>';
		
		$PREPARE_HOST_TABLE .= '<tr>';
		$PREPARE_HOST_TABLE .= '<td width="155px">'._('Host Name').'</td>';
		$PREPARE_HOST_TABLE .= '<td width="645px">'.$PrepareInfo -> Hostname.'</td>';
		$PREPARE_HOST_TABLE .= '</tr>';
		
		$PREPARE_HOST_TABLE .= '<tr>';
		$PREPARE_HOST_TABLE .= '<td width="155px">'._('Packer Type').'</td>';
		$PREPARE_HOST_TABLE .= '<td width="645px">'.$PrepareInfo -> HostType.'</td>';
		$PREPARE_HOST_TABLE .= '</tr>';	
		
		$PREPARE_HOST_TABLE .= '<tr>';
		$PREPARE_HOST_TABLE .= '<td width="155px">'._('OS Type').'</td>';
		$PREPARE_HOST_TABLE .= '<td width="645px">'.$PrepareInfo -> OSName.'</td>';
		$PREPARE_HOST_TABLE .= '</tr>';
		
		$PREPARE_HOST_TABLE .= '<tr>';
		$PREPARE_HOST_TABLE .= '<td width="155px">'._('Host Address').'</td>';
		$PREPARE_HOST_TABLE .= '<td width="645px">'.$PrepareInfo -> Address.'</td>';
		$PREPARE_HOST_TABLE .= '</tr>';		
		
		$DISK_INFO = $PrepareInfo -> Disk;

		for ($i=0; $i<count($DISK_INFO); $i++)
		{
			if ($DISK_INFO[$i] -> is_boot == TRUE)
			{
				$IS_BOOT = '<i class="fa fa-check-circle-o" aria-hidden="true"></i> ';
			}
			else
			{
				$IS_BOOT = '<i class="fa fa-circle-o" aria-hidden="true"></i> ';
			}
			
			if ($DISK_INFO[$i] -> is_skip == TRUE)
			{
				$STRIKE = 'strike_line';
			}
			else
			{
				$STRIKE = '';
			}
			
			$DISK_NAME = $DISK_INFO[$i] -> disk_name;
			
			$DISK_SIZE = ($DISK_INFO[$i] -> size) / 1024 / 1024 / 1024;
			
			$PREPARE_HOST_TABLE .= '<tr>';
			$PREPARE_HOST_TABLE .= '<td width="155px">'._('Disk').' '.$i.'</td>';
			$PREPARE_HOST_TABLE .= '<td width="645px" class="'.$STRIKE.'">'.$IS_BOOT.$DISK_NAME.' ('.round($DISK_SIZE).'GB)</td>';
			$PREPARE_HOST_TABLE .= '</tr>';
		}
		$PREPARE_HOST_TABLE .= '</tbody></table>';
		/* Host Information */
		
		
		/* Pair Volume */
		if ($target_type != 'ExportType' AND $target_type != 'Local Recover Kit')
		{
			$PREPARE_HOST_TABLE .= '<div style=" margin-top: 5px; display: block;"></div>';
			$PREPARE_HOST_TABLE .= '<table class="ParpareHostTable">';
			$PREPARE_HOST_TABLE .= '<thead><tr>';
			$PREPARE_HOST_TABLE .= '<th width="800px" colspan="2">'._('Replication Disk Information').'</th>';
			$PREPARE_HOST_TABLE .= '</tr><tbody>';
			if ($PrepareInfo -> CloudVolume != 'processing..')
			{
				$x=0;
				foreach ($PrepareInfo -> CloudVolume as $CLOUD_DISK_NAME => $CLOUD_DISK_SIZE)
				{
					$PREPARE_HOST_TABLE .= '<tr>';
					$PREPARE_HOST_TABLE .= '<td width="155px">'._('Disk').' '.$x.'</td>';
					$PREPARE_HOST_TABLE .= '<td width="645px">'.$CLOUD_DISK_NAME.' ('.$CLOUD_DISK_SIZE.'GB)</td>';
					$PREPARE_HOST_TABLE .= '</tr>';
					$x++;
				}
			}
			else
			{
				$PREPARE_HOST_TABLE .= '<tr>';
				$PREPARE_HOST_TABLE .= '<td width="155px">'._('Disk').'</td>';
				$PREPARE_HOST_TABLE .= '<td width="645px">'._($PrepareInfo -> CloudVolume).'</td>';
				$PREPARE_HOST_TABLE .= '</tr>';
			}
			$PREPARE_HOST_TABLE .= '</tbody></table>';
		}
		/* Pair Volume */
		
		
		/* Job Configuration */
		$PREPARE_HOST_TABLE .= '<div style=" margin-top: 5px; display: block;"></div>';
		$PREPARE_HOST_TABLE .= '<table class="ParpareHostTable">';
		$PREPARE_HOST_TABLE .= '<thead><tr>';
		$PREPARE_HOST_TABLE .= '<th width="800px" colspan="4">'._('Replication Configuration').'</th>';
		$PREPARE_HOST_TABLE .= '</tr><tbody>';
		
		if ($PrepareInfo -> JobConfig -> triggers[0] -> start == '')
		{
			$START_TIME = 'Run at beginning.';
		}
		else
		{
			$START_TIME = $PrepareInfo -> JobConfig -> triggers[0] -> start;
		}
		
		$PREPARE_HOST_TABLE .= '<tr>';
		$PREPARE_HOST_TABLE .= '<td width="155px">'._('Process ID').'</td>';
		$PREPARE_HOST_TABLE .= '<td width="645px"><span id="job_uuid">'.$PrepareInfo -> JobUUID.'</span><span style="float:right;" id="CopyUUID"><i class="fa fa-clipboard"></span></i></td>';
		$PREPARE_HOST_TABLE .= '</tr>';

		if ($PrepareInfo -> JobConfig -> is_continuous_data_replication == TRUE)
		{
			$PREPARE_HOST_TABLE .= '<tr>';
			$PREPARE_HOST_TABLE .= '<td width="155px">'._('Snapshot Size').'</td>';
			$PREPARE_HOST_TABLE .= '<td width="215px">'._('Data: ').round((($PrepareInfo -> TransferData -> backup_size) / 1024), 2).'KB</td>';
			$PREPARE_HOST_TABLE .= '<td width="215px">'._('Progress: ').round((($PrepareInfo -> TransferData -> progress_size) / 1024), 2).'KB</td>';
			$PREPARE_HOST_TABLE .= '<td width="215px">'._('Transfer: ').round((($PrepareInfo -> TransferData -> loader_data) / 1024), 2).'KB</td>';
			$PREPARE_HOST_TABLE .= '</tr>';
		}
		
		#$PREPARE_HOST_TABLE .= '<tr>';
		#$PREPARE_HOST_TABLE .= '<td width="155px">Total Transfer Size</td>';
		#$PREPARE_HOST_TABLE .= '<td width="645px" colspan="3">'.round((($PrepareInfo -> TransferData -> total_transport_data) / 1024 / 1024), 2).'MB</td>';
		#$PREPARE_HOST_TABLE .= '</tr>';
		
		$PREPARE_HOST_TABLE .= '<tr>';
		$PREPARE_HOST_TABLE .= '<td width="155px">'._('Start Time').'</td>';
		$PREPARE_HOST_TABLE .= '<td width="645px" colspan="3">'.$START_TIME.'</td>';
		$PREPARE_HOST_TABLE .= '</tr>';
		
		$PREPARE_HOST_TABLE .= '<tr>';
		$PREPARE_HOST_TABLE .= '<td width="155px">'._('No. of Snapshot').'</td>';
		$PREPARE_HOST_TABLE .= '<td width="645px" colspan="3">'.$PrepareInfo -> JobConfig -> snapshot_rotation.'</td>';
		$PREPARE_HOST_TABLE .= '</tr>';
		
		$PREPARE_HOST_TABLE .= '<tr>';
		$PREPARE_HOST_TABLE .= '<td width="155px">'._('Packer Thread').'</td>';
		$PREPARE_HOST_TABLE .= '<td width="645px" colspan="3">'.$PrepareInfo -> JobConfig -> worker_thread_number.'</td>';
		$PREPARE_HOST_TABLE .= '</tr>';
		
		$PREPARE_HOST_TABLE .= '<tr>';
		$PREPARE_HOST_TABLE .= '<td width="155px">'._('Replication Thread').'</td>';
		$PREPARE_HOST_TABLE .= '<td width="645px" colspan="3">'.$PrepareInfo -> JobConfig -> loader_thread_number.'</td>';
		$PREPARE_HOST_TABLE .= '</tr>';
		
		$PREPARE_HOST_TABLE .= '<tr>';
		$PREPARE_HOST_TABLE .= '<td width="155px">'._('Replication Trigger %').'</td>';
		$PREPARE_HOST_TABLE .= '<td width="645px" colspan="3">'.$PrepareInfo -> JobConfig -> loader_trigger_percentage.'</td>';
		$PREPARE_HOST_TABLE .= '</tr>';	
		
		if ($PrepareInfo -> JobConfig -> export_path != '')
		{
			$PREPARE_HOST_TABLE .= '<tr>';
			$PREPARE_HOST_TABLE .= '<td width="155px">'._('Export Path').'</td>';
			$PREPARE_HOST_TABLE .= '<td width="645px" colspan="3">'.$PrepareInfo -> JobConfig -> export_path.'</td>';
			$PREPARE_HOST_TABLE .= '</tr>';
			
			switch ($PrepareInfo -> JobConfig -> export_type)
			{
				case 0:
					$EXPORT_TYPE = 'VHD';
				break;
				
				case 1:
					$EXPORT_TYPE = 'VHDX';
				break;
			}			
			$PREPARE_HOST_TABLE .= '<tr>';
			$PREPARE_HOST_TABLE .= '<td width="155px">'._('Export Type').'</td>';
			$PREPARE_HOST_TABLE .= '<td width="645px">'.$EXPORT_TYPE.'</td>';
			$PREPARE_HOST_TABLE .= '</tr>';			
		}
		
		$PREPARE_HOST_TABLE .= '</tbody></table>';
		/* Job Configuration */
		
		#BEGIN JAVASCRIPT
		$PREPARE_HOST_TABLE .= '<script>';
		$PREPARE_HOST_TABLE .= 'function CopuJobUUID(){';
		$PREPARE_HOST_TABLE .= 'var range = document.createRange();';
		$PREPARE_HOST_TABLE .= 'range.selectNode(document.getElementById("job_uuid"));';
		$PREPARE_HOST_TABLE .= 'window.getSelection().removeAllRanges();';
		$PREPARE_HOST_TABLE .= 'window.getSelection().addRange(range);';		
		$PREPARE_HOST_TABLE .= 'document.execCommand("copy");';
		$PREPARE_HOST_TABLE .= 'window.getSelection().removeAllRanges();';
		$PREPARE_HOST_TABLE .= 'BootstrapDialog.show({title:"'._('Information').'", message:"'._('Job UUID copied!').'", type: BootstrapDialog.TYPE_PRIMARY, draggable: true, buttons:[{label:"'._('Close').'", action: function(dialogItself){dialogItself.close();}}]});';
		$PREPARE_HOST_TABLE .= '}';	
		$PREPARE_HOST_TABLE .= '$("#CopyUUID").click(function(){CopuJobUUID();})';		
		$PREPARE_HOST_TABLE .= '</script>';

		print_r($PREPARE_HOST_TABLE);
	break;
	
	############################
	# DISPLAY SERVICE JOB INFORMATION
	############################
	case "DisplayRecoveryJobInfo":
		$ACTION = 'DisplayRecoveryJobInfo';
		$REST_DATA = array('Action'   => $ACTION,
						   'JobsUUID' => $_REQUEST['JOBS_UUID']
					);
		$RecoveryJobInfo = CurlPostToRemote($REST_DATA);

		$RecoveryInfo = json_decode($RecoveryJobInfo,false);
		
		#BEGIN JAVASCRIPT
		$RECOVERY_HOST_TABLE  = '<script>';
		$RECOVERY_HOST_TABLE .= 'function CopuJobUUID(){';
		$RECOVERY_HOST_TABLE .= 'var range = document.createRange();';
		$RECOVERY_HOST_TABLE .= 'range.selectNode(document.getElementById("job_uuid"));';
		$RECOVERY_HOST_TABLE .= 'window.getSelection().removeAllRanges();';
		$RECOVERY_HOST_TABLE .= 'window.getSelection().addRange(range);';		
		$RECOVERY_HOST_TABLE .= 'document.execCommand("copy");';
		$RECOVERY_HOST_TABLE .= 'window.getSelection().removeAllRanges();';
		$RECOVERY_HOST_TABLE .= 'BootstrapDialog.show({title:"'._('Information').'", message:"'._('Job UUID copied!').'", type: BootstrapDialog.TYPE_PRIMARY, draggable: true, buttons:[{label:"'._('Close').'", action: function(dialogItself){dialogItself.close();}}]});';
		$RECOVERY_HOST_TABLE .= '}';	
		$RECOVERY_HOST_TABLE .= '$("#CopyUUID").click(function(){CopuJobUUID();})';		
		$RECOVERY_HOST_TABLE .= '</script>';
				
		$DISPLAY_RECOVERY_INFO = (($RecoveryInfo -> RecoveryType != 'Export') AND ($RecoveryInfo -> RecoveryType != 'Recover Kit') AND ($RecoveryInfo -> RecoveryType != 'Disk To Disk'))?true:false;	
		
		$RECOVERY_HOST_TABLE .= '';		
		$RECOVERY_HOST_TABLE .= '<table class="RecoverHostTable">';
		$RECOVERY_HOST_TABLE .= '<thead><tr>';
		$RECOVERY_HOST_TABLE .= '<th width="800px" colspan="2">'._('Host Information').'</th>';
		$RECOVERY_HOST_TABLE .= '</tr></thead><tbody>';
		
		$RECOVERY_HOST_TABLE .= '<tr>';
		$RECOVERY_HOST_TABLE .= '<td width="160px">'._('Host Name').'</td>';
		$RECOVERY_HOST_TABLE .= '<td width="640px">'.$RecoveryInfo -> Hostname.'</td>';
		$RECOVERY_HOST_TABLE .= '</tr>';
			
		$RECOVERY_HOST_TABLE .= '<tr>';
		$RECOVERY_HOST_TABLE .= '<td width="160px">'._('Recovery Type').'</td>';
		$RECOVERY_HOST_TABLE .= '<td width="640px">'.$RecoveryInfo -> RecoveryType.'</td>';
		$RECOVERY_HOST_TABLE .= '</tr>';
		
		$RECOVERY_HOST_TABLE .= '<tr>';
		$RECOVERY_HOST_TABLE .= '<td width="160px">'._('OS Name').'</td>';
		$RECOVERY_HOST_TABLE .= '<td width="640px">'.$RecoveryInfo -> OSName.'</td>';
		$RECOVERY_HOST_TABLE .= '</tr>';		
		
		$RECOVERY_HOST_TABLE .= '<tr>';
		$RECOVERY_HOST_TABLE .= '<td width="160px">'._('OS Type').'</td>';
		$RECOVERY_HOST_TABLE .= '<td width="640px">'.$RecoveryInfo -> OSType.'</td>';
		$RECOVERY_HOST_TABLE .= '</tr>';
			
		$DISK_INFO = $RecoveryInfo -> Disk;
		
		if ($DISK_INFO != 'processing..')
		{
			$DISK_SIZE = '';
			for ($i=0; $i<count($DISK_INFO); $i++)
			{
				$DISK_SIZE .= 'Disk'.$i.' ('.round($DISK_INFO[$i] / 1024 /1024).'GB) / ';
			}
		}
		else
		{
			$DISK_SIZE = $DISK_INFO;
		}
		$RECOVERY_HOST_TABLE .= '<tr>';
		$RECOVERY_HOST_TABLE .= '<td width="160px">'._('Disk Info').'</td>';
		$RECOVERY_HOST_TABLE .= '<td width="640px">'.rtrim($DISK_SIZE,' / ').'</td>';
		$RECOVERY_HOST_TABLE .= '</tr>';	

		if ($DISPLAY_RECOVERY_INFO == FALSE)
		{
			$RECOVERY_HOST_TABLE .= '<tr>';
			$RECOVERY_HOST_TABLE .= '<td width="160px">'._('Job ID').'</td>';
			$RECOVERY_HOST_TABLE .= '<td width="640px"><span id="job_uuid">'.$RecoveryInfo -> JobUUID.'</span><span style="float:right;" id="CopyUUID"><i class="fa fa-clipboard"></span></i></td>';
			$RECOVERY_HOST_TABLE .= '</tr>';	
		}
		
		$RECOVERY_HOST_TABLE .= '</tbody></table>';
		
		if ($DISPLAY_RECOVERY_INFO == TRUE)
		{
			$RECOVERY_HOST_TABLE .= '<br>';
			$RECOVERY_HOST_TABLE .= '<table class="RecoverHostTable">';
			$RECOVERY_HOST_TABLE .= '<thead><tr>';
			$RECOVERY_HOST_TABLE .= '<th width="800px" colspan="2">'._('Recovery Information').'</th>';
			$RECOVERY_HOST_TABLE .= '</tr><tbody>';
				
			$RECOVERY_HOST_TABLE .= '<tr>';
			$RECOVERY_HOST_TABLE .= '<td width="160px">'._('Job ID').'</td>';
			$RECOVERY_HOST_TABLE .= '<td width="640px"><span id="job_uuid">'.$RecoveryInfo -> JobUUID.'</span><span style="float:right;" id="CopyUUID"><i class="fa fa-clipboard"></span></i></td>';
			$RECOVERY_HOST_TABLE .= '</tr>';	
			
			if ($RecoveryInfo -> CloudType == 'Azure')
			{
				$HostNameTitle = _('VM Host Name');
				$HostTypeTitle = _('VM Information');
			}
			elseif ($RecoveryInfo -> CloudType == 'AWS')
			{
				$HostNameTitle = _('EC2 Host Name');
				$HostTypeTitle = _('EC2 Information');
			}
			elseif ($RecoveryInfo -> CloudType == 'Aliyun')
			{
				$HostNameTitle = _('ECS Host Name');
				$HostTypeTitle = _('ECS Information');
			}
			elseif ($RecoveryInfo -> CloudType == 'VMware')
			{
				$HostNameTitle = _('VM Name');
				$HostTypeTitle = _('Flavor');
			}
			else
			{
				$HostNameTitle = _('Instance Hostname');
				$HostTypeTitle = _('Flavor');
			}		

			#IS DATAMODE INSTANCE
			if ($RecoveryInfo -> RecoveryJobSpec -> datamode_instance != "NoAssociatedDataModeInstance")
			{
				$Bootable = ($RecoveryInfo -> RecoveryJobSpec -> is_datamode_boot == TRUE) ? '<i class="fa fa fa-power-off" aria-hidden="true"></i> ' : '';
				$DataModeInstance = ' ('.$Bootable._('Data Mode').')';
			}
			else
			{
				$DataModeInstance = '';
			}

			$RECOVERY_HOST_TABLE .= '<tr>';
			$RECOVERY_HOST_TABLE .= '<td width="160px">'.$HostNameTitle.'</td>';
			$RECOVERY_HOST_TABLE .= '<td width="640px">'.$RecoveryInfo -> InstanceHostname.$DataModeInstance.'</td>';
			$RECOVERY_HOST_TABLE .= '</tr>';
				
			$RECOVERY_HOST_TABLE .= '<tr>';
			$RECOVERY_HOST_TABLE .= '<td width="160px">'.$HostTypeTitle.'</td>';
			if ($RecoveryInfo -> Flavor -> name == 'processing..')
			{
				$RECOVERY_HOST_TABLE .= '<td width="640px">'.$RecoveryInfo -> Flavor -> name.'</td>';
			}
			else
			{
				$RECOVERY_HOST_TABLE .= '<td width="640px">'.$RecoveryInfo -> Flavor -> name.' / '.$RecoveryInfo -> Flavor -> vcpus.'Cores / '.$RecoveryInfo -> Flavor -> ram.'MB</td>';	
			}
			$RECOVERY_HOST_TABLE .= '</tr>';
						
			$RECOVERY_HOST_TABLE .= '<tr>';
			$RECOVERY_HOST_TABLE .= '<td width="160px">'._('Security Groups').'</td>';
			$RECOVERY_HOST_TABLE .= '<td width="640px">'.$RecoveryInfo -> SecurityGroups.'</td>';
			$RECOVERY_HOST_TABLE .= '</tr>';
			
			$RECOVERY_HOST_TABLE .= '<tr>';
			$RECOVERY_HOST_TABLE .= '<td width="160px">'._('Hypervisor / Region').'</td>';
			$RECOVERY_HOST_TABLE .= '<td width="640px">'.$RecoveryInfo -> HypervisorHostname.'</td>';
			$RECOVERY_HOST_TABLE .= '</tr>';
			
			if( isset( $RecoveryInfo -> NicInfo ))
				$NIC_INFO = $RecoveryInfo -> NicInfo;
			else
				$NIC_INFO = array();
				
			for ($x=0; $x<count($NIC_INFO); $x++)
			{
				$NIC_ADDR = $NIC_INFO[$x] -> addr;
				$NIC_TYPE = $NIC_INFO[$x] -> type;
				$NIC_MAC  = $NIC_INFO[$x] -> mac;		
				
				$RECOVERY_HOST_TABLE .= '<tr>';
				$RECOVERY_HOST_TABLE .= '<td width="160px">'._('Adapter').' '.$x.'</td>';
				if ($NIC_ADDR == 'processing..')
				{
					$RECOVERY_HOST_TABLE .= '<td width="640px">'.$NIC_ADDR.'</td>';
				}
				else
				{
					$RECOVERY_HOST_TABLE .= '<td width="640px">'.$NIC_ADDR.' / '.$NIC_MAC.' / '.$NIC_TYPE.'</td>';
				}
				$RECOVERY_HOST_TABLE .= '</tr>';
			}
			
			if ($RecoveryInfo -> RecoveryJobSpec -> rcvy_pre_script != '')
			{			
				$RECOVERY_HOST_TABLE .= '<tr>';
				$RECOVERY_HOST_TABLE .= '<td width="160px">'._('Pre Script').'</td>';
				$RECOVERY_HOST_TABLE .= '<td width="640px">'.$RecoveryInfo -> RecoveryJobSpec -> rcvy_pre_script.'</td>';
				$RECOVERY_HOST_TABLE .= '</tr>';
			}
			
			if ($RecoveryInfo -> RecoveryJobSpec -> rcvy_post_script != '')
			{			
				$RECOVERY_HOST_TABLE .= '<tr>';
				$RECOVERY_HOST_TABLE .= '<td width="160px">'._('Post Script').'</td>';
				$RECOVERY_HOST_TABLE .= '<td width="640px">'.$RecoveryInfo -> RecoveryJobSpec -> rcvy_post_script.'</td>';
				$RECOVERY_HOST_TABLE .= '</tr>';
			}
			
			$RECOVERY_HOST_TABLE .= '<tr>';
			$RECOVERY_HOST_TABLE .= '<td width="160px">'._('Recover Cloud Type').'</td>';
			$RECOVERY_HOST_TABLE .= '<td width="640px">'.$RecoveryInfo -> CloudType.'</td>';
			$RECOVERY_HOST_TABLE .= '</tr>';
			
			if ($RecoveryInfo -> CloudType == 'Aliyun')
			{
				$RECOVERY_HOST_TABLE .= '<tr>';
				$RECOVERY_HOST_TABLE .= '<td width="160px">'._('Admin Password').'</td>';
				$RECOVERY_HOST_TABLE .= '<td width="640px">'.$RecoveryInfo -> AdminPassword.'</td>';
				$RECOVERY_HOST_TABLE .= '</tr>';
			}
		}
		
		$RECOVERY_HOST_TABLE .= '</tbody></table>';
		
		print_r($RECOVERY_HOST_TABLE);		
	break;
	
	############################
	# REPLICA SELECT HOST UUID
	############################
	case "ReplicaSelectHostUUID":
	
	if (isset($_REQUEST['HOST_INFO']))
	{
		$ACTION = 'ReplicaSelectHostUUID';
	
		$HOST_INFO = explode('|',$_REQUEST['HOST_INFO']);
		$_SESSION['HOST_UUID'] 		= $HOST_INFO[0];
		$_SESSION['HOST_OS']   		= $HOST_INFO[1];
		$_SESSION['HOST_TYPE'] 		= $HOST_INFO[2];
		$_SESSION['PRIORITY_ADDR']	= $HOST_INFO[3];
		print_r(json_encode(array('Code' => true, 'Msg' => '')));
	}
	else
	{
		print_r(json_encode(array('Code' => false, 'Msg' => _('Please select a host.'))));
	}		
	break;
	
	############################
	# REPLICA SELECT HOST UUID
	############################
	case "SelectTargetServer":
	
	$ACTION    = 'SelectTargetServer';
	if ($_REQUEST['LOAD_UUID'] != '' AND $_REQUEST['LAUN_UUID'] != '')
	{
		$_SESSION['CLOUD_TYPE']= $_REQUEST['CLOUD_TYPE'];
		$_SESSION['LOAD_UUID'] = $_REQUEST['LOAD_UUID'];
		$_SESSION['LAUN_UUID'] = $_REQUEST['LAUN_UUID'];
		$_SESSION['SERV_TYPE'] = str_replace(' ', '', $_REQUEST['SERV_TYPE']);
		print_r(json_encode(array('Code' => true)));
	}
	else
	{
		print_r(json_encode(array('Code' => false)));
	}
	break;
		
	############################
	# REPLICA SELECT SERVICE UUID (TO BE DELETE)
	############################
	case "ReplicaSelectServiceUUID":
	
	$ACTION    = 'ReplicaSelectServiceUUID';
	if ($_REQUEST['CONN_UUID'] != '')
	{
		$_SESSION['CONN_UUID'] = $_REQUEST['CONN_UUID'];
		print_r(json_encode(array('Code' => true)));
	}
	else
	{
		print_r(json_encode(array('Code' => false)));
	}
	break;
	
	############################
	# WORKLOAD CONFIGURATION
	############################
	case "WorkloadConfiguration":
	
	$ACTION = 'WorkloadConfiguration';
	$_SESSION['REPLICA_SETTINGS'] = array(
									'SkipDisk' => $_REQUEST['SKIP_DISK'],
									'CloudMappingDisk' => $_REQUEST['CLOUD_MAP_DISK'],
									'PreSnapshotScript' => addslashes($_REQUEST['PRE_SNAPSHOT_SCRIPT']),
									'PostSnapshotScript' => addslashes($_REQUEST['POST_SNAPSHOT_SCRIPT']));
	print_r(json_encode(true));
	
	break;
	
	############################
	# REPLICA CONFIGURATION
	############################
	case "ReplicaConfiguration":
	
	$ACTION = 'ReplicaConfiguration';

	$INTERVAL_SETTING = $_REQUEST['REPLICA_SETTINGS']['IntervalMinutes'];
	$SNAPSHOT_NUMBER = $_REQUEST['REPLICA_SETTINGS']['SnapshotsNumber'];
	$EXPORT_PATH = $_REQUEST['REPLICA_SETTINGS']['ExportPath'];
	$DISK_MAP_COUNT = array_unique(explode(",",$_SESSION['REPLICA_SETTINGS']['CloudMappingDisk']));
	$EXPORT_JOB_READY = ((count($DISK_MAP_COUNT) == 1) AND in_array("CreateOnDemand",$DISK_MAP_COUNT))?true:false;
		
	if (is_numeric($INTERVAL_SETTING) == false)
	{
		$Response = array('Code' => false, 'msg' => _('Invalid interval minutes input.'));
	}
	elseif ($INTERVAL_SETTING != 0 AND $INTERVAL_SETTING < 15)
	{
		$Response = array('Code' => false, 'msg' => _('Interval minutes can not less than 15.'));
	}
	elseif (is_numeric($SNAPSHOT_NUMBER) == false)
	{
		$Response = array('Code' => false, 'msg' => _('Invalid snapshot number input.'));
	}
	elseif ($SNAPSHOT_NUMBER != -1 AND ($SNAPSHOT_NUMBER < 1 OR $SNAPSHOT_NUMBER > 999))
	{
		$Response = array('Code' => false, 'msg' => _('Snapshot number input must between 1 and 999.'));
	}
	elseif (($_REQUEST['SERVICE_TYPE'] == 'OnPremise' OR $_REQUEST['SERVICE_LAUNCHER'] == 'false') AND $EXPORT_PATH == '' AND $EXPORT_JOB_READY == TRUE)
	{
		$Response = array('Code' => false, 'msg' => _('Image Export Path is required on selected transport server.'));
	}
	else
	{
		$_REQUEST['REPLICA_SETTINGS']['ExportPath'] = str_replace('\\','\\\\',$_REQUEST['REPLICA_SETTINGS']['ExportPath']);
		$_SESSION['REPLICA_SETTINGS'] =(isset($_SESSION['REPLICA_SETTINGS']))?$_SESSION['REPLICA_SETTINGS'] = array_merge($_SESSION['REPLICA_SETTINGS'],$_REQUEST['REPLICA_SETTINGS']):$_REQUEST['REPLICA_SETTINGS'];
		$Response = array('Code' => true, 'msg' => 'Good To Go!');
	}
	print_r(json_encode($Response));
	break;
		
	############################
	# QUERY SERVER INFORMATION
	############################
	case "QueryServerInformation":
	
	$ACTION    = 'QueryServerInformation';
	$REST_DATA = array('Action'   => $ACTION,
					   'ServUUID' => $_REQUEST['SERV_UUID']
				);
	$output = CurlPostToRemote($REST_DATA);
	
	$json = json_decode( $output, true);
	
	if( isset( $json["SERV_INFO"]["is_azure_mgmt_disk"] ) )
		$_SESSION["AzureManageTransport"] = $json["SERV_INFO"]["is_azure_mgmt_disk"];
	
	print_r($output);
	break;
		
	############################
	# QUERY HOST INFORMATION
	############################
	case "QueryHostInformation":
	
	$ACTION    = 'QueryHostInformation';
	$REST_DATA = array('Action'   => $ACTION,
					   'HostUUID' => $_REQUEST['HOST_UUID']
				);
	$output = CurlPostToRemote($REST_DATA);
	
	print_r($output);
	break;
		
	############################
	# QUERY CONNECTION INFORMATION
	############################
	case "QueryConnectionInformation":
	
	$ACTION    = 'QueryConnectionInformation';
	$REST_DATA = array('Action'   => $ACTION,				
					   'ConnUUID' => $_REQUEST['CONN_UUID']
				);
	$output = CurlPostToRemote($REST_DATA);
	
	print_r($output);
	break;
	
	############################
	# QUERY CONNECTION INFORMATION
	############################
	case "QueryConnectionInformationByServUUID":
	
	$ACTION    = 'QueryConnectionInformationByServUUID';
	$REST_DATA = array('Action'   => $ACTION,				
					   'HostUUID' => $_REQUEST['HOST_UUID'],
					   'LoadUUID' => $_REQUEST['LOAD_UUID'],
					   'LaunUUID' => $_REQUEST['LAUN_UUID']
				);
	$output = CurlPostToRemote($REST_DATA);
	
	print_r($output);
	break;
	
	############################
	# QUERY AVAILABLE CONNECTION
	############################
	case "QueryAvailableConnection":
	
	$ACTION    = 'QueryAvailableConnection';
	$REST_DATA = array('Action'   => $ACTION,
					   'AcctUUID' => $_REQUEST['ACCT_UUID']
				);
	$output = CurlPostToRemote($REST_DATA);
	
	print_r($output);
	break;
	
	
	############################
	# TEST CONNECTION
	############################
	case "TestConnection":
	
	$ACTION    = 'TestConnection';
	$REST_DATA = array('Action'   => $ACTION,
					   'SchdUUID' => $_REQUEST['SCHD_UUID'],
					   'CarrUUID' => $_REQUEST['CARR_UUID'],
					   'LoadUUID' => $_REQUEST['LOAD_UUID'],
					   'LaunUUID' => $_REQUEST['LAUN_UUID'],
					   'ConnDEST' => $_REQUEST['CONN_DEST']
				);
	$output = CurlPostToRemote($REST_DATA);

	print_r($output);
	break;
	
	
	############################
	# ADD CONNECTION
	############################
	case "AddConnection":
	
	$ACTION    = 'AddConnection';
	$REST_DATA = array('Action'   => $ACTION,
					   'SchdUUID' => $_REQUEST['SCHD_UUID'],
					   'CarrUUID' => $_REQUEST['CARR_UUID'],
					   'LoadUUID' => $_REQUEST['LOAD_UUID'],
					   'LaunUUID' => $_REQUEST['LAUN_UUID'],
					   'ConnDEST' => $_REQUEST['CONN_DEST']
				);
	$Response = CurlPostToRemote($REST_DATA);

	if ($Response == TRUE)
	{
		$output = json_encode(array('Code' => true,'Msg' => 'New connection created.'));
	}
	else
	{
		$output = json_encode(array('Code' => false,'Msg' => 'Cannot create connection.'));
	}	
	print_r($output);
	break;
	
	
	############################
	# SELECT EDIT HOST
	############################
	case "SelectEditHost":
		
	if (isset($_REQUEST['HOST_UUID']))
	{
		$ACTION    = 'SelectEditHost';
		$REST_DATA = array('Action'   => $ACTION,
						   'HostUUID' => $_REQUEST['HOST_UUID']			
						);
		
		$HOST_INFO = CurlPostToRemote($REST_DATA);
		
		if ($HOST_INFO != false)
		{
			$_SESSION['HOST_UUID'] = $_REQUEST['HOST_UUID'];
			$Response = array('Code' => true,'Response' => $HOST_INFO);
		}
		else
		{
			$Response = array('Code' => false,'Response' => _('Invalid host type.'));
		}
	}
	else
	{
		$Response = array('Code' => false,'Response' => _('Please select a host.'));
	}
	
	print_r(json_encode($Response));	
	break;
	
	
	############################
	# QUERY HOST INFORMATION
	############################
	case "QueryHostInfo":
		$ACTION    = 'QueryHostInfo';
		$REST_DATA = array('Action'   => $ACTION,
						   'HostUUID' => $_REQUEST['HOST_UUID']			
						);		
		$HOST_INFO = CurlPostToRemote($REST_DATA);
	
		print_r($HOST_INFO);
	break;	
	
	############################
	# QUERY SELECT HOST INFORMATION
	############################
	case "QuerySelectHostInfo":
		$ACTION    = 'QuerySelectHostInfo';
		$REST_DATA = array('Action'   => $ACTION,
						   'HostUUID' => $_REQUEST['HOST_UUID']			
						);		
		$HOST_INFO_JSON = CurlPostToRemote($REST_DATA);
	
		$HOST_INFO = json_decode($HOST_INFO_JSON,false);
		
		if ($HOST_INFO -> HOST_TYPE == 'Physical' OR $HOST_INFO -> HOST_TYPE == 'Offline')
		{
			if (!isset($HOST_INFO -> HOST_INFO -> manufacturer)){$meta_manufacturer = 'Generic Physical';}else{$meta_manufacturer = $HOST_INFO -> HOST_INFO -> manufacturer;}
		
			$Hostname 		= $HOST_INFO -> HOST_INFO -> client_name;
			$IPAddress		= $HOST_INFO -> HOST_ADDR;
			$Manufacturer 	= $meta_manufacturer;
			$CPUCore		= ($HOST_INFO -> HOST_INFO -> logical_processors != 0)?$HOST_INFO -> HOST_INFO -> logical_processors:$HOST_INFO -> HOST_INFO -> processors;
			$MemorySize		= $HOST_INFO -> HOST_INFO -> physical_memory;
			$OSType			= $HOST_INFO -> HOST_INFO -> os_name;
			$DiskInfo		= $HOST_INFO -> HOST_INFO -> disk_infos;
		}
		else
		{
			if (!isset($HOST_INFO -> HOST_INFO -> manufacturer)){$meta_manufacturer = 'VMware, Inc.';}else{$meta_manufacturer = $HOST_INFO -> HOST_INFO -> manufacturer;}
			
			$Hostname 		= $HOST_INFO -> HOST_NAME;
			$IPAddress		= $HOST_INFO -> HOST_ADDR;
			$Manufacturer 	= $meta_manufacturer;
			$CPUCore		= $HOST_INFO -> HOST_INFO -> number_of_cpu;
			$MemorySize		= $HOST_INFO -> HOST_INFO -> memory_mb;			
			$OSType			= $HOST_INFO -> HOST_INFO -> guest_os_name;			
			$DiskInfo		= $HOST_INFO -> HOST_INFO -> disks;
		}
		
		$HOST_INFO_ARRAY = array(
								_('Host Name')		=> $Hostname,
								_('OS Type')		=> $OSType,	
								_('CPU / Memory')	=> $CPUCore.'Cores / '.$MemorySize.'MB',
								_('Host IP')		=> $IPAddress,							
								'Disk Info'			=> $DiskInfo								
							);
		
		$HOST_INFO_TABLE = '';
		$HOST_INFO_TABLE .= '<table class="QueryHostTable">';
		$HOST_INFO_TABLE .= '<thead><tr>';
		if ($HOST_INFO -> HOST_TYPE != 'Virtual')
		{
			$HOST_INFO_TABLE .= '<th width="800px" colspan="2" style="line-height:28px;">'._('Host').'<button id="TakeXray" class="btn btn-danger pull-right btn-sm"><i id="UpdateSensorSpin" class="fa fa-cog fa-lg fa-fw"></i> X-Ray</button></th>';
		}
		else
		{
			$HOST_INFO_TABLE .= '<th width="800px" colspan="2">'._('Host').'</th>';
		}
		$HOST_INFO_TABLE .= '</tr></thead><tbody>';
		foreach($HOST_INFO_ARRAY as $Name => $Value)
		{
			switch ($Name)
			{
				case 'Disk Info':
					for ($i=0; $i<count($Value); $i++)
					{
						#CHECK BOOT DEVICE
						if (isset($Value[$i] -> boot_from_disk) AND $Value[$i] -> boot_from_disk == TRUE)
						{
							$IS_BOOT = '<i class="fa fa-check-circle-o" aria-hidden="true"></i> ';
						}
						else
						{
							$IS_BOOT = '<i class="fa fa-circle-o" aria-hidden="true"></i> ';
						}
						
						#QUERY DISK NAME
						if (isset($Value[$i] -> friendly_name))
						{
							$DISK_NAME = $Value[$i] -> friendly_name;
						}
						else
						{
							$DISK_NAME = $Value[$i] -> name;
						}
						
						#DISK SIZE
						$DISK_SIZE = ($Value[$i] -> size)/1024/1024/1024;						
						
						$HOST_INFO_TABLE .= '<tr>';
						$HOST_INFO_TABLE .= '<td width="150px">'._('Disk').' '.$i.'</td>';
						$HOST_INFO_TABLE .= '<td width="650px">'.$IS_BOOT.$DISK_NAME.' ('.$DISK_SIZE.'GB)</div></td>';
						$HOST_INFO_TABLE .= '</tr>';						
					}				
				break;
				
				case 'IPAddress':
					print_r($Value);
				
					for ($i=0; $i<count($Value); $i++)
					{

					}				
				break;
			
				default:			
					$HOST_INFO_TABLE .= '<tr>';
					$HOST_INFO_TABLE .= '<td width="150px">'.$Name.'</td>';
					$HOST_INFO_TABLE .= '<td width="650px">'.$Value.'</div></td>';
					$HOST_INFO_TABLE .= '</tr>';
			}
		}
			
		$HOST_INFO_TABLE .= '</tbody></table>';
		
		if (count((array)$HOST_INFO -> HOST_SERV) > 1)
		{		
			$TS_name 			= $HOST_INFO -> HOST_SERV -> HOST_NAME;
			$TS_Path			= chop($HOST_INFO -> HOST_SERV -> SERV_INFO -> path, '\Launcher');
			$TS_Version  		= $HOST_INFO -> HOST_SERV -> SERV_INFO -> version;
			$TS_Manufacturer  	= $HOST_INFO -> HOST_SERV -> SERV_INFO -> manufacturer;
			$TS_CPUCore  		= $HOST_INFO -> HOST_SERV -> SERV_INFO -> processors;
			$TS_MemorySize		= $HOST_INFO -> HOST_SERV -> SERV_INFO -> physical_memory;
			$TS_OSType			= $HOST_INFO -> HOST_SERV -> SERV_INFO -> os_name;
			$TS_NetworkInfo 	= $HOST_INFO -> HOST_SERV -> SERV_INFO -> network_infos;

			$TS_Hostname		= $TS_name;
			$TS_Information		= $TS_Path.' ('.$TS_Version.')';
			$TS_System			= $TS_CPUCore.'Cores / '.$TS_MemorySize.'MB';
		}
		else
		{
			$TS_Hostname	= _('To Be Discovered');
			$TS_Information = _('To Be Discovered');
			$TS_OSType		= _('To Be Discovered');
			$TS_System		= _('To Be Discovered');
			$TS_NetworkInfo	= _('To Be Discovered');
		}
		$SERV_INFO_ARRAY = array(
								_('Host Name') 				=> $TS_Hostname,
								_('Transport Information') 	=> $TS_Information,
								_('OS Type')				=> $TS_OSType,
								_('CPU / Memory')			=> $TS_System,
								'Network Info'				=> $TS_NetworkInfo
							);
	
		$HOST_INFO_TABLE .= '<br>';
		$HOST_INFO_TABLE .= '<table class="QueryHostTable">';
		$HOST_INFO_TABLE .= '<thead><tr>';
		$HOST_INFO_TABLE .= '<th width="800px" colspan="2" style="line-height:28px;">'._('Transport Server').'</th>';
		$HOST_INFO_TABLE .= '</tr></thead><tbody>';
	
		foreach($SERV_INFO_ARRAY as $Name => $Value)
		{
			switch ($Name)
			{
				case 'Network Info':
					if ($Value != 'To Be Discovered')
					{
						for ($i=0; $i<count($Value); $i++)
						{
							$HOST_INFO_TABLE .= '<tr>';
							$HOST_INFO_TABLE .= '<td width="150px">'._('Adapter').' '.$i.'</td>';
							if( isset( $Value[$i] -> ip_addresses[0] ) )
								$HOST_INFO_TABLE .= '<td width="650px">'.$Value[$i] -> description.' ('.$Value[$i] -> ip_addresses[0].')</div></td>';
							else
								$HOST_INFO_TABLE .= '<td width="650px">'.$Value[$i] -> description.' (None)</div></td>';
							$HOST_INFO_TABLE .= '</tr>';
						}
					}
					else
					{
						$HOST_INFO_TABLE .= '<tr>';
						$HOST_INFO_TABLE .= '<td width="150px">'._('Adapter').'</td>';
						$HOST_INFO_TABLE .= '<td width="650px">'.$Value.'</div></td>';
						$HOST_INFO_TABLE .= '</tr>';
					}
				break;
			
				default:			
					$HOST_INFO_TABLE .= '<tr>';
					$HOST_INFO_TABLE .= '<td width="150px">'.$Name.'</td>';
					$HOST_INFO_TABLE .= '<td width="650px">'.$Value.'</div></td>';
					$HOST_INFO_TABLE .= '</tr>';
			}
		}
		
		$HOST_INFO_TABLE .= '</tbody></table>';
		
		$HOST_INFO_TABLE .= '<script>';
		$HOST_INFO_TABLE .= '$("#TakeXray").click(function TakeXray(){$("#UpdateSensorSpin").addClass("fa-spin");$("#LoadingOverLay").addClass("TransparentOverlay SpinnerLoading");$.ajax({type:"POST",dataType:"JSON",url:"_include/_exec/mgmt_service.php",';
		$HOST_INFO_TABLE .= 'data:{"ACTION":"TakeXray","ACCT_UUID":"'.$_SESSION['admin']['ACCT_UUID'].'","XRAY_PATH":"'.str_replace("\\","/",str_replace("_exec/mgmt_service.php","_inc/_xray/",$_SERVER['SCRIPT_FILENAME'])).'","XRAY_TYPE":"host","SERV_UUID":"'.$_REQUEST["HOST_UUID"].'"},';
		$HOST_INFO_TABLE .= 'success:function(xray){$("#UpdateSensorSpin").removeClass("fa-spin");$("#LoadingOverLay").removeClass("TransparentOverlay SpinnerLoading");if (xray != false){window.location.href = "./_include/_inc/_xray/"+xray}}});})';
		$HOST_INFO_TABLE .= '</script>';
	
		print_r($HOST_INFO_TABLE);
	break;
	
	
	############################
	# DELETE HOST PACKER
	############################
	case "DeletePackerHost":
		
	if (isset($_REQUEST['HOST_UUID']))
	{
		$ACTION    = 'DeletePackerHost';
		$REST_DATA = array('Action'   => $ACTION,
						   'AcctUUID' => $_REQUEST['ACCT_UUID'],
						   'HostUUID' => $_REQUEST['HOST_UUID']			
				);
		$Response = CurlPostToRemote($REST_DATA);
	}
	else
	{
		$Response = array('Code' => false, 'Msg' => _('Please select a host.'));
	}
	print_r($Response);
	break;
	
	############################
	# REFLUSH PACKER
	############################
	case "ReflushPacker":
	case "ReflushPackerAsync":

	if (isset($_REQUEST['HOST_UUID']))
	{
		$HOST_UUID = $_REQUEST['HOST_UUID'];
	}
	else
	{
		$HOST_UUID = '';
	}
	
	$REST_DATA = array('Action'   => $_REQUEST['ACTION'],
					   'AcctUUID' => $_REQUEST['ACCT_UUID'],
					   'HostUUID' => $HOST_UUID
			);
	$Response = CurlPostToRemote($REST_DATA);

	print_r(json_encode(true));
	break;
		
	############################
	# BEGIN TO RUN REPLICA
	############################
	case "BeginToRunReplica":
	case "BeginToRunReplicaAsync":

	$REST_DATA = array('Action'   			 => $_REQUEST['ACTION'],
					   'AcctUUID' 			 => $_REQUEST['ACCT_UUID'],
					   'RegnUUID' 			 => $_REQUEST['REGN_UUID'],
					   'HostUUID' 			 => $_REQUEST['HOST_UUID'],
					   'CarrUUID' 			 => $_REQUEST['CARR_UUID'],
					   'LoadUUID' 			 => $_REQUEST['LOAD_UUID'],
					   'LaunUUID' 			 => $_REQUEST['LAUN_UUID'],
					   'ReplicaSettings'	 => $_REQUEST['REPLICA_SETTINGS'],
					   'MgmtADDR' 			 => $_REQUEST['MGMT_ADDR']
				);
	
	if ($_REQUEST['ACTION'] == 'BeginToRunReplica')
	{
		$output = CurlPostToRemote($REST_DATA,2500);
	}
	else
	{
		$output = CurlPostToRemote($REST_DATA);
	}	
	print_r($output);
	break;
	
	############################
	# SYNC SELECT REPLICA
	############################
	case "SyncSelectReplica":
	case "SyncSelectReplicaAsync":
	
	if (isset($_REQUEST['REPL_UUID']))
	{
		$REST_DATA = array('Action'   		=> $_REQUEST['ACTION'],
						   'ReplUUID' 		=> $_REQUEST['REPL_UUID'],
						   'SyncTYPE' 		=> $_REQUEST['SYNC_TYPE'],
						   'DebugSnapshot'	=> $_REQUEST['DEBUG_SNAPSHOT'],
						);
						
		if($_REQUEST['ACTION'] == 'SyncSelectReplica')
		{
			CurlPostToRemote($REST_DATA,25);
		}
		else
		{
			CurlPostToRemote($REST_DATA);
		}
		
		$Response = json_encode(array('msg' => _('Synchronization started.')));		
	}
	else
	{
		$Response = json_encode(array('msg' => _('Please select a preparation process.')));
	}
	print_r($Response);
	break;
	
	############################
	# DELETE SELECT REPLICA
	############################
	case "DeleteSelectReplica":
	case "DeleteSelectReplicaAsync":
	
	if (isset($_REQUEST['REPL_UUID']))
	{
		$REST_DATA = array('Action'   => $_REQUEST['ACTION'],
						   'ReplUUID' => $_REQUEST['REPL_UUID']
					);
		if($_REQUEST['ACTION'] == 'DeleteSelectReplica')
		{
			CurlPostToRemote($REST_DATA,25);
		}
		else
		{
			CurlPostToRemote($REST_DATA);
		}
		$Response = json_encode(array('msg' => _('Deleting replication job.')));
	}
	else
	{
		$Response = json_encode(array('msg' => _('Please select a replication process.')));
	}
	print_r($Response);
	break;
		
	############################
	# EDIT SELECT REPLICA
	############################
	case "EditSelectReplica":
	
	if (isset($_REQUEST['REPL_UUID']))
	{
		$ACTION    = 'EditSelectReplica';
		$JSON_DATA = array('Action'   => $ACTION,
						   'ReplUUID' => $_REQUEST['REPL_UUID']);
	
		$_SESSION['EDIT_REPLICA_UUID'] = $_REQUEST['REPL_UUID'];
		$Response = json_encode(array('Code' => true, 'msg' => ''));
	}
	else
	{
		$Response = json_encode(array('Code' => false, 'msg' => _('Please select a preparation process.')));
	}
	print_r($Response);
	break;

	############################
	# QUERY SELECT REPLICA SNAPSHOT SCRIPT
	############################
	case "QueryReplicaSnapshotScript":
		$ACTION    = 'QueryReplicaSnapshotScript';
		$REST_DATA = array('Action'   => $ACTION,
						   'ReplUUID' => $_REQUEST['REPL_UUID']
					);
		$Response = CurlPostToRemote($REST_DATA);
		
		$DecodeResponse = json_decode($Response);
		
		$SNAPSHOT_SCRIPT_TABLE = '';		
		$SNAPSHOT_SCRIPT_TABLE .= '<table id="SnapshotScriptTable">';
		$SNAPSHOT_SCRIPT_TABLE .= '<thead><tr>';
		$SNAPSHOT_SCRIPT_TABLE .= '<th><label for="comment">'._('Pre Snapshot Script').':</label></th>';
		$SNAPSHOT_SCRIPT_TABLE .= '</tr></thead>';
		
		$SNAPSHOT_SCRIPT_TABLE .= '<tr>';
		$SNAPSHOT_SCRIPT_TABLE .= '<td><textarea class="form-control" rows="5" cols="50" id="PreSnapshotScript" style="overflow:hidden; resize:none;" placeholder="'._('Input Command Here').'">'.$DecodeResponse -> pre_snapshot_script.'</textarea></td>';
		$SNAPSHOT_SCRIPT_TABLE .= '</tr>';
		
		$SNAPSHOT_SCRIPT_TABLE .= '<tr>';
		$SNAPSHOT_SCRIPT_TABLE .= '<td style="border:none; border-style:none; height:3px;"></td>';
		$SNAPSHOT_SCRIPT_TABLE .= '</tr>';
		
		$SNAPSHOT_SCRIPT_TABLE .= '<thead><tr>';
		$SNAPSHOT_SCRIPT_TABLE .= '<th><label for="comment">'._('Post Snapshot Script').':</label></th>';
		$SNAPSHOT_SCRIPT_TABLE .= '</tr></thead>';
		
		$SNAPSHOT_SCRIPT_TABLE .= '<tr>';
		$SNAPSHOT_SCRIPT_TABLE .= '<td><textarea class="form-control" rows="5" cols="50" id="PostSnapshotScript" style="overflow:hidden; resize:none;" placeholder="'._('Input Command Here').'">'.$DecodeResponse -> post_snapshot_script.'</textarea></td>';
		$SNAPSHOT_SCRIPT_TABLE .= '</tr>';
		
		$SNAPSHOT_SCRIPT_TABLE .= '</table>';
		
		print_r($SNAPSHOT_SCRIPT_TABLE);	
	break;
	
	############################
	# QUERY REPLICA EXCLUDED PATHS
	############################
	case "QueryReplicaExcludedPaths":
		$ACTION    = 'QueryReplicaExcludedPaths';
		$REST_DATA = array('Action'   => $ACTION,
						   'ReplUUID' => $_REQUEST['REPL_UUID']
					);
		$Response = CurlPostToRemote($REST_DATA);
		
		$DecodeResponse = json_decode($Response);
		
		$EXCLUDED_PATHS_TABLE = '';		
		$EXCLUDED_PATHS_TABLE .= '<table id="ExcludedPathsTable">';

		$EXCLUDED_PATHS_TABLE .= '<tr>';
		$EXCLUDED_PATHS_TABLE .= '<td><textarea class="form-control" rows="8" cols="30" id="ExcludedPaths" style="overflow:hidden; resize:none;" placeholder="'._('Replication Exclude paths').'"></textarea></td>';
		$EXCLUDED_PATHS_TABLE .= '</tr>';
		
		$EXCLUDED_PATHS_TABLE .= '</table>';
		
		$EXCLUDED_PATHS_TABLE .= '<script>';
		$EXCLUDED_PATHS_TABLE .= 'function getCookie(cname){var name = cname + "="; var decodedCookie = decodeURIComponent(document.cookie); var ca = decodedCookie.split(";"); for(var i = 0; i <ca.length; i++) { var c = ca[i]; while (c.charAt(0) == " ") {c = c.substring(1);}if (c.indexOf(name) == 0) {return c.substring(name.length, c.length);}}return "";}';
		
		$EXCLUDED_PATHS_TABLE .= 'if(getCookie("input_path") != ""){document.getElementById("ExcludedPaths").value = getCookie("input_path").replace(/\,/g,"\n");}';
		
		$EXCLUDED_PATHS_TABLE .= '</script>';
		
		print_r($EXCLUDED_PATHS_TABLE);	
	break;
	
	############################
	# QUERY SELECT REPLICA CONFIGURATION
	############################
	case "QueryReplicaConfiguration":
		$ACTION    = 'QueryReplicaConfiguration';
		$REST_DATA = array('Action'   => $ACTION,
						   'ReplUUID' => $_REQUEST['REPL_UUID']
					);
		$Response = CurlPostToRemote($REST_DATA);
		
		$DecodeResponse = json_decode($Response);
		
		$UserSetTimeUTC = strtotime($DecodeResponse -> triggers[0] -> start);
		
		if ($UserSetTimeUTC != '')
		{
			$DecodeResponse -> user_set_time = Misc::time_convert_with_zone($UserSetTimeUTC);
		}
		else
		{
			$DecodeResponse -> user_set_time = 'Run Now';
		}
		$Response = json_encode($DecodeResponse);		
		
		print_r($Response);	
	break;
	
	############################
	# UPDATE REPLICA CONFIGURATION
	############################
	case "UpdateReplicaConfiguration":
		$ACTION = 'UpdateReplicaConfiguration';
		
		#INTERVAL MINUTES INPUT CHECK
		$INTERVAL_SETTING = $_REQUEST['REPLICA_SETTINGS']['IntervalMinutes'];
		$SNAPSHOT_NUMBER = $_REQUEST['REPLICA_SETTINGS']['SnapshotsNumber'];
		
		if (is_numeric($INTERVAL_SETTING) == false)
		{
			$Response = array('Code' => false, 'msg' => _('Invalid interval. Please enter a different value.'));
		}
		elseif ($INTERVAL_SETTING != 0 AND $INTERVAL_SETTING < 15)
		{
			$Response = array('Code' => false, 'msg' => _('Interval minutes can not less than 15.'));
		}
		elseif (is_numeric($SNAPSHOT_NUMBER) == false)
		{
			$Response = array('Code' => false, 'msg' => _('Invalid snapshot number input.'));
		}
		elseif ($SNAPSHOT_NUMBER != -1 AND ($SNAPSHOT_NUMBER < 1 OR $SNAPSHOT_NUMBER > 999))
		{
			$Response = array('Code' => false, 'msg' => _('Snapshot number input must between 1 and 999.'));
		}
		else
		{
			$_REQUEST['REPLICA_SETTINGS']['PreSnapshotScript']= addslashes($_REQUEST['REPLICA_SETTINGS']['PreSnapshotScript']);
			$_REQUEST['REPLICA_SETTINGS']['PostSnapshotScript']= addslashes($_REQUEST['REPLICA_SETTINGS']['PostSnapshotScript']);
			
			$_REQUEST['REPLICA_SETTINGS']['ExportPath'] = str_replace('\\','\\\\',$_REQUEST['REPLICA_SETTINGS']['ExportPath']);
			
			#REPLICA CONFIGURATION
			$REST_DATA = array(
								'Action'			  => $ACTION,
								'ReplUUID'			  => $_REQUEST['REPL_UUID'],
								'ReplicaSettings'	  => json_encode($_REQUEST['REPLICA_SETTINGS'])
							);
						
			$Output = CurlPostToRemote($REST_DATA);
			if ($Output == TRUE)
			{
				$Response = array('Code' => $Output, 'msg' => _('Schedule updated.') );
			}
			else
			{
				$Response = array('Code' => $Output, 'msg' => _('Failed to update schedule. Please review configuration.') );
			}
		}		
		print_r(json_encode($Response));
	break;
	
	############################
	# SELECT REPLICA TO SESSION
	############################
	case "SelectReplicaToSession":
		if (isset($_REQUEST['REPL_UUID']))
		{
			$_SESSION['REPL_UUID'] = $_REQUEST['REPL_UUID'];
			print_r(json_encode(array('Code' => true)));
		}
		else
		{
			print_r(json_encode(array('Code' => false, 'Msg' => _('Please select a preparation process.'))));
		}
	break;
	
	############################
	# GET UNINITIALIZED DISKS SCSI
	############################
	case "GetUninitializedDisksSCSI":
	
	$ACTION    = 'GetUninitializedDisksSCSI';
	$REST_DATA = array('Action'   => $ACTION,
					   'ConnUUID' => $_REQUEST['CONN_UUID']
				);
	$output = CurlPostToRemote($REST_DATA);
	
	print_r($output);	
	break;

	
	############################
	# QUERY REPLICA INFORMATION
	############################
	case "QueryReplicaInformation":
	
	if (isset($_REQUEST['REPL_UUID']))
	{
		$ACTION    = 'QueryReplicaInformation';
		$REST_DATA = array('Action'   => $ACTION,
						   'ReplUUID' => $_REQUEST['REPL_UUID']
					);
		$output = CurlPostToRemote($REST_DATA);
	}
	else
	{
		$output = false;
	}	
	print_r($output);
	break;
	
	
	############################
	# QUERY REPLICA DISK INFORMATION
	############################
	case "QueryReplicaDiskInformation":
		
	$ACTION    = 'QueryReplicaDiskInformation';
	$REST_DATA = array('Action'   => $ACTION,
					   'ReplUUID' => $_REQUEST['REPL_UUID']
				);
	$output = CurlPostToRemote($REST_DATA);
	
	print_r($output);
	break;
	
	
	############################
	# LIST AVAILABLE SERVICE
	############################
	case "ListAvailableService":
		
	$ACTION    = 'ListAvailableService';
	$REST_DATA = array('Action'   => $ACTION,
					   'AcctUUID' => $_REQUEST['ACCT_UUID']
				);
	$ListService = CurlPostToRemote($REST_DATA);
	
	$ListServiceArray = json_decode($ListService,true);	
	if ($ListServiceArray != false)
	{		
		$COUNT_SERVICE = count($ListServiceArray);
	
		for ($i=0; $i<$COUNT_SERVICE; $i++)
		{
			
			array_unshift( $ListServiceArray[$i]['SERV_ARGUMENTS'], $ListServiceArray[$i]['SERV_FORMAT']);
			
			$msg = call_user_func_array('translate', $ListServiceArray[$i]['SERV_ARGUMENTS']);
			
			$LIST_SERV[$i] = array(
								'SERV_UUID' 	 => $ListServiceArray[$i]['SERV_UUID'],
								'HOST_NAME' 	 => $ListServiceArray[$i]['HOST_NAME'],
								//'SERV_MESG' 	 => $ListServiceArray[$i]['SERV_MESG'],
								'SERV_MESG'		 => $msg,
								'SERV_FORMAT' 	 => $ListServiceArray[$i]['SERV_FORMAT'],
								'SERV_ARGUMENTS' => $ListServiceArray[$i]['SERV_ARGUMENTS'],
								'SERV_TIME' 	 => Misc::time_convert_with_zone($ListServiceArray[$i]['SYNC_TIME']),
								'STATUS'		 => $ListServiceArray[$i]['STATUS']
							);
		}
		print_r(json_encode($LIST_SERV));
	}
	else
	{
		print_r($ListService);
	}
	break;
	
	
	############################
	# SELECT SERVICE REPLICA
	############################
	case "SelectServiceReplica":
	
	if (isset($_REQUEST['REPL_UUID']))
	{
		$ACTION = 'SelectServiceReplica';
		$REST_DATA = array('Action'   => $ACTION,
						   'ReplUUID' => $_REQUEST['REPL_UUID']	
					);
		$REPL_JSON = CurlPostToRemote($REST_DATA);
		$REPL_ARRAY = json_decode($REPL_JSON,true);

		$_SESSION['HOST_NAME']			= $REPL_ARRAY['HOST_NAME'];
		$_SESSION['PLAN_UUID'] 	   		= $REPL_ARRAY['PLAN_UUID'];
		$_SESSION['PLAN_JSON'] 	   		= $REPL_ARRAY['PLAN_JSON'];
		$_SESSION['REPL_UUID'] 	   		= $REPL_ARRAY['REPL_UUID'];
		$_SESSION['CLUSTER_UUID']  		= $REPL_ARRAY['CLUSTER_UUID'];
		$_SESSION['SERV_UUID']	   		= $REPL_ARRAY['SERV_UUID'];
		$_SESSION['SERV_REGN']	   		= $REPL_ARRAY['SERV_REGN'];
		$_SESSION['CLOUD_TYPE']	   		= $REPL_ARRAY['CLOUD_TYPE'];
		$_SESSION['VENDOR_NAME']   		= $REPL_ARRAY['VENDOR_NAME'];
		$_SESSION['SERVER_UUID']  		= $REPL_ARRAY['SERVER_UUID'];
		$_SESSION['REPL_OS_TYPE']  		= $REPL_ARRAY['OS_TYPE'];
		$_SESSION['HOST_TYPE']  		= $REPL_ARRAY['HOST_TYPE'];
		$_SESSION['IsAzureBlobMode']	= $REPL_ARRAY['JOBS_JSON']['is_azure_blob_mode'];		
		
		#if ($REPL_ARRAY['IS_RCD_JOB'] == true AND $REPL_ARRAY['CLOUD_TYPE'] == 'UnknownCloudType')
		if ($REPL_ARRAY['IS_RCD_JOB'] == true)
		{
			$DIRECT_TO = 'WP';
		}
		elseif ($REPL_ARRAY['IS_EXPORT_JOB'] == true)
		{
			$DIRECT_TO = 'EX';
		}
		elseif ($REPL_ARRAY['IS_TEMPLATE'] == true)
		{
			$DIRECT_TO = 'RP';
		}
		elseif ($REPL_ARRAY['CLOUD_TYPE'] == 'UnknownCloudType')
		{
			$DIRECT_TO = 'WP';
		}
		else
		{
			$DIRECT_TO = 'RE';
		}
		$output = json_encode(array('Code' => true,'DIRECT_TO' => $DIRECT_TO,'CLOUD_TYPE' => $REPL_ARRAY['CLOUD_TYPE']));
	}
	else
	{
		$output = json_encode(array('Code' => _('Please select a host.')));
	}
	print_r($output);
	break;
	
	
	############################
	# RECOVERY NEXT ROUTE
	############################
	case "RecoveryNextRoute":

	if (isset($_REQUEST['RECY_TYPE']))
	{
		$SERV_REGN = $_REQUEST['SERV_REGN'];
		$RECY_TYPE = $_REQUEST['RECY_TYPE'];
		$CodeType  = true;
		
		$_SESSION['RECY_TYPE'] = $_REQUEST['RECY_TYPE'];

		switch($RECY_TYPE)
		{
			case "RECOVERY_PM":
				$_SESSION['SNAP_UUID'] = 'Planned_Migration';
				if ($_SESSION['CLOUD_TYPE'] == 'OPENSTACK')
				{
					$location = 'SelectListSnapshot';				
				}
				else if ($_SESSION['CLOUD_TYPE'] == 'AWS')
				{
					$location = 'SelectListEbsSnapshot';
				}
				else if ($_SESSION['CLOUD_TYPE'] == 'Azure')
				{
					$location = 'SelectListAzureSnapshot';
				}
				else if ($_SESSION['CLOUD_TYPE'] == 'Aliyun')
				{
					$location = 'InstanceAliyunConfigurations';
				}
				else if ($_SESSION['CLOUD_TYPE'] == 'Tencent')
				{
					$location = 'InstanceTencentConfigurations';
				}
				else if ($_SESSION['CLOUD_TYPE'] == 'Ctyun')
				{
					#$location = 'InstanceCtyunConfigurations';
					$location = 'SelectListCtyunSnapshot';
				}
				else if ($_SESSION['CLOUD_TYPE'] == 'VMWare')
				{
					$location = 'InstanceVMWareConfigurations';
				}
			break;
			
			case "RECOVERY_DR":
			case "RECOVERY_DT":
				if ($_SESSION['CLOUD_TYPE'] == 'OPENSTACK')
				{
					$location = 'SelectListSnapshot';
				}
				else if ($_SESSION['CLOUD_TYPE'] == 'AWS')
				{
					$location = 'SelectListEbsSnapshot';				
				}
				else if ($_SESSION['CLOUD_TYPE'] == 'Azure' OR $_SESSION['CLOUD_TYPE'] == 'AzureBlob')
				{
					$location = 'SelectListAzureSnapshot';
				}
				else if ($_SESSION['CLOUD_TYPE'] == 'Aliyun')
				{
					$location = 'SelectListAliyunSnapshot';
				}
				else if ($_SESSION['CLOUD_TYPE'] == 'Tencent')
				{
					$location = 'SelectListTencentSnapshot';
				}
				else if ($_SESSION['CLOUD_TYPE'] == 'Ctyun')
				{
					$location = 'SelectListCtyunSnapshot';
				}
				else if ($_SESSION['CLOUD_TYPE'] == 'VMWare')
				{
					$location = 'SelectListVMWareSnapshot';
				}
			break;
			
			default:
				$CodeType = false;
				$location = '';
			break;
		}

		$output = json_encode(array('Code' => $CodeType, 'Route' => $location));
	}
	else
	{
		$output = json_encode(array('Code' => false, 'Route' => _('Please select a recovery type.')));
	}
	print_r($output);
	break;
	
	
	############################
	# RECOVERY PLAN NEXT ROUTE
	############################
	case "RecoveryPlanNextRoute":

	if (isset($_REQUEST['RECY_TYPE']))
	{
		$SERV_REGN = $_REQUEST['SERV_REGN'];
		$RECY_TYPE = $_REQUEST['RECY_TYPE'];
				
		$_SESSION['RECY_TYPE'] = $_REQUEST['RECY_TYPE'];
		switch($_REQUEST['CLOUD_TYPE'])
		{
			case "OPENSTACK":
				//$location = 'PlanInstanceConfigurations';
				$location = 'PlanSelectListSnapshot';
			break;
			
			case "AWS":
				//$location = 'PlanInstanceAwsConfigurations';
				$location = 'PlanSelectListEbsSnapshot';
			break;
			
			case "Azure":
			case "AzureBlob":
				$location = 'PlanSelectListSnapshot';
				//$location = 'PlanInstanceAzureConfigurations';
			break;
			
			case "Aliyun":
				$location = 'PlanSelectListSnapshot';						 
				//$location = 'PlanInstanceAliyunConfigurations';
			break;
			
			case "Tencent":
				$location = 'PlanInstanceTencentConfigurations';
			break;
			
			case "Ctyun":
				$location = 'PlanSelectListSnapshot';
			break;
			
			case "VMWare":
				$location = 'PlanInstanceVMWareConfigurations';
			break;
		}		
		$output = json_encode(array('Code' => true, 'Route' => $location));
	}
	else
	{
		$output = json_encode(array('Code' => false, 'Route' => _('Please select a recovery type.')));
	}
	print_r($output);
	break;
	
	############################
	# SELECT SERVICE SNAPSHOT
	############################
	case "SelectServiceSnapshot":
	
	$ACTION = 'SelectServiceSnapshot';
	
	if (isset($_REQUEST['SNAP_UUID']))
	{
		$DISK_COUNT = explode(',',$_REQUEST['SNAP_UUID']);

		$SNAP_ARRAY = array_diff(explode(',',$_REQUEST['SNAP_UUID']),array('false'));
		
		$_SESSION['SNAP_UUID'] = $_REQUEST['SNAP_UUID'];
		
		if ($_SESSION['RECY_TYPE'] == 'RECOVERY_PM')
		{
			if (count($SNAP_ARRAY) != count($DISK_COUNT))
			{	
				$output = json_encode(array('Code' => true));
			}
			else
			{
				$output = json_encode(array('Code' => false));
			}
		}
		else
		{
			if (count($SNAP_ARRAY) != 0)
			{		
				$output = json_encode(array('Code' => true));
			}
			else
			{
				$output = json_encode(array('Code' => false));
			}
		}
	}
	else
	{
		$output = json_encode(array('Code' => false));
	}

	print_r($output);
	break;
	
	
	case "ServiceConfiguration":
		if (isset($_SESSION['SNAP_UUID']))
		{
			$_REQUEST['SERVICE_SETTINGS']['snap_uuid'] = $_SESSION['SNAP_UUID'];	
		}
		$_SESSION['SERVICE_SETTINGS'] = $_REQUEST['SERVICE_SETTINGS'];
	
		$output = json_encode(array('Code' => true));
		print_r($output);
	break;
	
	
	############################
	# SELECT SERVICE PLAN VOLUME
	############################
	case "SelectServicePlanVolume":
		if (isset($_REQUEST['VOLUME_UUID']))
		{
			$VOLUME_ARRAY = array_diff(explode(',',$_REQUEST['VOLUME_UUID']),array('false'));
			
			if (count($VOLUME_ARRAY) != 0)
			{
				$_SESSION['VOLUME_UUID'] = $_REQUEST['VOLUME_UUID'];
				$output = json_encode(array('Code' => true));
			}
			else
			{
				$output = json_encode(array('Code' => false));
			}
		}
		else
		{
			$output = json_encode(array('Code' => false));
		}

	print_r($output);
	
	break;
	
	
	############################
	# SELECT SERVICE INSTANCE CONFIG
	############################
	case "SelectInstanceConfig":
	
	$ACTION = 'SelectInstanceConfig';
	
	$_SESSION['NETWORK_UUID'] 		= $_REQUEST['NETWORK_UUID'];
	$_SESSION['RCVY_PRE_SCRIPT']	= $_REQUEST['RCVY_PRE_SCRIPT'];
	$_SESSION['RCVY_POST_SCRIPT']	= $_REQUEST['RCVY_POST_SCRIPT'];
	
	if(isset($_REQUEST['FLAVOR_ID']))
		$_SESSION['FLAVOR_ID']    		= $_REQUEST['FLAVOR_ID'];
	
	if(isset($_REQUEST['SGROUP_UUID']))
		$_SESSION['SGROUP_UUID']  		= $_REQUEST['SGROUP_UUID'];
	
	if(isset($_REQUEST['SWITCH_UUID']))
	{
		$_SESSION['SWITCH_UUID'] = $_REQUEST['SWITCH_UUID'];
	}
	
	if(isset($_REQUEST['SUBNET_UUID']))
	{
		$_SESSION['SUBNET_UUID'] = $_REQUEST['SUBNET_UUID'];
	}
	
	if(isset($_REQUEST['PUBLIC_IP']))
	{
		$_SESSION['PUBLIC_ADDR_ID'] = $_REQUEST['PUBLIC_IP'];
	}
	
	if(isset($_REQUEST['PRIVATE_IP']))
	{
		$_SESSION['PRIVATE_ADDR_ID'] = $_REQUEST['PRIVATE_IP'];
	}
	
	if(isset($_REQUEST['DISK_TYPE']))
	{
		$_SESSION['DISK_TYPE'] = $_REQUEST['DISK_TYPE'];
	}
	
	if(isset($_REQUEST['AVAILABILITYSET']))
	{
		if( $_REQUEST['AVAILABILITYSET'] == "false" )
			$_SESSION['AVAILABILITYSET'] = false;
		else
			$_SESSION['AVAILABILITYSET'] = $_REQUEST['AVAILABILITYSET'];
	}
	
	if (isset($_REQUEST['HOST_TAG_NAME']))
	{
		$_SESSION['HOST_TAG_NAME'] = $_REQUEST['HOST_TAG_NAME'];
	}

	if(isset($_REQUEST['CPU']))
		$_SESSION['CPU'] = $_REQUEST['CPU'];
	
	if(isset($_REQUEST['Memory']))
		$_SESSION['Memory'] = $_REQUEST['Memory'];

	$output = json_encode(array('Code' => true));
	
	print_r($output);
	break;
	
	
	############################
	# SELECT SERVICE AWS INSTANCE CONFIG
	############################
	case "SelectAwsInstanceConfig":
	
	$ACTION = 'SelectAwsInstanceConfig';
	
	$_SESSION['FLAVOR_ID']    	 = $_REQUEST['FLAVOR_ID'];
	$_SESSION['NETWORK_UUID'] 	 = $_REQUEST['NETWORK_UUID'];
	$_SESSION['SGROUP_UUID']  	 = $_REQUEST['SGROUP_UUID'];
	$_SESSION['PUBLIC_ADDR_ID']  = $_REQUEST['PUBLIC_ADDR_ID'];
	
	$output = json_encode(array('Code' => true));
	
	print_r($output);
	break;
	
	
	############################
	# SUMMARY PAGE ROUTE
	############################
	case "SummaryPageRoute":
		
	$SERVER_REGN    = $_REQUEST['SERV_REGN'];
	$RECOVERY_TYPE  = $_REQUEST['RECY_TYPE'];
	$CLOUD_TYPE 	= $_REQUEST['CLOUD_TYPE'];
	if ($RECOVERY_TYPE == 'RECOVERY_PM')
	{
		$BACK_ROUTE = 'SelectRecoverPlan';		
	}
	else
	{
		if ($CLOUD_TYPE == 'OPENSTACK')
		{
			$BACK_ROUTE = 'SelectListSnapshot';
		}
		else if ($CLOUD_TYPE == 'AWS')
		{
			$BACK_ROUTE = 'SelectListEbsSnapshot';
		}
		else if ($CLOUD_TYPE == 'Azure' OR $CLOUD_TYPE == 'AzureBlob')
		{
			$BACK_ROUTE = 'SelectListAzureSnapshot';
		}
		else if ($CLOUD_TYPE == 'Aliyun')
		{
			$BACK_ROUTE = 'SelectListAliyunSnapshot';
		}
		else if ($CLOUD_TYPE == 'Ctyun')
		{
			$BACK_ROUTE = 'SelectListCtyunSnapshot';
		}
		else if ($CLOUD_TYPE == 'Tencent')
		{
			$BACK_ROUTE = 'SelectListTencentSnapshot';
		}
		else if ($CLOUD_TYPE == 'VMWare')
		{
			$BACK_ROUTE = 'SelectListVMWareSnapshot';
		}					   
	}
	
	$output = json_encode(array('Back' => $BACK_ROUTE));
	
	print_r($output);
	break;
	
		
	############################
	# BEGIN TO RUN RECOVERY SERVICE
	############################
	case "BeginToRunService":
	case "BeginToRunServiceAsync":
	
	$SERVICE_SETTINGS = json_decode($_REQUEST['SERVICE_SETTINGS']);
	
	if (isset($_REQUEST['TRIGGER_SYNC']) and $_REQUEST['TRIGGER_SYNC'] == 'on')
	{
		$SERVICE_SETTINGS -> trigger_sync = true;
	}
	else
	{
		$SERVICE_SETTINGS -> trigger_sync = false;
	}	
	
	$REST_DATA = array('Action'   	 	 => $_REQUEST['ACTION'],
					   'AcctUUID' 	 	 => $_REQUEST['ACCT_UUID'],
					   'RegnUUID' 	 	 => $_REQUEST['REGN_UUID'],
					   'ReplUUID' 	 	 => $_REQUEST['REPL_UUID'],
					   'PlanUUID'		 => $_REQUEST['PLAN_UUID'],
					   'RecyType' 	 	 => $_REQUEST['RECY_TYPE'],
					   'ServiceSettings' => json_encode($SERVICE_SETTINGS)
				);	
	
	if ($_REQUEST['ACTION'] == 'BeginToRunService')
	{
		$output = CurlPostToRemote($REST_DATA,2500);
	}
	else
	{
		$output = CurlPostToRemote($REST_DATA);
	}	
	print_r($output);
	exit;
	/*
	if (isset($_REQUEST['TRIGGER_SYNC']) and $_REQUEST['TRIGGER_SYNC'] == 'on')
	{
	   $TriggerSync = 'on';
	}
	else
	{
	   $TriggerSync = 'off';
	}	

	if (isset($_REQUEST['PUBLIC_ADDR_ID']))
	{
		$PublicAddrId = $_REQUEST['PUBLIC_ADDR_ID'];
	}
	else
	{
		$PublicAddrId = 'DynamicAssign';
	}
	
	if (isset($_REQUEST['PRIVATE_ADDR_ID']))
	{
		$PrivateAddrId = $_REQUEST['PRIVATE_ADDR_ID'];
	}
	else
	{
		$PrivateAddrId = 'DynamicAssign';
	}
	
	$_REQUEST['FLAVOR_ID'] = isset($_REQUEST['FLAVOR_ID'])?$_REQUEST['FLAVOR_ID']:null;
	
	$_REQUEST['SGROUP_UUID'] = isset($_REQUEST['SGROUP_UUID'])?$_REQUEST['SGROUP_UUID']:null;
	
	$REST_DATA = array('Action'   	 	=> $_REQUEST['ACTION'],
					   'AcctUUID' 	 	=> $_REQUEST['ACCT_UUID'],
					   'RegnUUID' 	 	=> $_REQUEST['REGN_UUID'],
					   'ReplUUID' 	 	=> $_REQUEST['REPL_UUID'],					   
					   'RecyType' 	 	=> $_REQUEST['RECY_TYPE'],
					   'FlavorId' 	 	=> $_REQUEST['FLAVOR_ID'],
					   'NetworkUUID' 	=> $_REQUEST['NETWORK_UUID'],
					   'SgroupUUID'  	=> $_REQUEST['SGROUP_UUID'],						   
					   'SnapUUID' 	 	=> $_REQUEST['SNAP_UUID'],
					   'ClusterUUID' 	=> $_REQUEST['CLUSTER_UUID'],
					   'PrivateAddrId'	=> $PrivateAddrId,
					   'PublicAddrId'	=> $PublicAddrId,					   
					   'TriggerSync' 	=> $TriggerSync,
					   'RcvyPreScript'	=> $_REQUEST['PRE_SCRIPT'],
					   'RcvyPostScript'	=> $_REQUEST['POST_SCRIPT'],
					   'PlanUUID'		=> $_REQUEST['PLAN_UUID']
				);
	
	if(isset($_REQUEST['SWITCH_UUID']))
	{
		$REST_DATA["SwitchUUID"] = $_REQUEST['SWITCH_UUID'];
	}
	
	if(isset($_REQUEST['SUBNET_UUID']))
	{
		$REST_DATA["SubnetUUID"] = $_REQUEST['SUBNET_UUID'];
	}
	
	if(isset($_REQUEST['DISK_TYPE']))
	{
		$REST_DATA["DISK_TYPE"] = $_REQUEST['DISK_TYPE'];
	}
	
	if(isset($_REQUEST['AVAILABILITYSET']))
	{
		$REST_DATA["AVAILABILITYSET"] = $_REQUEST['AVAILABILITYSET'];
	}
	
	if(isset($_REQUEST['CPU']))
	{
		$REST_DATA["CPU"] = $_REQUEST['CPU'];
	}
	
	if(isset($_REQUEST['Memory']))
	{
		$REST_DATA["Memory"] = $_REQUEST['Memory'];
	}
	
	if ($_REQUEST['ACTION'] == 'BeginToRunService')
	{
		$output = CurlPostToRemote($REST_DATA,2500);
	}
	else
	{
		$output = CurlPostToRemote($REST_DATA);
	}
	
	print_r($output);
	*/
	break;
	
	case "BeginToRunVMWareServiceAsync":
	
	$SERVICE_SETTINGS = json_decode($_REQUEST['SERVICE_SETTINGS']);
	
	if (isset($_REQUEST['TRIGGER_SYNC']) and $_REQUEST['TRIGGER_SYNC'] == 'on')
	{
		$SERVICE_SETTINGS -> trigger_sync = true;
	}
	else
	{
		$SERVICE_SETTINGS -> trigger_sync = false;
	}	
	
	$REST_DATA = array('Action'   	 	 => $_REQUEST['ACTION'],
					   'AcctUUID' 	 	 => $_REQUEST['ACCT_UUID'],
					   'RegnUUID' 	 	 => $_REQUEST['REGN_UUID'],
					   'ReplUUID' 	 	 => $_REQUEST['REPL_UUID'],
					   'PlanUUID'		 => $_REQUEST['PLAN_UUID'],
					   'RecyType' 	 	 => $_REQUEST['RECY_TYPE'],
					   'ServiceSettings' => json_encode($SERVICE_SETTINGS)
				);	
	
	$output = CurlPostToRemote($REST_DATA);

	print_r($output);
	break;
	############################
	# QUERY SERVICE INFORMATION
	############################
	case "QueryServiceInformation":
	
	$ACTION    = 'QueryServiceInformation';
	$REST_DATA = array('Action'  => $ACTION,
					   'JobUUID' => $_REQUEST['JOB_UUID']
				);
	$output = CurlPostToRemote($REST_DATA);
	
	print_r($output);
	break;
		
	############################
	# DELETE SELECT RECOVER
	############################
	case "DeleteSelectRecover":
	case "DeleteSelectRecoverAsync":	
	if (isset($_REQUEST['SERV_UUID']))
	{
		$REST_DATA = array('Action'   	 	=> $_REQUEST['ACTION'],
						   'ServUUID' 	 	=> $_REQUEST['SERV_UUID'],
						   'InstanceAction'	=> $_REQUEST['INSTANCE_ACTION'],
						   'DeleteSnapshot' => $_REQUEST['DELETE_SNAPSHOT']
						);
		
		if ($_REQUEST['ACTION'] == 'DeleteSelectRecover')
		{
			CurlPostToRemote($REST_DATA,20);
		}
		else
		{
			CurlPostToRemote($REST_DATA);
		}		
		$Response = json_encode(array('msg' => _('Deleting recovery process.')));
	}
	else
	{
		$Response = json_encode(array('msg' => _('Please select a recovery process.')));
	}
	
	print_r($Response);
	break;
	
	
	############################
	# BEGIN TO RUN RECOVER KIT SERVICE
	############################
	case "BeginToRunRecoverKitService":
	case "BeginToRunRecoverKitServiceAsync":
	
	if (isset($_REQUEST['TRIGGER_SYNC']) and $_REQUEST['TRIGGER_SYNC'] == 'on')
	{
	   $TriggerSync = 'on';
	}
	else
	{
	   $TriggerSync = 'off';
	}

	
	if (isset($_REQUEST['AUTO_REBOOT']) and $_REQUEST['AUTO_REBOOT'] == 'on')
	{
	   $AutoReboot = 'on';
	}
	else
	{
	   $AutoReboot = 'off';
	}	
	
	$REST_DATA = array('Action'   	 => $_REQUEST['ACTION'],
					   'AcctUUID' 	 => $_REQUEST['ACCT_UUID'],
					   'RegnUUID' 	 => $_REQUEST['REGN_UUID'],
					   'ReplUUID' 	 => $_REQUEST['REPL_UUID'],
					   'ConnUUID'	 => $_REQUEST['CONN_UUID'],
					   'TriggerSync' => $TriggerSync,
					   'ConverType'	 => $_REQUEST['CONVERT_TYPE'],
					   'AutoReboot'	 => $AutoReboot
				);
	
	if ($_REQUEST['ACTION'] == 'BeginToRunRecoverKitService')
	{
		$output = CurlPostToRemote($REST_DATA,20);
	}
	else
	{
		$output = CurlPostToRemote($REST_DATA);
	}	
	print_r($output);
	break;
	
	
	############################
	# BEGIN TO RUN IMAGE EXPORT SERVICE
	############################
	case "BeginToRunRecoverImageExportService":
	case "BeginToRunRecoverImageExportServiceAsync":
	if (isset($_REQUEST['TRIGGER_SYNC']) and $_REQUEST['TRIGGER_SYNC'] == 'on')
	{
	   $TriggerSync = 'on';
	}
	else
	{
	   $TriggerSync = 'off';
	}	
	
	$REST_DATA = array('Action'   	 => $_REQUEST['ACTION'],
					   'AcctUUID' 	 => $_REQUEST['ACCT_UUID'],
					   'RegnUUID' 	 => $_REQUEST['REGN_UUID'],
					   'ReplUUID' 	 => $_REQUEST['REPL_UUID'],
					   'ConnUUID'	 => $_REQUEST['CONN_UUID'],
					   'ConvertType' => $_REQUEST['CONVERT_TYPE'],
					   'TriggerSync' => $TriggerSync
				);
				
	if ($_REQUEST['ACTION'] == 'BeginToRunRecoverImageExportService')
	{
		$output = CurlPostToRemote($REST_DATA,2500);
	}
	else
	{
		$output = CurlPostToRemote($REST_DATA);
	}	
	print_r($output);
	break;
	
	
	############################
	# SAVE RECOVER PLAN
	############################
	case "ListAvailableRecoverPlan":
		$REST_DATA = array('Action'   	 => $_REQUEST['ACTION'],
						   'AcctUUID'	 => $_REQUEST['ACCT_UUID']
						);
		$output = CurlPostToRemote($REST_DATA);
		$PLAN = json_decode($output);
		if ($PLAN != false)
		{
			for ($i=0; $i<count($PLAN); $i++)
			{
				$SERV_PLAN[] = array(
									'PLAN_UUID' 	=> $PLAN[$i] -> PLAN_UUID,
									'HOST_NAME'		=> $PLAN[$i] -> HOST_NAME,
									'CLOUD_TYPE'	=> $PLAN[$i] -> CLOUD_TYPE,
									'VENDOR_NAME'	=> $PLAN[$i] -> VENDOR_NAME,
									'RECOVER_TYPE'	=> $PLAN[$i] -> RECOVER_TYPE,
									'TIMESTAMP'		=> Misc::time_convert_with_zone($PLAN[$i] -> TIMESTAMP)
								);
			}
			
			print_r(json_encode($SERV_PLAN));
		}
		else
		{
			print_r($output);
		}	
	break;
	
	
	############################
	# SAVE RECOVER PLAN
	############################
	case "SaveRecoverPlan":
		$REST_DATA = array('Action'   	 => $_REQUEST['ACTION'],
						   'AcctUUID'	 => $_REQUEST['ACCT_UUID'],
						   'RegnUUID'	 => $_REQUEST['REGN_UUID'],
						   'RecoverPlan' => $_REQUEST['RECOVER_PLAN']
						);
		$output = CurlPostToRemote($REST_DATA);
		print_r($output);	
	break;
	
	
	############################
	# UPDATE RECOVER PLAN
	############################
	case "UpdateRecoverPlan":
		$REST_DATA = array('Action'   	 => $_REQUEST['ACTION'],
						   'PlanUUID' 	 => $_REQUEST['PLAN_UUID'],
						   'RecoverPlan' => $_REQUEST['RECOVER_PLAN']
						);
		$output = CurlPostToRemote($REST_DATA);
		print_r($output);	
	break;
	
	
	############################
	# DELETE RECOVER PLAN
	############################
	case "DeleteRecoverPlan":
		$REST_DATA = array('Action'   => $_REQUEST['ACTION'],
						   'PlanUUID' => $_REQUEST['PLAN_UUID']
						);
		$output = CurlPostToRemote($REST_DATA);
		print_r($output);	
	break;
	
	
	############################
	# EDIT RECOVER PLAN
	############################
	case "EditRecoverPlan":
		$REST_DATA = array('Action'   => $_REQUEST['ACTION'],
						   'PlanUUID' => $_REQUEST['PLAN_UUID']
						);
		$output = CurlPostToRemote($REST_DATA);
		
		$PLAN_JOBS = json_decode($output);
		$_SESSION['EDIT_PLAN'] = $PLAN_JOBS;
				
		print_r(json_encode(array('Code' => true)));	
	break;
	
	
	############################
	# QUERY RECOVER PLAN
	############################
	case "QueryRecoverPlan":
		$REST_DATA = array('Action'    => $_REQUEST['ACTION'],
						   'AcctUUID'  => $_REQUEST['ACCT_UUID'],
						   'PlanUUID'  => $_REQUEST['PLAN_UUID']
						);
		$output = CurlPostToRemote($REST_DATA);

		$DecodeResponse = json_decode($output);
	
		$SERVICE_PLAN_TABLE = '';		
		$SERVICE_PLAN_TABLE .= '<table id="ServicePlanTable">';
		$SERVICE_PLAN_TABLE .= '<thead><tr>';
		$SERVICE_PLAN_TABLE .= '<th colspan="2"><label for="comment">'._('Recovery Plan').'</label></th>';
		$SERVICE_PLAN_TABLE .= '</tr></thead>';
		
		$SERVICE_PLAN_TABLE .= '<tr>';
		$SERVICE_PLAN_TABLE .= '<td width="180px">'._('Hostname').'</td>';
		$SERVICE_PLAN_TABLE .= '<td width="620px">'.$DecodeResponse -> Hostname.'</div></td>';
		$SERVICE_PLAN_TABLE .= '</tr>';
		
		$SERVICE_PLAN_TABLE .= '<tr>';
		$SERVICE_PLAN_TABLE .= '<td width="180px">'._('Cloud Type').'</td>';
		$SERVICE_PLAN_TABLE .= '<td width="620px">'.$DecodeResponse -> CloudType.'</div></td>';
		$SERVICE_PLAN_TABLE .= '</tr>';
		
		$SERVICE_PLAN_TABLE .= '<tr>';
		$SERVICE_PLAN_TABLE .= '<td width="180px">'._('Recovery Type').'</td>';
		$SERVICE_PLAN_TABLE .= '<td width="620px">'.$DecodeResponse -> RecoveryType.'</div></td>';
		$SERVICE_PLAN_TABLE .= '</tr>';
		
		$SERVICE_PLAN_TABLE .= '<tr>';
		$SERVICE_PLAN_TABLE .= '<td width="180px">'._('Host Type').'</td>';
		$SERVICE_PLAN_TABLE .= '<td width="620px">'.$DecodeResponse -> HostType.'</div></td>';
		$SERVICE_PLAN_TABLE .= '</tr>';
		
		$SERVICE_PLAN_TABLE .= '<tr>';
		$SERVICE_PLAN_TABLE .= '<td width="180px">'._('OS Type').'</td>';
		$SERVICE_PLAN_TABLE .= '<td width="620px">'.$DecodeResponse -> OSType.'</div></td>';
		$SERVICE_PLAN_TABLE .= '</tr>';
		
		$SERVICE_PLAN_TABLE .= '<tr>';
		$SERVICE_PLAN_TABLE .= '<td width="180px">'._('OS Name').'</td>';
		$SERVICE_PLAN_TABLE .= '<td width="620px">'.$DecodeResponse -> OSName.'</div></td>';
		$SERVICE_PLAN_TABLE .= '</tr>';
		
		$SERVICE_PLAN_TABLE .= '<tr>';
		$SERVICE_PLAN_TABLE .= '<td width="180px">'._('Network').'</td>';
		$SERVICE_PLAN_TABLE .= '<td width="620px">'.$DecodeResponse -> Network -> name.'</div></td>';
		$SERVICE_PLAN_TABLE .= '</tr>';
		
		$SERVICE_PLAN_TABLE .= '<tr>';
		$SERVICE_PLAN_TABLE .= '<td width="180px">'._('Security Group').'</td>';
		$SERVICE_PLAN_TABLE .= '<td width="620px">'.$DecodeResponse -> SecurityGroups.'</div></td>';
		$SERVICE_PLAN_TABLE .= '</tr>';
		
		$SERVICE_PLAN_TABLE .= '<tr>';
		$SERVICE_PLAN_TABLE .= '<td width="180px">'._('Instance Configuration').'</td>';
		$SERVICE_PLAN_TABLE .= '<td width="620px">'.$DecodeResponse -> Flavor -> name.' / '.$DecodeResponse -> Flavor -> vcpus.'Cores / '.$DecodeResponse -> Flavor -> ram.'</div></td>';	
		$SERVICE_PLAN_TABLE .= '</tr>';
		
		#REPLICA DISK INFOMATION
		$DISK_INFO = $DecodeResponse -> Disk;

		for ($i=0; $i<count($DISK_INFO); $i++)
		{
			if ($DISK_INFO[$i] -> IS_SKIP == FALSE)
			{
				if ($DISK_INFO[$i] -> IS_BOOT == TRUE)
				{
					$IS_BOOT = '<i class="fa fa-check-circle-o" aria-hidden="true"></i> ';
				}
				else
				{
					$IS_BOOT = '<i class="fa fa-circle-o" aria-hidden="true"></i> ';
				}	
			
				$DISK_NAME = $DISK_INFO[$i] -> DISK_NAME;
				$DISK_SIZE = ($DISK_INFO[$i] -> DISK_SIZE)/1024/1024/1024;
			
				$SERVICE_PLAN_TABLE .= '<tr>';
				$SERVICE_PLAN_TABLE .= '<td width="180px">'._('Disk').' '.$i.'</td>';
				$SERVICE_PLAN_TABLE .= '<td width="620px">'.$IS_BOOT.''.$DISK_NAME.' ('.$DISK_SIZE.'GB)</div></td>';	
				$SERVICE_PLAN_TABLE .= '</tr>';
			}
		}
				
		$SERVICE_PLAN_TABLE .= '<tr>';
		$SERVICE_PLAN_TABLE .= '<td width="180px">'._('Recovery Plan ID').'</td>';
		$SERVICE_PLAN_TABLE .= '<td width="620px">'.$DecodeResponse -> PlanUUID.'</div></td>';
		$SERVICE_PLAN_TABLE .= '</tr>';
		
		$SERVICE_PLAN_TABLE .= '</table>';
		
		print_r($SERVICE_PLAN_TABLE);	
	break;
	
	
	############################
	# BEGIN TO TAKE XRAY
	############################
	case "TakeXray":
		
	$ACTION    = 'TakeXray';
	$REST_DATA = array('Action'   	=> $ACTION,
					   'AcctUUID' 	=> $_REQUEST['ACCT_UUID'],
					   'XrayPATH'	=> $_REQUEST['XRAY_PATH'],
					   'XrayTYPE'	=> $_REQUEST['XRAY_TYPE'],
					   'ServUUID'	=> $_REQUEST['SERV_UUID']
				);
	
	$output = CurlPostToRemote($REST_DATA);
	print_r($output);
	break;


	############################
	# QUERY LICENSE
	############################
	case "QueryLicense":
		
	$ACTION    = 'QueryLicense';
	$REST_DATA = array('Action' => $ACTION);
	
	$output = CurlPostToRemote($REST_DATA);

	#Begin List Licenses Table
	$LICENSE_TABLE  = '<table id="license_body">';
	$LICENSE_TABLE .= '<thead><tr>';
	$LICENSE_TABLE .= '<th width="260px">'._('Activation Key').'</th>';
	$LICENSE_TABLE .= '<th width="135px">'._('Activation Date').'</th>';
	$LICENSE_TABLE .= '<th width="135px">'._('Expiration Date').'</th>';
	$LICENSE_TABLE .= '<th width="84px">'._('Credits').'</th>';
	$LICENSE_TABLE .= '<th width="70px">'._('Used').'</th>';
	$LICENSE_TABLE .= '<th width="120px">'._('Status').'</th>';
	$LICENSE_TABLE .= '<th width="50px" class="TextCenter"><b><i class="fa fa-file-text-o" style="color:#FFFFFF"></i></b></th>';
	$LICENSE_TABLE .= '<th width="50px" class="TextCenter"><b><i class="fa fa-key" style="color:#FFFFFF"></i></b></th>';
	$LICENSE_TABLE .= '<th width="50px" class="TextCenter"><b><i class="fa fa-trash-o" style="color:#FFFFFF"></i></b></th>';
	$LICENSE_TABLE .= '</tr></thead><tbody>';	
	
	if (isset(json_decode($output) -> licenses))
	{
		$QUERY_LICENSE = json_decode($output) -> licenses;
		
		$LICENSE_COUNT = count($QUERY_LICENSE);
		if ($LICENSE_COUNT != 0)
		{
			$FILTER_LICENSE = array();
			#FILTER OUT DELETED LICENSE
			for ($x=0; $x<$LICENSE_COUNT; $x++)
			{
				$FILTER_LICENSE[] = $QUERY_LICENSE[$x];
			}
			
			#BEGIN TO LOOP LICENSE TABLE
			$FILTER_LICENSE_COUNT = count($FILTER_LICENSE);
			if ($FILTER_LICENSE_COUNT != 0)
			{
				for ($i=0; $i<$FILTER_LICENSE_COUNT; $i++)
				{			
					#LICENSE KEY/NAME
					if ($FILTER_LICENSE[$i] -> key == '00000000000000000000')
					{
						$LICENSE_KEY = _('License count exceeded.');
					}
					else
					{
						$LICENSE_KEY = $FILTER_LICENSE[$i] -> key;
					}			
					
					#ACTIVATED DATE CONVERT
					if ($FILTER_LICENSE[$i] -> activated != '')
					{
						$ACTIVATED_INFO = strtotime($FILTER_LICENSE[$i] -> activated);
						$ACTIVATED_DATE = Misc::time_convert_with_zone($ACTIVATED_INFO);
						$ACTIVATED_DATE = date_create($ACTIVATED_DATE);
						$ACTIVATED_DATE = date_format($ACTIVATED_DATE,"Y-m-d");
					}
					else
					{
						$ACTIVATED_DATE = _('NotApplicable');
					}
					
					#EXPIRED DATE CONVERT
					if ($FILTER_LICENSE[$i] -> expired_date != '')
					{
						$EXPIRED_INFO = strtotime($FILTER_LICENSE[$i] -> expired_date);
						$EXPIRED_DATE = Misc::time_convert_with_zone($EXPIRED_INFO);
						$EXPIRED_DATE = date_create($EXPIRED_DATE);
						$EXPIRED_DATE = date_format($EXPIRED_DATE,"Y-m-t");
					}
					else
					{
						$EXPIRED_DATE = _('NotApplicable');
					}
											
					#REMOVE STATUS
					if ($FILTER_LICENSE[$i] -> key == '00000000000000000000')
					{
						$is_status = _('NA');
					}
					else
					{
						if ($FILTER_LICENSE[$i] -> status == 'i')
						{
							$is_status = _('Expired');		
						}
						elseif ($FILTER_LICENSE[$i] -> status == 'n')
						{
							$is_status = _('Not Activated');
						}
						else
						{						
							$is_status = _('Activated');
						}
					}

					#ICON STATUS
					if ($FILTER_LICENSE[$i] -> status == 'n' and $FILTER_LICENSE[$i] -> key != '00000000000000000000')
					{
						$QueryLicense = 'QueryLicense';
						$QueryIcon = '<i class="fa fa-file-text-o" style="color:#008ae6; cursor:pointer;"></i>';
						
						$ActiveLicense = 'ActiveLicense';
						$ActiveIcon = '<i class="fa fa-key" style="color:#008000; cursor:pointer;"></i>';
						
						$RemoveLicense = 'RemoveLicense';
						$RemoveIcon = '<i class="fa fa-trash-o" style="color:#ff4000; cursor:pointer;"></i>';
					}
					else
					{
						$QueryLicense = '';
						$QueryIcon = '<i class="fa fa-file-text-o" style="color:grey"></i>';
						
						$ActiveLicense = '';
						$ActiveIcon = '<i class="fa fa-key" style="color:grey"></i>';
						
						$RemoveLicense = '';
						$RemoveIcon = '<i class="fa fa-trash-o" style="color:grey"></i>';
					}				
			
					$LICENSE_TABLE .= '<tr>';
					$LICENSE_TABLE .= '<td>'.$LICENSE_KEY.'</td>';
					$LICENSE_TABLE .= '<td>'.$ACTIVATED_DATE.'</td>';
					$LICENSE_TABLE .= '<td>'.$EXPIRED_DATE.'</td>';
					$LICENSE_TABLE .= '<td>'.$FILTER_LICENSE[$i] -> count.'</td>';
					$LICENSE_TABLE .= '<td>'.$FILTER_LICENSE[$i] -> consumed.'</td>';
					$LICENSE_TABLE .= '<td>'.$is_status.'</td>';
					$LICENSE_TABLE .= '<td class="TextCenter"><div class="'.$QueryLicense.'" data-license='.$LICENSE_KEY.'>'.$QueryIcon.'</div></td>';
					$LICENSE_TABLE .= '<td class="TextCenter"><div class="'.$ActiveLicense.'" data-license='.$LICENSE_KEY.'>'.$ActiveIcon.'</div></td>';
					$LICENSE_TABLE .= '<td class="TextCenter"><div class="'.$RemoveLicense.'" data-license='.$LICENSE_KEY.'>'.$RemoveIcon.'</div></td>';
					$LICENSE_TABLE .= '</tr>';					
				}
			}
			else
			{
				$LICENSE_TABLE .= '<tr>';
				$LICENSE_TABLE .= '<td colspan="9">'._('Please apply a license.').'</td>';
				$LICENSE_TABLE .= '<td style="display: none;"></td>';
				$LICENSE_TABLE .= '<td style="display: none;"></td>';
				$LICENSE_TABLE .= '<td style="display: none;"></td>';
				$LICENSE_TABLE .= '<td style="display: none;"></td>';
				$LICENSE_TABLE .= '<td style="display: none;"></td>';
				$LICENSE_TABLE .= '<td style="display: none;"></td>';
				$LICENSE_TABLE .= '<td style="display: none;"></td>';
				$LICENSE_TABLE .= '<td style="display: none;"></td>';
				$LICENSE_TABLE .= '</tr>';
			}
		}
		else
		{
			$LICENSE_TABLE .= '<tr>';
			$LICENSE_TABLE .= '<td colspan="9">'._('Please apply a license.').'</td>';
			$LICENSE_TABLE .= '<td style="display: none;"></td>';
			$LICENSE_TABLE .= '<td style="display: none;"></td>';
			$LICENSE_TABLE .= '<td style="display: none;"></td>';
			$LICENSE_TABLE .= '<td style="display: none;"></td>';
			$LICENSE_TABLE .= '<td style="display: none;"></td>';
			$LICENSE_TABLE .= '<td style="display: none;"></td>';
			$LICENSE_TABLE .= '<td style="display: none;"></td>';
			$LICENSE_TABLE .= '<td style="display: none;"></td>';
			$LICENSE_TABLE .= '</tr>';
		}
	}
	else
	{
		$LICENSE_TABLE .= '<tr>';
		$LICENSE_TABLE .= '<td colspan="9">'._('Failed to connect to transport service.').'</td>';
		$LICENSE_TABLE .= '</tr>';
	}
	$LICENSE_TABLE .= '</tbody></table>';
		
	############################
	#	Begin License History
	############################
	$LICENSE_TABLE .= '<table id="license_history">';
	$LICENSE_TABLE .= '<thead><tr>';	
	$LICENSE_TABLE .= '<th width="35px;" class="TextCenter"><b><i class="fa fa-minus-square-o"></i></b></th>';
	$LICENSE_TABLE .= '<th width="550px"><b>'._('Hostname').'<//b></th>';
	$LICENSE_TABLE .= '<th width="110px"><b>'._('Type').'<//b></th>';
	$LICENSE_TABLE .= '<th width="90px"><b>'._('Consumed').'<//b></th>';
	$LICENSE_TABLE .= '<th width="35px;" class="TextCenter"><b><i class="fa fa-history"></i></b></th>';
	$LICENSE_TABLE .= '</tr></thead><tbody>';
	
	
	if (isset(json_decode($output) -> histories))
	{
		$QUERY_LICENSE_HISTORY = json_decode($output) -> histories;
		
		$LICENSE_HISTORY_COUNT = count($QUERY_LICENSE_HISTORY);
		if ($LICENSE_HISTORY_COUNT != 0)
		{
			for ($w=0; $w<$LICENSE_HISTORY_COUNT; $w++)
			{
				$LICENSE_TABLE .= '<tr>';
				$LICENSE_TABLE .= '<td class="TextCenter">'.($w+1).'</td>';
				$LICENSE_TABLE .= '<td>'.$QUERY_LICENSE_HISTORY[$w] -> name.'</td>';
				$LICENSE_TABLE .= '<td>'.$QUERY_LICENSE_HISTORY[$w] -> type.'</td>';
				$LICENSE_TABLE .= '<td>'.count($QUERY_LICENSE_HISTORY[$w] -> histories).'</td>';
				$LICENSE_TABLE .= '<td class="TextCenter"><div class= "QueryLicenseHistory" data-machine_id='.$QUERY_LICENSE_HISTORY[$w] -> machine_id.' style="cursor:pointer;"><i class="fa fa-ellipsis-h"></i></div></td>';
				$LICENSE_TABLE .= '</tr>';
			}
		}
		else
		{
			$LICENSE_TABLE .= '<tr>';
			$LICENSE_TABLE .= '<td colspan="5">'._('No History.').'</td>';
			$LICENSE_TABLE .= '<td style="display: none;"></td>';
			$LICENSE_TABLE .= '<td style="display: none;"></td>';
			$LICENSE_TABLE .= '<td style="display: none;"></td>';
			$LICENSE_TABLE .= '<td style="display: none;"></td>';
			$LICENSE_TABLE .= '</tr>';			
		}
	}
	else
	{
		$LICENSE_TABLE .= '<tr>';
		$LICENSE_TABLE .= '<td colspan="5">'._('No History.').'</td>';
		$LICENSE_TABLE .= '<td style="display: none;"></td>';
		$LICENSE_TABLE .= '<td style="display: none;"></td>';
		$LICENSE_TABLE .= '<td style="display: none;"></td>';
		$LICENSE_TABLE .= '<td style="display: none;"></td>';
		$LICENSE_TABLE .= '</tr>';	
	}
	$LICENSE_TABLE .= '</tbody></table>';
	
	print_r($LICENSE_TABLE);
	break;
	
	############################
	# QUERY PACKAGE INFO
	############################
	case "QueryPackageInfo":
		
	$ACTION    = 'QueryPackageInfo';
	$REST_DATA = array('Action'   		=> $ACTION,
					   'LicenseKey'		=> $_REQUEST['LICENSE_KEY']
				);
	$PACKAGE_INFO = CurlPostToRemote($REST_DATA);
	
	$LICENSE_FILE_NAME = 'SaaSaMeLicense_'.substr($_REQUEST['LICENSE_KEY'],-5).'.license';
		
	$STATUS = TRUE;	
	$PACKAGE_TABLE  = '<script>';			
	$PACKAGE_TABLE .= 'var clipboard = new Clipboard("#CopyPackageInfo");';	
	$PACKAGE_TABLE .= 'clipboard.on("success", function(event) {';
	$PACKAGE_TABLE .= 'event.clearSelection();';
	$PACKAGE_TABLE .= 'event.trigger.textContent = "Copied";';
	$PACKAGE_TABLE .= '$("#CopyPackageInfo").prop("disabled", true);';
	$PACKAGE_TABLE .= '$("#CopyPackageInfo").removeClass("btn-primary").addClass("btn-default");';
	$PACKAGE_TABLE .= '$(".modal-backdrop").remove();';
	$PACKAGE_TABLE .= 'BootstrapDialog.show({title:'._('License Information').',message:'._('License package copied!').', type: BootstrapDialog.TYPE_PRIMARY, draggable: true, buttons:[{label:'._('Close').', action: function(){$.each(BootstrapDialog.dialogs, function(id, dialog){dialog.close();});}}]});';
	$PACKAGE_TABLE .= '});';	
	$PACKAGE_TABLE .= '</script>';
	
	$PACKAGE_TABLE .= '<table style="table-layout: fixed; width: 100%">';
	$PACKAGE_TABLE .= '<tr>';
	$PACKAGE_TABLE .= '<td style="word-wrap: break-word" id="PackageText">'.$PACKAGE_INFO.'</td>';
	$PACKAGE_TABLE .= '</tr>';
	$PACKAGE_TABLE .= '</table>';
	
	$PACKAGE_TABLE .= '<br>';
	$PACKAGE_TABLE .= '<div class="btn-toolbar" style="width:775px">';
	
	#SKIP DOWNLOAD BUTTON FOR MSIE/EDGE
	if (!preg_match('/MSIE|Trident|Edge/',$_SERVER['HTTP_USER_AGENT']))
	{
		$PACKAGE_TABLE .= '<a href="data:text/plain;charset=UTF-8,'.$PACKAGE_INFO.'" class="btn btn-primary pull-right btn-lg" download="'.$LICENSE_FILE_NAME.'">Download</a>';
	}
	
	$PACKAGE_TABLE .= '<button id="CopyPackageInfo" class="btn btn-primary pull-right btn-lg" data-clipboard-action="copy" data-clipboard-target="#PackageText">Copy</button>';
	$PACKAGE_TABLE .= '</div>';
		
	print_r(json_encode(array('string' => $PACKAGE_INFO, 'filename' => $LICENSE_FILE_NAME,'msg' => $PACKAGE_TABLE, 'status' => $STATUS)));
	break;	
	
	############################
	# REMOVE LICENSE
	############################
	case "RemoveLicense":
		
	$ACTION    = 'RemoveLicense';
	$REST_DATA = array('Action'   	=> $ACTION,
					   'LicenseKey'	=> $_REQUEST['LICENSE_KEY']
				);
	
	$output = CurlPostToRemote($REST_DATA);
	print_r($output);
	break;	
	
	############################
	# QUERY LICENSE HISTORY
	############################
	case "QueryLicenseHistory":
		$ACTION    		= 'QueryLicense';
		$REST_DATA 		= array('Action' => $ACTION);
		$MACHINE_UUID 	= $_REQUEST['MACHINE_UUID'];
	
		$output = CurlPostToRemote($REST_DATA);
		
		#GET FULL LICENSE HISTORY
		$QUERY_LICENSE_HISTORY = json_decode($output) -> histories;
		
		#GET MATCH MACHINE UUID HISTORY
		for ($i=0; $i<count($QUERY_LICENSE_HISTORY); $i++)
		{
			if ($QUERY_LICENSE_HISTORY[$i] -> machine_id == $MACHINE_UUID)
			{
				$LICENSE_HISTORY = $QUERY_LICENSE_HISTORY[$i] -> histories;
				break;			
			}			
		}
		
		#RECAL THE YEAR AND MONTH
		for ($x=0; $x<count($LICENSE_HISTORY); $x++)
		{
			$DATE_INFO[] = (int)(($LICENSE_HISTORY[$x] - 1)/12).'-'.(($LICENSE_HISTORY[$x] - 1) % 12 + 1);
		}
		
		$LICENSE_HISTORY_INFO_TABLE = '';
		$LICENSE_HISTORY_INFO_TABLE .= '<table>';
		$LICENSE_HISTORY_INFO_TABLE .= '<thead><tr>';
		$LICENSE_HISTORY_INFO_TABLE .= '<th width="380px" colspan="2">'._('Consume Date Information').'</th>';
		$LICENSE_HISTORY_INFO_TABLE .= '</tr></thead><tbody>';
		for ($k=0; $k<count($DATE_INFO); $k++)
		{
			$LICENSE_HISTORY_INFO_TABLE .= '<tr>';
			$LICENSE_HISTORY_INFO_TABLE .= '<td>'.$DATE_INFO[$k].'</td>';
			$LICENSE_HISTORY_INFO_TABLE .= '</tr>';
		}
		$LICENSE_HISTORY_INFO_TABLE .= '</tbody></table>';
		print_r($LICENSE_HISTORY_INFO_TABLE);
	break;
	
	############################
	# ONLINE LICENSE ACTIVATION
	############################
	case "OnlineLicenseActivation":
		
	$ACTION    = 'OnlineLicenseActivation';
	$REST_DATA = array('Action'   	 	=> $ACTION,
					   'LicenseName'	=> $_REQUEST['LICENSE_NAME'],
					   'LicenseEmail'	=> $_REQUEST['LICENSE_EMAIL'],
					   'LicenseKey'		=> $_REQUEST['LICENSE_KEY']
				);
	
	$STATUS_CODE = 0;
	if ($_REQUEST['LICENSE_NAME'] == '' OR $_REQUEST['LICENSE_EMAIL'] == '' OR $_REQUEST['LICENSE_KEY'] == '')
	{		
		$PACKAGE_TABLE = _('Please fill in all fields');
		$STATUS = FALSE;
	}
	else
	{
		if (!filter_var($_REQUEST['LICENSE_EMAIL'], FILTER_VALIDATE_EMAIL))
		{
			$STATUS = FALSE;
			$PACKAGE_TABLE = _("Invalid email format.");
		}
		else
		{		
			if (strlen($_REQUEST['LICENSE_KEY']) == 25)
			{
				$LICENSE_STATUS = json_decode(CurlPostToRemote($REST_DATA));
				
				$STATUS = $LICENSE_STATUS -> status;
				
				if ($LICENSE_STATUS -> status == TRUE)
				{
					$PACKAGE_TABLE = _('Transport is activated.');
				}
				else
				{
					if ($LICENSE_STATUS -> why == '')
					{
						$STATUS_CODE = 1;
						$PACKAGE_TABLE = _('Product activation failed.');
					}
					else
					{
						$PACKAGE_TABLE = $LICENSE_STATUS -> why;
					}
				}
			}
			else
			{
				$STATUS = FALSE;
				$PACKAGE_TABLE  = _('Invalid license key format.');	
			}
		}
	}
	
	print_r(json_encode(array('msg' => _($PACKAGE_TABLE), 'status' => $STATUS, 'status_code' => $STATUS_CODE)));
	break;	
	
	############################
	# OFFLINE LICENSE ACTIVATION
	############################
	case "OfflineLicenseActivation":
		
	$ACTION    = 'OfflineLicenseActivation';
	$REST_DATA = array('Action'   	 => $ACTION,
					   'LicenseKey'  => $_REQUEST['LICENSE_KEY'],	
					   'LicenseText' => $_REQUEST['LICENSE_TEXT']
				);
	
	$output = CurlPostToRemote($REST_DATA);
	print_r($output);
	break;
	
	############################
	# TEST SMTP
	############################
	case "TestSMTP":
	
	$ACTION = 'TestSMTP';
	$REST_DATA = array('Action'   => $ACTION,
					   'Language' => $_REQUEST['LANGUAGE'],
					   'SMTPHost' => $_REQUEST['SMTP_HOST'],	
					   'SMTPType' => $_REQUEST['SMTP_TYPE'],
					   'SMTPPort' => $_REQUEST['SMTP_PORT'],
					   'SMTPUser' => $_REQUEST['SMTP_USER'],
					   'SMTPPass' => $_REQUEST['SMTP_PASS'],
					   'SMTPFrom' => $_REQUEST['SMTP_FROM'],
					   'SMTPTo'   => $_REQUEST['SMTP_TO']
				);
	$output = CurlPostToRemote($REST_DATA);
	
	$SMTPMsg = array('status' => json_decode($output) -> status,'reason' => _(json_decode($output) -> reason));
	
	print_r(json_encode($SMTPMsg));
	break;
	
	############################
	# MGMT RECOVERY SCRIPT
	############################
	case "MgmtRecoveryScript":
	case "ReloadMgmtRecoveryScript":
	$ACTION = 'MgmtRecoveryScript';
	$REST_DATA = array('Action' => $ACTION);
	
	$output = CurlPostToRemote($REST_DATA);
	$RecoveryScriptFiles = json_decode($output, FALSE);
	
	$RECOVERY_SCRIPT_TABLE = '';
	$RECOVERY_SCRIPT_TABLE .= '<div id="RecoveryScriptDiv"></div>'; #DIV FOR REQUERY RECOVERY SCRIPT
	$RECOVERY_SCRIPT_TABLE .= '<table id="RecoveryScriptTable">';
	$RECOVERY_SCRIPT_TABLE .= '<thead><tr>';
	$RECOVERY_SCRIPT_TABLE .= '<th width="65px" class="TextCenter"><input type="checkbox" id="checkAll" /></th>';
	$RECOVERY_SCRIPT_TABLE .= '<th width="720px">'._('Name').'</th>';
	$RECOVERY_SCRIPT_TABLE .= '<th width="165px">'._('Date Modified').'</th>';
	$RECOVERY_SCRIPT_TABLE .= '<th width="80px">'._('Size').'</th>';
	$RECOVERY_SCRIPT_TABLE .= '</tr></thead><tbody>';
	
	if (count((array)$RecoveryScriptFiles) != 0)
	{
		foreach ($RecoveryScriptFiles as $FileName => $FileMeta)
		{
			$FileSize = explode('@',$FileMeta)[0];
			$FileTime = explode('@',$FileMeta)[1];
			
			if (round(($FileSize/1024/1024),2) == 0)
			{
				$FileSize = $FileSize.' bytes';
			}
			else
			{
				$FileSize = sprintf("%01.2f",round(($FileSize/1024/1024),2)).' MB';
			}
			
			$RECOVERY_SCRIPT_TABLE .= '<tr>';
			$RECOVERY_SCRIPT_TABLE .= '<td width="65px" class="TextCenter"><input type="checkbox" name="select_file" value="'.$FileName.'"></td>';
			$RECOVERY_SCRIPT_TABLE .= '<td width="720px"><a href="https://'.$_SERVER['HTTP_HOST'].'/restful/ServiceManagement?DownloadFile='.$FileName.'" style="color:#4682B4; text-decoration:none;">'.$FileName.'</a></td>';
			$RECOVERY_SCRIPT_TABLE .= '<td width="165px">'.Misc::time_convert_with_zone($FileTime).'</td>';
			$RECOVERY_SCRIPT_TABLE .= '<td width="80px">'.$FileSize.'</td>';
			$RECOVERY_SCRIPT_TABLE .= '</tr>';
		}
	}
	else
	{
		$RECOVERY_SCRIPT_TABLE .= '<tr>';
		$RECOVERY_SCRIPT_TABLE .= '<td width="65px" class="TextCenter"><input type="checkbox" name="select_file" value="" disabled ></td>';
		$RECOVERY_SCRIPT_TABLE .= '<td width="970px" colspan="3"><div>'._('No Files').'</div></td>';	
		$RECOVERY_SCRIPT_TABLE .= '</tr>';
		$RECOVERY_SCRIPT_TABLE .= '<script>$("#FileDeleteBtn").removeClass("btn-danger").addClass("btn-default");$("#FileDeleteBtn").prop("disabled", true);</script>';	# DISABLE DELETE BUTTON	
	}
	
	$RECOVERY_SCRIPT_TABLE .= '</tbody></table>';	
	$RECOVERY_SCRIPT_TABLE .= '<input id="UploadFile" type="file" style="display:none;"/>';
	
	$RECOVERY_SCRIPT_TABLE .= '<script>';
	if($ACTION == 'MgmtRecoveryScript' AND count((array)$RecoveryScriptFiles) > 10) #ENABLE DATATABLE
	{
		$RECOVERY_SCRIPT_TABLE .= '$(document).ready(function(){';
		$RECOVERY_SCRIPT_TABLE .= '$("#RecoveryScriptTable").DataTable({paging:true,ordering:false,searching:false,bLengthChange:false,pageLength:10,pagingType:"simple_numbers"});';		
		$RECOVERY_SCRIPT_TABLE .= '});';
	}
	$RECOVERY_SCRIPT_TABLE .= '$(document).ready(function(){';
	$RECOVERY_SCRIPT_TABLE .= '$("#FileUploadBtn").removeClass("btn-warning").addClass("btn-default");$("#FileUploadBtn").prop("disabled", true);'; #DISABLE UPLOAD BUTTON
	$RECOVERY_SCRIPT_TABLE .= '$("#BrowseFileBtn").click(function(){$("#UploadFile").click();});});$("#UploadFile").change(function(){$("#BrowseFileBtn").text($("#UploadFile")[0].files[0].name).button("refresh");$("#FileUploadBtn").removeClass("btn-default").addClass("btn-warning");$("#FileUploadBtn").prop("disabled", false);});'; #DISPLAY SELECT FILE NAME AND ENABLE UPLOAD BUTTON
	$RECOVERY_SCRIPT_TABLE .= '$("#checkAll").click(function() {$("#RecoveryScriptTable input:checkbox:enabled").prop("checked", this.checked);});'; #CHECK ALL CHECKBOX
	$RECOVERY_SCRIPT_TABLE .= '</script>';
	
	print_r($RECOVERY_SCRIPT_TABLE);
	break;
	
	############################
	# LIST RECOVERY SCRIPT
	############################
	case "ListRecoveryScript":
	
	$ACTION = 'ListRecoveryScript';
	$REST_DATA = array(
					'Action' => $ACTION,
					'FilterType' => $_REQUEST['FILTER_TYPE']
				);
	
	$output = CurlPostToRemote($REST_DATA);
	print_r($output);
	break;	
	
	############################
	# FILE UPLOAD
	############################
	case "FileUpload":		
	
	$ACTION = 'FileUpload';
	$REST_DATA = array(
					'Action'	=> $ACTION,
					'FileInfo'	=> $_FILES['UploadFile']
				);		
	$output = CurlPostToRemote($REST_DATA);
	print_r($output);
	break;
	
	############################
	# DELETE FILES
	############################
	case "DeleteFiles":		
	
	$ACTION = 'DeleteFiles';
	if (isset($_REQUEST['SELECT_FILES']))
	{
		$REST_DATA = array(
					'Action'	  => $ACTION,
					'SelectFiles' => $_REQUEST['SELECT_FILES']
					);		
		CurlPostToRemote($REST_DATA);		
	}
	//print_r(json_encode(true));
	break;
	
	############################
	# LIST RESTORE BACKUP
	############################
	case "ListRestoreBackup":
	
	$ACTION = 'ListRestoreBackup';
	$REST_DATA = array('Action' => $ACTION);
	
	$output = CurlPostToRemote($REST_DATA);
	$ListSqlFiles = json_decode($output, FALSE);

	$RESTORE_SQL_TABLE = '';
	$RESTORE_SQL_TABLE .= '<table id="SqlFileTable">';
	$RESTORE_SQL_TABLE .= '<thead><tr>';
	$RESTORE_SQL_TABLE .= '<th width="65px" class="TextCenter">-</th>';
	$RESTORE_SQL_TABLE .= '<th width="720px">'._('Name').'</th>';
	$RESTORE_SQL_TABLE .= '<th width="165px">'._('Date Modified').'</th>';
	$RESTORE_SQL_TABLE .= '<th width="80px">'._('Size').'</th>';
	$RESTORE_SQL_TABLE .= '</tr></thead><tbody>';
	
	if (count((array)$ListSqlFiles) != 0)
	{
		foreach ($ListSqlFiles as $FileName => $FileMeta)
		{
			$FileSize = explode('@',$FileMeta)[0];
			$FileTime = explode('@',$FileMeta)[1];
			
			if (round(($FileSize/1024/1024),2) == 0)
			{
				$FileSize = $FileSize.' bytes';
			}
			else
			{
				$FileSize = sprintf("%01.2f",round(($FileSize/1024/1024),2)).' MB';
			}
			
			$RESTORE_SQL_TABLE .= '<tr>';
			$RESTORE_SQL_TABLE .= '<td width="65px" class="TextCenter"><input type="radio" name="select_sql_file" value="'.$FileName.'"></td>';
			$RESTORE_SQL_TABLE .= '<td width="720px">'.$FileName.'</td>';
			$RESTORE_SQL_TABLE .= '<td width="165px">'.Misc::time_convert_with_zone($FileTime).'</td>';
			$RESTORE_SQL_TABLE .= '<td width="80px">'.$FileSize.'</td>';
			$RESTORE_SQL_TABLE .= '</tr>';
		}	
	}
	else
	{
		$RESTORE_SQL_TABLE .= '<tr>';
		$RESTORE_SQL_TABLE .= '<td width="65px" class="TextCenter"><input type="radio" name="select_sql_file" value="" disabled ></td>';
		$RESTORE_SQL_TABLE .= '<td width="970px" colspan="3"><div>'._('No Files').'</div></td>';	
		$RESTORE_SQL_TABLE .= '</tr>';
	}
	$RESTORE_SQL_TABLE .= '</tbody></table>';

	print_r($RESTORE_SQL_TABLE);
	break;
	
	############################
	# RESTORE DATABASE
	############################
	case "RestoreDatabase":
		
	$REST_DATA = array(
				'Action'		=> 'RestoreDatabase',
				'RestoreFile'	=> $_REQUEST['RESTORE_FILE'],
				'SecurityCode'	=> $_REQUEST['SECURITY_CODE']
			);
	$output = CurlPostToRemote($REST_DATA);
	print_r($output);
	break;
	
	############################
	# DEBUG REPAIR REPLICA JOB
	############################
	case "QueryRepairTransport":
	
	$REST_DATA = array(
					'Action'		=> 'QueryRepairTransport',
					'ReplUUID'		=> $_REQUEST['REPL_UUID']
				);
	$output = CurlPostToRemote($REST_DATA);
	
	$Response = json_decode($output,false);
	
	$SourceTransport = $Response -> SourceTransport;
	$ListTransport = $Response -> ListTransport;

	$REPAIR_REPLICA_TABLE = '';
	$REPAIR_REPLICA_TABLE .= '<table id="RepairReplica">';
	$REPAIR_REPLICA_TABLE .= '<thead><tr>';
	$REPAIR_REPLICA_TABLE .= '<th width="65px" class="TextCenter">-</th>';
	$REPAIR_REPLICA_TABLE .= '<th width="430px">'._('Name').'</th>';
	$REPAIR_REPLICA_TABLE .= '<th width="255px">'._('Address').'</th>';
	$REPAIR_REPLICA_TABLE .= '<th width="145px">'._('Type').'</th>';
	$REPAIR_REPLICA_TABLE .= '<th width="135px">'._('Version').'</th>';
	$REPAIR_REPLICA_TABLE .= '</tr></thead><tbody>';
	
	if (count((array)$ListTransport) != 0)
	{
		foreach ($ListTransport as $TransportKey => $TransportValue)
		{
			$SERVER_UUID = $TransportValue -> SERV_UUID;
			$SERVER_NAME = $TransportValue -> HOST_NAME;
			$SERVER_ADDR = implode(',',$TransportValue -> SERV_ADDR);
			$VERDOR_NAME = $TransportValue -> VENDOR_NAME == 'UnknownVendorType' ? 'On-Premises':$TransportValue -> VENDOR_NAME;
			$VERSION_NUM = $TransportValue -> SERV_VERN;
			
			if ($SERVER_UUID == $SourceTransport)
			{
				$SELECTED_TRANSPORT = ' checked';
			}
			else
			{
				$SELECTED_TRANSPORT = ' disabled';
			}			
			
			$REPAIR_REPLICA_TABLE .= '<tr>';
			$REPAIR_REPLICA_TABLE .= '<td width="65px" class="TextCenter"><input type="radio" name="select_transport" value="'.$SERVER_UUID.'"'.$SELECTED_TRANSPORT.'></td>';
			$REPAIR_REPLICA_TABLE .= '<td width="430px">'.$SERVER_NAME.'</td>';
			$REPAIR_REPLICA_TABLE .= '<td width="255px">'.$SERVER_ADDR.'</td>';
			$REPAIR_REPLICA_TABLE .= '<td width="145px">'.$VERDOR_NAME.'</td>';
			$REPAIR_REPLICA_TABLE .= '<td width="135px">'.$VERSION_NUM.'</td>';
			$REPAIR_REPLICA_TABLE .= '</tr>';
		}
	}
	else
	{
		$REPAIR_REPLICA_TABLE .= '<tr>';
		$REPAIR_REPLICA_TABLE .= '<td width="65px" class="TextCenter"><input type="radio" name="select_transport" value="" disabled ></td>';
		$REPAIR_REPLICA_TABLE .= '<td width="970px" colspan="5"><div>'._('No Files').'</div></td>';	
		$REPAIR_REPLICA_TABLE .= '</tr>';
	}
	$REPAIR_REPLICA_TABLE .= '</tbody></table>';

	$REPAIR_REPLICA_TABLE .= '<script>$(".selectpicker").selectpicker("refresh");</script>';

	print_r($REPAIR_REPLICA_TABLE);	
	break;
	
	############################
	# DEBUG REPAIR REPLICA JOB
	############################
	case "DebugRepairReplicaAsync":
	
	$REST_DATA = array(
					'Action'			=> 'DebugRepairReplicaAsync',
					'ReplUUID' 			=> $_REQUEST['REPL_UUID'],
					'TargetTransport' 	=> $_REQUEST['TARGET_TRANSPORT'],
					'RepairSyncMode'	=> $_REQUEST['REPAIR_SYNC_MODE'],
					'SecurityCode'		=> $_REQUEST['SECURITY_CODE']
				);		
	CurlPostToRemote($REST_DATA);
	print_r(json_encode(true));
	break;
	
	############################
	# DEBUG DELETE REPLICA JOB
	############################
	case "DebugDeleteReplica":
	
	$REST_DATA = array(
					'Action'		=> 'DebugDeleteReplica',
					'ReplUUID' 		=> $_REQUEST['REPL_UUID'],
					'SecurityCode'	=> $_REQUEST['SECURITY_CODE']
				);		
	CurlPostToRemote($REST_DATA);
	print_r(json_encode(true));
	break;
	
	############################
	# DEBUG DELETE SERVICE JOB
	############################
	case "DebugDeleteService":
	
	$REST_DATA = array(
					'Action'		=> 'DebugDeleteService',
					'ServUUID' 		=> $_REQUEST['SERV_UUID'],
					'SecurityCode'	=> $_REQUEST['SECURITY_CODE']
				);		
	CurlPostToRemote($REST_DATA);		
	break;
	
	############################
	# CONFIGURE DATAMODE AGENT
	############################
	case "ConfigureDataModeAgent":
	
	$ACTION = 'ConfigureDataModeAgent';
	$REST_DATA = array(
					'Action'	   => $ACTION,
					'ReplUUID'	   => $_REQUEST['REPL_UUID'],
					'DiskFilter'   => $_REQUEST['DISK_FILTER'],
					'RecoveryMode' => $_REQUEST['RECOVERY_MODE']
				);		
	$MAPPING_JSON = CurlPostToRemote($REST_DATA);

	$MAPPING_JSON = json_decode($MAPPING_JSON) -> partitions;
	
	#$MAPPING_TABLE = '<style>#MappingTable{padding-top:6px;!important; padding-left:11px;!important;}</style>';
	
	$MAPPING_TABLE  = '<table id="MappingTable" style="width:620px;">';
	$MAPPING_TABLE .= '<thead><tr>';
	$MAPPING_TABLE .= '<th width="70px">'._('Disk').'</th>';
	$MAPPING_TABLE .= '<th width="70px">'._('Partition').'</th>';
	$MAPPING_TABLE .= '<th width="120px">'._('Original Cfg.').'</th>';
	$MAPPING_TABLE .= '<th width="115px">'._('Partition Type').'</th>';
	$MAPPING_TABLE .= '<th width="120x">'._('Disk Signature').'</th>';
	$MAPPING_TABLE .= '<th width="110px">'._('Assignment').'</th>';
	$MAPPING_TABLE .= '</tr></thead><tbody>';

	for ($i=0; $i<count($MAPPING_JSON); $i++)
	{						
		$MAPPING_TABLE .= '<tr>';
		$MAPPING_TABLE .= '<td width="70px">'.$MAPPING_JSON[$i] -> volume.'</td>';
		$MAPPING_TABLE .= '<td width="70px">'.$i.'</td>';
		$MAPPING_TABLE .= '<td width="120px">'.$MAPPING_JSON[$i] -> drive_letter.'</td>';
		$MAPPING_TABLE .= '<td width="115px">'.$MAPPING_JSON[$i] -> partition_style_name.'</td>';
		$MAPPING_TABLE .= '<td width="120px">'.$MAPPING_JSON[$i] -> signature.'</td>';
		$MAPPING_TABLE .= '<td><input id="set_letter'.$i.'" class="form-control set_letter" style="height:30px;" value=""></td>';
		$MAPPING_TABLE .= '</tr>';
		
		$MAPPING_TABLE .= '<div id="guid'.$i.'" style="display: none;">'.$MAPPING_JSON[$i] -> guid.'</div>';
		$MAPPING_TABLE .= '<div id="is_system_disk'.$i.'" style="display: none;">'.$MAPPING_JSON[$i] -> is_system_disk.'</div>';
		$MAPPING_TABLE .= '<div id="partition_offset'.$i.'" style="display: none;">'.$MAPPING_JSON[$i] -> partition_offset.'</div>';
		$MAPPING_TABLE .= '<div id="partition_size'.$i.'" style="display: none;">'.$MAPPING_JSON[$i] -> partition_size.'</div>';
		$MAPPING_TABLE .= '<div id="partition_number'.$i.'" style="display: none;">'.$MAPPING_JSON[$i] -> partition_number.'</div>';
		$MAPPING_TABLE .= '<div id="partition_style'.$i.'" style="display: none;">'.$MAPPING_JSON[$i] -> partition_style.'</div>';
		$MAPPING_TABLE .= '<div id="signature'.$i.'" style="display: none;">'.$MAPPING_JSON[$i] -> signature.'</div>';
	}
	
	$MAPPING_TABLE .= '</tbody></table>';
	
	$MAPPING_TABLE .= '<script>';
	$MAPPING_TABLE .= '$("#AgentCfgBtn").prop("disabled",!0);';
	$MAPPING_TABLE .= '$(".set_letter").on("input",function(t){0==$(".set_letter").filter(function(){return $("#AgentCfgBtn").removeClass("btn-default").addClass("btn-primary"),$("#AgentCfgBtn").prop("disabled",!1),""!=this.value}).length&&($("#AgentCfgBtn").removeClass("btn-primary").addClass("btn-default"),$("#AgentCfgBtn").prop("disabled",!0))});';
	$MAPPING_TABLE .= '</script>';
	
	print_r($MAPPING_TABLE);

	break;
	
	############################
	# GENERATE DATAMODE AGENT PACKAGE
	############################
	case "GenerateDataModeAgentPackage":
		$ACTION = 'GenerateDataModeAgentPackage';
		
		$REST_DATA = array(
					'Action'		=> $ACTION,
					'AgentPath'		=> $_REQUEST['AGENT_PATH'],
					'PartitionRef'  => $_REQUEST['PARTITION_REF'],
					'OSType'		=> $_REQUEST['OS_TYPE']
				);
				
		$FILE_NAME = CurlPostToRemote($REST_DATA);		
		print_r($FILE_NAME);
	break;

	############################
	# GET REPLICA SERVICE NUM
	############################
	case "getReplicaServiceNum":
		$ACTION = 'getReplicaServiceNum';
		
		$REST_DATA = array(
					'Action'	=> $ACTION,
					'AcctUUID'	=> $_REQUEST['ACCT_UUID'],
					'serverId'  => $_REQUEST['serverId']
				);
				
		$FILE_NAME = CurlPostToRemote($REST_DATA);		
		print_r($FILE_NAME);
	break;

	############################
	# GET CASCADED REPLICA INFO
	############################
	case "GetCascadedReplicaInfo":
		$ACTION = 'GetCascadedReplicaInfo';
		
		$REST_DATA = array(
					'Action'		=> $ACTION,
					'PackerUUID'	=> $_REQUEST['PACKER_UUID']
				);
				
		$Response = CurlPostToRemote($REST_DATA);		
		print_r($Response);
	break;
	
	############################
	# GET TRANSPORT INFO BY MACHINEID
	############################
	case "GetTransportInfoByMachineId":
		$REST_DATA = array(
					'Action'	=> $_REQUEST['ACTION'],
					'MachineId'	=> $_REQUEST['MACHINE_ID']
				);
				
		$Response = CurlPostToRemote($REST_DATA);

		print_r($Response);
	break;
	
	############################
	# DISPLAY NETWORK TOPOLOGY
	############################
	case "NetworkTopology":
		$REST_DATA = array(
					'Action'	=> $_REQUEST['ACTION'],
					'JobUUID' 	=> $_REQUEST['JOB_UUID'],
					'Type'		=> $_REQUEST['TYPE']
				);
				
		$Response = CurlPostToRemote($REST_DATA);	
		
		$NetworkTopology = json_decode($Response);
				
		$DISPLAY_NETWORK_TOPOLOGY  = '<div id="MyNetworkTopology"></div>';
		$DISPLAY_NETWORK_TOPOLOGY .= '<script>';
		$DISPLAY_NETWORK_TOPOLOGY .= 'var nodes = new vis.DataSet('.$NetworkTopology -> node.');';
		$DISPLAY_NETWORK_TOPOLOGY .= 'var edges = new vis.DataSet('.$NetworkTopology -> edge.');';
		$DISPLAY_NETWORK_TOPOLOGY .= 'var container = document.getElementById("MyNetworkTopology");';
		$DISPLAY_NETWORK_TOPOLOGY .= 'var data = {nodes: nodes, edges: edges};';
		$DISPLAY_NETWORK_TOPOLOGY .= 'var options={';
		$DISPLAY_NETWORK_TOPOLOGY .= 'width:"820px", height:"400px",';
		$DISPLAY_NETWORK_TOPOLOGY .= 'physics:false,';
		$DISPLAY_NETWORK_TOPOLOGY .= 'nodes:{shadow:true},';		
		//$DISPLAY_NETWORK_TOPOLOGY .= 'layout:{physics:false,improvedLayout:false,hierarchical:{enabled:true,levelSeparation:250,nodeSpacing:200,treeSpacing:180,blockShifting:true,edgeMinimization:false,parentCentralization:true,direction:"LR",sortMethod:"directed"}},';
		$DISPLAY_NETWORK_TOPOLOGY .= 'edges:{arrowStrikethrough:false,smooth:{type:"straightCross",forceDirection:"none",roundness:0}}';
		$DISPLAY_NETWORK_TOPOLOGY .='};';
		$DISPLAY_NETWORK_TOPOLOGY .= 'var network = new vis.Network(container, data, options);';
		$DISPLAY_NETWORK_TOPOLOGY .= 'setTimeout(function() {network.fit()},0)';
		$DISPLAY_NETWORK_TOPOLOGY .= '</script>';

		print_r($DISPLAY_NETWORK_TOPOLOGY);		
	break;
}
