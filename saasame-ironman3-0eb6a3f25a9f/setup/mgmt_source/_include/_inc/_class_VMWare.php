<?php
require_once '_class_thrift_controller.php';
require_once 'Common_Model.php';
require_once '_class_csvProc.php';
require_once 'OSType.php';

class VM_Ware
{

	use csvProc;
	use Thrift_Controller;

	public function __construct()
	{
		$this->initThriftClient();
	}

	/** 
 * get transport information
 * @param string $transportIp
*/
	public function getTransportServerInfo($transportIp)
	{

		$transportInfo = $this->Client->ping_p($transportIp);

		return $transportInfo;
	}

	/** 
 * get VMWare hosts
 * @param string $transportIp
 * @param string $EsxIp
 * @param string $username
 * @param string $password
*/
	public function getVirtualHosts($transportIp, $EsxIp, $username, $password)
	{

		$ret = false;
		foreach ($transportIp as $TransportAddr) {
			try {
				return $this->Client->get_virtual_hosts_p($TransportAddr, $EsxIp, $username, $password);
			} catch (invalid_operation $e) {

				$ret = array(
					"success" 	=> false,
					"format" 	=> $e->format,
					"arguments" => $e->arguments,
					"why" 		=> $e->why
				);
			} catch (Exception $e) {

				$ret = array(
					"success" 	=> false,
					"why" 		=> $e->getMessage()
				);
			}
		}

		return $ret;
	}

	/** 
 * get VMWare hosts
 * @param string $transportIp
 * @param string $EsxIp
 * @param string $username
 * @param string $password
*/
	public function getDatacenterFolderList($transportIp, $EsxIp, $username, $password, $datacenterName)
	{

		foreach ($transportIp as $TransportAddr) {
			try {
				return $this->Client->get_datacenter_folder_list_p($TransportAddr, $EsxIp, $username, $password, $datacenterName);
			} catch (Throwable $e) {
				continue;
			}
		}

		return false;
	}

	/** 
 * shot down VMWare hosts
 * @param array $transportIp
 * @param string $EsxIp
 * @param string $username
 * @param string $password
 * @param string $machine_id
*/
	public function powerOffVirtualMachine($transportIp, $EsxIp, $username, $password, $machine_id)
	{

		foreach ($transportIp as $TransportAddr) {
			if ($this->Client->power_off_virtual_machine_p($TransportAddr, $EsxIp, $username, $password, $machine_id) == true)
				return true;
		}

		return false;
	}

	/** 
 * remove VMWare hosts
 * @param array $transportIp
 * @param string $EsxIp
 * @param string $username
 * @param string $password
 * @param string $machine_id
*/
	public function removeVirtualMachine($transportIp, $EsxIp, $username, $password, $machine_id)
	{
		foreach ($transportIp as $TransportAddr) {
			if ($this->Client->remove_virtual_machine_p($TransportAddr, $EsxIp, $username, $password, $machine_id) == true)
				return true;
		}

		return false;
	}

	/** 
 * get VMWare hosts snapshots
 * @param string $transportIp
 * @param string $EsxIp
 * @param string $username
 * @param string $password
 * @param string $machine_id
*/
	public function getVirtualMachineSnapshots($transportIp, $EsxIp, $username, $password, $machine_id)
	{
		return $this->Client->get_virtual_machine_snapshots_p($transportIp, $EsxIp, $username, $password, $machine_id);
	}

	/** 
 * get virtual machine detail
 * @param array $transportIp
 * @param string $EsxIp
 * @param string $username
 * @param string $password
 * @param string $machine_id
*/
	public function getVirtualMachineDetail($transportIp, $EsxIp, $username, $password, $machine_id)
	{
		foreach ($transportIp as $TransportAddr) {
			try {
				return $this->Client->get_virtual_machine_detail_p($TransportAddr, $EsxIp, $username, $password, $machine_id);
			} catch (Throwable $e) {
				continue;
			}
		}

		return false;
	}

	/** 
 * remove VMWare hosts snapshot
 * @param array $transportIp
 * @param string $EsxIp
 * @param string $username
 * @param string $password
 * @param string $machine_id
*/
	public function removeVirtualMachineSnapshot($transportIp, $EsxIp, $username, $password, $machine_id, $snapshot_id)
	{
		foreach ($transportIp as $TransportAddr) {
			if ($this->Client->remove_virtual_machine_snapshot_p($TransportAddr, $EsxIp, $username, $password, $machine_id, $snapshot_id) == true)
				return true;
		}

		return false;
	}

	public function getVMWareSettingConfig($transportIp, $EsxIp, $username, $password, $EsxName)
	{
		$hosts = $this->getVirtualHosts($transportIp, $EsxIp, $username, $password);

		$key = array_search($EsxName, array_column($hosts, 'name'));

		$folder_path = $this->getDatacenterFolderList($transportIp, $EsxIp, $username, $password, $hosts[$key]->datacenter_name);

		$major = explode('.', $hosts[$key]->version)[0];

		$cpu = (isset($hosts[$key]->number_of_cpu_threads) and $hosts[$key]->number_of_cpu_threads != 0) ? $hosts[$key]->number_of_cpu_threads : $hosts[$key]->number_of_cpu_cores;

		$memory = $hosts[$key]->size_of_memory / 1024 / 1024;

		$ret = $this->getRange($major, $cpu, $memory);

		$ret["EsxInfo"] = $hosts[$key];

		$ret["EsxInfo"]->folder_path = $folder_path;

		return $ret;
	}

	public function getConfigOption($hosts_info, $replicaInfo, $os_Info = null)
	{
		$os = "";

		$esxi_ver = $this->getVMWareVersion($hosts_info->product_name);

		$data = $this->csv_to_array(__DIR__ . "\softwareguide.csv");

		if ($os_Info != null) {
			$osinfo = $os_Info;
			$os = $osinfo["platfrom"];
		} else {
			if (isset($replicaInfo["guest_os_name"]))
				$os = $replicaInfo["guest_os_name"];
			else {
				$os = $replicaInfo["os_name"];

				if ($replicaInfo["architecture"] == "amd64")
					$os .= " 64-bit";
			}

			$osinfo = $this->getOSFromHostInfo($os);
		}

		if (isset($replicaInfo["guest_id"]))
			$osinfo = $this->getOSFromGuestId($replicaInfo["guest_id"], $os); //$osinfo["guestId"] = $replicaInfo["guest_id"];

		$k = $this->multidimensional_search($data, array(
			"OSRelease" => $osinfo["platfrom"],
			"位数" => $osinfo["architecture"],
			"VMwareRelease" => $esxi_ver
		));

		$adapterType = $this->getSupportAdapterType($data[$k]);

		$SCSIType = $this->getSupportSCSIControllerType($data[$k]);

		return array(
			"adapterType" => $adapterType,
			"SCSIType"	  => $SCSIType,
			"osInfo"	  => $osinfo,
			"firmware"	  => $replicaInfo["firmware"]
		);
	}

	private function getRange($major, $cpu, $memory)
	{
		$nMaxCpu = 0;
		$m_nMemoryMB = 128;
		if ($major <= 3) {
				$nMaxCpu = 4;
				$m_nMemoryMB = 65532;
			} else if ($major == 4) {
				$nMaxCpu = 8;
				$m_nMemoryMB = 255 * 1024;
			} else if ($major == 5 && $major == 0) {
				$nMaxCpu = 32;
				$m_nMemoryMB = 1011 * 1024;
			} else if ($major >= 5) {
				$nMaxCpu = 64;
				$m_nMemoryMB = 1011 * 1024;
			}

		return array("MaxCpu" => min($cpu, $nMaxCpu), "MaxMemory" => intval(min($memory, $m_nMemoryMB)));
	}

