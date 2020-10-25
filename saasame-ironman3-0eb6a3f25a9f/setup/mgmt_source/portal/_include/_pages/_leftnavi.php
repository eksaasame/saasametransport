<div id="showLeftNav" class="showLeftNav" style="padding-left:0px;">
	<div id='cssmenu'>
	<ul>
		<!-- <li id='home'><a href='./'><i class="fa fa-home" aria-hidden="true"></i>&nbsp;Home</a></li> -->
		<li id='environment' class='has-sub'>
			<a href='#'><i class="fa fa-sitemap fa-fw"></i>&nbsp;<?php echo _("Environment"); ?></a>
			<ul>
				<li><a href='MgmtCloudConnection' 	  id='MgmtCloudConnection'><i class="fa fa-cloud fa-fw"></i>&nbsp;<?php echo _("Cloud"); ?></a></li>
				<li><a href='MgmtTransportConnection' id='MgmtTransportConnection'><i class="fa fa-subway fa-fw"></i>&nbsp;<?php echo _("Transport"); ?></a></li>
				<li><a href='MgmtHostConnection' 	  id='MgmtHostConnection'><i class="fa fa-server fa-fw"></i>&nbsp;<?php echo _("Host"); ?></a></li>
			</ul>
		</li>
		<li><a href='MgmtPrepareWorkload' id='MgmtPrepareWorkload'><i class="fa fa-files-o fa-fw"></i>&nbsp;<?php echo _("Replication"); ?></a></li>
		<li id='recovery' class='has-sub'>
			<a href='#'><i class="fa fa-mixcloud fa-fw"></i>&nbsp;<?php echo _("Recovery"); ?></a>
			<ul>
				<li><a href='MgmtRecoverWorkload' id='MgmtRecoverWorkload'><i class="fa fa-mixcloud fa-fw"></i>&nbsp;<?php echo _("Recovery"); ?></a></li>
				<li><a href='MgmtRecoverPlan' id='MgmtRecoverPlan'><i class="fa fa-clone fa-fw"></i>&nbsp;<?php echo _("Recovery Plan"); ?></a></li>		
			</ul>
		</li>
	</ul>
	</div>
</div>

