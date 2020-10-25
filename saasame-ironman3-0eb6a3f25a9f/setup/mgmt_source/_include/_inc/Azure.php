<?php

include_once 'Integrate_Interface.php';

class Azure_Controller implements SaasameInterface
{
    private $AccessKey;

    private $AccessSecret;

    private $SubscriptionId;

    private $UserName;

    private $Password;

    private $TenantId;

    private $Location;

    private $AccessToken;

    private $TokenInfo;

    private $client;

    private $ResourceGroup;

    private $CloudId;

    private $ReplicaId;

    private $ControlUrl;

    private $LoginUrl;

    private $ResourceUrl;

    public function __construct( ) {

	}

    public function SetEndpointType( $type ) {

        $azureInfo = Misc_Class::define_mgmt_setting();

        $this->ControlUrl  = $azureInfo->$type->Auzre_ControlUrl;
        $this->LoginUrl    = $azureInfo->$type->Auzre_LoginUrl;
        $this->ResourceUrl = $azureInfo->$type->Azure_Resource;
    }

    private function Getkey() {
        return $this->AccessKey;
    }

    private function GetSecret() {
        return $this->AccessSecret;
    }

    public function SetLocation( $Location ) {
        $this->Location = $Location;
    }

    public function SetClientId( $ClientId ) {
        $this->AccessKey = $ClientId;
    }

    public function SetClientSecret( $ClientSecret ) {
        $this->AccessSecret = $ClientSecret;
    }

    public function SetSubscriptionId( $SubscriptionId ) {
        $this->SubscriptionId = $SubscriptionId;
    }

    public function SetUserName( $UserName ) {
        $this->UserName = $UserName;
    }

    public function SetPassword( $Password ) {
        $this->Password = $Password;
    }

    public function SetReplicaId( $ReplicaId ) {
        $this->ReplicaId = $ReplicaId;
    }

    public function SetTenantId( $TenantId ) {
        $this->TenantId = $TenantId;
    }

    public function SetAccessToken( $AccessToken ) {
        $this->AccessToken = $AccessToken;
    }

    public function SetResourceGroup( $ResourceGroup ) {

        if( $ResourceGroup != null || $ResourceGroup != '')
            $this->ResourceGroup = $ResourceGroup;
    }

    public function GetResourceGroup( ) {
        return $this->ResourceGroup;
    }

    public function GetAccessToken() {
        return $this->AccessToken;
    }

    public function GetTokenInfo() {
        return $this->TokenInfo;
    }

    public function CreateInstance($ImageId, $InstanceType) {
        return $response;
    }
    
    public function DeleteInstance( $InstanceId ) {
        return $response;
    }

    public function ListInstances() {
        return $response;
    }

    public function GetInstanceDetail(  ) {
        return $response;
    }

    public function StartInstance( $InstanceId ) {
        return $response;
    }

    public function StopInstance( $InstanceId ) {
        return $response;
    }

    public function CreateVolume() {}

    public function DeleteVolume( $vol ) {}

    public function ListVolume() {}

    public function CreateSnapshot( $DiskId ) {
        return $response;
    }

    public function DeleteSnapshot( $SnapshotId ) {
        return $response;
    }

    public function ListSnapshots( $ext_param ) {
        return $response;
    }

    public function CreateImage( $ext_param ) {
        return $response;
    }

    public function ListImages( $ext_param ) {
        return $response;
    }

    public function CreateVolFromSnapshot() {}

    public function DeleteSanpshot() {}

    public function ListSnapshot() {}

    public function AttachVolToInstance() {}

    public function DetachVolFromInstance() {}

    public function CheckConnect() {
        return $response;
    }

    public function GetSnapshotList( ) {
        return $response;
    }

    public function CreateMachine() {}

    public function DescribeRegionsRequest() {
    }

    public function getImagesList(){
        return $response;
    }

    public function ReplaceSystemDisk( $InstanceId, $ImageId ) {
        return $response;
    }

    public function getInstanceTypeList() {
        return $response;
    }

    public function CreateDisk( $ZoneId, $Size ) {
        return $response;
    }

    public function DeleteDisk( $DiskId ) {
        return $response;
    }

    public function AttachDisk( $InstanceId, $DiskId ) {
        return $response;
    }

    public function DetachDisk( $InstanceId, $DiskId ) {
        return $response;
    }

    public function RollbackDisk( $SnapshotId, $DiskId ) {
        return $response;
    }

    public function getZones() {
        return $response;
    }

    public function fortest()  {
        return $response;
    }

    public function DefaultConfig( $TENANT_ID, $SUBSCRIPTION_ID, $CLIENT_ID, $CLIENT_SECRET) {
        $this->SetSubscriptionId( $SUBSCRIPTION_ID );
		//$this->SetUserName( $USER_NAME );
		//$this->SetPassword( $PASSWORD );
		$this->SetTenantId( $TENANT_ID );
        $this->SetClientId( $CLIENT_ID );
        $this->SetClientSecret( $CLIENT_SECRET );
		//$this->SetLocation( 'westus' );
		//$this->SetResourceGroup( 'saasame_rg' );
    }
    public function GetOAuth2AuthCode() {
        $endpoint = "https://'.$this->LoginUrl.'/".$this->TenantId."/oauth2/authorize?";

        $endpoint .= "client_id=".$this->AccessKey;
        $endpoint .= "&response_type=code";
        $endpoint .= "&redirect_uri=http%3A%2F%2Flocalhost%2Fmyapp%2F";
        $endpoint .= "&response_mode=query";
        $endpoint .= "&resource=https%3A%2F%2Fservice.contoso.com%2F";
        $endpoint .= "&state=12345";

        $curl = curl_init($endpoint);
        curl_setopt($curl, CURLOPT_CUSTOMREQUEST, 'GET');
 
        $json_response = curl_exec($curl);

        $status = curl_getinfo($curl, CURLINFO_HTTP_CODE);

        // evaluate for success response
        if ($status != 200) {
            throw new Exception("Error: call to URL $endpoint failed with status $status, response $json_response, curl_error " . curl_error($curl) . ", curl_errno " . curl_errno($curl) . "\n");
        }
        curl_close($curl);

        $this->AccessToken = json_decode($json_response, true)['access_token'];

        return $json_response;
    }
    public function GetOAuth2TokenByAssigned( ) {
        
        $endpoint = "https://'.$this->LoginUrl.'/".$this->TenantId."/oauth2/token";

       
        $params = array('grant_type'    => 'password',
                        'resource'      => 'https://management.core.windows.net/',
                        'client_id'     => $this->AccessKey,
                        'client_secret' => $this->AccessSecret,
                        'username'      => $this->UserName,
                        'password'      => $this->Password,
                        'scope'         => 'openid'
                        );

        $curl = curl_init($endpoint);
        curl_setopt($curl, CURLOPT_HEADER, true);
        curl_setopt($curl, CURLOPT_RETURNTRANSFER, true);
        curl_setopt($curl, CURLOPT_POST, true);
        curl_setopt($curl, CURLOPT_HEADER,'Content-Type: application/x-www-form-urlencoded');
        curl_setopt($curl, CURLOPT_HEADER,'Accept: application/json');

        // Remove comment if you have a setup that causes ssl validation to fail
        //curl_setopt($curl, CURLOPT_SSL_VERIFYPEER, false);
        $postData = "";

        //This is needed to properly form post the credentials object
        foreach($params as $k => $v) {
            $postData .= $k . '='.urlencode($v).'&';
        }

        $postData = rtrim($postData, '&');

        curl_setopt($curl, CURLOPT_POSTFIELDS, $postData);

        $json_response = curl_exec($curl);

        $status = curl_getinfo($curl, CURLINFO_HTTP_CODE);

        // evaluate for success response
        if ($status != 200) {
            throw new Exception("Error: call to URL $endpoint failed with status $status, response $json_response, curl_error " . curl_error($curl) . ", curl_errno " . curl_errno($curl) . "\n");
        }
        curl_close($curl);

        $this->AccessToken = json_decode($json_response, true)['access_token'];

        return $json_response;
    }

    public function GetOAuth2Token( ) {
        
        $endpoint = "https://".$this->LoginUrl."/".$this->TenantId."/oauth2/token?api-version=1.0";
       
        $params = array('grant_type'    => 'client_credentials',
                        'resource'      => 'https://'.$this->ResourceUrl,
                        'client_id'     => $this->AccessKey,
                        'client_secret' => $this->AccessSecret
                        );

        $curl = curl_init($endpoint);
        curl_setopt($curl, CURLOPT_HEADER, true);
        curl_setopt($curl, CURLOPT_RETURNTRANSFER, true);
        curl_setopt($curl, CURLOPT_POST, true);
        curl_setopt($curl, CURLOPT_HEADER,'Content-Type: application/x-www-form-urlencoded');
        curl_setopt($curl, CURLOPT_HEADER,'Accept: application/json');

        $postData = "";

        //This is needed to properly form post the credentials object
        foreach($params as $k => $v) {
            $postData .= $k . '='.urlencode($v).'&';
        }

        $postData = rtrim($postData, '&');

        curl_setopt($curl, CURLOPT_POSTFIELDS, $postData);

        $json_response = curl_exec($curl);

        $status = curl_getinfo($curl, CURLINFO_HTTP_CODE);

        // evaluate for success response
        if ($status != 200) {
            throw new Exception("Error: call to URL $endpoint failed with status $status, response $json_response, curl_error " . curl_error($curl) . ", curl_errno " . curl_errno($curl) . "\n");
        }
        curl_close($curl);

        $this->TokenInfo = $json_response;

        $this->AccessToken = json_decode($json_response, true)['access_token'];

        return $json_response;
    }

    private function ProcSendData($URL, $Mehod, $REST_DATA, $TIME_OUT = 0, $err_ret = false) {

        try{
            return $this->CurlToAzure( $URL, $Mehod, $REST_DATA, $TIME_OUT, $err_ret );
        }
        catch( Exception $e ){ //handle the api version error

            $errorMsg = $e->getMessage();

            $spos = strpos( $errorMsg, "The supported api-versions are" );

            if( $spos === false )
                throw new Exception( $errorMsg );
            
            if( strpos( $errorMsg, "for api version " ) !== false ){
                $nstr = substr( $errorMsg, strpos( $errorMsg, "for api version " ) + strlen("for api version ") );
            }
            else if( strpos( $errorMsg, "and API version " ) !== false )
                $nstr = substr( $errorMsg, strpos( $errorMsg, "and API version " ) + strlen("and API version ") );

            $data = explode( "'", $nstr );

            $nowVersion = $data[1];

            $nstr1 = substr( $errorMsg, $spos + strlen("The supported api-versions are ") );

            $data = explode( "'", $nstr1 );

            $newVersion = explode( ",", $data[1] );

            $nURL = str_replace( $nowVersion, trim(end($newVersion)), $URL );

            return $this->CurlToAzure( $nURL, $Mehod, $REST_DATA, $TIME_OUT, $err_ret );
        }
    }

