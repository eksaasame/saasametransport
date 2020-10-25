<?php
$GetPostData = file_get_contents("php://input"); 
$PostData = json_decode($GetPostData,true);

$SelectAction = $PostData['Action'];
$ReplMgmt = new Replica_Class();

if (isset($SelectAction))
{
	switch ($SelectAction)
	{
		############################
		# Initialize New Replica
		############################
		case "InitializeNewReplica":		
			$AcctUUID = $PostData['AcctUUID'];
			$RegnUUID = $PostData['RegnUUID'];
			$ConnUUID = $PostData['ConnUUID'];
			$ServUUID = $PostData['ServUUID'];
					
			print_r($ReplMgmt->save_and_exec_replica($AcctUUID,$RegnUUID,$ServUUID,$ConnUUID));		
		break;	
		
		############################
		# List All Replicas
		############################
		case "ListAllReplicas":
			$AcctUUID = $PostData['AcctUUID'];
			
			print_r($ReplMgmt->list_replica($AcctUUID,null));	
		break;
		
		############################
		# Query Replica Data
		############################
		case "QuerySingleReplica":
			$ReplUUID = $PostData['ReplUUID'];	
			
			print_r($ReplMgmt->list_replica(null,$ReplUUID));	
		break;
		
		############################
		# Delete Replica
		############################
		case "DeleteSingleReplica":
			$ReplUUID = $PostData['ReplUUID'];	
		
			print_r($ReplMgmt->delete_replica($ReplUUID));	
		break;
	}	
}