	function getESXAuthInfo($serverId)
	{
		$CommonModel = new Common_Model();

		$TransportInfo = $CommonModel->getTransportInfo($serverId);

		$VMWareInfo = $CommonModel->query_cloud_connection_information($TransportInfo["CloudId"]);

		$ret = array(
			"TransportIp" 	=> json_decode($TransportInfo["TransportIp"], true)[0],
			"EsxIp" 		=> $VMWareInfo["DEFAULT_ADDR"],
			"Username"		=> json_decode($VMWareInfo["ACCESS_KEY"], true)["INPUT_HOST_USER"],
			"Password"		=> json_decode($VMWareInfo["SECRET_KEY"], true)["INPUT_HOST_PASS"],
			"ConnectAddr"	=> $TransportInfo["ConnectAddr"]
		);

		return $ret;
	}

	function getRecoveryInfo($serviceId)
	{
		$CommonModel = new Common_Model();

		$RecoveryInfo = $CommonModel->getRecoveryInfo($serviceId);

		$EsxInfo = $this->getESXAuthInfo($RecoveryInfo[0]["serverId"]);

		$ReplicaInfo = $CommonModel->getReplicatInfo($RecoveryInfo[0]["replicaId"]);

		$replica_job = json_decode($ReplicaInfo[0]["rep_job_json"], true);

		//$settingRange = $this->getVMWareSettingConfig( $EsxInfo["ConnectAddr"], $EsxInfo["EsxIp"], $EsxInfo["Username"], $EsxInfo["Password"], $replica_job["VMWARE_ESX"] );

		//$configOtion = $this->getConfigOption( $settingRange["EsxInfo"], $ReplicaInfo[0] );

		$replica_job = json_decode($RecoveryInfo[0]["rep_job_json"], true);
		$service_job = json_decode($RecoveryInfo[0]["service_job_json"], true);
		$snapshots = json_decode($RecoveryInfo[0]["snapshots"], true);

		$network = array();
		$networkAdapter = array();
		foreach (json_decode($RecoveryInfo[0]["Network"], true) as $net) {
			array_push($network, $net["network"]);
			array_push($networkAdapter, $net["type"]);
		}

		$ret = array(
			"EsxInfo" => $EsxInfo,
			"EsxName" => $replica_job["VMWARE_ESX"],
			"datastore" => $service_job["VMWARE_STORAGE"],
			"folder_path" => $service_job["VMWARE_FOLDER"],
			"CPU" => $service_job["CPU"],
			"Memory" => $service_job["Memory"],
			"vmName" => $service_job["hostname_tag"],
			"Network" => $RecoveryInfo[0]["Network"],
			"MachineId" => $RecoveryInfo[0]["replicaId"],
			"RecoveryMachineId" => $RecoveryInfo[0]["instanceId"],
			"Snapshot" => $snapshots[0],
			"networkAdapter" => $networkAdapter,
			"network" => $network,
			"firmware" => $ReplicaInfo[0]["firmware"],
			"Convert" => $service_job["CONVERT"],
			"VMTool" => $service_job["VM_TOOL"],
			"SCSI_CONTROLLER" => $this->scsiTypeMapping($service_job["VMWARE_SCSI_CONTROLLER"]),
			"networkSetting" => isset($service_job["NETWORK_SETTING"]) ? $service_job["NETWORK_SETTING"] : null,
			"OSTYPE_DISPLAY" => $service_job["OSTYPE_DISPLAY"],
			"OSTYPE" => $service_job["OSTYPE"],
			"ConnectionType" => $ReplicaInfo[0]["connectionType"],
			"guest_id" => $ReplicaInfo[0]["guest_id"]
		);

		return $ret;
	}

	private function scsiTypeMapping($type)
	{
		switch ($type) {
			case "LSI Logic":
				return \saasame\transport\hv_controller_type::HV_CTRL_LSI_LOGIC;
			case "BusLogic":
				return \saasame\transport\hv_controller_type::HV_CTRL_BUS_LOGIC;
			case "LSI Logic SAS":
				return \saasame\transport\hv_controller_type::HV_CTRL_LSI_LOGIC_SAS;
			case "VMware Paravirtual":
				return \saasame\transport\hv_controller_type::HV_CTRL_PARA_VIRT_SCSI;
			default:
				return \saasame\transport\hv_controller_type::HV_CTRL_ANY;
		}
	}

	public function scsiTypeMappingInt($type)
	{
		switch ($type) {
			case \saasame\transport\hv_controller_type::HV_CTRL_LSI_LOGIC:
				return "LSI Logic";
			case \saasame\transport\hv_controller_type::HV_CTRL_BUS_LOGIC:
				return "BusLogic";
			case \saasame\transport\hv_controller_type::HV_CTRL_LSI_LOGIC_SAS:
				return "LSI Logic SAS";
			case \saasame\transport\hv_controller_type::HV_CTRL_PARA_VIRT_SCSI:
				return "VMware Paravirtual";
			default:
				return "Any";
		}
	}

	function terminate_instances($cloudId, $serviceId, $action)
	{
		$recoveryInfo = $this->getRecoveryInfo($serviceId);

		$this->powerOffVirtualMachine(
			$recoveryInfo["EsxInfo"]["ConnectAddr"],
			$recoveryInfo["EsxInfo"]["EsxIp"],
			$recoveryInfo["EsxInfo"]["Username"],
			$recoveryInfo["EsxInfo"]["Password"],
			$recoveryInfo["RecoveryMachineId"]
		);

		if ($action  == "PlannedMigration" || ($action  == "DisasterRecovery" && $recoveryInfo["ConnectionType"] == \saasame\transport\hv_connection_type::HV_CONNECTION_TYPE_HOST))
			return true;

		$this->removeVirtualMachine(
			$recoveryInfo["EsxInfo"]["ConnectAddr"],
			$recoveryInfo["EsxInfo"]["EsxIp"],
			$recoveryInfo["EsxInfo"]["Username"],
			$recoveryInfo["EsxInfo"]["Password"],
			$recoveryInfo["RecoveryMachineId"]
		);

		return true;
	}

	function deleteReplicaMechine($cloudId, $serverId, $replicaId)
	{
		$EsxInfo = $this->getESXAuthInfo($serverId);

		$this->removeVirtualMachine(
			$EsxInfo["ConnectAddr"],
			$EsxInfo["EsxIp"],
			$EsxInfo["Username"],
			$EsxInfo["Password"],
			$replicaId
		);
	}

	function getVirtualMachineInfo($cloudId, $replicaId, $machineId)
	{

		$CommonModel = new Common_Model();

		$replicaInfo = $CommonModel->getReplicatInfo($replicaId);

		$EsxInfo = $this->getESXAuthInfo($replicaInfo[0]["ServerId"]);

		$info = $this->getVirtualMachineDetail(
			$EsxInfo["ConnectAddr"],
			$EsxInfo["EsxIp"],
			$EsxInfo["Username"],
			$EsxInfo["Password"],
			$machineId
		);

		return $info;
	}