    public function CurlToAzure($URL, $Mehod, $REST_DATA, $TIME_OUT = 0, $err_ret = false)
    {
        $ch = curl_init($URL);

        curl_setopt($ch, CURLOPT_SSL_VERIFYPEER, false);

        $retry_count = 0;

        do {

            if( !isset( $this->AccessToken ) ) 
                $this->GetOAuth2Token( );

            $header = array('Content-Type: application/json',
                'Authorization: Bearer '.$this->AccessToken,
                'Host: '.$this->ControlUrl.'');
        
            if( isset($REST_DATA) ) {
                curl_setopt($ch, CURLOPT_POSTFIELDS, $REST_DATA);
            }
        
            curl_setopt($ch, CURLOPT_CUSTOMREQUEST, $Mehod);
            curl_setopt($ch, CURLOPT_HEADER, false);
            curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
            curl_setopt($ch, CURLOPT_HTTPHEADER, $header);
            curl_setopt($ch, CURLOPT_TIMEOUT_MS, $TIME_OUT);
            $output = curl_exec($ch);	

            $status = curl_getinfo($ch, CURLINFO_HTTP_CODE);

            //$this->gen_debug_file($URL, $Mehod, $REST_DATA, $output, $status);

            if ($status == 200 || $status == 201 || $status == 202 || $status == 204) {
                curl_close($ch);
                return $output;
            }

            // evaluate for success response
            switch( $status )
            {
                case 401:
                {
                    $token_info = $this->GetOAuth2Token();
                    if( isset( $this->CloudId ) ) {
                        $AzureModel = new Common_Model();
                        $AzureModel->update_cloud_token_info( $this->CloudId, $token_info );
                    }
                    break;
                }
                case 404:
                {
                    //do not throw exception case
                    if( $err_ret )
                        return json_encode( array( "success" => false, "code" => $status ), JSON_UNESCAPED_SLASHES);
                    
                    // do not write log case
                    if( strpos( $output, "The supported api-versions are" ) === false )
                        $this->gen_debug_file($URL, $Mehod, $REST_DATA, $output, $status, "Azure_debug");

                    throw new Exception(json_encode( array("detail"=>"Error: call to URL $URL failed with status $status, response $output, curl_error " . curl_error($ch) . ", curl_errno " . curl_errno($ch) . "\n",
                                                "output"=> $output)));
                    
                }
                default:
                    if( strpos( $output, "The supported api-versions are" ) === false )
                        $this->gen_debug_file($URL, $Mehod, $REST_DATA, $output, $status, "Azure_debug");
                    
                    $msg = "Error: call to URL $URL failed with status $status, response $output, curl_error " . curl_error($ch) . ", curl_errno " . curl_errno($ch) . "\n";
                    throw new Exception(json_encode( array("detail"=>$msg,"output"=> $output)));
                    break;
            }

            $retry_count++;

        }while( $retry_count < 3 );
        
        curl_close($ch);
        
        return $output;
    }
    
    public function gen_debug_file($URL, $Mehod, $REST_DATA, $output, $status, $path = "Azure log") {

        $req = array(
            "URL" => $URL,
            "Method" => $Mehod,
            "REST_DATA" => $REST_DATA
        );

        $out = array(
            "return" => json_decode( $output, true),
            "status" => $status,
            "callstack" => debug_backtrace()
        );

        Misc_Class::openstack_debug($path,(object)$req,json_encode( $out ));
    }

    public function ListVMInResourceGroup( ) {
        $URL = 'https://'.$this->ControlUrl.'/'.
            'subscriptions/'.$this->SubscriptionId.
            '/resourceGroups/'.$this->ResourceGroup.
            '/providers/Microsoft.Compute/virtualmachines?'.
            'api-version=2016-04-30-preview';
        
        return $this->ProcSendData( $URL, 'GET', null );
    }

    public function ListVMISubscription( ) {
        $URL = 'https://'.$this->ControlUrl.'/'.
            'subscriptions/'.$this->SubscriptionId.
            '/providers/Microsoft.Compute/virtualmachines?'.
            'api-version=2016-04-30-preview';
        
        return $this->ProcSendData( $URL, 'GET', null );
    }

    public function GetVMInformation( $VMName, $ResourceGroup = null ) {

        if( $ResourceGroup == null )
            $ResourceGroup = $this->ResourceGroup;

        $URL = "https://".$this->ControlUrl."/".
        "subscriptions/".$this->SubscriptionId."/".
        "resourceGroups/".$ResourceGroup."/".
        "providers/Microsoft.Compute/".
        "virtualMachines/".$VMName."?".
        "api-version=2016-03-30";

        return $this->ProcSendData( $URL, 'GET', null, 0, true );
    }

    public function GetClassicVMInformation( ) {
        $URL = "https://".$this->ControlUrl."/".
        "subscriptions/".$this->SubscriptionId."/".
       // "resourceGroups/".$this->ResourceGroup."/".
        "providers/Microsoft.ClassicCompute/".
        "virtualMachines?".
        "api-version=2017-04-01";

        return $this->ProcSendData( $URL, 'GET', null, 0 );
    }

    public function GetVMInstanceInformation( $VMName ) {

        $URL = "https://".$this->ControlUrl."/".
        "subscriptions/".$this->SubscriptionId."/".
        "resourceGroups/".$this->ResourceGroup."/".
        "providers/Microsoft.Compute/".
        "virtualMachines/".$VMName."/".
        "InstanceView?".
        "api-version=2016-03-30";

        return $this->ProcSendData( $URL, 'GET', null );
    }

    public function ListDisksInSubscription() {
        $URL = 'https://'.$this->ControlUrl.'/'.
            'subscriptions/'.$this->SubscriptionId.
            '/providers/Microsoft.Compute/disks?'.
            'api-version=2016-04-30-preview';

        return $this->ProcSendData( $URL, 'GET', null );
    }

    public function GetDisksDetail( $name ) {
        $URL = 'https://'.$this->ControlUrl.'/'.
            'subscriptions/'.$this->SubscriptionId.'/'.
            'resourceGroups/'.$this->ResourceGroup.'/'.
            'providers/Microsoft.Compute/'.
            'disks/'.$name.'?'.
            'api-version=2016-04-30-preview';

        return $this->ProcSendData( $URL, 'GET', null );
    }

    public function CreateDiskI( $DiskName , $size, $tags = null) {
        $URL = 'https://'.$this->ControlUrl.'/'.
            'subscriptions/'.$this->SubscriptionId.'/'.
            'resourceGroups/'.$this->ResourceGroup.'/'.
            'providers/Microsoft.Compute/'.
            'disks/'.$DiskName.'?'.
            'api-version=2016-04-30-preview';

        $json = array( "name" => $DiskName,
                        "location" => $this->Location,
                        "properties" => array( 
                            "creationData" => array(  
                                "createOption" => "Empty"
                            ),
                            "diskSizeGB" => $size
                        )
                    );

        if( isset( $tags ) )
            $json["tags"] = $tags;

        return $this->ProcSendData( $URL, 'PUT', json_encode($json) );
    }

    public function CreateDiskFromUnmanagementDisk( $DiskName , $connectionString, $container, $filename, $size ,$tags = null ) {
        $URL = 'https://'.$this->ControlUrl.'/'.
            'subscriptions/'.$this->SubscriptionId.'/'.
            'resourceGroups/'.$this->ResourceGroup.'/'.
            'providers/Microsoft.Compute/'.
            'disks/'.$DiskName.'?'.
            'api-version=2017-03-30';

        $blobEndpoint = $this->parseConnectionString( $connectionString );

        $json = array( "name" => $DiskName,
                        "location" => $this->Location,
                        "properties" => array( 
                            "creationData" => array(  
                                "createOption" => "Import",
                                "sourceUri" => "https://".$blobEndpoint["AccountName"].".blob.".$blobEndpoint["EndpointSuffix"]."/".$container."/".$filename,
                                //"storageAccountId" => "subscriptions/".$this->SubscriptionId."/resourceGroups/".$this->ResourceGroup."/providers/Microsoft.Storage/storageAccounts/".$storageAccount
                            ),
                            "diskSizeGB" => $size
                        )
                    );

        if( isset( $tags ) )
            $json["tags"] = $tags;

        return $this->ProcSendData( $URL, 'PUT', json_encode($json) );
    }

    public function CreateDiskFromSnapshot( $DiskName , $SnapshotName, $tags = null) {
        $URL = 'https://'.$this->ControlUrl.'/'.
            'subscriptions/'.$this->SubscriptionId.'/'.
            'resourceGroups/'.$this->ResourceGroup.'/'.
            'providers/Microsoft.Compute/'.
            'disks/'.$DiskName.'?'.
            'api-version=2016-04-30-preview';

        $json = array( "name" => $DiskName,
                        "location" => $this->Location,
                        "properties" => array( 
                            "creationData" => array(  
                                "createOption" => "Copy",
                                "sourceResourceId" => "subscriptions/".$this->SubscriptionId."/resourceGroups/".$this->ResourceGroup."/providers/Microsoft.Compute/snapshots/".$SnapshotName
                            )
                        )
                    );

        if( isset( $tags ) )
            $json["tags"] = $tags;

        return $this->ProcSendData( $URL, 'PUT', json_encode($json) );
    }

    public function DeleteDiskI( $DiskName ) {
        $URL = 'https://'.$this->ControlUrl.'/'.
            'subscriptions/'.$this->SubscriptionId.'/'.
            'resourceGroups/'.$this->ResourceGroup.'/'.
            'providers/Microsoft.Compute/'.
            'disks/'.$DiskName.'?'.
            'api-version=2016-04-30-preview';

        return $this->ProcSendData( $URL, 'DELETE', '' );
    }

    public function CreateSnapshotI( $SnapshotName , $DiskName, $tags) {

        $URL = 'https://'.$this->ControlUrl.'/'.
            'subscriptions/'.$this->SubscriptionId.'/'.
            'resourceGroups/'.$this->ResourceGroup.'/'.
            'providers/Microsoft.Compute/'.
            'snapshots/'.$SnapshotName.'?'.
            'api-version=2016-04-30-preview';

        $json = array( "name" => $SnapshotName,
                        "location" => $this->Location,
                        "properties" => array( 
                            "creationData" => array(
                                "createOption" => "Copy",
                                "sourceUri" => "/subscriptions/".$this->SubscriptionId."/resourceGroups/".$this->ResourceGroup."/providers/Microsoft.Compute/disks/".$DiskName
                            )
                        )
                    );

        if( isset( $tags ) )
            $json["tags"] = $tags;
        
        return $this->ProcSendData( $URL, 'PUT', json_encode($json) );
    }

    public function DeleteSnapshotI( $snapshotName ) {
        $URL = 'https://'.$this->ControlUrl.'/'.
            'subscriptions/'.$this->SubscriptionId.
            '/resourceGroups/'.$this->ResourceGroup.
            '/providers/Microsoft.Compute/'.
            'snapshots/'.$snapshotName.'?'.
            'api-version=2016-04-30-preview';

        return $this->ProcSendData( $URL, 'DELETE', '' );
    }

    public function ListSnapshotsInSubscriptions( ) {
        $URL = 'https://'.$this->ControlUrl.'/'.
            'subscriptions/'.$this->SubscriptionId.
            '/providers/Microsoft.Compute/'.
            'snapshots/?'.
            'api-version=2016-04-30-preview';

        return $this->ProcSendData( $URL, 'GET', '' );
    }

    public function GetSnapshotDetail( $snapshotName ) {
        $URL = 'https://'.$this->ControlUrl.'/'.
            'subscriptions/'.$this->SubscriptionId.'/'.
            'resourceGroups/'.$this->ResourceGroup.'/'.
            'providers/Microsoft.Compute/'.
            'snapshots/'.$snapshotName.'?'.
            'api-version=2016-04-30-preview';

        return $this->ProcSendData( $URL, 'GET', '' );
    }

