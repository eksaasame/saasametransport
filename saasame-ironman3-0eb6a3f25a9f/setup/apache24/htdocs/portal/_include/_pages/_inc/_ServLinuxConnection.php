<script>
	<!-- Begin to Automatic Exec -->
	ListCloud();
	
	/* List OpenStack Connection */
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
								Platform = 'OpenStack';
								AccessKeyId = this.CLUSTER_USER;
								Disabled = '';
							break;
							
							case 'AWS':
								Platform = 'AWS';
								AccessKeyId = this.CLUSTER_USER;
								Disabled = '';
							break;
							
							case 'Azure':
								Platform = 'Azure';
								AccessKeyId = JSON.parse(this.CLUSTER_USER)['TENANT_ID'];
								Disabled = ' disabled';
							break;
							
							case 'Aliyun':
								Platform = 'Alibaba Cloud';
								AccessKeyId = this.CLUSTER_USER;
								Disabled = '';
								Disabled = ' disabled';
							break;
						}
							
						$('#ClusterTable > tbody').append('<tr>\
							<td class="TextCenter"><input type="radio" id="CLUSTER_UUID" name="CLUSTER_UUID" value="'+this.CLUSTER_UUID+'|'+this.CLUSTER_ADDR+'|'+this.CLOUD_TYPE+'" '+Disabled+'/></td>\
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
					$('#ClusterTable > tbody').append('<tr><td colspan="5" class="padding_left_30">Please add a cloud connection</td></tr>');
				}
			},
			error: function(xhr)
			{
			}
		});
	}
	
	/* Select Cloud Connection UUID */
	function SelectConnection(){
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
							window.location.href = "SelectLinuxLauncher";
							break;
						case 'AWS':
							window.location.href = "SelectAwsLinuxLauncher";
							break;
						case 'Azure':
							window.location.href = "SelectAzureLinuxLauncher";
							break;
						default:
							window.location.href = "SelectLinuxLauncher";				
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
			SelectConnection();
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
					<th width="185px"><?php echo _("Platform"); ?></th>
					<th width="165px"><?php echo _("Project / User"); ?></th>
					<th width="430px"><?php echo _("Username / Access key ID / Tenant ID"); ?></th>
					<th width="160px"><?php echo _("Time"); ?></th>						
				</tr>
				</thead>

				<tbody>
				</tbody>
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
