<?php
set_time_limit(0);
session_start();
require_once '../_inc/_class_main.php';
New Misc();

define('ROOT_PATH', __DIR__);
require_once(ROOT_PATH . "\..\_inc\languages\setlang.php");

function CurlPostToRemote($REST_DATA,$TIME_OUT = 0)
{
	$URL = 'http://127.0.0.1:8080/restful/VMWareWebServices';
	
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

	$output = json_decode($output);
	if (isset($output -> Msg))
	{
		$output -> Msg = _($output -> Msg);
	}	
	return json_encode($output);
	#return json_decode($output,true);
}

function ProcMessage( $outputString ){
	
	$s = json_decode( $outputString, true );

	if( !isset($s["format"]) || $s["format"] == "" ){
		
		$r = array(
			"success" => $s["success"],
			"why" => isset($s["why"])?$s["why"]:""
		);
	
		return json_encode( $r );
	}
	
	array_unshift( $s["arguments"], $s["format"] );

	$why = call_user_func_array( 'translate', $s["arguments"] );

	$r = array(
		"success" => $s["success"],
		"why" => $why
	);
	
	return json_encode( $r );
	
}

$SelectAction = $_REQUEST["ACTION"];
switch ($SelectAction)
{
	############################
	# Initialize New Connection
	############################
	case "InitializeNewConnection":
				 
		$REST_DATA = array(
						'Action'				=> 'InitializeNewConnection',
						'AcctUUID'  			=> $_REQUEST['ACCT_UUID'],
						'RegnUUID'  			=> $_REQUEST['REGN_UUID'],
						'INPUT_HOST_IP' 		=> $_REQUEST['INPUT_HOST_IP'],
						'INPUT_HOST_USER' 		=> $_REQUEST['INPUT_HOST_USER'],					
						'INPUT_HOST_PASS'		=> $_REQUEST['INPUT_HOST_PASS'],
						'TRANSPORT_UUID' 		=> $_REQUEST['TRANSPORT_UUID']
					);
					
		$output = CurlPostToRemote($REST_DATA);
	
		print_r($output);
	break;

	case "UpdateConnection":
				 
		$REST_DATA = array(
						'Action'				=> 'UpdateConnection',
						'CLUSTER_UUID'  		=> $_REQUEST['CLUSTER_UUID'],
						'INPUT_HOST_IP' 		=> $_REQUEST['INPUT_HOST_IP'],
						'INPUT_HOST_USER' 		=> $_REQUEST['INPUT_HOST_USER'],					
						'INPUT_HOST_PASS'		=> $_REQUEST['INPUT_HOST_PASS'],
						'TRANSPORT_UUID' 		=> $_REQUEST['TRANSPORT_UUID']
					);
					
		$output = CurlPostToRemote($REST_DATA);
	
		print_r($output);
	break;
	
	case "TestEsxConnection":
				 
		$REST_DATA = array(
						'Action'			=> 'TestEsxConnection',
						'ESX_ADDR' 			=> $_REQUEST['ESX_ADDR'],
						'ESX_USER' 			=> $_REQUEST['ESX_USER'],					
						'ESX_PASS'			=> $_REQUEST['ESX_PASS'],
						'SERVER_ID'			=> $_REQUEST['SERVER_ID'],
						'TRANSPORT_IP'		=> $_REQUEST['TRANSPORT_IP']
					);
	
		$output = CurlPostToRemote($REST_DATA);
		
		$msg = json_decode( ProcMessage( $output ), true );
		
		if( strpos($msg["why"],"Invalid method name: 'get_virtual_hosts_p" ) !== false )
			$msg["why"] = _("Unsupported transport server version, requires build 900 and later.");
				
		print_r( json_encode( $msg ) );
	break;
	
	case "ListSnapshot":
	
		$REST_DATA = array(
						'Action'			=> 'ListSnapshot',
						'CLUSTER_UUID'		=> $_REQUEST['CLUSTER_UUID'],
						'ReplicatId'		=> $_REQUEST['ReplicatId']
					);
	
		$output = CurlPostToRemote($REST_DATA);
		
		$snapshots = json_decode($output);
		
		for ($i=0; $i<count($snapshots); $i++)
		{
			$description = explode("@",Misc::convert_snapshot_time_with_zone($snapshots[$i] -> name, $snapshots[$i] -> time));
			$msg[] = array('name' => $description[0], 'time' => $description[1]);
		}
		
		print_r( json_encode($msg) );
	break;
	
	case "getVMWareStorage":
	
		$REST_DATA = array(
						'Action'			=> 'getVMWareStorage',
						'LAUN_UUID'			=> $_REQUEST['LAUN_UUID']
					);
	
		$output = CurlPostToRemote($REST_DATA);
				
		print_r( $output );
	
	break;
	
	case "getVMWareSettingConfig":
	
		$REST_DATA = array(
						'Action'			=> 'getVMWareSettingConfig',
						'SERV_UUID'			=> $_REQUEST['SERV_UUID'],
						'ReplicatId'		=> $_REQUEST['ReplicatId']
					);
	
		$output = CurlPostToRemote($REST_DATA);
				
		print_r( $output );
	
	break;
	
	case "getConfigDefault":
	
		$REST_DATA = array(
						'Action'			=> 'getConfigDefault',
						'os'				=> $_REQUEST['os'],
						'SERV_UUID'			=> $_REQUEST['SERV_UUID'],
						'ReplicatId'		=> $_REQUEST['ReplicatId']
					);
	
		$output = CurlPostToRemote($REST_DATA);
				
		print_r( $output );
	
	break;
	
	case "QueryAccessCredentialInformation":
	
		$ACTION    = 'QueryAccessCredentialInformation';
		$REST_DATA = array(
					'Action' 			=> $ACTION,
					'ClusterUUID'		=> $_REQUEST['CLUSTER_UUID']									
				);
				
		
		$output = CurlPostToRemote($REST_DATA);
	
		echo $output;	
	
	break;
	
	default:
	break;
}

?>