	function getSnapshotList($replicaId)
	{

		$CommonModel = new Common_Model();

		$replicaInfo = $CommonModel->getReplicatInfo($replicaId);

		$EsxInfo = $this->getESXAuthInfo($replicaInfo[0]["ServerId"]);

		$machineInfo = $this->getVirtualMachineDetail(
			$EsxInfo["ConnectAddr"],
			$EsxInfo["EsxIp"],
			$EsxInfo["Username"],
			$EsxInfo["Password"],
			$replicaId
		);

		$snapshotList = array();

		foreach ($machineInfo->root_snapshot_list as $snapshots) {

			while (true) {

				$temp = array(
					"name" => $snapshots->name,
					"time" => explode(',', $snapshots->description)[1]
				);

				array_push($snapshotList, $temp);

				if (!isset($snapshots->child_snapshot_list[0]))
					break;

				$snapshots = $snapshots->child_snapshot_list[0];
			}
		}

		usort($snapshotList, array("VM_Ware", "snapshot_cmp_desc"));

		return $snapshotList;
	}

	function snapshot_control($replicaId, $numSnapshot)
	{

		$CommonModel = new Common_Model();

		$replicaInfo = $CommonModel->getReplicatInfo($replicaId);

		$EsxInfo = $this->getESXAuthInfo($replicaInfo[0]["ServerId"]);

		$snapshotList = $this->getSnapshotList($replicaId);

		$this->filterSnapshots($replicaId, $snapshotList);

		usort($snapshotList, array("VM_Ware", "snapshot_cmp_asc"));

		while (count($snapshotList) > $numSnapshot) {
			$this->removeVirtualMachineSnapshot(
				$EsxInfo["ConnectAddr"],
				$EsxInfo["EsxIp"],
				$EsxInfo["Username"],
				$EsxInfo["Password"],
				$replicaId,
				$snapshotList[0]["name"]
			);

			array_shift($snapshotList);
		}
	}

	/**
	 * filter the snapshot that is doing recovery
	 */
	function filterSnapshots($replicaId, &$snapshotList)
	{

		$CommonModel = new Common_Model();

		$recoveryJobs = $CommonModel->getRunningSnapshot($replicaId);

		foreach ($recoveryJobs as $recoveryJob)
			foreach (json_decode($recoveryJob["snapshots"], true) as $snap) {

				$key = array_search($snap, array_column($snapshotList, 'name'));

				if ($key !== false)
					array_splice($snapshotList, $key);
			}
	}

	static function snapshot_cmp_desc($a, $b)
	{

		if (strtotime($a["time"]) == strtotime($b["time"]))
			return 0;

		return (strtotime($a["time"]) > strtotime($b["time"])) ? -1 : +1;
	}

	static function snapshot_cmp_asc($a, $b)
	{

		if (strtotime($a["time"]) == strtotime($b["time"]))
			return 0;

		return (strtotime($a["time"]) < strtotime($b["time"])) ? -1 : +1;
	}

	public function multidimensional_search($parents, $searched)
	{
		if (empty($searched) || empty($parents)) {
			return false;
		}

		foreach ($parents as $key => $value) {
			$exists = true;
			foreach ($searched as $skey => $svalue) {
				$exists = ($exists && isset($parents[$key][$skey]) && $parents[$key][$skey] == $svalue);
			}
			if ($exists) {
				return $key;
			}
		}

		return 54;
	}

	public function getSupportAdapterType($info)
	{
		$support = array();

		if ($info["e1000 (Networking)"] == "Supported" || $info["e1000 (Networking)"] == "Recommended")
			if ($info["e1000 (Networking)"] == "Recommended")
				array_unshift($support, "E1000");
			else
				array_push($support, "E1000");

		if ($info["e1000e (Networking)"] == "Supported" || $info["e1000e (Networking)"] == "Recommended")
			if ($info["e1000e (Networking)"] == "Recommended")
				array_unshift($support, "E1000E");
			else
				array_push($support, "E1000E");

		if ($info["Enhanced VMXNET (Networking)"] == "Supported" || $info["Enhanced VMXNET (Networking)"] == "Recommended")
			if ($info["Enhanced VMXNET (Networking)"] == "Recommended")
				array_unshift($support, "Vmxnet2");
			else
				array_push($support, "Vmxnet2");

		if ($info["VMXNET (Networking)"] == "Supported" || $info["VMXNET (Networking)"] == "Recommended")
			if ($info["VMXNET (Networking)"] == "Recommended")
				array_unshift($support, "Vmxnet");
			else
				array_push($support, "Vmxnet");

		if ($info["VMXNET 3 (Networking)"] == "Supported" || $info["VMXNET 3 (Networking)"] == "Recommended")
			if ($info["VMXNET 3 (Networking)"] == "Recommended")
				array_unshift($support, "Vmxnet3");
			else
				array_push($support, "Vmxnet3");

		if ($info["Vlance (Networking)"] == "Supported" || $info["Vlance (Networking)"] == "Recommended")
			if ($info["Vlance (Networking)"] == "Recommended")
				array_unshift($support, "Vlance");
			else
				array_push($support, "Vlance");

		if ($info["VXLAN-Rx Filter (Networking)"] == "Supported" || $info["VXLAN-Rx Filter (Networking)"] == "Recommended")
			if ($info["VXLAN-Rx Filter (Networking)"] == "Recommended")
				array_unshift($support, "VXLAN-Rx Filter");
			else
				array_push($support, "VXLAN-Rx Filter");

		if ($info["Flexible"] == "Supported" || $info["Flexible"] == "Recommended")
			if ($info["Flexible"] == "Recommended")
				array_unshift($support, "Flexible");
			else
				array_push($support, "Flexible");

		return $support;
	}

	public function getSupportSCSIControllerType($info)
	{
		$support = array();

		if ($info["BusLogic (Storage)"] == "Supported" || $info["BusLogic (Storage)"] == "Recommended")
			if ($info["BusLogic (Storage)"] == "Recommended")
				array_unshift($support, "BusLogic");
			else
				array_push($support, "BusLogic");

		if ($info["LSI Logic (Storage)"] == "Supported" || $info["LSI Logic (Storage)"] == "Recommended")
			if ($info["LSI Logic (Storage)"] == "Recommended")
				array_unshift($support, "LSI Logic");
			else
				array_push($support, "LSI Logic");

		if ($info["LSI Logic SAS (Storage)"] == "Supported" || $info["LSI Logic SAS (Storage)"] == "Recommended")
			if ($info["LSI Logic SAS (Storage)"] == "Recommended")
				array_unshift($support, "LSI Logic SAS");
			else
				array_push($support, "LSI Logic SAS");

		if ($info["VMware Paravirtual (Storage)"] == "Supported" || $info["VMware Paravirtual (Storage)"] == "Recommended")
			if ($info["VMware Paravirtual (Storage)"] == "Recommended")
				array_unshift($support, "VMware Paravirtual");
			else
				array_push($support, "VMware Paravirtual");

		return $support;
	}

