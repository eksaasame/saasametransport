<?php

require_once 'Common_Model.php';
require_once '..\_exec\_ServiceManagementFunction.php';
require_once '_class_VMWare.php';
require_once '_class_csvProc.php';

class import{

    use csvProc;
    
    private $accountId;

    public function __construct() {
		$this->accountId = null;
	}

    private function getServerIdByAddr( $addr, $type ){

        $model = new common_model();

        $serverInfo = $model->getServerByAddr( $addr, $type );

        if( $serverInfo != false )
            return $serverInfo["_SERV_UUID"];

        return false;
    }
  
    private function getVMIdByName( $serverId, $addr, $user, $pass, $name ){

        $CommonModel = new Common_Model();
	
        $TransportInfo = $CommonModel->getTransportInfo( $serverId );

        $vmClient = new VM_Ware();

        $hosts = $vmClient->getVirtualHosts( $TransportInfo["ConnectAddr"], $addr, $user, $pass );

        foreach( $hosts as $host ){
            $key = array_search( $name, (array)$host->vms );
            if( $key != false )
                return $key;
        }

        return false;
    } 

    private function addHost( $HostInfo ){

        $serverId = $this->getServerIdByAddr( $HostInfo["SourceTransportIP"], 'Carrier' );
    
        if( $HostInfo["SERV_TYPE"] == "Physical Packer" )
            addHost($HostInfo["ACCT_UUID"],'','',$serverId,$HostInfo["HOST_ADDR"],'','',$HostInfo["SERV_TYPE"],
                $HostInfo["OS_TYPE"],$HostInfo["PRIORITY_ADDR"],'');
        else if( $HostInfo["SERV_TYPE"] == "Virtual Packer" ){

            $VMId = $this->getVMIdByName( $serverId, $HostInfo["HOST_ADDR"] ,$HostInfo["HOST_USER"] ,$HostInfo["HOST_PASS"], $HostInfo["SELECT_VM_HOST"]);

            addHost($HostInfo["ACCT_UUID"],'','',$serverId,$HostInfo["HOST_ADDR"],$HostInfo["HOST_USER"],$HostInfo["HOST_PASS"],$HostInfo["SERV_TYPE"],
                '','',array($VMId));
        }
    }

    public function importHosts( $csv_string ){

        $csv_string = 'ACCT_UUID,SourceTransportIP,HOST_ADDR,SERV_TYPE,OS_TYPE,PRIORITY_ADDR,REGN_UUID,OPEN_UUID,HOST_USER,HOST_PASS,SYST_TYPE
1e205f31-b637-4057-9e35-591123701383,192.168.31.14,192.168.31.19,Physical Packer,Windows,192.168.31.14,,,,,';

        $csv_string = 'ACCT_UUID,SourceTransportIP,HOST_ADDR,SERV_TYPE,OS_TYPE,PRIORITY_ADDR,REGN_UUID,OPEN_UUID,HOST_USER,HOST_PASS,SYST_TYPE,SELECT_VM_HOST
1e205f31-b637-4057-9e35-591123701383,192.168.31.14,192.168.31.222,Virtual Packer,,,,,root,abc@123,,55-Win2008R2';

        $HostsInfo = $this->parse_csv( $csv_string );

        foreach( $HostsInfo as $key => $HostInfo ){

            $this->addHost( $HostInfo );
        }

       // print_r($HostsInfo);
    }
}
?>