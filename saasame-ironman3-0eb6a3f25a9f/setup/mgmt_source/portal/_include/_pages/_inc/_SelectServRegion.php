<div id="container_wizard">
	<div id='wrapper_block_wizard'>	
		<div class="page">
			<div id='title_block_wizard'>
				<div id="title_h1">
					<i class="fa fa-train fa-fw"></i>&nbsp;<?php echo _("Transport"); ?>					
				</div>										
			</div>
			
			<div id='title_block_wizard'>
				<ul class='nav nav-wizard'>
					<li style='width:25%' class='active'><a><?php echo _("Step 1 - Select Type"); ?></a></li>
					<li style='width:25%'><a>				<?php echo _("Step 2 - Select Connection"); ?></a></li>
					<li style='width:25%'><a>				<?php echo _("Step 3 - Select Server"); ?></a></li>
					<li style='width:25%'><a>	 			<?php echo _("Step 4 - Verify Services"); ?></a></li>
				</ul>
			</div>
			
			<div id='form_block_wizard'>
				<div class="col-lg-12">
					<a href="VerifyOnPremiseTransportServices" 	class="btn btn-sq btn-window">		<i class="fa fa-server  fa-5x" style="padding:8px;"></i><br/><?php echo _("General Purpose"); ?></a>
					<a href="ServCloudConnection"  				class="btn btn-sq btn-cloud"> 		<i class="fa fa-mixcloud fa-5x" style="padding:8px;"></i><br/><?php echo _("Cloud"); ?></a>
					<a href="VerifyRecoveryKitServices"			class="btn btn-sq btn-RecoverKit">  <i class="fa fa-archive  fa-5x" style="padding:8px;"></i><br/><?php echo _("Recovery Kit"); ?></a>
					<?php
						if ($_SESSION['optionlevel'] != 'USER')
						{
							echo '<a href="ServLinuxConnection"	class="btn btn-sq btn-linux"><i class="fa fa-linux fa-5x" style="padding:8px;"></i><br/>Linux Launcher</a>';
						}
					?>
				</div>
			</div>
		</div>
	</div> <!-- id: wrapper_block-->	
</div><!-- /container -->