	public function getOSFromHostInfo($host)
	{

		$host = strtolower($host);
		$platfrom = "";
		$guestId = "";
		$architecture = "64";
		$ostype = OSType::Unknow;

		if (strpos($host, "windows")) {
			if (strpos($host, "2019")) {
				$platfrom = "Windows Server 2019";
				$guestId = "";
			} else if (strpos($host, "2016")) {
				$platfrom = "Windows Server 2016";
				$guestId = "windows9Server64Guest";
				$ostype = OSType::Microsoft_Windows_Server_2016_64_bit;
			} else if (strpos($host, "2012")) {
				$platfrom = "Windows Server 2012";
				$guestId = "windows8Server64Guest";
				$ostype = OSType::Microsoft_Windows_Server_2012_64_bit;
				if (strpos($host, "r2")) {
					$platfrom .= "  R2";
					$guestId = "windows8Server64Guest";
					$ostype = OSType::Microsoft_Windows_Server_2012_R2_64_bit;
				}
			} else if (strpos($host, "2003")) {
				$platfrom = "Windows Server 2003";
				$architecture = 32;

				if (strpos($host, "web")) {
					$guestId = "winNetWebGuest";
					$ostype = OSType::Microsoft_Windows_Server_2003_Web_Edition_32_bit;
				} else if (strpos($host, "standard")) {
					if (strpos($host, "64-bit")) {
						$guestId = "winNetStandard64Guest";
						$architecture = 64;
						$ostype = OSType::Microsoft_Windows_Server_2003_Standard_64_bit;
					} else {
						$guestId = "winNetStandardGuest";
						$ostype = OSType::Microsoft_Windows_Server_2003_Standard_32_bit;
					}
				} else if (strpos($host, "enterprise")) {
					if (strpos($host, "64-bit")) {
						$guestId = "winNetEnterprise64Guest";
						$architecture = 64;
						$ostype = OSType::Microsoft_Windows_Server_2003_64_bit;
					} else {
						$guestId = "winNetEnterpriseGuest";
						$ostype = OSType::Microsoft_Windows_Server_2003_32_bit;
					}
				} else if (strpos($host, "datacenter")) {
					if (strpos($host, "64-bit")) {
						$guestId = "winNetDatacenter64Guest";
						$architecture = 64;
						$ostype = OSType::Microsoft_Windows_Server_2003_Datacenter_64_bit;
					} else {
						$guestId = "winNetDatacenterGuest";
						$ostype = OSType::Microsoft_Windows_Server_2003_Datacenter_32_bit;
					}
				} else if (strpos($host, "business")) {
					$guestId = "winNetBusinessGuest";
					$ostype = OSType::Microsoft_Windows_Small_Business_Server_2003;
				}
			} else if (strpos($host, "2008")) {
				$platfrom = "Windows Server 2008";

				if (strpos($host, "64-bit")) {
					$guestId = "winLonghorn64Guest";
					$architecture = 64;
					$ostype = OSType::Microsoft_Windows_Server_2008_64_bit;
				} else {
					$guestId = "winLonghornGuest";
					$ostype = OSType::Microsoft_Windows_Server_2008_32_bit;
				}

				if (strpos($host, "r2")) {
					$platfrom .= "  R2";
					$guestId = "windows7Server64Guest";
					$architecture = 64;
					$ostype = OSType::Microsoft_Windows_Server_2008_R2_64_bit;
				}
			} else if (strpos($host, "vista")) {
				$platfrom = "Windows Vista";

				if (strpos($host, "64-bit")) {
					$guestId = "winVista64Guest";
					$ostype = OSType::Microsoft_Windows_Vista_64_bit;
				} else {
					$guestId = "winVistaGuest";
					$architecture = 32;
					$ostype = OSType::Microsoft_Windows_Vista_32_bit;
				}
			} else if (strpos($host, "xp")) {
				$platfrom = "Windows XP";
				$guestId = "";
				$architecture = 32;
			} else if (strpos($host, "windows 8.1")) {
				$platfrom = "Windows 8";

				if (strpos($host, "64-bit")) {
					$guestId = "windows8_64Guest";
					$ostype = OSType::Microsoft_Windows_8_64_bit;
				} else {
					$guestId = "windows8Guest";
					$architecture = 32;
					$ostype = OSType::Microsoft_Windows_8_32_bit;
				}
			} else if (strpos($host, "windows 8")) {
				$platfrom = "Windows 8";

				if (strpos($host, "64-bit")) {
					$guestId = "windows8_64Guest";
					$ostype = OSType::Microsoft_Windows_8_64_bit;
				} else {
					$guestId = "windows8Guest";
					$architecture = 32;
					$ostype = OSType::Microsoft_Windows_8_32_bit;
				}
			} else if (strpos($host, "windows 7")) {
				$platfrom = "Windows 7";

				if (strpos($host, "64-bit")) {
					$guestId = "windows7_64Guest";
					$ostype = OSType::Microsoft_Windows_7_64_bit;
				} else {
					$guestId = "windows7Guest";
					$architecture = 32;
					$ostype = OSType::Microsoft_Windows_7_32_bit;
				}
			} else if (strpos($host, "windows 10")) {
				$platfrom = "Windows 10";

				if (strpos($host, "64-bit")) {
					$guestId = "windows9_64Guest";
					$ostype = OSType::Microsoft_Windows_10_64_bit;
				} else {
					$guestId = "windows9Guest";
					$architecture = 32;
					$ostype = OSType::Microsoft_Windows_10_32_bit;
				}
			}
		} else {
			if (strpos($host, "red hat") !== false || strpos($host, "redhat") !== false) {

				if (strpos($host, "release 7") !== false) {
					$platfrom = "Red Hat 7";
					if (strpos($host, "64-bit") !== false) {
						$guestId = "rhel7_64Guest";
						$architecture = 64;
						$ostype = OSType::Red_Hat_Enterprise_Linux_7_64_bit;
					} else {
						$guestId = "rhel7Guest";
						$architecture = 32;
						$ostype = OSType::Red_Hat_Enterprise_Linux_7_32_bit;
					}
				} else if (strpos($host, "release 6") !== false) {
					$platfrom = "Red Hat 6";
					if (strpos($host, "64-bit") !== false) {
						$guestId = "rhel6_64Guest";
						$architecture = 64;
						$ostype = OSType::Red_Hat_Enterprise_Linux_6_64_bit;
					} else {
						$guestId = "rhel6Guest";
						$architecture = 32;
						$ostype = OSType::Red_Hat_Enterprise_Linux_6_32_bit;
					}
				} else if (strpos($host, "release 5") !== false) {
					$platfrom = "Red Hat 5";
					if (strpos($host, "64-bit") !== false) {
						$guestId = "rhel5_64Guest";
						$architecture = 64;
						$ostype = OSType::Red_Hat_Enterprise_Linux_5_64_bit;
					} else {
						$guestId = "rhel5Guest";
						$architecture = 32;
						$ostype = OSType::Red_Hat_Enterprise_Linux_5_32_bit;
					}
				} else if (strpos($host, "release 4") !== false) {
					$platfrom = "Red Hat 4";
					if (strpos($host, "64-bit") !== false) {
						$guestId = "rhel4_64Guest";
						$architecture = 64;
						$ostype = OSType::Red_Hat_Enterprise_Linux_4_64_bit;
					} else {
						$guestId = "rhel4Guest";
						$architecture = 32;
						$ostype = OSType::Red_Hat_Enterprise_Linux_4_32_bit;
					}
				}
			} else if (strpos($host, "centos") !== false) {
				$platfrom = "CentOS";
				if (strpos($host, "64-bit") !== false) {
					$guestId = "centos64Guest";
					$architecture = 64;
					$ostype = OSType::CentOS_4_5_6_7_64_bit;
				} else {
					$guestId = "centosGuest";
					$architecture = 32;
					$ostype = OSType::CentOS_4_5_6_7_32_bit;
				}
			} else if (strpos($host, "ubuntu") !== false) {
				$platfrom = "Ubuntu";
				if (strpos($host, "64-bit") !== false) {
					$guestId = "ubuntu64Guest";
					$architecture = 64;
					$ostype = OSType::Ubuntu_Linux_64_bit;
				} else {
					$guestId = "ubuntuGuest";
					$architecture = 32;
					$ostype = OSType::Ubuntu_Linux_32_bit;
				}
			} else if (strpos($host, "debian") !== false) {
				if (strpos($host, " 8") !== false) {
					$platfrom = "Debian 8";
					if (strpos($host, "64-bit") !== false) {
						$guestId = "debian8_64Guest";
						$architecture = 64;
						$ostype = OSType::Debian_GNU_Linux_8_64_bit;
					} else {
						$guestId = "debian8Guest";
						$architecture = 32;
						$ostype = OSType::Debian_GNU_Linux_8_32_bit;
					}
				} else if (strpos($host, " 7") !== false) {
					$platfrom = "Debian 7";
					if (strpos($host, "64-bit") !== false) {
						$guestId = "debian7_64Guest";
						$architecture = 64;
						$ostype = OSType::Debian_GNU_Linux_7_64_bit;
					} else {
						$guestId = "debian7Guest";
						$architecture = 32;
						$ostype = OSType::Debian_GNU_Linux_7_32_bit;
					}
				} else if (strpos($host, " 6") !== false) {
					$platfrom = "Debian 6";
					if (strpos($host, "64-bit") !== false) {
						$guestId = "debian6_64Guest";
						$architecture = 64;
						$ostype = OSType::Debian_GNU_Linux_6_64_bit;
					} else {
						$guestId = "debian6Guest";
						$architecture = 32;
						$ostype = OSType::Debian_GNU_Linux_6_32_bit;
					}
				}
			} else if (strpos($host, "suse") !== false || strpos($host, "sles") !== false) {
				if (strpos($host, " 12") !== false) {
					$platfrom = "SUSE 12";
					if (strpos($host, "64-bit") !== false) {
						$guestId = "sles12_64Guest";
						$architecture = 64;
						$ostype = OSType::SUSE_Linux_Enterprise_12_64_bit;
					} else {
						$guestId = "sles12Guest";
						$architecture = 32;
						$ostype = OSType::SUSE_Linux_Enterprise_12_32_bit;
					}
				} else if (strpos($host, " 11") !== false) {
					$platfrom = "SUSE 11";
					if (strpos($host, "64-bit") !== false) {
						$guestId = "sles11_64Guest";
						$architecture = 64;
						$ostype = OSType::SUSE_Linux_Enterprise_11_64_bit;
					} else {
						$guestId = "sles11Guest";
						$architecture = 32;
						$ostype = OSType::SUSE_Linux_Enterprise_11_32_bit;
					}
				} else if (strpos($host, " 10") !== false) {
					$platfrom = "SUSE 10";
					if (strpos($host, "64-bit") !== false) {
						$guestId = "sles10_64Guest";
						$architecture = 64;
						$ostype = OSType::SUSE_Linux_Enterprise_10_64_bit;
					} else {
						$guestId = "sles10Guest";
						$architecture = 32;
						$ostype = OSType::SUSE_Linux_Enterprise_10_32_bit;
					}
				} else if (strpos($host, " 8") !== false || strpos($host, " 9") !== false) {
					$platfrom = "SUSE 8/9";
					if (strpos($host, "64-bit") !== false) {
						$guestId = "sles8_64Guest";
						$architecture = 64;
						$ostype = OSType::SUSE_Linux_Enterprise_8_9_64_bit;
					} else {
						$guestId = "sles8Guest";
						$architecture = 32;
						$ostype = OSType::SUSE_Linux_Enterprise_8_9_32_bit;
					}
				}
			}
		}

		return array("platfrom" => $platfrom, "guestId" => $guestId, "architecture" => $architecture, "OSType" => $ostype);
	}

