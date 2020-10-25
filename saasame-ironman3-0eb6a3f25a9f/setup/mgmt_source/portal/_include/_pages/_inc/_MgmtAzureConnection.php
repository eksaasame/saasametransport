<?php if (!isset($_SESSION['CLUSTER_UUID'])){ $_SESSION['CLUSTER_UUID'] = null; }?>
<script>
	/* Determine URL Segment */
	QueryEndpoint();
	QueryAvailableService();
	
	function DetermineSegment($SET_SEGMENT){
		$URL_SEGMENT = window.location.pathname.split('/').pop();
		if ($URL_SEGMENT == $SET_SEGMENT)
		{
			QueryAccessCredentialInformation();
		}
		else
		{
			SUBSCRIPTION_ID = '';
			$('.selectpicker').selectpicker('refresh');
		}
	}

	function QueryEndpoint(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_azure.php',
			data:{
				 'ACTION'	 		:'QueryEndpoint'
			},
			success:function(jso)
			{
				$.each(jso, function(key,value)
				{
					$('#INPUT_ENDPOINT').append(new Option( key, value, true, false));
				});
				
				DetermineSegment('EditAzureConnection');

			}
		})
	}
	
	function getReplicaServiceNum( serverId ){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'    :'getReplicaServiceNum',
				 'ACCT_UUID' :'<?php echo $_SESSION['admin']['ACCT_UUID']; ?>',
				 'serverId'  :serverId
			},
			success:function( response )
			{
				rNum = response.replicaNum;
				
				if( response.replicaNum == 0 )
					$('#premisesTransport').prop('disabled', false);
				
				if( window.location.pathname.split('/').pop() != "EditAzureConnection" && $('#INPUT_STORAGE_CONNCTION_STRING').val() != ""){
					$('#premisesTransport').prop('disabled', false);
				}
				
				$('.selectpicker').selectpicker('refresh');
			}
		});
	}
	
	function QueryAvailableService(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'    :'QueryAvailableService',
				 'ACCT_UUID' :'<?php echo $_SESSION['admin']['ACCT_UUID']; ?>',
				 'SERV_TYPE' :'Carrier',
				 'SYST_TYPE' :''
			},
			success:function(jso)
			{
				if (jso != false)
				{
					option_level = '<?php echo $_SESSION['optionlevel']; ?>';
					if (option_level != 'USER')
					{
						$('.super_hidden').show();
					}
					
					$.each(jso, function(key,value)
					{
						var HOST_TYPE = value['VENDOR_NAME'];
						var mapObj = {UnknownVendorType:"General Purpose",OPENSTACK:"OpenStack",OpenStack:"OpenStack",AzureBlob:"Azure",Azure:"Azure",Aliyun:"Alibaba Cloud",Ctyun:"天翼云"};
						HOST_TYPE = HOST_TYPE.replace(/UnknownVendorType|OPENSTACK|AzureBlob|Aliyun|Ctyun/gi, function(matched){return mapObj[matched];});	

						if( window.location.pathname.split('/').pop() == 'EditAzureConnection' ){
							if( '<?php echo isset($_SESSION['CLUSTER_UUID'])?$_SESSION['CLUSTER_UUID']:null; ?>' == value['OPEN_UUID'] && value['SERV_INFO']["is_promote"] == true ){
								$('#premisesTransport').append(new Option('['+HOST_TYPE+'] '+this.HOST_NAME+' - '+this.SERV_ADDR, this.SERV_UUID , true, true));
								getReplicaServiceNum( this.LAUN_UUID );
							}else if( HOST_TYPE == "General Purpose" )
								$('#premisesTransport').append(new Option('['+HOST_TYPE+'] '+this.HOST_NAME+' - '+this.SERV_ADDR, this.SERV_UUID, true, false));
						}
						else if( HOST_TYPE == "General Purpose" )
							$('#premisesTransport').append(new Option('['+HOST_TYPE+'] '+this.HOST_NAME+' - '+this.SERV_ADDR, this.SERV_UUID, true, false));
					});
					
					if( $('#premisesTransport').val() == 0 )
						$('#premisesTransport').prop('disabled', false);
					
					$('.selectpicker').selectpicker('refresh');
				}		
			},
			error: function(xhr)
			{

			}
		});
	}

	/* Query Access Credential Information */
	function QueryAccessCredentialInformation(){
		
		$('#SELECT_SUBSCRIPTION_ID').prop('disabled', true);
		
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_azure.php',
			data:{
				 'ACTION'	 		:'QueryAccessCredentialInformation',
				 'CLUSTER_UUID'		:'<?php echo $_SESSION['CLUSTER_UUID']; ?>'	 
			},
			success:function(jso)
			{
				$('#INPUT_ACCESS_KEY').val(jso.ACCESS_KEY);
				$('#INPUT_SECRET_KEY').val(jso.SECRET_KEY);
				$('#INPUT_TENANT_ID').val(jso.TENANT_ID);
				$('#INPUT_STORAGE_CONNCTION_STRING').val(jso.CONNECTION_STRING);

				//just for display
				$('#SELECT_SUBSCRIPTION_ID').append(new Option(jso.SUBSCRIPTION_NAME,jso.SUBSCRIPTION_NAME, true, false));
				
				SUBSCRIPTION_ID = jso.SUBSCRIPTION_ID;
				
				$('#INPUT_ENDPOINT').val(jso.ENDPOINT);
				
				$('#INPUT_ENDPOINT').prop('disabled', true);
				
				$('.selectpicker').selectpicker('refresh');
				
				$('#BackToMgmt').remove();
				$("#CheckAndSubmit").text("<?php echo _('Update'); ?>");
			}
		})
	}

	/* Verify Azure Credential */
	function VerifyAzureAccessCredential(){
			$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_Azure.php',
			data:{
				 'ACTION'	 					:'VerifyAzureAccessCredential',
				 'AZ_USER_NAME' 				:$("#INPUT_USER_NAME").val(),
				 'AZ_PASSWORD' 					:$("#INPUT_PASSWORD").val(),
				 'AZ_ACCESS_KEY' 				:$("#INPUT_ACCESS_KEY").val(),
				 'AZ_SECRET_KEY' 				:$("#INPUT_SECRET_KEY").val(),
				 'AZ_TENANT_ID' 				:$("#INPUT_TENANT_ID").val(),
				 'AZ_STORAGE_CONNCTION_STRING' 	:$("#INPUT_STORAGE_CONNCTION_STRING").val(),
				 'AZ_ENDPOINT' 					:$("#INPUT_ENDPOINT").val()
				 //'AZ_SUBSCRIPTION_ID' 		:$("#SELECT_SUBSCRIPTION_ID").val()				 	 
			},
			success:function(jso)
			{
				$('#LoadingOverLay').removeClass('GrayOverlay GearLoading');
				if (jso.status != false)
				{
					//QueryAvailableService();
					
					$('#INPUT_USER_NAME').prop('disabled', true);
					$('#INPUT_PASSWORD').prop('disabled', true);
					$('#INPUT_ACCESS_KEY').prop('disabled', true);
					$('#INPUT_SECRET_KEY').prop('disabled', true);
					$('#INPUT_TENANT_ID').prop('disabled', true);
					$('#INPUT_STORAGE_CONNCTION_STRING').prop('disabled', true);
					$('#premisesTransport').prop('disabled', true);
				
					$('#SELECT_SUBSCRIPTION_ID').empty();
				
					$.each(JSON.parse(jso.msg).value, function(key,value)
					{
						$('#SELECT_SUBSCRIPTION_ID').append(new Option(value.displayName,JSON.stringify(value), true, false));
					});
					
					$('#SELECT_SUBSCRIPTION_ID').prop('disabled', false);
					if (SUBSCRIPTION_ID != '')
					{
						$("#SELECT_SUBSCRIPTION_ID").val(SUBSCRIPTION_ID);
					}
					
					$('.selectpicker').selectpicker('refresh');
					
					$('#CheckAndSubmit').prop('disabled', false);
					$('#CheckAndSubmit').removeClass('btn-default').addClass('btn-primary');
				}
				else
				{
					$('#VerifyConnection').prop('disabled',false);
					BootstrapDialog.show({
						title: '<?php echo _("Service Message"); ?>',
						type: BootstrapDialog.TYPE_DANGER,
						message: jso.msg,						
						draggable: true,
						buttons:[{
							label: '<?php echo _("Close"); ?>',
							action: function(dialogRef){
							dialogRef.close();
							}
						}],
					});
					$('#CheckAndSubmit').prop('disabled', true);					
					$('#VerifyConnection').removeClass('btn-default').addClass('btn-primary')
				}
			},
			error: function(xhr)
			{
				$('#LoadingOverLay').removeClass('GrayOverlay GearLoading');
			},
		});
	}
	
	
	/* Initialize New Azure Connection */
	function InitializeNewAzureConnection(){
		
		var subscription = JSON.parse( $("#SELECT_SUBSCRIPTION_ID").val() );
		
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_Azure.php',
			data:{
				 'ACTION'	 					:'InitializeNewAzureConnection',
				 'ACCT_UUID' 					:'<?php echo $_SESSION['admin']['ACCT_UUID']; ?>',
				 'REGN_UUID' 					:'<?php echo $_SESSION['admin']['REGN_UUID']; ?>',
				 //'AZ_USER_NAME' 				:$("#INPUT_USER_NAME").val(),
				 //'AZ_PASSWORD' 				:$("#INPUT_PASSWORD").val(),
				 'AZ_ACCESS_KEY' 			    :$("#INPUT_ACCESS_KEY").val(),
				 'AZ_SECRET_KEY' 			    :$("#INPUT_SECRET_KEY").val(),
				 'AZ_TENANT_ID' 			    :$("#INPUT_TENANT_ID").val(),
				 'AZ_SUBSCRIPTION_ID' 		    :subscription.subscriptionId,
				 'AZ_SUBSCRIPTION_NAME'			:subscription.displayName,
				 'AZ_STORAGE_CONNCTION_STRING'  :$("#INPUT_STORAGE_CONNCTION_STRING").val(),
				 'AZ_ENDPOINT' 				    :$("#INPUT_ENDPOINT").val(),
				 'AZ_ON_PREMISES_TRANSPORT'		:$('#premisesTransport').val()
			},
			success:function(jso)
			{
				$('#LoadingOverLay').removeClass('GrayOverlay GearLoading');
				$('#CheckAndSubmit').prop('disabled', true);
				if (jso.Code == true)
				{
					BootstrapDialog.show({
						title: '<?php echo _("Service Message"); ?>',
						message: jso.Msg,
						type: BootstrapDialog.type_info,
						draggable: true,
						buttons:[{
							label: '<?php echo _("Close"); ?>',
							action: function(dialogRef){
							dialogRef.close();
							}
						}],
						onhide: function(dialogRef){
							window.location.href = "MgmtCloudConnection"
						},
					});
				}
				else
				{
					BootstrapDialog.show({
						title: '<?php echo _("Service Message"); ?>',
						type: BootstrapDialog.type_danger,
						message: jso.Msg,						
						draggable: true,
						buttons:[{
							label: '<?php echo _("Close"); ?>',
							action: function(dialogRef){
							dialogRef.close();
							}
						}],
					});
    			}
			},
			error: function(xhr)
			{
				$('#CheckAndSubmit').prop('disabled', true);
        	}
		});	    
	}
	
	
	/* Update Azure Connection */
	function UpdateAzureConnection(){
		
		var subscription = JSON.parse( $("#SELECT_SUBSCRIPTION_ID").val() );
		
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_Azure.php',
			data:{
				 'ACTION'	 					:'UpdateAzureConnection',
				 'CLUSTER_UUID'					:'<?php echo $_SESSION['CLUSTER_UUID']; ?>',
				 //'AZ_USER_NAME' 				:$("#INPUT_USER_NAME").val(),
				 //'AZ_PASSWORD' 				:$("#INPUT_PASSWORD").val(),
				 'AZ_ACCESS_KEY' 				:$("#INPUT_ACCESS_KEY").val(),
				 'AZ_SECRET_KEY' 				:$("#INPUT_SECRET_KEY").val(),
				 'AZ_TENANT_ID' 				:$("#INPUT_TENANT_ID").val(),
				 'AZ_SUBSCRIPTION_ID' 		    :subscription.subscriptionId,
				 'AZ_SUBSCRIPTION_NAME'			:subscription.displayName,
				 'AZ_STORAGE_CONNCTION_STRING' 	:$("#INPUT_STORAGE_CONNCTION_STRING").val(),
				 'AZ_ENDPOINT' 					:$("#INPUT_ENDPOINT").val()	,
				 'AZ_ON_PREMISES_TRANSPORT'		:$('#premisesTransport').val()
			},
			success:function(jso)
			{
				$('#LoadingOverLay').removeClass('GrayOverlay GearLoading');
				$('#CheckAndSubmit').prop('disabled', true);
				if (jso.Code == true)
				{
					BootstrapDialog.show({
						title: '<?php echo _("Service Message"); ?>',
						message: jso.Msg,
						type: BootstrapDialog.type_info,
						draggable: true,
						buttons:[{
							label: '<?php echo _("Close"); ?>',
							action: function(dialogRef){
							dialogRef.close();
							}
						}],
						onhide: function(dialogRef){
							window.location.href = "MgmtCloudConnection"
						},
					});
				}
				else
				{
					BootstrapDialog.show({
						title: '<?php echo _("Service Message"); ?>',
						type: BootstrapDialog.type_danger,
						message: jso.Msg,						
						draggable: true,
						buttons:[{
							label: '<?php echo _("Close"); ?>',
							action: function(dialogRef){
							dialogRef.close();
							}
						}],
					});
    			}
			},
			error: function(xhr)
			{
				$('#CheckAndSubmit').prop('disabled', true);
        	}
		});	
	}
	
	/* Submit Trigger */
	$(function(){
		
		$('#INPUT_STORAGE_CONNCTION_STRING').on('change', function() {
			
			if( rNum > 0 ){
				$('#premisesTransport').prop('disabled', true);	
				$('.selectpicker').selectpicker('refresh');
				return;
			}
			
			if( $('#INPUT_STORAGE_CONNCTION_STRING').val() != "" )
				$('#premisesTransport').prop('disabled', false);
			else{
				$('#premisesTransport').prop('disabled', true);	
				$('#premisesTransport').val(0);	
			}
					
			$('.selectpicker').selectpicker('refresh');
				
		});


		$("#BackToMgmt").click(function(){
			window.location.href = "SelectCloudRegistration";
		})
		
		$("#CancelToMgmt").click(function(){
			window.location.href = "MgmtCloudConnection";
		})
		
		$("#VerifyConnection").click(function(){
			$("#LoadingOverLay").addClass("GrayOverlay GearLoading");
			$('#VerifyConnection').prop('disabled', true);
			$('#VerifyConnection').removeClass('btn-primary').addClass('btn-default');
			VerifyAzureAccessCredential();
		})
		
		$("#CheckAndSubmit").click(function(){
			$("#LoadingOverLay").addClass("GrayOverlay GearLoading");
			if ($URL_SEGMENT == 'AddAzureConnection')
			{				
				InitializeNewAzureConnection();
			}
			else
			{
				UpdateAzureConnection();
			}
		})
	});