    public function ListAvailabilitySetInRG( ) {
        $URL = 'https://'.$this->ControlUrl.'/'.
            'subscriptions/'.$this->SubscriptionId.
            '/resourceGroups/'.$this->ResourceGroup.
            '/providers/Microsoft.Compute/availabilitySets?'.
            'api-version=2017-12-01';
        
        return $this->ProcSendData( $URL, 'GET', null );
    }

    /*public function CreateVM( $VMName , $resource_group, $location) {
        $URL = 'https://'.$this->ControlUrl.'/'.
            'subscriptions/'.$this->SubscriptionId.
            '/resourceGroups/'.$resource_group.
            '/providers/Microsoft.Compute/'.
            'virtualMachines/'.$VMName.'?'.
            'api-version=2016-04-30-preview';

        $json = $this->GetVMConfig( $VMName , $resource_group, $location );

        return $this->ProcSendData( $URL, 'PUT', json_encode($json, JSON_UNESCAPED_SLASHES) );
    }*/

    public function DeleteVM( $VMName ) {
        $URL = 'https://'.$this->ControlUrl.'/'.
            'subscriptions/'.$this->SubscriptionId.
            '/resourceGroups/'.$this->ResourceGroup.
            '/providers/Microsoft.Compute/'.
            'virtualMachines/'.$VMName.'?'.
            'api-version=2016-04-30-preview';

        return $this->ProcSendData( $URL, 'DELETE', '' );
    }

    public function PowerControlVM( $VMId, $status = true ) {

        if( $status )
            $power = "start";
        else
            $power = "PowerOff";

        $URL = 'https://'.$this->ControlUrl.$VMId.'/'.$power.'?'.
            'api-version=2018-06-01';

        $this->ProcSendData( $URL, 'POST', '' );

        return true;
    }

    /*public function GetVMConfig( $VMName , $resource_group, $location ) {
        $json = array( "name" => $VMName,
                        "location" => $location,
                        "tags" => array(
                            "department" => "finance" 
                            ),
                        "properties" => array(
                            "licenseType" => "Windows_Server",
                            "hardwareProfile" => array( 
                                "vmSize" => "Standard_A1" 
                                ),
                            "storageProfile" => array(
                                "imageReference" => array(
                                    "publisher" => "MicrosoftWindowsServerEssentials",    
                                    "offer" => "WindowsServerEssentials",    
                                    "sku" => "WindowsServerEssentials",    
                                    "version" => "latest"//,
                                    //"id" => "/subscriptions/".$this->SubscriptionId."/providers/Microsoft.Compute/locations/westus/publishers/MicrosoftWindowsServerEssentials/ArtifactTypes/vmimage/offers/WindowsServerEssentials/skus/WindowsServerEssentials/versions/latest"
                                ),
                                "osDisk" => array(
                                    "name" => "osdisk",
                                    "osType" => "Windows",
                                    "createOption" => "fromImage",
                                    "diskSizeGB" => "40"
                                )
                            ),
                            "osProfile" => array(
                                "computerName" => "myvm1",
                                "adminUsername" => "qwerty333",
                                "adminPassword" => "1qaz@WSX3edc",
                                "windowsConfiguration" => array(
                                    "provisionVMAgent" => true,
                                    "winRM" => array( 
                                        "listeners" => array(
                                            array(
                                                "protocol" => "http"
                                            )
                                        ) 
                                    ),
                                    "enableAutomaticUpdates" => false,
                                    "timeZone" => "Pacific Standard Time"
                            )
                        ),
                        "networkProfile" => array(
                            "networkInterfaces" => array(
                                array(
                                    "id" => "/subscriptions/".$this->SubscriptionId."/resourceGroups/".$resource_group."/providers/Microsoft.Network/networkInterfaces/mynic1",
                                    "properties" => array(
                                        "primary" => true
                                    )
                                )
                            )
                        ),
                        "diagnosticsProfile" => array(
                            "bootDiagnostics" => array(
                                "enabled" => true,
                                "storageUri" => " http://testsaasame.blob.core.windows.net/"
                            )
                        )
                    )
                );

            return $json;
    }*/

    public function CreateVMFromVHD( $VMName , $DiskName, $VmSize, $NetworkInterfaceName, $StorageInfo, $dataDiskArray = null, $param = null) {

        $blobEndpoint = $this->parseConnectionString( $StorageInfo["connectionString"] );
        
        $URL = 'https://'.$this->ControlUrl.'/'.
            'subscriptions/'.$this->SubscriptionId.
            '/resourceGroups/'.$this->ResourceGroup.
            '/providers/Microsoft.Compute/'.
            'virtualMachines/'.$VMName.'?'.
            'api-version=2016-04-30-preview';

        $dataDiskConfig = null;
        
        if( $dataDiskArray ) {

            $dataDiskConfig = array();

            foreach( $dataDiskArray as $key => $disk ) {
                $diskConfig = array(
                    "name" => $disk,
                    "lun" => $key,
                    "createOption" => "Attach",
                    "vhd" => array(
                        "uri"=>"https://".$blobEndpoint["AccountName"].
                        ".blob.".$blobEndpoint["EndpointSuffix"]."/".$StorageInfo["container"].
                        "/".$disk
                    )
                );

                array_push( $dataDiskConfig , $diskConfig );
            }
        }

        $json = $this->GetVMConfigFromDisk( $VMName , $DiskName, $VmSize, $NetworkInterfaceName, $dataDiskConfig, $param );

        unset( $json["properties"]["storageProfile"]["osDisk"]["managedDisk"] );

        $json["properties"]["storageProfile"]["osDisk"]["createOption"] = "Attach";
        $json["properties"]["storageProfile"]["osDisk"]["caching"] = "ReadWrite";
        $json["properties"]["storageProfile"]["osDisk"]["name"] = $DiskName;

        $json["properties"]["storageProfile"]["osDisk"]["vhd"] = array( 
            "uri" => "https://".$blobEndpoint["AccountName"].
            ".blob.".$blobEndpoint["EndpointSuffix"]."/".$StorageInfo["container"].
            "/".$DiskName
        );

        return $this->ProcSendData( $URL, 'PUT', json_encode($json, JSON_UNESCAPED_SLASHES) );
    }

    public function CreateVMFromDisk( $VMName , $DiskName, $VmSize, $NetworkInterfaceName, $dataDiskArray = null, $param = null) {
        $URL = 'https://'.$this->ControlUrl.'/'.
            'subscriptions/'.$this->SubscriptionId.
            '/resourceGroups/'.$this->ResourceGroup.
            '/providers/Microsoft.Compute/'.
            'virtualMachines/'.$VMName.'?'.
            'api-version=2016-04-30-preview';

        $dataDiskConfig = null;
        
        if( $dataDiskArray ) {

            $dataDiskConfig = array();

            foreach( $dataDiskArray as $key => $disk ) {
                $diskConfig = array(
                   // "name" => $disk,
                    "lun" => $key,
                    "createOption" => "Attach",
                    "managedDisk" => array(
                        "id" => "/subscriptions/".$this->SubscriptionId."/resourceGroups/".$this->ResourceGroup."/providers/Microsoft.Compute/disks/".$disk,
                        "storageAccountType" => "Standard_LRS"
                    )
                );

                if( isset( $param["DiskType"] ) && $param["DiskType"] == "SSD" )
                    $diskConfig["managedDisk"]["storageAccountType"] = "Premium_LRS";

                array_push( $dataDiskConfig , $diskConfig );
            }
        }

        $json = $this->GetVMConfigFromDisk( $VMName , $DiskName, $VmSize, $NetworkInterfaceName, $dataDiskConfig, $param );

        return $this->ProcSendData( $URL, 'PUT', json_encode($json, JSON_UNESCAPED_SLASHES) );
    }

    public function GetVMConfigFromDisk( $VMName , $diskName , $VmSize, $NetworkInterfaceName, $dataDisk = null, $param = null ) {
        $json = array( "name" => $VMName,
                        "location" => $this->Location,
                        "tags" => array(
                            "factory" => "Created by Saasame." 
                            ),
                        "properties" => array(
                            //"licenseType" => "Windows_Server",
                            "hardwareProfile" => array( 
                                "vmSize" => $VmSize 
                                ),
                            "storageProfile" => array(
                                "osDisk" => array(
                                    "osType" => "Windows",
                                    "createOption" => "Attach",
                                    "managedDisk" => array(
                                        //"id" => "[resourceId('Microsoft.Compute/disks', [concat('".$diskName."', copyindex())])]",
                                        "id" => "/subscriptions/".$this->SubscriptionId."/resourceGroups/".$this->ResourceGroup."/providers/Microsoft.Compute/disks/".$diskName,
                                        "storageAccountType" => "Standard_LRS"
                                    )
                                )
                            ),
                            "networkProfile" => array(
                                "networkInterfaces" => array(
                                    array(
                                        "id" => "/subscriptions/".$this->SubscriptionId."/resourceGroups/".$this->ResourceGroup."/providers/Microsoft.Network/networkInterfaces/".$NetworkInterfaceName,
                                        "properties" => array(
                                            "primary" => true
                                        )
                                    )
                                )
                            )/*,
                            "diagnosticsProfile" => array(
                                "bootDiagnostics" => array(
                                    "enabled" => true,
                                    "storageUri" => " http://testsaasame.blob.core.windows.net/"
                                )
                            )*/
                        )
                    );

        if( $dataDisk ) 
            $json["properties"]["storageProfile"]["dataDisks"] = $dataDisk ;
        
        if( isset( $param["HostName"] ) )
            $json["tags"]["HostName"] = $param["HostName"];

        if( isset( $param["OsType"] ) )
            $json["properties"]["storageProfile"]["osDisk"]["osType"] = $param["OsType"];

        if( isset( $param["AvailabilitySetId"] ) && $param["AvailabilitySetId"] != false)
            $json["properties"]["availabilitySet"]["id"] = $param["AvailabilitySetId"];

        if( isset( $param["DiskType"] ) && $param["DiskType"] == "SSD" )
            $json["properties"]["storageProfile"]["osDisk"]["managedDisk"]["storageAccountType"] = "Premium_LRS";
            
        return $json;
    }


    public function GetVMSize() {

        $URL = 'https://'.$this->ControlUrl.'/subscriptions/'.$this->SubscriptionId.'/providers/Microsoft.Compute/locations/'.$this->Location.'/vmSizes?api-version=2016-04-30-preview';

        return $this->ProcSendData( $URL, 'GET', null );
    }

    public function GetNetworkLsit() {
        $URL = 'https://'.$this->ControlUrl.'/'.
            'subscriptions/'.$this->SubscriptionId.'/'.
            'providers/Microsoft.Network/'.
            'networkInterfaces?'.
            'api-version=2017-03-01';

        return $this->ProcSendData( $URL, 'GET', null );
    }