	public function getOSFromGuestId($guestId, $os)
	{
		if ($guestId == "windows9Server64Guest")
			return array("platfrom" => "Windows Server 2016", "guestId" => $guestId, "architecture" => "64", "OSType" => OSType::Microsoft_Windows_Server_2016_64_bit);
		else if ($guestId == "windows8Server64Guest")
			return array("platfrom" => "Windows Server 2012", "guestId" => $guestId, "architecture" => "64", "OSType" => OSType::Microsoft_Windows_Server_2012_64_bit);
		else if ($guestId == "winNetWebGuest")
			return array("platfrom" => "Windows Server 2003", "guestId" => $guestId, "architecture" => "32", "OSType" => OSType::Microsoft_Windows_Server_2003_Web_Edition_32_bit);
		else if ($guestId == "winNetStandard64Guest")
			return array("platfrom" => "Windows Server 2003", "guestId" => $guestId, "architecture" => "64", "OSType" => OSType::Microsoft_Windows_Server_2003_Standard_64_bit);
		else if ($guestId == "winNetStandardGuest")
			return array("platfrom" => "Windows Server 2003", "guestId" => $guestId, "architecture" => "32", "OSType" => OSType::Microsoft_Windows_Server_2003_Standard_32_bit);
		else if ($guestId == "winNetEnterprise64Guest")
			return array("platfrom" => "Windows Server 2003", "guestId" => $guestId, "architecture" => "64", "OSType" => OSType::Microsoft_Windows_Server_2003_64_bit);
		else if ($guestId == "winNetEnterpriseGuest")
			return array("platfrom" => "Windows Server 2003", "guestId" => $guestId, "architecture" => "32", "OSType" => OSType::Microsoft_Windows_Server_2003_32_bit);
		else if ($guestId == "winNetDatacenter64Guest")
			return array("platfrom" => "Windows Server 2003", "guestId" => $guestId, "architecture" => "64", "OSType" => OSType::Microsoft_Windows_Server_2003_Datacenter_64_bit);
		else if ($guestId == "winNetDatacenterGuest")
			return array("platfrom" => "Windows Server 2003", "guestId" => $guestId, "architecture" => "32", "OSType" => OSType::Microsoft_Windows_Server_2003_Datacenter_32_bit);
		else if ($guestId == "winNetBusinessGuest")
			return array("platfrom" => "Windows Server 2003", "guestId" => $guestId, "architecture" => "32", "OSType" => OSType::Microsoft_Windows_Small_Business_Server_2003);
		else if ($guestId == "winLonghorn64Guest")
			return array("platfrom" => "Windows Server 2008", "guestId" => $guestId, "architecture" => "64", "OSType" => OSType::Microsoft_Windows_Server_2008_64_bit);
		else if ($guestId == "winLonghornGuest")
			return array("platfrom" => "Windows Server 2008", "guestId" => $guestId, "architecture" => "32", "OSType" => OSType::Microsoft_Windows_Server_2008_32_bit);
		else if ($guestId == "windows7Server64Guest")
			return array("platfrom" => "Windows Server 2008", "guestId" => $guestId, "architecture" => "64", "OSType" => OSType::Microsoft_Windows_Server_2008_R2_64_bit);
		else if ($guestId == "winVista64Guest")
			return array("platfrom" => "Windows Vista", "guestId" => $guestId, "architecture" => "64", "OSType" => OSType::Microsoft_Windows_Vista_64_bit);
		else if ($guestId == "winVistaGuest")
			return array("platfrom" => "Windows Vista", "guestId" => $guestId, "architecture" => "32", "OSType" => OSType::Microsoft_Windows_Vista_32_bit);
		else if ($guestId == "windows8_64Guest")
			return array("platfrom" => "Windows 8", "guestId" => $guestId, "architecture" => "64", "OSType" => OSType::Microsoft_Windows_8_64_bit);
		else if ($guestId == "windows8Guest")
			return array("platfrom" => "Windows 8", "guestId" => $guestId, "architecture" => "32", "OSType" => OSType::Microsoft_Windows_8_32_bit);
		else if ($guestId == "windows7_64Guest")
			return array("platfrom" => "Windows 7", "guestId" => $guestId, "architecture" => "64", "OSType" => OSType::Microsoft_Windows_7_64_bit);
		else if ($guestId == "windows7Guest")
			return array("platfrom" => "Windows 7", "guestId" => $guestId, "architecture" => "32", "OSType" => OSType::Microsoft_Windows_7_32_bit);
		else if ($guestId == "windows9_64Guest")
			return array("platfrom" => "Windows 10", "guestId" => $guestId, "architecture" => "64", "OSType" => OSType::Microsoft_Windows_10_64_bit);
		else if ($guestId == "windows9Guest")
			return array("platfrom" => "Windows 10", "guestId" => $guestId, "architecture" => "32", "OSType" => OSType::Microsoft_Windows_10_32_bit);
		else if ($guestId == "rhel7_64Guest")
			return array("platfrom" => "Red Hat 7", "guestId" => $guestId, "architecture" => "64", "OSType" => OSType::Red_Hat_Enterprise_Linux_7_64_bit);
		else if ($guestId == "rhel7Guest")
			return array("platfrom" => "Red Hat 7", "guestId" => $guestId, "architecture" => "32", "OSType" => OSType::Red_Hat_Enterprise_Linux_7_32_bit);
		else if ($guestId == "rhel6_64Guest")
			return array("platfrom" => "Red Hat 6", "guestId" => $guestId, "architecture" => "64", "OSType" => OSType::Red_Hat_Enterprise_Linux_6_64_bit);
		else if ($guestId == "rhel6Guest")
			return array("platfrom" => "Red Hat 6", "guestId" => $guestId, "architecture" => "32", "OSType" => OSType::Red_Hat_Enterprise_Linux_6_32_bit);
		else if ($guestId == "rhel5_64Guest")
			return array("platfrom" => "Red Hat 5", "guestId" => $guestId, "architecture" => "64", "OSType" => OSType::Red_Hat_Enterprise_Linux_5_64_bit);
		else if ($guestId == "rhel5Guest")
			return array("platfrom" => "Red Hat 5", "guestId" => $guestId, "architecture" => "32", "OSType" => OSType::Red_Hat_Enterprise_Linux_5_32_bit);
		else if ($guestId == "rhel4_64Guest")
			return array("platfrom" => "Red Hat 4", "guestId" => $guestId, "architecture" => "64", "OSType" => OSType::Red_Hat_Enterprise_Linux_4_64_bit);
		else if ($guestId == "rhel4Guest")
			return array("platfrom" => "Red Hat 4", "guestId" => $guestId, "architecture" => "32", "OSType" => OSType::Red_Hat_Enterprise_Linux_4_32_bit);
		else if ($guestId == "centos64Guest")
			return array("platfrom" => "CentOS", "guestId" => $guestId, "architecture" => "64", "OSType" => OSType::CentOS_4_5_6_7_64_bit);
		else if ($guestId == "centosGuest")
			return array("platfrom" => "CentOS", "guestId" => $guestId, "architecture" => "32", "OSType" => OSType::CentOS_4_5_6_7_32_bit);
		else if ($guestId == "ubuntu64Guest")
			return array("platfrom" => "Ubuntu", "guestId" => $guestId, "architecture" => "64", "OSType" => OSType::Ubuntu_Linux_64_bit);
		else if ($guestId == "ubuntuGuest")
			return array("platfrom" => "Ubuntu", "guestId" => $guestId, "architecture" => "32", "OSType" => OSType::Ubuntu_Linux_32_bit);
		else if ($guestId == "debian8_64Guest")
			return array("platfrom" => "Debian 8", "guestId" => $guestId, "architecture" => "64", "OSType" => OSType::Debian_GNU_Linux_8_64_bit);
		else if ($guestId == "debian8Guest")
			return array("platfrom" => "Debian 8", "guestId" => $guestId, "architecture" => "32", "OSType" => OSType::Debian_GNU_Linux_8_32_bit);
		else if ($guestId == "debian7_64Guest")
			return array("platfrom" => "Debian 7", "guestId" => $guestId, "architecture" => "64", "OSType" => OSType::Debian_GNU_Linux_7_64_bit);
		else if ($guestId == "debian7Guest")
			return array("platfrom" => "Debian 7", "guestId" => $guestId, "architecture" => "32", "OSType" => OSType::Debian_GNU_Linux_7_32_bit);
		else if ($guestId == "debian6_64Guest")
			return array("platfrom" => "Debian 6", "guestId" => $guestId, "architecture" => "64", "OSType" => OSType::Debian_GNU_Linux_6_64_bit);
		else if ($guestId == "debian6Guest")
			return array("platfrom" => "Debian 6", "guestId" => $guestId, "architecture" => "32", "OSType" => OSType::Debian_GNU_Linux_6_32_bit);
		else if ($guestId == "sles12_64Guest")
			return array("platfrom" => "SUSE 12", "guestId" => $guestId, "architecture" => "64", "OSType" => OSType::SUSE_Linux_Enterprise_12_64_bit);
		else if ($guestId == "sles12Guest")
			return array("platfrom" => "SUSE 12", "guestId" => $guestId, "architecture" => "32", "OSType" => OSType::SUSE_Linux_Enterprise_12_32_bit);
		else if ($guestId == "sles11_64Guest")
			return array("platfrom" => "SUSE 11", "guestId" => $guestId, "architecture" => "64", "OSType" => OSType::SUSE_Linux_Enterprise_11_64_bit);
		else if ($guestId == "sles11Guest")
			return array("platfrom" => "SUSE 11", "guestId" => $guestId, "architecture" => "32", "OSType" => OSType::SUSE_Linux_Enterprise_11_32_bit);
		else if ($guestId == "sles10_64Guest")
			return array("platfrom" => "SUSE 10", "guestId" => $guestId, "architecture" => "64", "OSType" => OSType::SUSE_Linux_Enterprise_10_64_bit);
		else if ($guestId == "sles10Guest")
			return array("platfrom" => "SUSE 10", "guestId" => $guestId, "architecture" => "32", "OSType" => OSType::SUSE_Linux_Enterprise_10_32_bit);
		else if ($guestId == "sles8_64Guest")
			return array("platfrom" => "SUSE 8/9", "guestId" => $guestId, "architecture" => "64", "OSType" => OSType::SUSE_Linux_Enterprise_8_9_64_bit);
		else if ($guestId == "sles8Guest")
			return array("platfrom" => "SUSE 8/9", "guestId" => $guestId, "architecture" => "32", "OSType" => OSType::SUSE_Linux_Enterprise_8_9_32_bit);

		return array("platfrom" => $os, "guestId" => $guestId, "architecture" => "64", "OSType" => OSType::Unknow);
	}