</script>

<div id='container_wizard'>
	<div id='wrapper_block_wizard'>
		<div id='title_block_wizard'>
			<div id="title_h1">
				<img style="padding-bottom:2px;" src="data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAACAAAAAaCAYAAADWm14/AAADGXpUWHRSYXcgcHJvZmlsZSB0eXBlIGV4aWYAAHja7ZVBluQmDIb3nCJHQBJC4jgYzHu5QY6fH4yrq3r6TXqSWWRRUAYsCyH0ISqcf/05wh8o5JxCUvNcco4oqaTCFQOPV6mrpZhWuwrvT3h/kQeq8dagKOjlevW85SfkmE3bgrdtp0KuT4bKuT8crx/qNsS+F9jyeyGha4G4DYe6DQnvldP1fmxPc3F73kLb+uPeiV9PmE0S46yZLKFNHM1ywdg5JkPc+nT06Hui7oU+vYdbleETn0IS0ebpocB9canoC1qWzFOSMCbJS34FHfEsHKJhXO64/rz8zPNwu76RvyB9jD6hvnf1TDqsgN4q8olQfvRfykk/5OEZ6eL2tHLOe8Sv8iPdIf4gF258Y3Qf47x2V1PGlvPe1L3FNYLeMaO1ZmVUw6PRZ7BnLaiOlGg4Rx2H7EBtVIiBcVCiTpUGnatv1OBi4pMNPXNjoRYgdMAo3OSCi0qDDcg74LM0HAeBlB++0Fq2rOUaORbu5AGnh2CMMOU/1fAdpTFmQhHNWPYrVvCLZ6bBi0mOKESCGojQ2EHVFeC7fi6Tq8RJbYbZscEaj2kB+A+lj8MlC7RAUdFfCUzWtwGECB4onCEBgZhJlDJFYw5GhEA6AFW4zpL4ABZS5Q4nOQlSzBhZgLUxx2ipsvIlxkUIECo5iIENkhKwUlKcH0uOM1RVNKlqVlPXojVLnhmWs+V5o1YTS6aWzcytWA0unlw9u7l78Vq4CG5cLcjH4qWUWrFoheWK2RUKtR58yJEOPfJhhx/lqI1Dk5aattyseSutdu7Skcc9d+veS68nnThKZzr1zKedfpazDhy1ISMNHXnY8FHCqA9qG+sP9Reo0abGi9RUtAc1SM1uEzSvE53MQIwTAbhNAiSBeTKLTinxJDeZxcLICmU4qRNOp0kMBNNJrIMe7D7IacCl+Vu4BYDg30EuTHTfIPcjt6+o9br+6GQRmmk4gxoF2YdfZcfvH/vwXcW7Jy75dXB9CV+o/Krt1Yd/O/Ft6G3obeht6G3of21I8MdVwt9Ug1c78kJhGwAAAGJ6VFh0UmF3IHByb2ZpbGUgdHlwZSBpcHRjAAB42j2Jyw2AMAxD75mCEZyPSroOyYVbD+wvrErgyJbjJ/d6So6tMPEMixmN4P3S1oL5yZpuDldP5mQfmxTJxTXoQQKx/l5AXjK3FOJhV/szAAAPVGlUWHRYTUw6Y29tLmFkb2JlLnhtcAAAAAAAPD94cGFja2V0IGJlZ2luPSLvu78iIGlkPSJXNU0wTXBDZWhpSHpyZVN6TlRjemtjOWQiPz4KPHg6eG1wbWV0YSB4bWxuczp4PSJhZG9iZTpuczptZXRhLyIgeDp4bXB0az0iWE1QIENvcmUgNC40LjAtRXhpdjIiPgogPHJkZjpSREYgeG1sbnM6cmRmPSJodHRwOi8vd3d3LnczLm9yZy8xOTk5LzAyLzIyLXJkZi1zeW50YXgtbnMjIj4KICA8cmRmOkRlc2NyaXB0aW9uIHJkZjphYm91dD0iIgogICAgeG1sbnM6aXB0Y0V4dD0iaHR0cDovL2lwdGMub3JnL3N0ZC9JcHRjNHhtcEV4dC8yMDA4LTAyLTI5LyIKICAgIHhtbG5zOnhtcE1NPSJodHRwOi8vbnMuYWRvYmUuY29tL3hhcC8xLjAvbW0vIgogICAgeG1sbnM6c3RFdnQ9Imh0dHA6Ly9ucy5hZG9iZS5jb20veGFwLzEuMC9zVHlwZS9SZXNvdXJjZUV2ZW50IyIKICAgIHhtbG5zOnBsdXM9Imh0dHA6Ly9ucy51c2VwbHVzLm9yZy9sZGYveG1wLzEuMC8iCiAgICB4bWxuczpHSU1QPSJodHRwOi8vd3d3LmdpbXAub3JnL3htcC8iCiAgICB4bWxuczpkYz0iaHR0cDovL3B1cmwub3JnL2RjL2VsZW1lbnRzLzEuMS8iCiAgICB4bWxuczp4bXA9Imh0dHA6Ly9ucy5hZG9iZS5jb20veGFwLzEuMC8iCiAgIHhtcE1NOkRvY3VtZW50SUQ9ImdpbXA6ZG9jaWQ6Z2ltcDo3YmE0MjAzYS05ZTY5LTRkNjAtOGViYy1iNmZiMDkwNjFlZmMiCiAgIHhtcE1NOkluc3RhbmNlSUQ9InhtcC5paWQ6M2Q1MGI3ZDktMmJhOC00NDc1LWJjNWUtNjkyMjgwZWU3ZWYwIgogICB4bXBNTTpPcmlnaW5hbERvY3VtZW50SUQ9InhtcC5kaWQ6ZDc1N2JmMDctYjZmYi00NTZiLTg4NTktZGQ3ZmQ3YmY5YTAxIgogICBHSU1QOkFQST0iMi4wIgogICBHSU1QOlBsYXRmb3JtPSJXaW5kb3dzIgogICBHSU1QOlRpbWVTdGFtcD0iMTUzNzk0MjU2MjEzMzk0MCIKICAgR0lNUDpWZXJzaW9uPSIyLjEwLjYiCiAgIGRjOkZvcm1hdD0iaW1hZ2UvcG5nIgogICB4bXA6Q3JlYXRvclRvb2w9IkdJTVAgMi4xMCI+CiAgIDxpcHRjRXh0OkxvY2F0aW9uQ3JlYXRlZD4KICAgIDxyZGY6QmFnLz4KICAgPC9pcHRjRXh0OkxvY2F0aW9uQ3JlYXRlZD4KICAgPGlwdGNFeHQ6TG9jYXRpb25TaG93bj4KICAgIDxyZGY6QmFnLz4KICAgPC9pcHRjRXh0OkxvY2F0aW9uU2hvd24+CiAgIDxpcHRjRXh0OkFydHdvcmtPck9iamVjdD4KICAgIDxyZGY6QmFnLz4KICAgPC9pcHRjRXh0OkFydHdvcmtPck9iamVjdD4KICAgPGlwdGNFeHQ6UmVnaXN0cnlJZD4KICAgIDxyZGY6QmFnLz4KICAgPC9pcHRjRXh0OlJlZ2lzdHJ5SWQ+CiAgIDx4bXBNTTpIaXN0b3J5PgogICAgPHJkZjpTZXE+CiAgICAgPHJkZjpsaQogICAgICBzdEV2dDphY3Rpb249InNhdmVkIgogICAgICBzdEV2dDpjaGFuZ2VkPSIvIgogICAgICBzdEV2dDppbnN0YW5jZUlEPSJ4bXAuaWlkOjNkMjM4YTRkLTE5ODYtNGJhZC05YmFlLTE4MWY5N2VmMmUxYSIKICAgICAgc3RFdnQ6c29mdHdhcmVBZ2VudD0iR2ltcCAyLjEwIChXaW5kb3dzKSIKICAgICAgc3RFdnQ6d2hlbj0iMjAxOC0wOS0yNlQxNDoxNjowMiIvPgogICAgPC9yZGY6U2VxPgogICA8L3htcE1NOkhpc3Rvcnk+CiAgIDxwbHVzOkltYWdlU3VwcGxpZXI+CiAgICA8cmRmOlNlcS8+CiAgIDwvcGx1czpJbWFnZVN1cHBsaWVyPgogICA8cGx1czpJbWFnZUNyZWF0b3I+CiAgICA8cmRmOlNlcS8+CiAgIDwvcGx1czpJbWFnZUNyZWF0b3I+CiAgIDxwbHVzOkNvcHlyaWdodE93bmVyPgogICAgPHJkZjpTZXEvPgogICA8L3BsdXM6Q29weXJpZ2h0T3duZXI+CiAgIDxwbHVzOkxpY2Vuc29yPgogICAgPHJkZjpTZXEvPgogICA8L3BsdXM6TGljZW5zb3I+CiAgPC9yZGY6RGVzY3JpcHRpb24+CiA8L3JkZjpSREY+CjwveDp4bXBtZXRhPgogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgCjw/eHBhY2tldCBlbmQ9InciPz7o2ZxvAAAABmJLR0QAAAAAAAD5Q7t/AAAACXBIWXMAAA7EAAAOxAGVKw4bAAAAB3RJTUUH4gkaBhACBa3KbAAAABl0RVh0Q29tbWVudABDcmVhdGVkIHdpdGggR0lNUFeBDhcAAAKHSURBVEjHpZY7aFRBFIb/b3ddjbpGTLHxBUKQRcEES8FOCAg+wcIgNoIENK2gNjbBwsJXK1aCBMHOB2InCFoIBlRsjGihghGDKNFNzNicG8dx5u7d3YHDXc6ce84/Z//57wFAklPnC0k1Sd98p3NOwOIzXJm/bADosHhF0qikpqQP/6CyorHivr9kp3eJk6VOjKQVki5JOiDpuaRBSVVvv5hB2wawFrgLzAN7gQpwDxihzeUnjT1jNghMAg54CvQADeA78BpY0wmAoiffDXy04gvAEfOfNp8DznUDINWBMjAG/PAKvQRqwDLgmef/AmzuBEDqb1gOXAHmvCIOOGExO4BmsHfTQHfVAYAqcNva7Rd4C/RZzOVgzwE/gV3dkhDgqDE9LHDW9nuBqci+A54YQTsmYR/wJpL4E7DO3t8X6U5mv4HRIgBSazyR+IIXM5GIyew9UO8EQAOYiST8CgzYe+uBzy0AOOBiDsEpRWQYSeOSeiMyfEvSlP2umZS3WsdNpuMSHzn9cORaOdOAbQFxDwOzBbpwx65lSPr/AISi4ttEkERACThjhMsDMAfsj5E9BHAyweqmCU5sVYFrBbrwAliZR8K6p/Oh3QeW5BB5FfCwBYAF4FSeEF3Nuc/DBZRzg30f8kBMAxtjHBjKIdNjYGmB6ytguwlVHojrfo6MSA9y2naoDfUE2BN8NUP7ZXxa7MDBHBZP2tewiHj5/rHENySzRzZFUZE0I+lYNqwGQvFK0mxiVvRjsXmwX9IWST2SpiXVE+K0U9KIpBs2FBeeipFUlrRaUkPSViuYWb9NykUU8p2kIUjNzX/H7gFL7hfbZCcuq7t1vhWAslcoFedadDDbj8XN/wECew78D7nKnAAAAABJRU5ErkJggg==">&nbsp;<?php echo _("Cloud"); ?>
			</div>
		</div>
		
		 <div id='form_block_wizard' style=’max-width:1289.87px;’>
			<ul class='nav nav-wizard'>
				<li style='width:50%'>				 <a><?php echo _("Step 1 - Select Cloud"); ?></a></li>
				<li style='width:50%' class='active'><a><?php echo _("Step 2 - Verify Cloud Connection"); ?></a></li>
			</ul>
		</div>
		
		<div id='form_block_wizard'>
			<label for='comment'><?php echo _("Endpoint"); ?></label>
			<div class='form-group has-primary'>				
				<select id='INPUT_ENDPOINT' class='selectpicker' data-width="100%" placeholder='Endpoint'>
				<!--<option value = 0 >International</option>
				<option value = 1 >China</option>-->
				</select>
			</div>
			<!--<label for='comment'>User Name:</label>
			<div class='form-group has-primary'>				
				<input type='text' id='INPUT_USER_NAME' class='form-control' value='' placeholder='User Name'>	
			</div>
			<label for='comment'>Password:</label>
			<div class='form-group has-primary'>				
				<input type='text' id='INPUT_PASSWORD' class='form-control' value='' placeholder='Password'>	
			</div>-->
			<label for='comment'><?php echo _("Application Id"); ?></label>
			<div class='form-group has-primary'>				
				<input type='text' id='INPUT_ACCESS_KEY' class='form-control' value='' placeholder='Client Id'>	
			</div>
			
			<label for='comment'><?php echo _("Security Key"); ?></label>
			<div class='form-group has-primary'>				
				<input type='text' id='INPUT_SECRET_KEY' class='form-control' value='' placeholder='Client Secret'>	
			</div>
			<label for='comment'><?php echo _("Directory Id"); ?></label>
			<div class='form-group has-primary'>				
				<input type='text' id='INPUT_TENANT_ID' class='form-control' value='' placeholder='Tenant Id'>	
			</div>
			<label for='comment'><?php echo _("Subscription Id"); ?></label>
			<div class='form-group has-primary'>
				<select id="SELECT_SUBSCRIPTION_ID" class="selectpicker" data-width="100%" placeholder='Subscription Id' disabled></select>
			</div>
			
			<label for='comment'><?php echo _("Storage Connection String"); ?></label>
			<div class='form-group has-primary'>
				<input id="INPUT_STORAGE_CONNCTION_STRING"   class="form-control" value='' placeholder='Optional'>
			</div>
			
			<div class="super_hidden">
				<label for='comment'><?php echo _("On-premises Transport"); ?></label>
				<div class='form-group has-primary'>
					<select id="premisesTransport" class="selectpicker" data-width="100%" placeholder='Subscription Id' disabled>
						<option value="0">Normal</option>
					</select>
				</div>
			</div>
		</div>
					
		<div id='title_block_wizard'>
			<div class='btn-toolbar'>
				<button id='BackToMgmt' 		class='btn btn-default pull-left  btn-lg'><?php echo _("Back"); ?></button>
				<button id='CancelToMgmt' 		class='btn btn-default pull-left  btn-lg'><?php echo _("Cancel"); ?></button>
				<button id='CheckAndSubmit' 	class='btn btn-default pull-right btn-lg' disabled><?php echo _("Submit"); ?></button>	
				<button id='VerifyConnection' 	class='btn btn-primary pull-right btn-lg'><?php echo _("Verify Connection"); ?></button>
			</div>
		</div>		
	</div> <!-- id: wrapper_block-->
</div>
