<script>
	/* Begin to Automatic Exec */
	UnsetSession();
	ListCloudConnection();
		
	/* Session Unset */
	function UnsetSession(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION':'UnsetSession'
			},			
		});
	}
	
	/* List Cloud Connection */
	function ListCloudConnection(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'    :'ListCloud',
				 'ACCT_UUID' :'<?php echo $_SESSION['admin']['ACCT_UUID']; ?>'
			},
			success:function(jso)
			{
				$('.default').remove();
				if (jso.Code != false)
				{
					$('#SelectCloudRegistration').prop('disabled', false);
					$('#SelectCloudRegistration').removeClass('btn-default').addClass('btn-primary');
					
					if (jso != false)
					{
						$.each(jso, function(key,value)
						{
							switch (this.CLOUD_TYPE)
							{
								case 'OPENSTACK':
									if (this.VENDOR_TYPE == 'Huawei Cloud')
									{	
										mapObj = {"cn-north-1":"华北-北京一", "cn-east-2":"华东-上海二", "cn-south-1":"华南-广州", "cn-northeast-1":"东北-大连", "ap-southeast-1":"亚太-香港"};
										REGION_NAME = this.REGION_NAME.replace(/cn-north-1|cn-east-2|cn-south-1|cn-northeast-1|ap-southeast-1/gi, function(matched){return mapObj[matched];});
										
										Platform = this.VENDOR_TYPE;
										AccessKeyId = REGION_NAME+', '+this.PROJECT_NAME+', '+this.CLUSTER_USER;
									}
									else
									{
										Platform = 'OpenStack';
										AccessKeyId = this.CLUSTER_ADDR+', '+this.PROJECT_NAME+', '+this.CLUSTER_USER+', '+this.REGION_NAME;
									}
								break;
								
								case 'AWS':
									Platform = this.PROJECT_NAME;
									AccessKeyId = this.CLUSTER_USER;
								break;
								
								case 'Azure':
									Platform = 'Azure';
									AccessKeyId = JSON.parse(this.CLUSTER_USER)['TENANT_ID']+'  ('+JSON.parse(this.CLUSTER_USER)['SUBSCRIPTION_NAME']+')';
								break;
								
								case 'Aliyun':
									Platform = 'Alibaba Cloud';
									AccessKeyId = this.CLUSTER_USER;
								break;
								
								case 'Tencent':
									Platform = 'Tencent Cloud';
									AccessKeyId = this.CLUSTER_USER;
								break;
								
								case 'Ctyun':
									Platform = '天翼云';
									AccessKeyId = this.PROJECT_NAME;
								break;
								
								case 'VMWare':
									Platform = 'VMWare';
									AccessKeyId = this.CLUSTER_ADDR+' ('+this.TransportName+')';
								break;
								
								default:
									Platform = 'unknow';
								break;
							}							
							
							$('#ClusterTable > tbody').append(
							'<tr><td width="60px" class="TextCenter"><input type="radio" id="CLUSTER_UUID" name="CLUSTER_UUID" value="'+this.CLUSTER_UUID+'" /></td>\
								 <td width="150px">'+Platform+'</td>\
								 <td width="600px"><div class="AuthKeyOverFlow">'+AccessKeyId+'</div></td>\
								 <td width="150px">'+this.TIMESTAMP+'</td>\
							</tr>');
						});
						
						$("#ClusterTable").on("click", "#CLUSTER_UUID", function(e){
							OnClickSelectCloud();
						});
					}
					else
					{
						$('#ClusterTable > tbody').append('<tr><td colspan="5" class="padding_left_30"><?php echo _("Click New to add a Cloud connection."); ?></td></tr>');
					}
				}
				else
				{
					$('#ClusterTable > tbody').append('<tr><td colspan="5" class="padding_left_30"><?php echo _("Failed to coneect to database."); ?></td></tr>');
				}
			},
			error: function(xhr)
			{
				
			}
		});
	}
	
	/* OnClick Select Cloud */
	function OnClickSelectCloud()
	{
		$('#EditCloudConnection').prop('disabled', false);
		$('#DeleteCloudConnection').prop('disabled', false);

		$('#EditCloudConnection').removeClass('btn-default').addClass('btn-primary');
		$('#DeleteCloudConnection').removeClass('btn-default').addClass('btn-primary');	
	}
	
	/* Edit Cloud Connection */
	function EditSelectCloudConnection(){
			$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'    	:'EditSelectCloud',
				 'CLUSTER_UUID' :$("#ClusterTable input[type='radio']:checked").val(),
			},
			success:function(jso)
			{
				if (jso.Code == true)
				{
					switch (jso.Msg)
					{
						case 'OPENSTACK':
							window.location.href = "EditOpenStackConnection"
						break;
						
						case 'HuaweiCloud':
							window.location.href = "EditHuaweiCloudConnection"
						break;
						
						case 'AWS':
							window.location.href = "EditAwsConnection"
						break;
						
						case 'Azure':
							window.location.href = "EditAzureConnection"
						break;
						
						case 'Aliyun':
							window.location.href = "EditAliyunConnection"
						break;
						
						case 'Ctyun':
							window.location.href = "EditCtyunConnection"
						break;
						
						case 'Tencent':
							window.location.href = "EditTencentConnection"
						break;
						
						case 'VMWare':
							window.location.href = "EditVMWareConnection"
						break;
					}
				}
				else
				{
					BootstrapDialog.show({
						title: '<?php echo _("Connection Message"); ?>',
						message: jso.Msg,
						type: BootstrapDialog.TYPE_DANGER,
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
				
			}
		});		
	}
	
	
	/* Delete Cloud Connection */
	function DeleteSelectCloudConnection(){
			$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'    	:'DeleteSelectCloud',
				 'CLUSTER_UUID' :$("#ClusterTable input[type='radio']:checked").val(),
			},
			success:function(jso)
			{
				if (jso.Code == true)
				{
					BootstrapDialog.show({
						title: '<?php echo _("Message"); ?>',
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
						title: '<?php echo _("Message"); ?>',
						message: jso.Msg,
						type: BootstrapDialog.TYPE_DANGER,
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
				
			}
		});		
	}
	
	/* Submit Trigger */
	$(function(){
		$("#SelectCloudRegistration").click(function(){
			window.location.href = "SelectCloudRegistration";
		})
		
		$("#EditCloudConnection").click(function(){
			EditSelectCloudConnection();
		})
		
		$("#DeleteCloudConnection").click(function(){
			BootstrapDialog.confirm("<?php echo _("Do you want to delete the selected cloud connection?"); ?>", function(result){
				if (result == true)
				{
					DeleteSelectCloudConnection();
				}
			});
		})
	});