    public function CreateNetworkInterface( $name, $netSecurityGroup, $virtualNetwork, $subnet, $publicIp = null, $privateIp = null, $config = null) {
        
        $URL = 'https://'.$this->ControlUrl.'/'.
            'subscriptions/'.$this->SubscriptionId.'/'.
            'resourceGroups/'.$this->ResourceGroup.'/'.
            'providers/Microsoft.Network/'.
            'networkInterfaces/'.$name.'?'.
            'api-version=2017-03-01';

        if( !$config )
            $json = $this->GetNetworkInterfaceConfig( $netSecurityGroup, $virtualNetwork, $subnet, $publicIp, $privateIp);
        else
            $json = $config;
            
        return $this->ProcSendData( $URL, 'PUT', json_encode($json, JSON_UNESCAPED_SLASHES) );
    }

    public function GetNetworkInterfaceConfig( $netSecurityGroup, $virtualNetwork, $subnet, $publicIp, $privateIp ) {
        $json = array(  "location" => $this->Location,
                        "tags" => array(
                            "factory" => "Created by SaaSaMe." 
                            ),
                        "properties" => array(
                            "networkSecurityGroup" => array( 
                                "id" => "/subscriptions/".$this->SubscriptionId."/resourceGroups/".$this->ResourceGroup."/providers/Microsoft.Network/networkSecurityGroups/".$netSecurityGroup
                                ),
                            "ipConfigurations" => array(
                                array(
                                    "name" => "ipconfig1",
                                    "properties" => array(
                                        "subnet" =>array(
                                            "id" => "/subscriptions/".$this->SubscriptionId."/resourceGroups/".$this->ResourceGroup."/providers/Microsoft.Network/virtualNetworks/".$virtualNetwork."/subnets/".$subnet
                                        ),
                                        "privateIPAllocationMethod" => "Dynamic",
                                        "publicIPAddress" => array(
                                            "id" => "/subscriptions/".$this->SubscriptionId."/resourceGroups/".$this->ResourceGroup."/providers/Microsoft.Network/publicIPAddresses/".$publicIp
                                        )
                                    )
                                )
                            )
                        )
                    );

        if( !$publicIp )
            unset( $json["properties"]["ipConfigurations"][0]["properties"]["publicIPAddress"] );

        if( $privateIp ){
            $json["properties"]["ipConfigurations"][0]["properties"]["privateIPAllocationMethod"] = "Static";
            $json["properties"]["ipConfigurations"][0]["properties"]["privateIPAddress"] = $privateIp;
        }

        return $json;
    }

    public function CheckIpaddressAvailability( $virtualNetwork, $ipaddress ) {

        $URL = 'https://'.$this->ControlUrl.'/'.
            'subscriptions/'.$this->SubscriptionId.'/'.
            'resourceGroups/'.$this->ResourceGroup.'/'.
            'providers/Microsoft.Network/'.
            'virtualNetworks/'.$virtualNetwork.'/'.
            'CheckIPAddressAvailability?ipAddress='.$ipaddress.'&'.
            'api-version=2018-01-01';

        return $this->ProcSendData( $URL, 'GET', null );
    }

    public function DeleteNetworkInterface( $name ) {
        
        $URL = 'https://'.$this->ControlUrl.'/'.
            'subscriptions/'.$this->SubscriptionId.'/'.
            'resourceGroups/'.$this->ResourceGroup.'/'.
            'providers/Microsoft.Network/'.
            'networkInterfaces/'.$name.'?'.
            'api-version=2016-09-01';

        return $this->ProcSendData( $URL, 'DELETE', null );
    }

    public function CreatePublicIp( $name, $ipStatic = false ) {
        
        $URL = 'https://'.$this->ControlUrl.'/'.
            'subscriptions/'.$this->SubscriptionId.'/'.
            'resourceGroups/'.$this->ResourceGroup.'/'.
            'providers/Microsoft.Network/'.
            'publicIPAddresses/'.$name.'?'.
            'api-version=2016-09-01';

        $json = $this->GetPublicIpConfig( $ipStatic );

        return $this->ProcSendData( $URL, 'PUT', json_encode($json, JSON_UNESCAPED_SLASHES) );
    }

    public function DeletePublicIp( $name ) {
        
        $URL = 'https://'.$this->ControlUrl.'/'.
            'subscriptions/'.$this->SubscriptionId.'/'.
            'resourceGroups/'.$this->ResourceGroup.'/'.
            'providers/Microsoft.Network/'.
            'publicIPAddresses/'.$name.'?'.
            'api-version=2016-09-01';

        return $this->ProcSendData( $URL, 'DELETE', null );
    }

    public function GetPublicIpConfig( $ipStatic ) {

        $json = array(  "location" => $this->Location,
                        "tags" => array(
                            "factory" => "Created by SaaSaMe." 
                        ),
                        "properties" => array(
                            "publicIPAllocationMethod" => "Dynamic"
                        )
                    );

        if( $ipStatic )
            $json["properties"]["publicIPAllocationMethod"] = "Static";

        return $json;
    }

    public function GetNetworkInterface( $NetworkInterfaceName ) {
        
        $URL = 'https://'.$this->ControlUrl.'/'.
            'subscriptions/'.$this->SubscriptionId.'/'.
            'resourceGroups/'.$this->ResourceGroup.'/'.
            'providers/Microsoft.Network/'.
            'networkInterfaces/'.$NetworkInterfaceName.'?'.
            'api-version=2016-09-01';

        return $this->ProcSendData( $URL, 'GET', null );
    }

    public function GetPublicIPLsit() {
        $URL = 'https://'.$this->ControlUrl.'/'.
            'subscriptions/'.$this->SubscriptionId.'/'.
            'providers/Microsoft.Network/'.
            'publicIPAddresses?'.
            'api-version=2017-03-01';

        return $this->ProcSendData( $URL, 'GET', null );
    }

    public function GetNetworkSecurityGroupsLsit() {
        $URL = 'https://'.$this->ControlUrl.'/'.
            'subscriptions/'.$this->SubscriptionId.'/'.
            'providers/Microsoft.Network/'.
            'networkSecurityGroups?'.
            'api-version=2017-03-01';

        return $this->ProcSendData( $URL, 'GET', null );
    }

    public function GetSubnetInVirtualNetwork( $virtualNetwork ) {
        
        $URL = 'https://'.$this->ControlUrl.'/'.
            'subscriptions/'.$this->SubscriptionId.'/'.
            'resourceGroups/'.$this->ResourceGroup.'/'.
            'providers/Microsoft.Network/'.
            'virtualNetworks/'.$virtualNetwork.'/'.
            'subnets?'.
            'api-version=2016-09-01';

        return $this->ProcSendData( $URL, 'GET', null );
    }

    public function GetSubnetDetail( $virtualNetwork, $subnet ) {

        $URL = 'https://'.$this->ControlUrl.'/'.
            'subscriptions/'.$this->SubscriptionId.'/'.
            'resourceGroups/'.$this->ResourceGroup.'/'.
            'providers/Microsoft.Network/'.
            'virtualNetworks/'.$virtualNetwork.'/'.
            'subnets/'.$subnet.'?'.
            'api-version=2016-09-01';

        return $this->ProcSendData( $URL, 'GET', null );
    }

    public function GetVirtualNetworkInResourceGroup( ) {
        
        $URL = 'https://'.$this->ControlUrl.'/'.
            'subscriptions/'.$this->SubscriptionId.'/'.
            'resourceGroups/'.$this->ResourceGroup.'/'.
            'providers/Microsoft.Network/'.
            'virtualNetworks?'.
            'api-version=2016-09-01';

        return $this->ProcSendData( $URL, 'GET', null );
    }

    public function GetVirtualNetworkInSubscription( ) {

        $URL = 'https://'.$this->ControlUrl.'/'.
            'subscriptions/'.$this->SubscriptionId.'/'.
            'providers/Microsoft.Network/'.
            'virtualNetworks?'.
            'api-version=2016-09-01';

        return $this->ProcSendData( $URL, 'GET', null );
    }

    private function GetStorageAccountList( ) {

        $URL = 'https://'.$this->ControlUrl.'/'.
            'subscriptions/'.$this->SubscriptionId.'/'.
            'providers/Microsoft.Storage/'.
            'storageAccounts/?'.
            'api-version=2018-07-01';

        return $this->ProcSendData( $URL, 'GET', null );
    }

    public function AddDiskConfig( &$VM_info, $DiskName ) {

        $map = array();
        foreach( $VM_info['properties']['storageProfile']['dataDisks'] as $DiskInfo ) {
            $map[ $DiskInfo['lun'] ] = true; 
        }

        for( $i = 0;; $i++ ){
            if( !array_key_exists( $i , $map) )
                break;
        }

        $diskInfo = array(
            "lun" => $i,
            "name" => $DiskName,
            "createOption" => "attach",
            "managedDisk" => array(
                "id" => "/subscriptions/".$this->SubscriptionId."/resourceGroups/".$this->ResourceGroup."/providers/Microsoft.Compute/disks/".$DiskName,
                "storageAccountType" => "Standard_LRS"
            )
        );

        array_push( $VM_info['properties']['storageProfile']['dataDisks'], $diskInfo );
    }

    public function AddVHDConfig( &$VM_info, $diskSize, $storageAccount, $container, $filename, $blobEndpoint  ) {

        $map = array();
        foreach( $VM_info['properties']['storageProfile']['dataDisks'] as $DiskInfo ) {
            $map[ $DiskInfo['lun'] ] = true; 
        }

        for( $i = 0;; $i++ ){
            if( !array_key_exists( $i , $map) )
                break;
        }

        $diskInfo = array(
            "lun" => $i,
            "diskSizeGB" => $diskSize,
            "createOption" => "attach",
            "name" => $filename,
            "vhd" => array(
                "uri" => "https://".$storageAccount.".blob.".$blobEndpoint."/".$container."/".$filename
            )
        );

        array_push( $VM_info['properties']['storageProfile']['dataDisks'], $diskInfo );
    }

    public function RemoveDiskConfig( &$VM_info, $rmDisk ) {

        for($i = 0; $i < count( $VM_info["properties"]["storageProfile"]["dataDisks"] ); $i++ ) {
            if( strcmp( $VM_info["properties"]["storageProfile"]["dataDisks"][$i]["name"], $rmDisk ) == 0 )
                array_splice($VM_info["properties"]["storageProfile"]["dataDisks"], $i, 1);
        }
    }

    public function AttachDiskToVM( $tarVMName, $DiskNameArray ) {

        $vm = $this->getNameAndRGFromId( $tarVMName );

        $VM_info = json_decode( $this->GetVMInformation( $vm["name"], $vm["rg"] ), true );

        foreach( $DiskNameArray as $DiskName )
            $this->AddDiskConfig( $VM_info, $DiskName );

        $json = array( 
            "properties" =>array(
                "storageProfile" => array(
                    "dataDisks" => $VM_info['properties']['storageProfile']['dataDisks']
                )
            ),
            "location" => $this->Location
        );

        $URL = 'https://'.$this->ControlUrl.'/'.
            'subscriptions/'.$this->SubscriptionId.
            '/resourceGroups/'.$vm["rg"].
            '/providers/Microsoft.Compute/virtualMachines/'.$vm["name"].'?'.
            'api-version=2016-04-30-preview';

        while( true ){

            try{
                return $this->ProcSendData( $URL, 'PUT', json_encode($json, JSON_UNESCAPED_SLASHES) );
            }   
            catch( Exception $e ){

                $errorMsg = $e->getMessage();

                if( strpos( $errorMsg, "AttachDiskWhileBeingDetached" ) === false )
                    throw new Exception( $errorMsg );

                sleep(15);
            }
        }
    }