	public function getOSFromOSType($ostype)
	{

		if ($ostype == OSType::Microsoft_Windows_Server_2016_64_bit)
			return array("platfrom" => "Windows Server 2016", "guestId" => "windows9Server64Guest", "architecture" => "64", "OSType" => OSType::Microsoft_Windows_Server_2016_64_bit);
		else if ($ostype == OSType::Microsoft_Windows_Server_2012_64_bit)
			return array("platfrom" => "Windows Server 2012", "guestId" => "windows8Server64Guest", "architecture" => "64", "OSType" => OSType::Microsoft_Windows_Server_2012_64_bit);
		else if ($ostype == OSType::Microsoft_Windows_Server_2003_Web_Edition_32_bit)
			return array("platfrom" => "Windows Server 2003", "guestId" => "winNetWebGuest", "architecture" => "32", "OSType" => OSType::Microsoft_Windows_Server_2003_Web_Edition_32_bit);
		else if ($ostype == OSType::Microsoft_Windows_Server_2003_Standard_64_bit)
			return array("platfrom" => "Windows Server 2003", "guestId" => "winNetStandard64Guest", "architecture" => "64", "OSType" => OSType::Microsoft_Windows_Server_2003_Standard_64_bit);
		else if ($ostype == OSType::Microsoft_Windows_Server_2003_Standard_32_bit)
			return array("platfrom" => "Windows Server 2003", "guestId" => "winNetStandardGuest", "architecture" => "32", "OSType" => OSType::Microsoft_Windows_Server_2003_Standard_32_bit);
		else if ($ostype == OSType::Microsoft_Windows_Server_2003_64_bit)
			return array("platfrom" => "Windows Server 2003", "guestId" => "winNetEnterprise64Guest", "architecture" => "64", "OSType" => OSType::Microsoft_Windows_Server_2003_64_bit);
		else if ($ostype == OSType::Microsoft_Windows_Server_2003_32_bit)
			return array("platfrom" => "Windows Server 2003", "guestId" => "winNetEnterpriseGuest", "architecture" => "32", "OSType" => OSType::Microsoft_Windows_Server_2003_32_bit);
		else if ($ostype == OSType::Microsoft_Windows_Server_2003_Datacenter_64_bit)
			return array("platfrom" => "Windows Server 2003", "guestId" => "winNetDatacenter64Guest", "architecture" => "64", "OSType" => OSType::Microsoft_Windows_Server_2003_Datacenter_64_bit);
		else if ($ostype == OSType::Microsoft_Windows_Server_2003_Datacenter_32_bit)
			return array("platfrom" => "Windows Server 2003", "guestId" => "winNetDatacenterGuest", "architecture" => "32", "OSType" => OSType::Microsoft_Windows_Server_2003_Datacenter_32_bit);
		else if ($ostype == OSType::Microsoft_Windows_Small_Business_Server_2003)
			return array("platfrom" => "Windows Server 2003", "guestId" => "winNetBusinessGuest", "architecture" => "32", "OSType" => OSType::Microsoft_Windows_Small_Business_Server_2003);
		else if ($ostype == OSType::Microsoft_Windows_Server_2008_64_bit)
			return array("platfrom" => "Windows Server 2008", "guestId" => "winLonghorn64Guest", "architecture" => "64", "OSType" => OSType::Microsoft_Windows_Server_2008_64_bit);
		else if ($ostype == OSType::Microsoft_Windows_Server_2008_32_bit)
			return array("platfrom" => "Windows Server 2008", "guestId" => "winLonghornGuest", "architecture" => "32", "OSType" => OSType::Microsoft_Windows_Server_2008_32_bit);
		else if ($ostype == OSType::Microsoft_Windows_Server_2008_R2_64_bit)
			return array("platfrom" => "Windows Server 2008", "guestId" => "windows7Server64Guest", "architecture" => "64", "OSType" => OSType::Microsoft_Windows_Server_2008_R2_64_bit);
		else if ($ostype == OSType::Microsoft_Windows_Vista_64_bit)
			return array("platfrom" => "Windows Vista", "guestId" => "winVista64Guest", "architecture" => "64", "OSType" => OSType::Microsoft_Windows_Vista_64_bit);
		else if ($ostype == OSType::Microsoft_Windows_Vista_32_bit)
			return array("platfrom" => "Windows Vista", "guestId" => "winVistaGuest", "architecture" => "32", "OSType" => OSType::Microsoft_Windows_Vista_32_bit);
		else if ($ostype == OSType::Microsoft_Windows_8_64_bit)
			return array("platfrom" => "Windows 8", "guestId" => "windows8_64Guest", "architecture" => "64", "OSType" => OSType::Microsoft_Windows_8_64_bit);
		else if ($ostype == OSType::Microsoft_Windows_8_32_bit)
			return array("platfrom" => "Windows 8", "guestId" => "windows8Guest", "architecture" => "32", "OSType" => OSType::Microsoft_Windows_8_32_bit);
		else if ($ostype == OSType::Microsoft_Windows_7_64_bit)
			return array("platfrom" => "Windows 7", "guestId" => "windows7_64Guest", "architecture" => "64", "OSType" => OSType::Microsoft_Windows_7_64_bit);
		else if ($ostype == OSType::Microsoft_Windows_7_32_bit)
			return array("platfrom" => "Windows 7", "guestId" => "windows7Guest", "architecture" => "32", "OSType" => OSType::Microsoft_Windows_7_32_bit);
		else if ($ostype == OSType::Microsoft_Windows_10_64_bit)
			return array("platfrom" => "Windows 10", "guestId" => "windows9_64Guest", "architecture" => "64", "OSType" => OSType::Microsoft_Windows_10_64_bit);
		else if ($ostype == OSType::Microsoft_Windows_10_32_bit)
			return array("platfrom" => "Windows 10", "guestId" => "windows9Guest", "architecture" => "32", "OSType" => OSType::Microsoft_Windows_10_32_bit);
		else if ($ostype == OSType::Red_Hat_Enterprise_Linux_7_64_bit)
			return array("platfrom" => "Red Hat 7", "guestId" => "rhel7_64Guest", "architecture" => "64", "OSType" => OSType::Red_Hat_Enterprise_Linux_7_64_bit);
		else if ($ostype == OSType::Red_Hat_Enterprise_Linux_7_32_bit)
			return array("platfrom" => "Red Hat 7", "guestId" => "rhel7Guest", "architecture" => "32", "OSType" => OSType::Red_Hat_Enterprise_Linux_7_32_bit);
		else if ($ostype == OSType::Red_Hat_Enterprise_Linux_6_64_bit)
			return array("platfrom" => "Red Hat 6", "guestId" => "rhel6_64Guest", "architecture" => "64", "OSType" => OSType::Red_Hat_Enterprise_Linux_6_64_bit);
		else if ($ostype == OSType::Red_Hat_Enterprise_Linux_6_32_bit)
			return array("platfrom" => "Red Hat 6", "guestId" => "rhel6Guest", "architecture" => "32", "OSType" => OSType::Red_Hat_Enterprise_Linux_6_32_bit);
		else if ($ostype == OSType::Red_Hat_Enterprise_Linux_5_64_bit)
			return array("platfrom" => "Red Hat 5", "guestId" => "rhel5_64Guest", "architecture" => "64", "OSType" => OSType::Red_Hat_Enterprise_Linux_5_64_bit);
		else if ($ostype == OSType::Red_Hat_Enterprise_Linux_5_32_bit)
			return array("platfrom" => "Red Hat 5", "guestId" => "rhel5Guest", "architecture" => "32", "OSType" => OSType::Red_Hat_Enterprise_Linux_5_32_bit);
		else if ($ostype == OSType::Red_Hat_Enterprise_Linux_4_64_bit)
			return array("platfrom" => "Red Hat 4", "guestId" => "rhel4_64Guest", "architecture" => "64", "OSType" => OSType::Red_Hat_Enterprise_Linux_4_64_bit);
		else if ($ostype == OSType::Red_Hat_Enterprise_Linux_4_32_bit)
			return array("platfrom" => "Red Hat 4", "guestId" => "rhel4Guest", "architecture" => "32", "OSType" => OSType::Red_Hat_Enterprise_Linux_4_32_bit);
		else if ($ostype == OSType::CentOS_4_5_6_7_64_bit)
			return array("platfrom" => "CentOS", "guestId" => "centos64Guest", "architecture" => "64", "OSType" => OSType::CentOS_4_5_6_7_64_bit);
		else if ($ostype == OSType::CentOS_4_5_6_7_32_bit)
			return array("platfrom" => "CentOS", "guestId" => "centosGuest", "architecture" => "32", "OSType" => OSType::CentOS_4_5_6_7_32_bit);
		else if ($ostype == OSType::Ubuntu_Linux_64_bit)
			return array("platfrom" => "Ubuntu", "guestId" => "ubuntu64Guest", "architecture" => "64", "OSType" => OSType::Ubuntu_Linux_64_bit);
		else if ($ostype == OSType::Ubuntu_Linux_32_bit)
			return array("platfrom" => "Ubuntu", "guestId" => "ubuntuGuest", "architecture" => "32", "OSType" => OSType::Ubuntu_Linux_32_bit);
		else if ($ostype == OSType::Debian_GNU_Linux_8_64_bit)
			return array("platfrom" => "Debian 8", "guestId" => "debian8_64Guest", "architecture" => "64", "OSType" => OSType::Debian_GNU_Linux_8_64_bit);
		else if ($ostype == OSType::Debian_GNU_Linux_8_32_bit)
			return array("platfrom" => "Debian 8", "guestId" => "debian8Guest", "architecture" => "32", "OSType" => OSType::Debian_GNU_Linux_8_32_bit);
		else if ($ostype == OSType::Debian_GNU_Linux_7_64_bit)
			return array("platfrom" => "Debian 7", "guestId" => "debian7_64Guest", "architecture" => "64", "OSType" => OSType::Debian_GNU_Linux_7_64_bit);
		else if ($ostype == OSType::Debian_GNU_Linux_7_32_bit)
			return array("platfrom" => "Debian 7", "guestId" => "debian7Guest", "architecture" => "32", "OSType" => OSType::Debian_GNU_Linux_7_32_bit);
		else if ($ostype == OSType::Debian_GNU_Linux_6_64_bit)
			return array("platfrom" => "Debian 6", "guestId" => "debian6_64Guest", "architecture" => "64", "OSType" => OSType::Debian_GNU_Linux_6_64_bit);
		else if ($ostype == OSType::Debian_GNU_Linux_6_32_bit)
			return array("platfrom" => "Debian 6", "guestId" => "debian6Guest", "architecture" => "32", "OSType" => OSType::Debian_GNU_Linux_6_32_bit);
		else if ($ostype == OSType::SUSE_Linux_Enterprise_12_64_bit)
			return array("platfrom" => "SUSE 12", "guestId" => "sles12_64Guest", "architecture" => "64", "OSType" => OSType::SUSE_Linux_Enterprise_12_64_bit);
		else if ($ostype == OSType::SUSE_Linux_Enterprise_12_32_bit)
			return array("platfrom" => "SUSE 12", "guestId" => "sles12Guest", "architecture" => "32", "OSType" => OSType::SUSE_Linux_Enterprise_12_32_bit);
		else if ($ostype == OSType::SUSE_Linux_Enterprise_11_64_bit)
			return array("platfrom" => "SUSE 11", "guestId" => "sles11_64Guest", "architecture" => "64", "OSType" => OSType::SUSE_Linux_Enterprise_11_64_bit);
		else if ($ostype == OSType::SUSE_Linux_Enterprise_11_32_bit)
			return array("platfrom" => "SUSE 11", "guestId" => "sles11Guest", "architecture" => "32", "OSType" => OSType::SUSE_Linux_Enterprise_11_32_bit);
		else if ($ostype == OSType::SUSE_Linux_Enterprise_10_64_bit)
			return array("platfrom" => "SUSE 10", "guestId" => "sles10_64Guest", "architecture" => "64", "OSType" => OSType::SUSE_Linux_Enterprise_10_64_bit);
		else if ($ostype == OSType::SUSE_Linux_Enterprise_10_32_bit)
			return array("platfrom" => "SUSE 10", "guestId" => "sles10Guest", "architecture" => "32", "OSType" => OSType::SUSE_Linux_Enterprise_10_32_bit);
		else if ($ostype == OSType::SUSE_Linux_Enterprise_8_9_64_bit)
			return array("platfrom" => "SUSE 8/9", "guestId" => "sles8_64Guest", "architecture" => "64", "OSType" => OSType::SUSE_Linux_Enterprise_8_9_64_bit);
		else if ($ostype == OSType::SUSE_Linux_Enterprise_8_9_32_bit)
			return array("platfrom" => "SUSE 8/9", "guestId" => "sles8Guest", "architecture" => "32", "OSType" => OSType::SUSE_Linux_Enterprise_8_9_32_bit);

		return array("platfrom" => "", "guestId" => "", "architecture" => "", "OSType" => OSType::Unknow);
	}

