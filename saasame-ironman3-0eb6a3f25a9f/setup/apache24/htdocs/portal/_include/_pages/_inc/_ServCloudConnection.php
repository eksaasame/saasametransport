<script>
	<!-- Begin to Automatic Exec -->
	ListCloud();
	
	/* List Cloud Connection */
	function ListCloud(){
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
				if (jso != false)
				{
					$.each(jso, function(key,value)
					{
						switch (this.CLOUD_TYPE)
						{
							case 'OPENSTACK':
								if (this.VENDOR_TYPE == 'Huawei Cloud')
								{	
									mapObj = {"cn-north-1":"华北-北京一", "cn-east-2":"华东-上海二", "cn-south-1":"华南-广州", "cn-northeast-1'":"东北-大连", "ap-southeast-1":"亚太-香港"};
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
								var json = JSON.parse(this.CLUSTER_USER);
								Platform = 'Azure';
								AccessKeyId = json['TENANT_ID']+"  ("+json['SUBSCRIPTION_NAME']+")";
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
							
							default:
								Platform = 'Unknow';
							break;
						}
						
						if( this.CLOUD_TYPE != "VMWare" )
							$('#ClusterTable > tbody').append('<tr>\
								<td width="60px" class="TextCenter"><input type="radio" id="CLUSTER_UUID" name="CLUSTER_UUID" value="'+this.CLUSTER_UUID+'|'+this.CLUSTER_ADDR+'|'+this.CLOUD_TYPE+'" /></td>\
								<td width="150px">'+Platform+'</td>\
								<td width="700px"><div class="AuthKeyOverFlow">'+AccessKeyId+'</div></td>\
								<td width="150px">'+this.TIMESTAMP+'</td>\
							</tr>');
					});

					$('#SelectAndNext').prop('disabled', false);
					$('#SelectAndNext').removeClass('btn-default').addClass('btn-primary');
				}
				else
				{
					$('#ClusterTable > tbody').append('<tr><td colspan="5" class="padding_left_30"><?php echo _("Please add a cloud connection"); ?></td></tr>');
				}
			},
			error: function(xhr)
			{
				
			}
		});
	}
	
	/* Select Cloud Connection UUID */
	function SelectCloudConnectionUUID(){
		$.ajax({
			type: 'POST',
			dataType:'JSON',
			url: '_include/_exec/mgmt_service.php',
			data:{
				 'ACTION'     :'SelectOpenStackClusterUUID', 
				 'CLOUD_INFO' :$("#ClusterTable input[type='radio']:checked").val()
			},
			success:function(jso)
			{
				if (jso.code == true)
				{
					switch( jso.RDIR )
					{
						case 'OPENSTACK':
							window.location.href = "SelectTransportInstance";
							break;
						case 'AWS':
							window.location.href = "SelectAwsTransportInstance";
							break;
						case 'Azure':
							window.location.href = "SelectAzureTransportInstance";
							break;
						case 'Aliyun':
							window.location.href = "SelectAliyunTransportInstance";
							break;
						case 'Tencent':
							window.location.href = "SelectTencentTransportInstance";
							break;
						case 'Ctyun':
							window.location.href = "SelectCtyunTransportInstance";
							break;
						default:
							window.location.href = "SelectTransportInstance";				
					}
				}
				else
				{
					BootstrapDialog.show({
					title: '<?php echo _("Service Message"); ?>',
					type: BootstrapDialog.TYPE_DANGER,
					message: jso.RDIR,						
					draggable: true,
					buttons:[{
						label: '<?php echo _("Close"); ?>',
						action: function(dialogRef){
						dialogRef.close();
						}}],
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
		$("#CancelToMgmt").click(function(){
			window.location.href = "MgmtTransportConnection";
		})
		
		$("#BackToLastPage").click(function(){
			window.location.href = "SelectServRegion";
		})
		
		$("#SelectAndNext").click(function(){
			SelectCloudConnectionUUID();
		})
	});
</script>

<div id='container_wizard'>
	<div id='wrapper_block_wizard'>	
		<div id='title_block_wizard'>
			<div id="title_h1">
				<i class="fa fa-train fa-fw"></i>&nbsp;<?php echo _("Transport"); ?>
			</div>										
		</div>
		
		<div id='title_block_wizard'>
			<ul class='nav nav-wizard'>
				<li style='width:25%'><a>				<?php echo _("Step 1 - Select Type"); ?></a></li>
				<li style='width:25%' class='active'><a><?php echo _("Step 2 - Select Connection"); ?></a></li>
				<li style='width:25%'><a>				<?php echo _("Step 3 - Select Server"); ?></a></li>
				<li style='width:25%'><a>	 			<?php echo _("Step 4 - Verify Services"); ?></a></li>
			</ul>
		</div>
			
		<div id='form_block_wizard'>
			<table id="ClusterTable">
				<thead>
				<tr>
					<th width="60px" class="TextCenter">#</th>
					<th width="150px"><?php echo _("Platform"); ?></th>
					<th width="700px"><?php echo _("Username / Access key ID / Tenant ID"); ?></th>
					<th width="150px"><?php echo _("Time"); ?></th>						
				</tr>
				</thead>
				<tbody></tbody>
			</table>
		</div>
		
		<div id='title_block_wizard'>		
			<div class='btn-toolbar'>
				<button id='BackToLastPage' 	class='btn btn-default pull-left  btn-lg'><?php echo _("Back"); ?></button>
				<button id='CancelToMgmt' 		class='btn btn-default pull-left  btn-lg'><?php echo _("Cancel"); ?></button>
				<button id='SelectAndNext' 		class='btn btn-default pull-right btn-lg' disabled><?php echo _("Next"); ?></button>				
			</div>
		</div>		
	</div> <!-- id: wrapper_block-->
</div>