    public function AttachVHDToVM( $tarVMName, $DiskInfos, $connectionString ) {

        $VM_info = json_decode( $this->GetVMInformation( $tarVMName ), true );

        $blobEndpoint = $this->parseConnectionString( $connectionString );

        foreach( $DiskInfos as $DiskInfo )
            $this->AddVHDConfig( $VM_info, $DiskInfo["diskSize"], 
                    $DiskInfo["storageAccount"], $DiskInfo["container"], $DiskInfo["filename"],
                    $blobEndpoint["EndpointSuffix"] );

        $json = array( 
            "properties" =>array(
                "storageProfile" => array(
                    "dataDisks" => $VM_info['properties']['storageProfile']['dataDisks']
                )
            ),
            "location" => $this->Location
        );

        $URL = 'https://'.$this->ControlUrl.'/'.
            'subscriptions/'.$this->SubscriptionId.
            '/resourceGroups/'.$this->ResourceGroup.
            '/providers/Microsoft.Compute/virtualMachines/'.$tarVMName.'?'.
            'api-version=2016-04-30-preview';

        while( true ){

            try{
                return $this->ProcSendData( $URL, 'PUT', json_encode($json, JSON_UNESCAPED_SLASHES) );
            }   
            catch( Exception $e ){

                $errorMsg = $e->getMessage();

                if( strpos( $errorMsg, "AttachDiskWhileBeingDetached" ) === false )
                    throw new Exception( $errorMsg );

                sleep(15);
            }
        }
    }

    public function DetachDiskFromVM( $tarVMName, $DiskNameArray ) {

        $vm = $this->getNameAndRGFromId( $tarVMName );

        $VM_info = json_decode( $this->GetVMInformation( $vm["name"], $vm["rg"] ), true );

        foreach( $DiskNameArray as $DiskName )
            $this->RemoveDiskConfig( $VM_info, $DiskName );

        $json = array( 
            "properties" =>array(
                "storageProfile" => array(
                    "dataDisks" => $VM_info['properties']['storageProfile']['dataDisks']
                )
            ),
            "location" => $this->Location
        );

        $URL = 'https://'.$this->ControlUrl.'/'.
            'subscriptions/'.$this->SubscriptionId.
            '/resourceGroups/'.$vm["rg"].
            '/providers/Microsoft.Compute/virtualMachines/'.$vm["name"].'?'.
            'api-version=2016-04-30-preview';

        while( true ){

            try{
                return $this->ProcSendData( $URL, 'PUT', json_encode($json, JSON_UNESCAPED_SLASHES) );
            }
            catch( Exception $e ){

                $errorMsg = $e->getMessage();

                if( strpos( $errorMsg, "AttachDiskWhileBeingDetached" ) === false )
                    throw new Exception( $errorMsg );

                sleep(15);
            }
        }
    }

    public function CreateResourceGroup( $rgName ) {

        $URL = 'https://'.$this->ControlUrl.'/'.
            'subscriptions/'.$this->SubscriptionId.
            '/resourceGroups/'.$rgName.'?'.
            'api-version=2016-09-01';

        $json = array(
            "location" => $this->Location
        );
        return $this->ProcSendData( $URL, 'PUT', json_encode($json, JSON_UNESCAPED_SLASHES) );
    }

    public function DeleteResourceGroup( $rgName ) {

        $URL = 'https://'.$this->ControlUrl.'/'.
            'subscriptions/'.$this->SubscriptionId.
            '/resourceGroups/'.$rgName.'?'.
            'api-version=2016-09-01';

        return $this->ProcSendData( $URL, 'DELETE','' );
    }

    public function ListLocation() {
        
        $URL = 'https://'.$this->ControlUrl.'/'.
            'subscriptions/'.$this->SubscriptionId.'/'.
            'locations?'.
            'api-version=2016-06-01';

        return $this->ProcSendData( $URL, 'GET', null );
    }

    public function ListSubscription() {

        $URL = 'https://'.$this->ControlUrl.'/'.
            'subscriptions?'.
            'api-version=2014-04-01-preview';

        return $this->ProcSendData( $URL, 'GET', null );
    }

    public function AuthAzure( $CloudId ) {

        $this->CloudId = $CloudId;
        
        $AzureModel = new Common_Model();

        $cloud = $AzureModel->query_cloud_connection_information( $CloudId );

        $url = $cloud["DEFAULT_ADDR"];

        if( $cloud["DEFAULT_ADDR"] == "None" )
            $url = "azure_international";

        $azureInfo = Misc_Class::define_mgmt_setting();

        $this->ControlUrl  = $azureInfo->$url->Auzre_ControlUrl;
        $this->LoginUrl    = $azureInfo->$url->Auzre_LoginUrl;
        $this->ResourceUrl = $azureInfo->$url->Azure_Resource;

        $cloud_info = json_decode( $cloud['ACCESS_KEY'], true );

        $key_info = json_decode( $cloud['SECRET_KEY'], true );
        
        $token_info = json_decode( $cloud['USER_UUID'], true );

        $this->DefaultConfig( $cloud_info['TENANT_ID'],$cloud_info['SUBSCRIPTION_ID'],$key_info['ACCESS_KEY'],$key_info['SECRET_KEY']);

        if( $cloud['USER_UUID'] == '' ||
            ( $token_info["expires_on"] -100 ) < strtotime( Misc_Class::current_utc_time() ) 
            ) {

            $token_info = $this->GetOAuth2Token();

            $AzureModel->update_cloud_token_info( $CloudId, $token_info );
        }
        else
            $this->AccessToken = $token_info["access_token"];

    }

    public function IsAzureStack( $CloudId ) {

        $this->CloudId = $CloudId;
        
        $AzureModel = new Common_Model();

        $cloud = $AzureModel->query_cloud_connection_information( $CloudId );

        $url = $cloud["DEFAULT_ADDR"];

        if( $cloud["DEFAULT_ADDR"] != "None" && $cloud["DEFAULT_ADDR"] != "azure_international" && $cloud["DEFAULT_ADDR"] != "azure_china")
            return true;

        return false;
    }

    public function listDataModeInstance( $cloudId, $serverId ){

        $serverVm = $this->getVMByServUUID( $serverId, $cloudId );
        
        $vm_list = $this->describe_all_instances( $cloudId );

        $vms = array();

        foreach( $vm_list as $vm ){
            if( $vm["location"] == $serverVm["ret"]["location"] && 
                $vm["name"] != $serverVm["ret"]["name"] )
                array_push( $vms, $vm);
        }
        return $vms;
    }

    public function describe_all_instances( $Cloud_UUID ){

        $this->AuthAzure( $Cloud_UUID );

        $vm_list = json_decode( $this->ListVMISubscription(), true);

        $vm_info = array();

        foreach( $vm_list['value'] as $instance ){
            $vm = $this->getVMdetailInfo( $instance );
            array_push( $vm_info, $vm);
        }

        return $vm_info;
    }

    public function getVMdetailInfo( $instance ) {

        $vm_info = array();

        $net_all = json_decode( $this->GetNetworkLsit( ), true); 
        
        $p_ip_all = json_decode( $this->GetPublicIPLsit(  ), true );

        $vm_info['name'] = $instance['name'];
        $vm_info['id'] = $instance['id'];
        $vm_info['type'] = $instance['properties']['hardwareProfile']['vmSize'];

        if( isset( $instance['properties']['diagnosticsProfile']['bootDiagnostics']['enabled'] ) )
            $vm_info['enable'] = $instance['properties']['diagnosticsProfile']['bootDiagnostics']['enabled'];
        
        $vm_info['enable'] = true;
        $vm_info['location'] = $instance['location'];
        //$vm_info['managed'] = isset($instance['properties']['storageProfile']['osDisk']['managedDisk'])?true:false;
        $vm_info['managed'] = isset($instance['properties']['storageProfile']['osDisk']['vhd'])?false:true;

        $rg = explode('/',$instance['id']);
        $vm_info['resource_group'] = $rg[4];

        $vm_info['netInterface'] = array();

        foreach( $instance['properties']['networkProfile']['networkInterfaces'] as $network_iterface )
        {
             $net = explode('/',$network_iterface['id']);
            
             $netArray = array();

             foreach( $net_all['value'] as $net_detail ){

                 if( $net_detail['name'] == $net[8] ){
                     $vm_info['network_interface'] = $net[8];
                     $vm_info['private_ip'] = $net_detail['properties']['ipConfigurations'][0]['properties']['privateIPAddress'];
                     if( isset($net_detail['properties']['macAddress']) )
                        $vm_info['mac'] = $net_detail['properties']['macAddress'];

                    if( isset( $net_detail['properties']['networkSecurityGroup']['id'] ) )
                        $vm_info['security_group'] = explode('/',$net_detail['properties']['networkSecurityGroup']['id'])[8];

                    if( isset( $net_detail['properties']['ipConfigurations'][0]['properties']['publicIPAddress']['id'] ) )
                        $publicIPName =  explode('/',$net_detail['properties']['ipConfigurations'][0]['properties']['publicIPAddress']['id']);

                     $netArray['network_interface'] = $net[8];
                     if( isset( $net_detail['properties']['ipConfigurations'][0]['properties']['privateIPAddress']  ) )
                        $netArray['private_ip'] = $net_detail['properties']['ipConfigurations'][0]['properties']['privateIPAddress'];
                    if( isset($net_detail['properties']['macAddress']) )
                        $netArray['mac'] = $net_detail['properties']['macAddress'];
                    
                    if( isset( $net_detail['properties']['networkSecurityGroup']['id'] ) )
                        $netArray['security_group'] = explode('/',$net_detail['properties']['networkSecurityGroup']['id'])[8];
                        
                     //$publicIPName =  explode('/',$net_detail['properties']['ipConfigurations'][0]['properties']['publicIPAddress']['id']);
                    break;
                 }
            }

             foreach( $p_ip_all['value'] as $p_ip ){

                $p_ip_id = explode('/',$p_ip['id']);

                if( isset($publicIPName[8]) && $p_ip['name'] == $publicIPName[8] && $p_ip_id[4] == $publicIPName[4]){
                    if( isset( $p_ip['properties']['ipAddress'] ))
                        $vm_info['public_ip'] = $p_ip['properties']['ipAddress'];
                    else
                        $vm_info['public_ip'] = "None";

                    if (isset( $p_ip['properties']['ipAddress'] ) )
                        $netArray['public_ip'] = $p_ip['properties']['ipAddress'];
                    else
                        $netArray['public_ip'] = "None";
                    break;
                }
             }

            if( isset( $netArray['mac']) ) 
                array_push( $vm_info['netInterface'], $netArray);
        }   

        return $vm_info;
    }

    public function describe_instance( $CLOUD_UUID, $HOST_REGN, $HOST_NAME, $RG = null) {

        if( $HOST_NAME == '' )
            return false;
            
        $this->SetLocation( $HOST_REGN );
        
        $this->AuthAzure( $CLOUD_UUID );

        if( isset( $RG ) )
            $this->SetResourceGroup( $RG );

        $vm_info = $this->GetVMInformation( $HOST_NAME );

        $ret = $this->getVMdetailInfo(json_decode( $vm_info, true ));

        return $ret;
    }