	public function getVMWareVersion($productName)
	{

		$productName = strtolower($productName);

		$pos = strpos($productName, "esxi");

		if ($pos !== false) {
			$esxi_ver = floatval(substr($productName, $pos + 4));

			if ($esxi_ver >= 6)
				return "ESXi 6.0";
			else if ($esxi_ver < 6)
				return "ESXi 5.5";
			else
				return "unknow";
		}

		if (strpos($productName, "esxi 6.0")) {
			if (strpos($productName, "u1"))
				return "ESXi 6.0 U1";
			else if (strpos($productName, "u2"))
				return "ESXi 6.0 U2";
			if (strpos($productName, "u3"))
				return "ESXi 6.0 U3";

			return "ESXi 6.0";
		} else if (strpos($productName, "esxi 5.5")) {
			if (strpos($productName, "u1"))
				return "ESXi 5.5 U1";
			else if (strpos($productName, "u2"))
				return "ESXi 5.5 U2";
			if (strpos($productName, "u3"))
				return "ESXi 5.5 U3";

			return "ESXi 5.5";
		} else if (strpos($productName, "esxi 6.5")) {
			if (strpos($productName, "u1"))
				return "ESXi 6.5 U1";
			else if (strpos($productName, "u2"))
				return "ESXi 6.5 U2";

			return "ESXi 6.5";
		} else if (strpos($productName, "esxi 6.7")) {
			if (strpos($productName, "u1"))
				return "ESXi 6.7 U1";

			return "ESXi 6.7";
		} else if (strpos($productName, "vmware cloud on qws"))
			return "VMware Cloud on AWS";

		return "ESXi 6.0";
	}

