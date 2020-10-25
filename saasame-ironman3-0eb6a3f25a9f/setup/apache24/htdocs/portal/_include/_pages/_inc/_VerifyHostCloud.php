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
								Platform = this.VENDOR_TYPE;
								AccessKeyId = this.CLUSTER_USER;
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

						}	
						
						if( this.CLOUD_TYPE != "VMWare" )
							$('#ClusterTable > tbody').append('<tr>\
								<td width="60px" class="TextCenter"><input type="radio" id="CLUSTER_UUID" name="CLUSTER_UUID" value="'+this.CLUSTER_UUID+'|'+this.CLUSTER_ADDR+'|'+this.CLOUD_TYPE+'" /></td>\
								<td width="185px">'+Platform+'</td>\
								<td width="165px">'+this.PROJECT_NAME+'</td>\
								<td width="430px"><div class="AuthKeyOverFlow">'+AccessKeyId+'</div></td>\
								<td width="160px">'+this.TIMESTAMP+'</td>\
							</tr>');
					});

					$('#SelectAndNext').prop('disabled', false);
					$('#SelectAndNext').removeClass('btn-default').addClass('btn-primary');
				}
				else
				{
					$('#ClusterTable > tbody').append('<tr><td colspan="8" class="padding_left_30"><?php echo _("Please add a cloud connection"); ?></td></tr>');
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
				 'ACTION'    	:'SelectOpenStackClusterUUID', 
				 'CLOUD_INFO'	:$("#ClusterTable input[type='radio']:checked").val()
			},
			success:function(jso)
			{
				if (jso.code == true)
				{
					if (jso.RDIR == 'OPENSTACK')
					{
						window.location.href = "SelectHostTransportInstance";
					}
					else if (jso.RDIR == 'Azure')
					{
						window.location.href = "SelectAzureHostTransportInstance";
					}
					else if (jso.RDIR == 'Aliyun')
					{
						window.location.href = "SelectAliyunHostTransportInstance";
					}
					else if (jso.RDIR == 'Tencent')
					{
						window.location.href = "SelectTencentHostTransportInstance";
					}
					else if (jso.RDIR == 'Ctyun')
					{
						window.location.href = "SelectCtyunHostTransportInstance";
					}
					else
					{
						window.location.href = "SelectAwsHostTransportInstance";
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
			window.location.href = "SelectHostType";
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
				<i class="fa fa-server fa-fw"></i>&nbsp;<?php echo _("Host"); ?>
			</div>										
		</div>
		
    <div id='title_block_wizard'>
      <ul class='nav nav-wizard'>
        <li style='width:25%'><a>				<?php echo _("Step 1 - Select Type"); ?> </a></li>
        <li style='width:25%' class='active'><a><?php echo _("Step 2 - Select Connection"); ?></a></li>
        <li style='width:25%'><a>				<?php echo _("Step 3 - Select Server"); ?></a></li>
        <li style='width:25%'><a>	 			<?php echo _("Step 4 - Verify Host"); ?></a></li>
      </ul>
    </div>
			
		<div id='form_block_wizard'>
			<table id="ClusterTable">
				<thead>
				<tr>
					<th width="60px" class="TextCenter">#</th>
					<th width="185px"><?php echo _("Platform"); ?></th>
					<th width="165px"><?php echo _("Project / User"); ?></th>
					<th width="430px"><?php echo _("Username / Access key ID / Tenant ID"); ?></th>
					<th width="160px"><?php echo _("Time"); ?></th>					
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