    public function describe_internal_network( $CLOUD_UUID, $zone, $selectNetwork = null ) {

        $this->SetLocation( $zone );
        
        $this->AuthAzure( $CLOUD_UUID );

        $virtualNetworks = json_decode( $this->GetVirtualNetworkInSubscription(), true );
        
        $subnetArray = array();

        if( isset($selectNetwork) ){

            $net = explode( '|', $selectNetwork );

            $virtualNetwork = $net[0];

            $subnet = $net[1];

            $this->SetResourceGroup( $net[2] );

            $subnet_info = json_decode( $this->GetSubnetDetail( $virtualNetwork, $subnet ), true );
            
            return $subnet_info;
        }

        foreach( $virtualNetworks["value"] as $virtualNetwork ) {

            if( strcmp( $virtualNetwork["location"], $zone ) == 0 ) {

                $id_info = explode( '/', $virtualNetwork["id"]);

                $this->SetResourceGroup( $id_info[4] );

                $subnets = json_decode( $this->GetSubnetInVirtualNetwork( $virtualNetwork["name"] ), true );

                foreach( $subnets["value"] as $subnet )
                    array_push( $subnetArray, $subnet );
            }
        }
        
        return $subnetArray;
    }

    public function begin_azure_volume_for_loader($CLUSTER_UUID,$ZONE_INFO,$VM_NAME,$VOLUME_SIZE,$HOST_NAME) {

        $this->SetLocation( $ZONE_INFO );

        $this->AuthAzure( $CLUSTER_UUID );

        $NOW_TIME = Misc_Class::current_utc_time();	

        $tags = array( "Description" => "Disk Created By Saasame Transport Service@".$NOW_TIME,
                        "Name" => $HOST_NAME.'@'.$NOW_TIME);

        $diskIndex = end( explode('_', $HOST_NAME ) );

        $HOST_NAME = "SaaSaMe-".$this->ReplicaId.'-'.$diskIndex.'-'.strtotime($NOW_TIME);

        $this->CreateDiskI( $HOST_NAME, $VOLUME_SIZE, $tags );

        do{
            $disk = json_decode( $this->GetDisksDetail( $HOST_NAME ), true );
        }while( (strcmp( $disk['properties']['provisioningState'], "Succeeded" ) != 0));

        $diskName = array( $HOST_NAME);

        $disk_list = $this->AttachDiskToVM( $VM_NAME, $diskName );
		
        do{
            $disk = json_decode( $this->GetDisksDetail( $HOST_NAME ), true );
        }while( (strcmp( $disk['properties']['diskState'], "Attached" ) != 0));

		$VOLUMD_INFO = (object)array(
								'serverId' 	 => $VM_NAME.'|'.$ZONE_INFO,							
								'volumeId'	 => $HOST_NAME,
								'volumeSize' => $VOLUME_SIZE,
								'volumePath' => $disk['id']
						);
		
		return $VOLUMD_INFO;
	}

    public function create_disk_snapshot($CLOUD_UUID,$ZONE_INFO,$DiskName,$HOST_NAME,$SNAP_TIME) {

        $this->SetLocation( $ZONE_INFO );

        $this->AuthAzure( $CLOUD_UUID );

		$tags = array( "Description" => "Snapshot Created By Saasame Transport Service@".$SNAP_TIME,
                        "Name" => $HOST_NAME.'@'.$SNAP_TIME);

        //$result = $this->CreateSnapshotI($HOST_NAME.'-'.strtotime($SNAP_TIME), $DiskName, $tags);
        $now = Misc_Class::current_utc_time();	

        $diskIndex = end( explode('_', $HOST_NAME ) );

        $snapshotName = "WS-".$this->ReplicaId.'-'.$diskIndex.'-'.strtotime($now);

        $result = $this->CreateSnapshotI( $snapshotName, $DiskName, $tags );
        
		return $result;
    }
    
    public function snapshot_control($CLOUD_UUID,$ZONE_INFO,$VOLUME_ID, $NUMBER_SNAPSHOT) {

        $this->SetLocation( $ZONE_INFO );

        $this->AuthAzure( $CLOUD_UUID );

        $snapshots = $this->ListSnapshotsInSubscriptions( );

        $snapshots_array = json_decode( $snapshots, true );

		if ($snapshots_array != FALSE)
        {
            $hit_snapshot = $this->FilterSnapshotsInDisk( $snapshots_array, $VOLUME_ID);

            usort( $hit_snapshot, array("Azure_Controller", "snapshot_cmp"));

		    $SLICE_SNAPSHOT = array_slice( $hit_snapshot, $NUMBER_SNAPSHOT );

		    for ($i=0; $i<count($SLICE_SNAPSHOT); $i++)
			    $this->DeleteSnapshotI($SLICE_SNAPSHOT[$i]['name']);				
	    }
    }

    static function snapshot_cmp( $a, $b) {

        if( strtotime( $a["properties"]["timeCreated"] ) == strtotime( $b["properties"]["timeCreated"] ) )
            return 0;

        return (strtotime( $a["properties"]["timeCreated"] ) > strtotime( $b["properties"]["timeCreated"] )) ? -1 : +1;
    }

    public function delete_replica_job( $CLOUD_UUID, $INSTANCE_REGN, $INSTANCE_UUID, $DiskInfoArray ) {

        $this->SetLocation( $INSTANCE_REGN );

        $this->AuthAzure( $CLOUD_UUID );

        $rm_disk_array = array();

        foreach( $DiskInfoArray as $diskInfo ) 
            array_push( $rm_disk_array, $diskInfo['OPEN_DISK_UUID'] );
        
        $this->DetachDiskFromVM( $INSTANCE_UUID, $rm_disk_array );

        $this->UntilDiskUnattach( $rm_disk_array );

        $snapshots = $this->ListSnapshotsInSubscriptions( );

        $snapshots_array = json_decode( $snapshots, true );

        foreach( $rm_disk_array as $diskName ) {

            $hit_snapshot = $this->FilterSnapshotsInDisk( $snapshots_array, $diskName, true);

            $this->DeleteDiskI( $diskName );
        }
    }

    public function UntilDiskUnattach( $diskArray) {

        foreach( $diskArray as $diskName ) {

            $count = 0;

            do{
                $disk = json_decode( $this->GetDisksDetail( $diskName ), true );

                $count++;

                if( $count >= 20 ) {
                    
                    if( isset( $this->ReplicaId ) )
                        $this->showReplicaLog( $this->ReplicaId, "Waiting for disk dettach timeout.". $diskName);
                    else
                        Misc_Class::function_debug('Azure', "Timeout log", "Waiting for disk dettach timeout.". $diskName);

                    break;
                }
                else
                    sleep(20);

            }while( (strcmp( $disk['properties']['diskState'], "Unattached" ) != 0));

        }
    }

/*
 * @brief   filter snapshot which create from specified volume
 * @param[in]   snapshotArray   snapshots array
 * @param[in]   diskName        volume name
 * @param[in,option]   delete   if true, delete the snapshot which created by specified volume
 * @return  filter result of snapshots information
 */

    public function FilterSnapshotsInDisk( $snapshotArray, $diskName , $delete = false) {

        $hit_snapshot = array();

        foreach( $snapshotArray["value"] as $snapshot ){

            if( isset( $snapshot["properties"]["creationData"]["sourceUri"] ))
                $objectId = $snapshot["properties"]["creationData"]["sourceUri"];
            else
                $objectId = $snapshot["properties"]["creationData"]["sourceResourceId"];

            $snapshot_source_name = explode('/',$objectId)[8];
            
            if( strcmp( $snapshot_source_name, $diskName ) == 0 ){
                array_push( $hit_snapshot, $snapshot);
                if( $delete )
                    $this->DeleteSnapshotI($snapshot['name']);
            }
        }

        return $hit_snapshot;
    }

/*
 * @brief   describe snapshot information by volume
 * @param[in]   CLOUD_UUID      service information, include cloud uuid, packer uuid...
 * @param[in]   HOST_REGN       zone
 * @param[in]   VOLUME_ID       volume name
 * @return  snapshot information
 */

    public function describe_snapshots( $CLOUD_UUID,$HOST_REGN,$VOLUME_ID ) {

        $this->SetLocation( $HOST_REGN );

        $this->AuthAzure( $CLOUD_UUID );

        $diskInfo = $this->getNameAndRGFromId( $VOLUME_ID );
        
        $snapshots = $this->ListSnapshotsInSubscriptions( );

        $snapshots_array = json_decode( $snapshots, true );

        $hit_snapshot = $this->FilterSnapshotsInDisk( $snapshots_array,  $diskInfo["name"] );

        return $hit_snapshot;
    }

/*
 * @brief   describe snapshot information by snapshot name
 * @param[in]   CLOUD_UUID      service information, include cloud uuid, packer uuid...
 * @param[in]   HOST_REGN       zone
 * @param[in]   SnapshotName    formate is "snapshot name | resource group"
 * @return  snapshot information
 */

    public function describe_snapshot_detail( $CLOUD_UUID,$HOST_REGN,$SnapshotName ) {

        $this->SetLocation( $HOST_REGN );

        $this->AuthAzure( $CLOUD_UUID );

        $data = explode( '|', $SnapshotName );

        if( isset( $data[1] ) )
            $this->SetResourceGroup( $data[1] );

        $snapshots = $this->GetSnapshotDetail( $data[0] );

        return json_decode( $snapshots, true );
    }

/*
 * @brief   describe instance type
 * @param[in]   CLOUD_UUID      service information, include cloud uuid, packer uuid...
 * @param[in]   zone            zone
 * @param[in,option]   type     if null, list all, otherwise describe the specified one
 * @return  instance type information
 */

    public function describe_instance_types( $CLOUD_UUID, $zone, $type = null ) {

        $this->SetLocation( $zone );

        $this->AuthAzure( $CLOUD_UUID );

        $size = $this->GetVMSize();

        $ret = json_decode( $size, true )['value'];

        if( !isset( $type ) ) {
            return $ret;
        }

        foreach( $ret as $vm_type ) {
            if( strcmp( $vm_type["name"], $type ) == 0 )
                return $vm_type;
        }

        return false;
    }

/*
 * @brief   describe network interface
 * @param[in]   CLOUD_UUID      service information, include cloud uuid, packer uuid...
 * @param[in,option]   NETWORK_UUID    if null, list all, otherwise describe the specified one
 * @return  network interface information
 */

    public function describe_available_network( $CLOUD_UUID ,$NETWORK_UUID = null) {

        $this->AuthAzure( $CLOUD_UUID );

        $net_interface = $this->GetNetworkLsit();

        $ret = json_decode( $net_interface, true )['value'];

        $eth_array = array();

        if( !isset( $NETWORK_UUID ) ) {

            foreach( $ret as $net_interface ) {

                if( !isset( $net_interface["properties"]["virtualMachine"] ) ) 
                    array_push( $eth_array, $net_interface);
                
            }
            return $eth_array;
        }

        foreach( $ret as $net_interface ) {
            if( strcmp( $net_interface["name"], $NETWORK_UUID ) == 0 )
                return $net_interface;
        }

        return false;
    }

/*
 * @brief   describe security group
 * @param[in]   CLOUD_UUID      service information, include cloud uuid, packer uuid...
 * @param[in]   zone            zone
 * @param[in,option]   SGROUP_UUID    if null, list all, otherwise describe the specified one
 * @return  security group information
 */