</script>

<div id='container'>
	<div id='wrapper_block'>	
		<div id='form_block'>
			<div class="page">
				<div id='title_block'>
					<div id="title_h1">
						<div class="btn-toolbar">
							<i class="fa fa-cloud fa-fw"></i>&nbsp;<?php echo _("Cloud"); ?>
							<button id='SelectCloudRegistration' 	class='btn btn-default pull-right' disabled><?php echo _("New"); ?></button>
							<button id='EditCloudConnection' 		class='btn btn-default pull-right' disabled><?php echo _("Edit"); ?></button>							
							<button id='DeleteCloudConnection' 		class='btn btn-default pull-right' disabled><?php echo _("Delete"); ?></button>
						</div>
					</div>										
				</div>
				
				<div id='form_block'>
					<table id="ClusterTable">
						<thead>
						<tr>
							<th width="60px" class="TextCenter">#</th>
							<th width="150px"><?php echo _("Platform"); ?></th>
							<th width="600px"><?php echo _("Username / Access key ID / Tenant ID"); ?></th>
							<th width="150px"><?php echo _("Time"); ?></th>						
						</tr>
						</thead>
					
						<tbody>
							<tr class="default">
								<td colspan="5" class="padding_left_30"><?php echo _("Checking management connection..."); ?></td>
							</tr>						
						</tbody>
					</table>
				</div>
			</div>
		</div> <!-- id: form_block-->		
	</div> <!-- id: wrapper_block-->
</div>