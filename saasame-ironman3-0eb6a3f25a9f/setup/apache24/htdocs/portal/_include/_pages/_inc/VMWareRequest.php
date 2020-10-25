<script>
	
	function ConvertSwitch( status ){
		$('#FIRMWARE').prop('disabled', status );
		$("#VM_TOOL").bootstrapToggle('enable');
		$("#VM_TOOL").bootstrapToggle(status?'on':'off');
		$("#VM_TOOL").bootstrapToggle( status?'enable':'disable');
		
		for( key in adapters ){
			$('#NetwoekConfig'+key).prop('disabled', !status );
		}
		
		$('.selectpicker').selectpicker('refresh');
	}

	network_setting = {};
	function getVMWareSettingRange(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_VMWare.php',
			data:{
				 'ACTION'    :'getVMWareSettingConfig',
				 'SERV_UUID' :'<?php echo $_SESSION['SERVER_UUID']; ?>',
				 'ReplicatId':'<?php echo $_SESSION['REPL_UUID']; ?>'
			},
			success:function( response )
			{
				ESX_INFO = response.EsxInfo;
				
				for( key in response.EsxInfo.networks ){
					$('#VM_Network').append(new Option(response.EsxInfo.networks[key], response.EsxInfo.networks[key], true, false));
				}
				var Cpu = document.getElementById("Cpu");
				var Memory = document.getElementById("Memory");
				Cpu.setAttribute("max",response.MaxCpu);
				Memory.setAttribute("max",response.MaxMemory);

				if( typeof response.cpu !== 'undefined' ){
					$("#Cpu").val( response.cpu );
				}
				
				if( typeof response.memory !== 'undefined' ){
					$("#Memory").val( response.memory );
				}
				
				for( key in response.EsxInfo.datastores ){
					$('#VMWARE_STORAGE').append(new Option(response.EsxInfo.datastores[key], response.EsxInfo.datastores[key], true, false));
				}
				
				for( key in response.EsxInfo.folder_path ){
					$('#VMWARE_FOLDER').append(new Option(response.EsxInfo.folder_path[key], response.EsxInfo.folder_path[key], true, false));
				}
				
				if( response.configOtion.osInfo.OSType == -1 ){
					$('#OSType').append(new Option(response.configOtion.osInfo.platfrom, response.configOtion.osInfo.OSType, true, true));
				}
				else
					$("#OSType").val( response.configOtion.osInfo.OSType );
				
				if( typeof response.EsxInfo.name !== 'undefined' && response.EsxInfo.name != '' )
					$("#ESX_Server").append(new Option(response.EsxInfo.name, response.EsxInfo.name, true, false));
				
				if( typeof CPU_val !== 'undefined' && CPU_val != '' )
					$("#Cpu").val( CPU_val );
				
				if( typeof Memory_val !== 'undefined' && Memory_val != '' )
					$("#Memory").val( Memory_val );
				
				if( typeof VMWARE_STORAGE !== 'undefined' && VMWARE_STORAGE != '' )
					$("#VMWARE_STORAGE").val( VMWARE_STORAGE );
				
				if( '<?php echo $_SESSION["RECY_TYPE"]; ?>' == "RECOVERY_PM" ){
					$("#VMWARE_STORAGE").val( response.ESXStorage );
					$('#VMWARE_STORAGE').prop('disabled', true );
				}

				if( typeof VMWARE_FOLDER !== 'undefined' && VMWARE_FOLDER != '' )
					$("#VMWARE_FOLDER").val( VMWARE_FOLDER );
				
				if( typeof NETWORK_UUID !== 'undefined' && NETWORK_UUID != '' )
					$("#VM_Network").val( NETWORK_UUID );
				
				if( typeof FIRMWARE !== 'undefined' && FIRMWARE != '' )
					$("#FIRMWARE").val( FIRMWARE );
				
				if( typeof response.configOtion.SCSIType !== 'undefined' && response.configOtion.SCSIType != '' ){
					for( SCSI_key in response.configOtion.SCSIType)
						$('#VMWARE_SCSI_CONTROLLER').append(new Option(response.configOtion.SCSIType[SCSI_key], response.configOtion.SCSIType[SCSI_key], true, false));
					
					if( typeof response.SCSIType !== 'undefined' && response.SCSIType != '' )
						$("#VMWARE_SCSI_CONTROLLER").val( response.SCSIType );
					
					if( typeof VMWARE_SCSI_CONTROLLER !== 'undefined' && VMWARE_SCSI_CONTROLLER != '' )
						$("#VMWARE_SCSI_CONTROLLER").val( VMWARE_SCSI_CONTROLLER );
				}
				
				if( typeof VM_TOOL !== 'undefined' && VM_TOOL != '' && VM_TOOL == 'false')
					$("#VM_TOOL").prop('checked', false ).change();
				
				if( typeof response.network_adapters !== 'undefined' && response.network_adapters != ''){
					
					adapters = JSON.parse( response.network_adapters );
					
					for( key in adapters ){
						createNetworkSelecter( key );
						
						for( net_key in response.EsxInfo.networks ){
							$('#VM_Network'+key).append(new Option(response.EsxInfo.networks[net_key], response.EsxInfo.networks[net_key], true, false));
						}
						
						for( adapterType_key in response.configOtion.adapterType ){
							$('#VM_Network_Adapter'+key).append(new Option(response.configOtion.adapterType[adapterType_key], response.configOtion.adapterType[adapterType_key], true, false));
						}	
						
						if( typeof adapters[key].type != 'undefined' )
							$('#VM_Network_Adapter'+key).val( adapters[key].type );
						
						if( NETWORK_UUID != '' ){
							$("#VM_Network"+key).val( NETWORK_UUID[key].network );
							$("#VM_Network_Adapter"+key).val( NETWORK_UUID[key].type );
						}
					}
				}
				
				if( typeof CONVERT !== 'undefined' && CONVERT != '' && CONVERT == 'false' ){
					$("#CONVERT").prop('checked', false ).change();
					ConvertSwitch( false );
				}
				else
					ConvertSwitch( true );
				
				
				$('.selectpicker').selectpicker('refresh');
			},
			error: function(xhr)
			{
				
			}
		});		
	}
	
	function getConfigDefault( val ){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_VMWare.php',
			data:{
				 'ACTION'    :'getConfigDefault',
				 'SERV_UUID' :'<?php echo $_SESSION['SERVER_UUID']; ?>',
				 'ReplicatId':'<?php echo $_SESSION['REPL_UUID']; ?>',
				 'os':val
			},
			success:function( response )
			{
				if( typeof response.configOtion.SCSIType !== 'undefined' && response.configOtion.SCSIType != '' ){
					
					$("#VMWARE_SCSI_CONTROLLER option").remove();
					
					for( SCSI_key in response.configOtion.SCSIType)
						$('#VMWARE_SCSI_CONTROLLER').append(new Option(response.configOtion.SCSIType[SCSI_key], response.configOtion.SCSIType[SCSI_key], true, false));
					
					if( typeof VMWARE_SCSI_CONTROLLER !== 'undefined' && VMWARE_SCSI_CONTROLLER != '' )
						$("#VMWARE_SCSI_CONTROLLER").val( VMWARE_SCSI_CONTROLLER );
				}
				
				
				if( typeof response.network_adapters !== 'undefined' && response.network_adapters != ''){
					
					adapters = JSON.parse( response.network_adapters );
					
					for( key in adapters ){
						
						$("#VM_Network_Adapter"+key+" option").remove();
						
						for( adapterType_key in response.configOtion.adapterType ){
							$('#VM_Network_Adapter'+key).append(new Option(response.configOtion.adapterType[adapterType_key], response.configOtion.adapterType[adapterType_key], true, false));
						}	
						
						if( NETWORK_UUID != '' ){
							$("#VM_Network"+key).val( NETWORK_UUID[key].network );
							$("#VM_Network_Adapter"+key).val( NETWORK_UUID[key].type );
						}
					}
				}
				
				$('.selectpicker').selectpicker('refresh');
			},
			error: function(xhr)
			{
				
			}
		});		
	}
	
	function paddingLeft(str,lenght){
		if(str.length >= lenght)
			return str;
		else
			return paddingLeft("0" +str,lenght);
	}

	function genMAC(){
		var b4 = paddingLeft( Math.round(Math.random() * 57).toString(16).toUpperCase(), 2 );
		var prefix=[/*"00:05:69:","00:0C:29:","00:1C:14:",*/"00:50:56:"];
		var hexDigits = "0123456789ABCDEF";
		var macAddress = prefix[0/*Math.round(Math.random() * prefix.length - 1)*/] + b4 +":";
		for (var i = 0; i < 2; i++) {
			macAddress+=hexDigits.charAt(Math.round(Math.random() * 15));
			macAddress+=hexDigits.charAt(Math.round(Math.random() * 15));
			if (i != 1) macAddress += ":";
		}
	
		return macAddress;
	}

	/* Configure DataMode Agent */
	function NetwoekConfig( key ){
		
		var ip="",subnet="",gateway="",dns="",mac = "";
		
		if( <?php echo isset($_SESSION["EDIT_PLAN"]->NETWORK_SETTING)?"true":"false" ?> )
			network_setting = <?php echo isset($_SESSION["EDIT_PLAN"]->NETWORK_SETTING)?json_encode($_SESSION["EDIT_PLAN"]->NETWORK_SETTING):"false"; ?>;
		
		if( typeof network_setting[key] != 'undefined' ){
			ip = network_setting[key].ip;
			subnet = network_setting[key].subnet;
			gateway = network_setting[key].gateway;
			dns = network_setting[key].dns;
			mac = network_setting[key].mac;
		}
		else
			mac = genMAC();
		
		BootstrapDialog.show({
			title: '<?php echo _('Network Setting'); ?>',
			cssClass: 'partition-agent-dialog',
			draggable: true,
			closable: true,
            message:   '<div class="form-group">\
							<label for="comment"><?php echo _("MAC address"); ?> :</label>\
							<input type="text" id="MAC" class="form-control" value="'+mac+'" placeholder="" >\
						</div>\
						<div class="form-group">\
							<label for="comment"><?php echo _("IP Address"); ?> :</label>\
							<input type="text" id="IP_ADDR" class="form-control" value="'+ip+'" placeholder="" >\
						</div>\
						<div class="form-group">\
							<label for="comment"><?php echo _("Subnet"); ?> :</label>\
							<input type="text" id="SUBNET" class="form-control" value="'+subnet+'" placeholder="" >\
						</div>\
						<div class="form-group">\
							<label for="comment"><?php echo _("Gateway"); ?> :</label>\
							<input type="text" id="GATEWAY" class="form-control" value="'+gateway+'" placeholder="" >\
						</div>\
						<div class="form-group">\
							<label for="comment"><?php echo _("DNS"); ?> :</label>\
							<input type="text" id="DNS" class="form-control" value="'+dns+'" placeholder="" >\
						</div>',
            data: {
                'key': key
            },
            buttons: [{
                label: '<?php echo _('Save'); ?>',
				data: {
					'key': key
				},
				cssClass: 'btn-default',
				action: function(dialogRef)
				{
					if( ( $("#IP_ADDR").val() != "" && $("#SUBNET").val() == "" ) ||
						( $("#IP_ADDR").val() == "" && $("#SUBNET").val() != "" ) ||
						( $("#IP_ADDR").val() == "" && $("#SUBNET").val() == "" && $("#GATEWAY").val() != "" ) ||
						( ( $("#IP_ADDR").val() != "" || $("#SUBNET").val() != "" || $("#GATEWAY").val() != "" || $("#DNS").val() != "") && $("#MAC").val() == "" ) ){
						
						alert("<?php echo _("Validation failed. Required information is missing or not valid."); ?>");
						return false;
					}
						
					if( $("#IP_ADDR").val() != "" && !ValidateIPaddress(  $("#IP_ADDR").val() )){
						alert("<?php echo _("You have entered an invalid IP address!"); ?>");
						return false;
					}
					
					if( $("#SUBNET").val() != "" && !ValidateIPaddress(  $("#SUBNET").val() )){
						alert("<?php echo _("You have entered an invalid SUBNET address!"); ?>");
						return false;
					}
					
					if( $("#GATEWAY").val() != "" && !ValidateIPaddress(  $("#GATEWAY").val() )){
						alert("<?php echo _("You have entered an invalid GATEWAY address!"); ?>");
						return false;
					}
					
					if( $("#DNS").val() != "" && !ValidateIPaddress(  $("#DNS").val() )){
						alert("<?php echo _("You have entered an invalid DNS address!"); ?>");
						return false;
					}
					
					var key = dialogRef.getData('key');
					
					network_setting[key] = {ip		: $("#IP_ADDR").val(),
											subnet	: $("#SUBNET").val(),
											gateway : $("#GATEWAY").val(),
											dns 	: $("#DNS").val(),
											mac 	: $("#MAC").val()};
					
					dialogRef.close();
					
				}				
            },
			{
                label: '<?php echo _('Cancel'); ?>',
				cssClass: 'btn-default',
				action: function(dialogRef)
				{
					dialogRef.close();
				}
			}]
        });
		
	}
	
	function createNetworkSelecter( key ){
		
		var formgroup = $("<div/>", {
		//	class: "form-group"
		});
		
		formgroup.append($("<label>", {
			for: "comment",
			style: "width:40%",
			text: "<?php echo _("VM Network Adapter"); ?>"+" "+key
		}));
		
		formgroup.append($("<label>", {
			for: "comment",
			style: "width:40%",
			text: "<?php echo _("VM Network"); ?>"+" "+key
		}));
		
		var net_select = $("<select/>", {
			type: "text",
			class: "selectpicker",
			id: "VM_Network" + key
		});
		
		var net_adapter_select = $("<select/>", {
			type: "text",
			class: "selectpicker",
			id: "VM_Network_Adapter" + key
		});
		
		var net_adapter_button = $("<button/>", {
			type: "text",
			class: "btn btn-primary btn-md",
			id: "NetwoekConfig" + key,
			text: "<?php echo _("Network Config"); ?>",
			onclick:"NetwoekConfig("+key+")"
		});
		
		net_select.attr('data-width', '40%');
		net_adapter_select.attr('data-width', '40%');
		
		formgroup.append(net_adapter_select);
		formgroup.append(net_select);
		formgroup.append(net_adapter_button);

		$("#network_adapters").append(formgroup);
	}
	
	function ValidateIPaddress(ipaddress) {  
		if (/^(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$/.test(ipaddress)) {  
			return (true)  
		}  
		return (false)  
	}  
	
	function SelectVMWareInstanceConfig(){
		
		var net = []

		for( key in adapters)
			net[key] = {network:$("#VM_Network"+key).val(),type:$("#VM_Network_Adapter"+key).val()};

		var SERVICE_SETTINGS = {
			'CPU':$("#Cpu").val(),
			'Memory':$("#Memory").val(),
			'NETWORK_UUID':net,
			'rcvy_pre_script':$("#RECOVERY_PRE_SCRIPT").val(),
			'rcvy_post_script':$("#RECOVERY_POST_SCRIPT").val(),
			'hostname_tag'	 :$("#InstanceName").val(),
			"VMWARE_STORAGE" : $("#VMWARE_STORAGE").val(),
			"VMWARE_FOLDER" : $("#VMWARE_FOLDER").val(),
			"VMWARE_SCSI_CONTROLLER" : $("#VMWARE_SCSI_CONTROLLER").val(),
			"CONVERT" : $('#CONVERT').is(":checked"),
			"VM_TOOL" : $('#VM_TOOL').is(":checked"),
			"FIRMWARE" : $("#FIRMWARE").val(),
			"NETWORK_SETTING" : network_setting,
			"OSTYPE" : $("#OSType").val(),
			"OSTYPE_DISPLAY" : $("#OSType option:selected").text()
		};
		
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'    		:'ServiceConfiguration',
				 'SERVICE_SETTINGS'	:SERVICE_SETTINGS
			},
			success:function(jso)
			{
				if (jso.Code == true)
				{
					window.location.href = NextPage;
				}
			},
			error: function(xhr)
			{
				
			}
		});
	}
	
	$(function(){

		$("#NetwoekConfig").click(function(){
			NetwoekConfig();
		})
		
		$("#SelectVMWareInstanceConfig").click(function( e ){
			
			SelectVMWareInstanceConfig();
			
		})
		
		$('#CONVERT').change(function() {
			
			ConvertSwitch( $(this).prop('checked') );
			
			$('.selectpicker').selectpicker('refresh');
		})
	
	});

</script>