    public function describe_security_groups( $CLOUD_UUID, $zone, $SGROUP_UUID = null ) {

        $this->AuthAzure( $CLOUD_UUID );

        $security_groups = $this->GetNetworkSecurityGroupsLsit();

        $ret = json_decode( $security_groups, true )['value'];

        if( isset( $SGROUP_UUID ) ) {
            foreach( $ret as $security_group ) {
                if( strcmp( $security_group["id"], $SGROUP_UUID ) == 0 )
                    return $security_group;
            }
        }

        $filter_array = array();

        foreach( $ret as $security_group ) {
            if( strcmp( $security_group["location"], $zone ) == 0 )
                array_push( $filter_array, $security_group);
        }

        return $filter_array;
    }

/*
 * @brief   create volume from snapshot
 * @param[in]   CLOUD_UUID      service information, include cloud uuid, packer uuid...
 * @param[in]   SERVER_ZONE     zone
 * @param[in]   SnapshotName    formate is "snapshot name | resource group"
 * @param[in]   HOST_NAME       vm name, for named volume
 * @return  volume name
 */

    public function create_volume_from_snapshot( $CLOUD_UUID,$SERVER_ZONE,$SnapshotName,$HOST_NAME ) {
    
        $this->SetLocation( $SERVER_ZONE );

        $this->AuthAzure( $CLOUD_UUID );

        $data = explode( '|', $SnapshotName );

        $snap_name = $data[0];

        $this->SetResourceGroup( $data[1] );

        $NOW_TIME = Misc_Class::current_utc_time();	

        $DiskName = "snap-".$HOST_NAME."_".strtotime($NOW_TIME);
        
        $tags = array( "Description" => "Snapshot Volume Created By Saasame Transport Service@".$NOW_TIME,
                        "Name" => $DiskName);

        $diskIndex = end( explode('_', $HOST_NAME ) );

        $cloudDiskName = "SaaSaMe-".$this->ReplicaId.'-'.$diskIndex.'-'.strtotime($NOW_TIME);

        //$cloudDiskName = strtotime($NOW_TIME);

        $this->CreateDiskFromSnapshot( $cloudDiskName , $snap_name, $tags );

       do{
            $disk = json_decode( $this->GetDisksDetail( $cloudDiskName ), true );
        }while( (strcmp( $disk['properties']['provisioningState'], "Succeeded" ) != 0));

        return $disk['name'];
    }

/*
 * @brief  attach volume and wait for the disk status is attached
 * @param[in]   CLOUD_UUID      service information, include cloud uuid, packer uuid...
 * @param[in]   SERVER_ZONE     zone
 * @param[in]   vmName          vm name
 * @param[in]   diskName        volume array, will attach to the vm
 * @param[in,option]   retry    count of retry to check the disk status is attached
 * @return  none
 */

    public function attach_volume( $CLOUD_UUID,$SERVER_ZONE,$vmName,$diskName, $retry = 10 ) {

        $this->SetLocation( $SERVER_ZONE );

        $this->AuthAzure( $CLOUD_UUID );

        $diskName_array = array( $diskName );

        $disk_list = $this->AttachDiskToVM( $vmName, $diskName_array );

        $count = 0;

        do{
            $disk = json_decode( $this->GetDisksDetail( $diskName ), true );

            if( $count >= $retry )
                break;
                
            if( $count != 0 )
                sleep( 5 );

            if( $disk['properties']['diskState'] == "Reserved" || $disk['properties']['diskState'] == "Attached" )
                break;

        }while( true );

        return $disk['name'];
    }

    public function getNameAndRGFromId( $id ){

        $vm = explode('/', $id );

        if( count( $vm ) >= 8 )
            return array( "name" => $vm[8], "rg" => $vm[4] );
        else
            return array( "name" => $id, "rg" => $this->ResourceGroup );
    }

/*
 * @brief  attach volume and wait for the disk status is attached
 * @param[in]   CLOUD_UUID      service information, include cloud uuid, packer uuid...
 * @param[in]   SERVER_ZONE     zone
 * @param[in]   vmName          vm name
 * @param[in]   diskName        volume array, will attach to the vm
 * @param[in,option]   retry    count of retry to check the disk status is attached
 * @return  none
 */

public function attach_vhd( $CLOUD_UUID, $SERVER_ZONE, $vmName, $diskinfo, $connectionString, $retry = 10 ) {

    $this->SetLocation( $SERVER_ZONE );

    $this->AuthAzure( $CLOUD_UUID );

    $diskinfo_array = array( $diskinfo );

    $disk_list = $this->AttachVHDToVM( $vmName, $diskinfo_array, $connectionString );

    return $diskinfo['filename'];
}

/*
 * @brief  detach volume and wait for the disk status is unattach
 * @param[in]   CLOUD_UUID      service information, include cloud uuid, packer uuid...
 * @param[in]   zone            zone
 * @param[in]   vm_name         vm name
 * @param[in]   rm_disk_array   volume array, first one is os, the others are data volume
 * @return  none
 */

    public function detach_volume($CLOUD_UUID, $zone, $vm_name, $rm_disk_array) {
        
        $this->SetLocation( $zone );

        $this->AuthAzure( $CLOUD_UUID );

        $this->DetachDiskFromVM( $vm_name, $rm_disk_array );

        $this->UntilDiskUnattach( $rm_disk_array );
    }

    public function detach_vhd($CLOUD_UUID, $zone, $vm_name, $rm_disk_array) {
        
        $this->SetLocation( $zone );

        $this->AuthAzure( $CLOUD_UUID );

        $this->DetachDiskFromVM( $vm_name, $rm_disk_array );

        //$this->UntilDiskUnattach( $rm_disk_array );
    }

/*
 * @brief  create net interface and public ip address for vm. and create vm.
 * @param[in]   serv_info       service information, include cloud uuid, packer uuid...
 * @param[in]   zone            zone
 * @param[in]   volumn_array    volume array, first one is os, the others are data volume
 * @param[in]   vm_name         vm name
 * @return  vm name
 */

    public function run_instance_from_disk( $serv_info, $zone, $volumn_array, $vm_name, $storageInfo = null ) {

        $this->SetLocation( $zone );

        $this->AuthAzure( $serv_info['CLUSTER_UUID'] );
		
        $NOW_TIME = Misc_Class::current_utc_time();	

        $serviceConfig = json_decode($serv_info['JOBS_JSON'], true);
        $net = explode( '|', $serv_info['NETWORK_UUID'] );

        $netInterfaceName = 'nic_'.strtotime($NOW_TIME);

        $allowPublicIP = $serviceConfig["elastic_address_id"];

        $specifiedPrivateIP = $serviceConfig["private_address_id"];

        $sgroup = explode( '/', $serv_info['SGROUP_UUID'] );

        $publicIp = null;

        $privateIp = null;

        if( $allowPublicIP != "No") {

            $publicIpName = 'publicIp_'.strtotime($NOW_TIME);

            $publicIp = json_decode( $this->CreatePublicIp( $publicIpName ), true )["name"];

        }

        if( $specifiedPrivateIP != "DynamicAssign") {
            $privateIp = $specifiedPrivateIP;
        }
        
        $NetworkInterface = $this->CreateNetworkInterface( $netInterfaceName, $sgroup[8], $net[0], $net[1], $publicIp, $privateIp);

        $osDiak = array_shift( $volumn_array );

        $dataDisk = $volumn_array;

        //$name = $vm_name.'_'.strtotime($NOW_TIME);
        //$name = "RW-".strtotime($NOW_TIME);
        $name = $serviceConfig["hostname_tag"];
        
        $rgVMs = $this->getVMInResourceGroup( $serv_info['CLUSTER_UUID'], $this->ResourceGroup );

        foreach($rgVMs["value"] as $vm){
			if( end( explode( '/',$vm["id"] ) ) == $name )
                $name .= '-'.(string)rand(0, 9999);
        }

        $param = array(
            "HostName" => $vm_name,
            "OsType" => ( $serv_info["OS_TYPE"] == "MS" )?"Windows":"Linux",
            "AvailabilitySetId" => $serviceConfig["availibility_set"],
            "DiskType" => $serviceConfig["disk_type"]
        );

        if( $storageInfo ){
            sleep(60);
            $vm_info = $this->CreateVMFromVHD( $name , $osDiak, 
                $serv_info['FLAVOR_ID'], $netInterfaceName, $storageInfo, $dataDisk, $param );
        }
        else{
            $vm_info = $this->CreateVMFromDisk( $name , $osDiak, 
                $serv_info['FLAVOR_ID'], $netInterfaceName, $dataDisk, $param );
        }

        $this->UntilVmStart( $name , 10 );

        return json_decode( $vm_info, true )["name"];
    }

/*
 * @brief  wait for the vm running
 * @VmName[in]              vm name
 * @loopCount[in,option]    how many times to retry, default = 10
 * @return  true
 */

    public function UntilVmStart( $VmName , $loopCount = 10) {

        $count = 0;
        do{

            $vm = json_decode( $this->GetVMInstanceInformation( $VmName ), true );

            $ready = false;
            foreach( $vm["statuses"] as $status ) {
                $info = explode( '/', $status["code"] );
                switch( $info[0] )
                {
                    case "ProvisioningState":
                        if( $info[1] == "succeeded" ){
                            //do nothing
                        }
                    break;
                    case "PowerState":
                        if( $info[1] == "running" ){
                            $ready = true;
                        }
                    break;
                    default:
                    break;
                }
            }

            $count++;  
            if( $ready )
                break;
            
            sleep( 10 );
                
        }while( $loopCount > $count  );

        return true;
    }

/*
 * @brief  terminate instance then clean up the network interface and public ip address
 * @param[in]   cloud_uuid      _CLUSTER_UUID in CLOUD_MGMT table
 * @param[in]   zone            zone
 * @param[in]   vm_name         vm name
 * @return  none
 */

    public function terminate_instances($cloud_uuid, $zone ,$vm_name ) {

        $this->SetLocation( $zone );

        $this->AuthAzure( $cloud_uuid );

        $VM_info = json_decode( $this->GetVMInformation( $vm_name ), true );
        
        $network_iterface_array = $VM_info["properties"]["networkProfile"]["networkInterfaces"];

        $this->DeleteVM( $vm_name );

        sleep(30);

        $flag = true;
        
        do{
            sleep(5);

            $VM_info = json_decode( $this->GetVMInformation( $vm_name ), true );

            if( isset($VM_info["success"]) && ($VM_info["success"] == false) )
                $flag = false;

        }while( $flag );

        foreach( $network_iterface_array as $net ) {
            $netId = explode( '/', $net["id"] );
            $net_name = $netId[8];

            $netInfo = json_decode( $this->GetNetworkInterface( $net_name ), true );

            if( isset( $netInfo["tags"]["factory"] ) && $netInfo["tags"]["factory"] == "Created by SaaSaMe." ) {

                $this->DeleteNetworkInterface( $net_name );

                sleep( 5 );

                foreach( $netInfo["properties"]["ipConfigurations"] as $ipconfig ){
                    $ipId = explode( '/', $ipconfig["properties"]["publicIPAddress"]["id"] );
                    $publicIpName = $ipId[8];
                    $this->DeletePublicIp( $publicIpName );
                }
            }
        }
    }

/*
 * @brief  delete specified snapshot
 * @param[in]   cloud_uuid      _CLUSTER_UUID in CLOUD_MGMT table
 * @param[in]   zone            zone
 * @param[in]   snapshot_name   snapshot name
 * @return  none
 */