<script>
	(function($){
		$(document).ready(function(){

		$('#cssmenu li.active').addClass('open').children('ul').show();
			$('#cssmenu li.has-sub>a').on('click', function(){
				$(this).removeAttr('href');
				var element = $(this).parent('li');
				if (element.hasClass('open')) {
					element.removeClass('open');
					element.find('li').removeClass('open');
					element.find('ul').slideUp(200);
				}
				else {
					element.addClass('open');
					element.children('ul').slideDown(200);
					element.siblings('li').children('ul').slideUp(200);
					element.siblings('li').removeClass('open');
					element.siblings('li').find('li').removeClass('open');
					element.siblings('li').find('ul').slideUp(200);
				}
			});
		});		
	})(jQuery);

	UrlLastSegment = window.location.href.split("/").pop();
	
	switch (UrlLastSegment) {
		case 'MgmtCloudConnection':
		case 'MgmtTransportConnection':
		case 'MgmtHostConnection':
			$('#environment').addClass("active" );
			$('#'+UrlLastSegment).addClass('Selected');
		break;
		
		case 'MgmtPrepareWorkload':
			$('#'+UrlLastSegment).addClass('Selected');
		break;

		case 'MgmtRecoverWorkload':
		case 'MgmtRecoverPlan':
			$('#recovery').addClass("active" );
			$('#'+UrlLastSegment).addClass('Selected');
		break;	
		
		case 'MgmtEnvironment':
		case 'MgmtAcct':
			$('#home').addClass("active");
			$('#home').addClass('Selected');			
		break;
		
		/* Left Navi Hidden */
		case 'AddOpenStackConnection':
		case 'EditOpenStackConnection':
		
		case 'AddAwsConnection':
		case 'EditAwsConnection':
	
		case 'AddHuaweiCloudConnection':
		case 'EditHuaweiCloudConnection':
	
		case 'AddVMWareConnection':
		case 'SelectListVMWareSnapshot':
		case 'InstanceVMWareConfigurations':
		case 'RecoverVMWareSummary':
		case 'EditVMWareConnection':
		case 'PlanInstanceVMWareConfigurations':
		case 'PlanRecoverVMWareSummary':
		case 'EditPlanInstanceVMWareConfigurations':
		case 'EditPlanRecoverVMWareSummary':
		
		case 'AddCtyunConnection':
		case 'EditCtyunConnection':
		case 'SelectCtyunTransportInstance':
		case 'VerifyCtyunTransportServices':
		case 'EditCtyunTransportServices':
		case 'SelectCtyunHostTransportInstance':
		case 'VerifyCtyunHostTransportType':
		case 'EditCtyunHostTransportType':
		case 'SelectListCtyunSnapshot':
		case 'InstanceCtyunConfigurations':
		case 'RecoverCtyunSummary':
		
		case 'AddAliyunConnection':
		case 'EditAliyunConnection':
		case 'SelectAliyunTransportInstance':
		case 'VerifyAliyunTransportServices':
		case 'EditAliyunTransportServices' :
		case 'SelectAliyunHostTransportInstance' :
		case 'VerifyAliyunHostTransportType' :
		case 'SelectListAliyunSnapshot' :
		case 'InstanceAliyunConfigurations' :
		case 'RecoverAliyunSummary' :
		
		case 'AddAzureConnection':
		case 'EditAzureConnection':
		case 'SelectAzureTransportInstance':
		case 'VerifyAzureTransportServices':
		case 'EditAzureTransportServices' :
		case 'SelectAzureHostTransportInstance' :
		case 'VerifyAzureHostTransportType' :
		case 'SelectListAzureSnapshot' :
		case 'InstanceAzureConfigurations' :
		case 'RecoverAzureSummary' :
		
		case 'AddTencentConnection':
		case 'EditTencentConnection':
		case 'SelectTencentTransportInstance':
		case 'VerifyTencentTransportServices':
		case 'EditTencentTransportServices' :
		case 'SelectTencentHostTransportInstance' :
		case 'VerifyTencentHostTransportType' :
		case 'SelectListTencentSnapshot' :
		case 'InstanceTencentConfigurations' :
		case 'RecoverTencentSummary' :
		
		case 'SelectCloudRegistration':
		case 'SelectServRegion':
		case 'ServCloudConnection':
		case 'ServLinuxConnection':
		case 'SelectLinuxLauncher':
		case 'SelectAwsLinuxLauncher':
		
		case 'VerifyLinuxLauncherService':
		case 'EditLinuxLauncherService':
		case 'VerifyAwsLinuxLauncherService':
		case 'EditAwsLinuxLauncherService':
		
		case 'SelectTransportInstance':
		case 'SelectAwsTransportInstance':
		case 'SelectHostTransportInstance':
		case 'SelectAwsHostTransportInstance':
		
		case 'VerifyCloudTransportServices':
		case 'EditCloudTransportServices':
		
		case 'VerifyAwsTransportServices':
		case 'EditAwsTransportServices':
		
		case 'VerifyHostTransportType':
		case 'EditHostTransportType':
		
		case 'VerifyAwsHostTransportType':
		case 'EditAwsHostTransportType':
		
		case 'VerifyOnPremiseTransportServices':
		case 'EditOnPremiseTransportServices':
		
		case 'VerifyRecoveryKitServices':
		case 'EditRecoveryKitServices':
		
		case 'SelectHostType':
		case 'VerifyHostPhysical':
		case 'EditHostPhysical':
				
		case 'VerifyHostVirtual':
		case 'EditHostVirtual':
		
		case 'VerifyHostCloud':
		case 'EditHostCloud':
		
		case 'VerifyHostOfflineClone':
		case 'EditHostOfflineClone':
				
		case 'SelectRegisteredHost':
		case 'SelectTargetTransportServer':
		case 'ConfigureWorkload':
		case 'ConfigureSchedule':
		case 'EditConfigureSchedule':
		case 'PrepareWorkloadSummary':
		
		case 'SelectRecoverHost':
		case 'SelectRecoverPlan':
		case 'SelectListSnapshot':
		case 'SelectListEbsSnapshot':		
		case 'InstanceConfigurations':
		case 'InstanceAwsConfigurations':
		case 'RecoverSummary':
		case 'RecoverAwsSummary':
		case 'RecoverKitSummary':
		case 'RecoverExportSummary':
		
		case 'PlanSelectRecoverReplica':
		case 'PlanSelectRecoverType':
		case 'EditPlanSelectRecoverType':
		
		case 'PlanSelectListSnapshot':
		case 'PlanSelectListEbsSnapshot':		
		
		case 'EditPlanSelectListSnapshot':
		case 'EditPlanSelectListEbsSnapshot':
		
		case 'PlanInstanceConfigurations':
		case 'PlanInstanceAwsConfigurations':
		case 'PlanInstanceAzureConfigurations':
		case 'PlanInstanceAliyunConfigurations':
		case 'PlanInstanceTencentConfigurations':
		case 'PlanInstanceCtyunConfigurations':
		
		case 'EditPlanInstanceConfigurations':
		case 'EditPlanInstanceAwsConfigurations':
		case 'EditPlanInstanceAzureConfigurations':
		case 'EditPlanInstanceAliyunConfigurations':
		case 'EditPlanInstanceTencentConfigurations':
		case 'EditPlanInstanceCtyunConfigurations':
				
		case 'PlanRecoverSummary':
		case 'PlanRecoverAwsSummary':
		case 'PlanRecoverAzureSummary':
		case 'PlanRecoverAliyunSummary':
		case 'PlanRecoverTencentSummary':
		case 'PlanRecoverCtyunSummary':

		case 'EditPlanRecoverSummary':
		case 'EditPlanRecoverAwsSummary':
		case 'EditPlanRecoverAzureSummary':
		case 'EditPlanRecoverAliyunSummary':
		case 'EditPlanRecoverTencentSummary':
		case 'EditPlanRecoverCtyunSummary':
			$('#showLeftNav').removeClass('showLeftNav');
			$('#showLeftNav').addClass('hideLeftNav');
		break;
		
		default:
			$('#MgmtPrepareWorkload').addClass('Selected');
	}
</script>