	public function procPreCreateLoaderJobDetail($replicaId, $hostName, $LUNS_MAPS, $SIZE_MAPS, &$loaderJob)
	{

		$CommonModel = new Common_Model();

		$ReplicatInfo = $CommonModel->getReplicatInfo($replicaId);

		$VMWareInfo = $CommonModel->query_cloud_connection_information($ReplicatInfo[0]["CloudId"]);

		$job = json_decode($ReplicatInfo[0]["rep_job_json"], true);

		$name = json_decode($VMWareInfo["ACCESS_KEY"], true)["INPUT_HOST_USER"];
		$pass = json_decode($VMWareInfo["SECRET_KEY"], true)["INPUT_HOST_PASS"];
		$endpoint = $VMWareInfo["DEFAULT_ADDR"];


		$loaderJob['disks_lun_mapping'] = $LUNS_MAPS;
		$loaderJob['disks_size_mapping'] = $SIZE_MAPS;

		$loaderJob["detect_type"] = \saasame\transport\disk_detect_type::VMWARE_VADP;

		$loaderJob["host_name"] = "(Replica)_" . $hostName;

		$loaderJob["thin_provisioned"] = $job["VMWARE_THIN_PROVISIONED"];

		$vm_ware_info = array(
			"host" => $endpoint,
			"username" => $name,
			"password" => $pass,
			"esx" => $job["VMWARE_ESX"],
			"datastore" => $job["VMWARE_STORAGE"],
			"folder_path" => $job["VMWARE_FOLDER"]
		);

		$connetion = new saasame\transport\vmware_connection_info($vm_ware_info);

		$loaderJob["vmware_connection"] = $connetion;
	}
}
 