    public function delete_snapshot( $cloud_uuid, $zone, $snapshot_name ) {

        $this->SetLocation( $zone );

        $this->AuthAzure( $cloud_uuid );

        $this->DeleteSnapshotI( $snapshot_name );
    }

/*
 * @brief  delete disk until the disk status is "Unattached"
 * @param[in]   cloud_uuid      _CLUSTER_UUID in CLOUD_MGMT table
 * @param[in]   zone            zone
 * @param[in]   disk_Name       disk name
 * @return  none
 */

    public function delete_volume( $cloud_uuid, $zone, $disk_Name ) {

        $this->SetLocation( $zone );

        $this->AuthAzure( $cloud_uuid );

        try{
            $disk = json_decode( $this->GetDisksDetail( $disk_Name ), true );
        }catch( Exception $e ){
            return false;
        }

        if( strcmp( $disk['properties']['diskState'], "Attached" ) == 0 || 
        strcmp( $disk['properties']['diskState'], "Reserved" ) == 0) {

            $vmName = explode( '/', $disk['properties']['ownerId'])[8];
            
            $rm_disk_array = array();
            
            array_push( $rm_disk_array, $disk_Name );
            
            if( isset( $disk['properties']['ownerId'] ) )
                $ownerId = $disk['properties']['ownerId'];
            else
                $ownerId = $disk['managedBy'];

            $this->DetachDiskFromVM( $ownerId, $rm_disk_array );
            
        }
        $count = 0;

        do{
            $disk = json_decode( $this->GetDisksDetail( $disk_Name ), true );

            $count++;

            if( $count >= 10 )
                break;
            else
            sleep(10);

        }while( (strcmp( $disk['properties']['diskState'], "Unattached" ) != 0));

        $this->DeleteDiskI( $disk_Name );
    }

/*
 * @brief  get new oauth2 token
 * @param[in]   tenantId            tenant id
 * @param[in]   cliendId            client id
 * @param[in]   clientSecret        client secret
 * @param[in]   subscriptionId      subscription id
 * @return  token information
 */

    public function GetNewOAuthTokenInfo( $tenantId, $cliendId, $clientSecret, $subscriptionId) {
        
        $this->DefaultConfig( $tenantId, $subscriptionId, $cliendId, $clientSecret);

        $token_info = $this->GetOAuth2Token();

       return $token_info;
    }

/*
 * @brief  get vm detail by name, location and private ip
 * @param[in]   VmName          vm name
 * @param[in]   zone            zone
 * @param[in]   ip              private ip address
 * @param[in]   cloud_uuid      _CLUSTER_UUID in CLOUD_MGMT table
 * @return  vm information
 */

    public function checkInstance( $VmName, $zone, $ips, $cloud_uuid = null) {

        if( isset($cloud_uuid) )
            $this->AuthAzure( $cloud_uuid );

        $vm_list = $this->describe_all_instances( $cloud_uuid );

        //$list = json_decode( $vm_list, true );

        $target = null;

        foreach( $vm_list as $vm_info ) {
            if( strcmp( $vm_info["name"], $VmName ) == 0 &&
                strcmp( $vm_info["location"], $zone ) == 0
                )
            {
                foreach( $ips as $ip ){
                    if( strcmp( $vm_info["private_ip"], $ip ) == 0 ){
                        $target = $vm_info;
                        break 2;
                    }
                }
            }
        }
        
        if( $target )
            return array( "success" => true, "ret" => $target );
        
        return array( "success" => false, "ret" => null );
    }

/*
 * @brief  use server uuid to get the vm name from database and get vm detail
 * @param[in]   serv_uuid         server uuid in server table
 * @param[in]   cloud_uuid        _CLUSTER_UUID in CLOUD_MGMT table
 * @return  vm information
 */

    public function getVMByServUUID( $serv_uuid, $cloud_uuid) {

        $AzureModel = new Common_Model();
        
        $info = $AzureModel->getServerInfo( $serv_uuid );

        $serverInfo = json_decode( $info["serverInfo"],true );

        if( isset( $serverInfo["is_promote"] ) && $serverInfo["is_promote"] == true ) {
            $this->SetResourceGroup( $serverInfo["rg"] );
            $this->SetLocation( $serverInfo["location"] );
            return false;
        }

		$data = explode( '|', $info['HOST_UUID'] );

		$location = $data[1];

		$name = $data[0];

		$ip_list = json_decode( $info['SERV_ADDR'], true );

		$ip = $ip_list;

		$vm = $this->checkInstance( $name, $location, $ip, $cloud_uuid);

        if( $vm["success"] == true )
            $this->SetResourceGroup( $vm["ret"]["resource_group"] );

        return $vm;
    }

/*
 * @brief  set resource group, depend on vm name, location and ip to choice the vm. 
 * @param[in]   serverArray         server information in database
 * @return  none
 */

    public function SetResourceGroupByServerInfo( $serverArray ) {

        $data = explode( '|', $serverArray['HOST_UUID'] );

		$location = $data[1];

		$name = $data[0];

		$ip_list = json_decode( $serverArray['SERV_ADDR'], true );

		$ip = $ip_list;

		$vm = $this->checkInstance( $name, $location, $ip, $cloud_uuid);

        $this->SetResourceGroup( $vm["resource_group"] );

    }

    public function showReplicaLog( $rId, $msg ) {

        $replica   = new Replica_Class();	
        
        $mesage = $replica->job_msg( $msg );

        $replica->update_job_msg( $rId , $mesage, 'Replica');
    }

    public function getInstanceInfoForReport( $param ) {

        $this->AuthAzure( $param["cloudId"] );

        $this->getVMByServUUID( $param["serverId"], $param["cloudId"] );

        $instanceStatus = json_decode( $this->GetVMInformation( $param["instanceId"] ), true );

        $vmDetail = $this->getVMdetailInfo( $instanceStatus );

        if( isset( $instanceStatus["success"] ) && $instanceStatus["success"] == false )
            return false;
        
        $instanceTypeDetail = $this->describe_instance_types( $param["cloudId"], $param["region"], $instanceStatus["properties"]["hardwareProfile"]["vmSize"] );

        $report = array(
            "Instance Name" => $vmDetail["name"],
            "Flavor" => $vmDetail["type"],
            "Security Group" => $vmDetail["security_group"],
            "CPU Core" => $instanceTypeDetail["numberOfCores"],
            "Memory" => $instanceTypeDetail["memoryInMB"],
            "maxDataDiskCount" => $instanceTypeDetail["maxDataDiskCount"]
        );

        $report["NetworkInterfaces"] = array();

        $report["Public IP address"] = array();

        foreach( $vmDetail["netInterface"] as $NetworkInterface ) {

            $network = array(
                "MacAddress" => $NetworkInterface["mac"],
                "Private IP address" => $NetworkInterface["private_ip"],
                "NetworkInterfaceId" => $NetworkInterface["network_interface"],
            );

            array_push( $report["NetworkInterfaces"], $network );

            array_push( $report["Public IP address"], $NetworkInterface["public_ip"] );
        }

        return $report;
    }

    public function describe_volume( $cloud_uuid, $region, $diskName ) {

        $this->AuthAzure( $cloud_uuid );
     
        $disk = json_decode( $this->GetDisksDetail( $diskName ), true );

        $disks = array(array(
            "diskId" => $disk["id"],
            "diskName" => $disk["name"],
            "size" => $disk["properties"]["diskSizeGB"]
        ));

        return $disks;
    }

    public function CheckPrivateIp( $cloud_uuid, $virtualNetwork, $ipaddress ) {
        
        $this->AuthAzure( $cloud_uuid );

        try{
            $result = $this->CheckIpaddressAvailability($virtualNetwork, $ipaddress);

            return $result;
        }
        catch( Exception $e ){
            $errorMsg = $e->getMessage();

            $spos = strpos( $errorMsg, "No HTTP resource was found that matches the request URI" );

            if( $spos === false )
                return json_encode( array("available" => false) );
            
            return json_encode( array("available" => true) );
        }
    }

    public function CreateDisk_BlobMode( $cloud_uuid, $diskName , $connectionString, $container, $filename, $region, $size ) {
        
        $this->AuthAzure( $cloud_uuid );

        $this->SetLocation( $region );
		
        $filename = $filename.'.vhd';
        
        $tags = array( "factory"=>"Created by SaaSaMe");

        $disk = $this->CreateDiskFromUnmanagementDisk( $diskName , $connectionString, $container, $filename, $size, $tags );

        while( true ){
            $disk = json_decode( $this->GetDisksDetail( $diskName ), true );

            if( $disk["properties"]["provisioningState"] != "Creating" )
                break;
            
            sleep( 5 );
        }

        return $disk;
    }

    public function parseConnectionString( $connectionString ) {

        $return = array();

        $datas = explode( ';', $connectionString);

        foreach( $datas as $data ){
            $d = explode( '=', $data);
            $return[$d[0]] = $d[1];
        }

        return $return;
    }

    public function setPublicIpStatic( $CLOUD_UUID, $HOST_REGN, $HOST_NAME, $RG = null) {

        if( $HOST_NAME == '' )
            return false;
            
        $this->SetLocation( $HOST_REGN );
        
        $this->AuthAzure( $CLOUD_UUID );

        if( isset( $RG ) )
            $this->SetResourceGroup( $RG );

        $vm_info = json_decode( $this->GetVMInformation( $HOST_NAME ), true );

        $networkInterfaces = $vm_info["properties"]["networkProfile"]["networkInterfaces"];
       // $ret = $this->getVMdetailInfo(json_decode( $vm_info, true ));

       foreach( $networkInterfaces as $networkInterface ){

            $nInfo = explode( '/', $networkInterface["id"] );

            $this->SetResourceGroup( $nInfo[4] ) ;

            $networkInterfaceDetail = json_decode( $this->GetNetworkInterface( $nInfo[8] ), true );

            foreach( $networkInterfaceDetail["properties"]["ipConfigurations"] as &$ipConifg ) {

                $ipConifg["properties"]["privateIPAllocationMethod"] = "Static";

                $ipConifg["properties"]["publicIPAddress"]["publicIPAllocationMethod"]  = "Static";

                $publicIpInfo = explode( '/', $ipConifg["properties"]["publicIPAddress"]["id"] );

                //update public ip static
                $ret = $this->CreatePublicIp( $publicIpInfo[8], true );
            }

            //update NetworkInterface 
            $ret = $this->CreateNetworkInterface( $nInfo[8], null, null, null, null, null, $networkInterfaceDetail );

       }

        return json_decode($ret,true);
    }

    public function ListAvailabilitySet( $cloudId, $RG){

        $this->AuthAzure( $cloudId );

        $this->SetResourceGroup( $RG );
        
        $AS = $this->ListAvailabilitySetInRG();

        return json_decode( $AS, true );

    }

    public function getVMInResourceGroup( $cloudId ){

        $this->AuthAzure( $cloudId );

       // $this->SetResourceGroup( $RG );

        $vms = $this->ListVMInResourceGroup();

        return json_decode( $vms, true );

    }

    public function GetStorageAccountDetail( $accountName ){

        $accList = json_decode( $this->GetStorageAccountList(), true );

        foreach( $accList["value"] as $acc ){
            if( $acc["name"] == $accountName ){
                return $acc;
            }
        }

        return false;
    }
};

?>