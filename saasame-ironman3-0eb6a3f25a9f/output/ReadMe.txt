Build 178 :
	1. Upgrade virtio-win driver to latest stable version
	2. Preinstall vc++ 2015 runtime in transport installation package.
	3. For VMWare packer :
	   a. Change PackerFilter reg key location from SOFTWARE\SaaSaME to SOFTWARE\SaaSaMe\Transport.
	   b. Fixed exclude chunks defect.
	   c. Add more debug information into logger.  
	   d. Remove packer job & temp snapshot when cancel the job

Build 179:
	1. VMWare packer :
	   a. Improve packer agent's error handle.
	   b. Fixed remove packer job defect.
	   c. Fixed remove temp snapshot failure defect.
	   d. Remove useless lines.
	   e. Check and clean up the garbage temp snapshot if create temp snapshot error returns
	2. Fix loader deal loop issue when it can't write the data to disk.
	3. Calculate replicated data size for repeat_job

Build 180:
	1. implement aws_s3 support
	2. improve webdav to support customize port
	3. improve webdav and s3 connection to support proxy

Build 181:
        1. Handle mgmt vmdk data out of sync issue.
        2. Fixed TPipe problem not handle returns from virtual packer.
        3. Change report information and timing to mgmt for loader needs.

Build 182:
	1. Improve AWS S3 Support (Add AWS region enum) 
	2. Implement LOCAL_FOLDER_EX connection type to support multi-target connections scenario.

Build 183:
	1. Fix query system memory issue
	2. Improve remove job feature.

Build 184:
	1. Improve job removing operation 
	2. Fix win2k3 conversion support. ( upgrade viostor.sys driver and enhance irm_conv_agnt service)
	3. Uninstall VM Tools during booting on KVM or AWS
	4. Enhance loader job to handle unexpected disconnection with management server
	5. Fix carrier server crash issue
	6. Virtual Packer Stability improvements

Build 185:
        1. Filter out template type of vm to avoid the crash of later detail vm information query

Build 186:
	1. Improve virtual packer error handle and stability
