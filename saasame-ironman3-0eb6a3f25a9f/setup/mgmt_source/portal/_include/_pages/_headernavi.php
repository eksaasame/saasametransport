<div class='header'>
<div id='breadcrumb'>
	<div class="btn-toolbar" style="padding:15px 15px 0px 15px;">
		<a href="./"><img src="<?php setLogo(); ?>" style="position:relative; top:-20px;"></a>
		<a href="Logout" 	class="btn btn-success 	pull-right" 	role="button"><i class="fa fa-sign-out fa-fw"></i><?php echo _("Log out"); ?></a>				
		<a href="MgmtAcct" 	class="btn btn-info		pull-right" 	role="button"><i class="fa fa-cog fa-fw"></i><?php echo _("Settings"); ?></a>
		<a href="./" 		class="btn btn-warning	pull-right" 	role="button"><i class="fa fa-home fa-fw"></i><?php echo _("Main"); ?></a>
	</div>
	<div style="position:relative; top:-33px; padding:0px 15px 0px 15px; text-align: right; color:#C8C8C8;">
		<i class="fa fa-gg"></i>&nbsp;Build: <?php echo $_SESSION['version']; ?>&nbsp;<i class="fa fa-gg"></i>
	</div>
</div>
</div>
