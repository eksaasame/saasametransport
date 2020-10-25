[Build 1.1.892] (2018/10/17)
- Improve open license db logic
- Disable auto mount disk devices
- Update MariaDB runtime to 10.3.10
- Update PHP runtime to 7.2.11
- Update Apache runtime to Apache 2.4.35
- Add CTyun be able to created the instance in the other subnet
- Add identiry protocol and port for openstack cloud registration from UI
- Rewrite create launcher job function to handle more options
- Add server UUID for register on-premise transport server
- Many UI tweaks
- Improve the performance to query vmware hosts info
- Update multi language files
- Improve physical transport job to avoid dead lock
- Fixed trigger replication full sync does not work
- Add user be able to edit the OpenStack API endpoints information from UI
- UI width tweak
- Improve carrier to avoid unexpected crash(close) issue
- Avoid physical_packer cannot open completed image issue during CDR
- Move OpenStack AZ control to management.ini settings
- Add machine information when take xray for both transport server and physical packers
- The license information will not be include whe the mgmt does not check by xray function
- Add history hover tooltip when the charcaters length more than 80
- Fixed the translate msg casue page not function
- Fixed edit configure replication workload does not display the saved value correctly
- Update multi-language files
- Add error handle when packer job was missing unexpected.
- Fix packer retry issue
- Fixed cancel redirect to correct page in Cloud mgmt pages

[Build 1.1.885] (2018/9/8)
- Add multi receiver support for email notification
- Improve loader/launcher interrupt function to avoid unexpected status file leave after job removing
- Tweak click detail display error
- Fixed recovery kit disk match error
- Add Chinese char naming support for format host function (take snapshot)
- Fixed physical linux packer does not be able to match RCD disk issue
- Improve the loader job to save replication progress in another job to avoid dead lock and corrupted status file
- Improve performance for delete replica job
- Fixed delete replica does not work issue
- Fix dead lock issue
- Fixed OpenStack list float address only show by it own project.
- Fixed Export job with filter disk selection does not execute correctly in virtual packer
- Change default option for OpenStack create volume from image to true
- Update hidden download path
- Issue #243: Loader and Launcher getting volume address retry algorithm.
- Fix issue 366
- Update file download as hidden path
- Switch off default reporting post
- Add job history messages format as multi-languages support
- Fixed re-add virtual packer host that will cause host page blank
- Tweak delete replica job on the situation the job does not submit to transport server yet
- Tweak mutex lock logic when the IP cannot be connected
- Tweak file managment page
- Add file management feature in settings
- Add packer encryption options from UI
- Update Apache runtime
- Update PHP runtime
- Update MariaDB runtime
- Implement error handle when source snapshot is damaged
- Package mgmt for build
- Add log for post script decompression
- Fix linux dr and mg fail in unmanagement transport on Azure.
- Fixed trigger sync function will casue job corruption
- Fixed the double quotes in pre/post snapshot will also cause job corruption
- Change the delet wait time button to 60 seconds
- Improve callback logic

[Build 1.1.880] (2018/8/21)
- Fixed logic error for job configuration on number of snapshot
- Add disabled checkbox for interval settings
- Remove removed scheduled and loader job on DR and PM mode
- Add force delete in the debug mode
- Change snapshot control input from drop-down to input
- Tweak trigger sync logic
- Trun off openstack debug log
- Combine floating address information into first fixed address nic
- Changed while register transport server user input address(FQDN) will move to first privority.
- Chagned delete recovery job submut button can be re-trigger after 90 seconds.
- Fixed cannot attach data disk in tencent cloud.
- Avoid unexpected message when system reboot

[Build 1.1.776] (2018/1/23)
- Add back remove launcher job command on Delete services
- Change UI display base on change license logic
- Improve license check logic
- Config WinPE Launcher/Loader/Packer services recovery policy
- Disable rescan buttom on planned migration pages
- Add disk name when create prepare disk on Ali cloud
- Fixed missing flags for planned migration
- Aliyun add disk name when create prepare disk and recovery system disk.
- Modify Azure tag name.

[Build 1.1.772] (2017/12/27)
- Fixed LVM extended partition issue
- code optimization for better control
- make carrier/loader progress precent while bigger than 98/97 display as 100/100
- Create single interface for OpenStack like cloud
- Fixed when the instance create failed, the create instance into loop (ticket #769)
- Fixed some UI display bugs
- Fixed assign public IP interface does not work issue
- Add private ip interface for create instance
- Add exception handler in carrier/loader services 

[Build 1.1.770] (2017/12/21)
- Fixed AliCloud snapshot/disk nameing issues
- Fixed some display bugs from UI
- Update AliCloud SDK
- Update CSS

[Build 1.1.769] (2017/12/13)
- Rewrite delete service function to fit all types of recover workload situations
- Fixed Ali cloud image name issue
- Fixed GB display does not correct shows on RecoverKit and Export job
- Changed Code-Sign certification
- Fix installation package issue
- Support Linux Packer

[Build 1.1.768] (2017/11/27)
- Fix can't register win2k12r2 Simple Chinese version issue 

[Build 1.1.767] (2017/11/25)
- Added create cloud by partation size option in super user mode
- Added asynchronous call on long run script
- Tweak keep/delete instance on planned migration
- Fixed API calls when the prepare options deos not give values
- Added return job_uuid when the API call prepare/recovery workload
- Improve launcher_job to support Linux gpt partition conversion
- Change the snapshot order in Azure when DR.
- Fixed sync replica job
- Add asynchronous api support
- Update PHP runtime to 7.1.11
- Add message when RAM or OSS no permission.
- Add retry logic for update status
- Remove disk name when prepare, because the name maybe include invalid character in aliyun.
- Add error message when create disk fail in Aliyun.
- Aemove "exit" when cannot get new disk.
- Fix recovery summery in aliyun only can show one disk information.
- Delete the snapshot which create by aliyun when import vhd to image.
- Tweak ajax pull request frequence as smart control
- Purge unused image files
- Fix the loader job cancel image issue
- Fix the expire license count issue
- Support winpe reboot
- Fix launcher_job crash issue when shrink volume

[Build 1.1.765] (2017/10/25)
- Tweak recover boot_disk_id location
- Fixed UI detail network adapter name
- Improved register server check logic
- Add retry on get host information API
- Fixed multiple disk convert issue
- add public ipaddress configuration for aliyun.
- modify password rule for aliyun.
- add ModifyInstanceAttribute to set password after create instance for aliyun.
- Fixed cannot get disk SCSI address information issue
- Fix mac address issue
- Added paging on the cloud select instances page
- Fixed web port redirect issue
- Rollback the Azure storageAccountType issue
- Fixed in Aliyun cannot get mgmt server private IP address issue
- Add password row at aliyun reocvery instances detail
- Fixed sync message double display issue
- Improve the scheduler job logic to show "Pending for available job resource" message
- Remove duplicated files
- Fixed when replica run as run-once job and re-enable as interval job the task will not function.
- Avoid service crash issue

[Build 1.1.762] (2017/10/18)
- Fxied set upload OpenStack image as private
- Fixed hostname does not display correctly in some place
- Added mgmt host private address to callback URLs
- Tweak sycn buttom control
- Remove .htaccess redirct settings
- Fixed bugs use accessKey to name bucketname
- force to delete ali cloud image
- Update Apache runtime to httpd-2.4.28-win64-VC14
- Update MariaDB runtime to mariadb-10.2.9-winx64
- Update PHP runtime to php-7.1.10-Win32-VC14-x64
- Add sync_control UI on list replica job page
- Fixed simple chinese does not display well on the portal
- move verivyOss from get launcher job detail to pre create launcher job.
- improve the imagex save and loader exception handleing
- improve the logic to get the job detail
- improve logic for embedded Linux conversion
- fix RCD to recovery Linux OS issue
- fixed list_disk_addr_info error when DR in Azure
- fixed prepare workload source cloud type display wrong type.
- change the named of snapshot on Azure to diskname+timestamp
- add error handling for Azure
- disable the error message of "cannot connect loader"
- change customized id location to fix GPT disk issue
- add more log for set disk customized id
- fix simple Chinese vhd file name issue
- output crash dumps

[Build 1.1.756] 
1. improve vhd uploading performance
2.  fixed azure name to timestring and add tags hostname
3. fixed bug when recovery more than 2 disk host, network interface and security group will be filter all.

[Build 1.0.738]
1. Changed the license UI and fixed one important bug.
   - while the transport server cannot get the disk address, the prepare workload job cannot be deleted.
   - also add retry check to get the disk address. (retry 4 times sleep 15 seconds each time)
2. Fixed display wrong count value from license history UI
3. Add license history display from UI
4. Add support ESX 4.x
5. UI fixed, automatic jump to first tab when the licnese applied
6. Update Apache runtime to 2.4.27
7. Update Mariadb runtime to 10.2.28
8. Update PHP runtime to 7.1.9
9. Query select host information will realtime query transport server information
10. Update libssh2.dll
11. Scheduled to sync license database
12. Compress irm_transporter.exe
13. Rewrite disk flock control
14. Add display detail host information from select host pages
15. Add auto detect disk unique id for support
16. Generate Win 10 and Win 8.1 base ISO files
17. Return IP addresses info for virtual machine
18. Fix license time zone issue
19. Add conversion for hyper-v agent
20. Fix batch file issue
21. Output license info by irm_transporter command.

[Build 1.0.733]
1. Fixed in some situtation the prepare/recovery workload will double submit.
2. Fixed license UI append error when first license applied.
3. Filter out deleted item from license UI
4. Fixed when the transport server cannot get the disk address, the prepare workload job cannot be deleted.
5. Added retry check to get the disk address.

[Build 1.0.731]
1. Implement License Control (Online, Offline activation)
2. Implement Customized id
3. Fix recovery Win2k3 on AWS 
4. Remove time bomb
5. support XFS 5 checksum version
6. separate offline packer and RCD into different iso
7. excluded unused transport files
8. add irm_transporter service
9. Rewrite the cloud API switch call for better control
10. Add Australia/Sydeney timezone into UI layer
11. fix carrier damaged image issue
12. improve gpt_to_mbr function
13. support Aliyun
14. fix snapshots time display for local time
15. doesn't support ext2fs big alloc feature
16. User be able to assign aviable Elastic IP address from AWS recovery
17. Enable the interface for the others cloud
18. Add new option from UI, add extra one GB
19. Tweak icons from the UI
20. Encrypt transport interface files
21. Reduce transport interface include file
22. avoid to reset network for Alibaba Cloud
23. support priority_carrier option
24. support unknown partition style

[Build 1.0.718]
1. Implement service proxy to reduce the sock ports usage
2. Fix reboot RCD issue
3. Fix code issue for unique_id
4. Add some debug option for Linux Launcher
5. Implement UNIQUE_ID detection
6. Improve the Linux launcher emulator operation
7. Remove unused file from installation package
8. Add mutex for VHD disk attachment

[Build 1.0.709]
1. Support to convert linux export image by linux launcher emulator

[Build 1.0.708]
1. Add ssh client api
2. fix installation failure when firewall is disabled
3. Add parameter for conversion os type
4. Fixed Linux launcher cannnot be select on register the transport server.
5. Fixed export job cannot be submit
6. Tweak smart select transport server be more felexable
7. string masking username and password for function debug output
8. create a public ipaddress and network interface when lunch an instance on Azure service
9. delete all above things when terminated instance on Azure service
10. add OS type on export launcher convert

[Build 1.0.707]
1. improve the VSS error handle
2. correct the wrong log message
3. Add Azure support
4. Reduce Azure registration on number of keys
5. Add reclaimed Azure access token after it's expired
6. Fixed xray cannnot be taken when the mgmt. can not connect to server/host
7. Fixed logic for keep_alive mode
8. Update mariadb runtime to 10.2.6
9. Set default SQL server not in innodb_strict_mode mode
10. Tweak UI on submit job sometimes may failed.

[Build 1.0.706]
1. Fixed VHD/VHDX multiple thread write error
2. Enable XFS filesystem filter support(improve the code)
3. Fixed check_export_folder for multiple address issue.
4. Add more debug info for vss snapshot and cluster

[Build 1.0.704]
1. Fixed loader hang up issue
2. Fixed virtual packer cannot change link to other transport server
3. Add XFS filesystem filter support
4. Recovery kit pull mode support

[Build 1.0.703]
1. Add azure and aliyun support files
2. Update PHP runtime to 7.1.5
3. Update Mariadb runtime to 10.1.23
4. Fixed the missing flag for register AWS cloud
5. Update SQL upgrade script

...............................................................................................................
Used Ports for transport services :

netsh advfirewall firewall add rule name="Open Port 18888" dir=in action=allow protocol=TCP localport=18888
netsh advfirewall firewall add rule name="Open Port 18889" dir=in action=allow protocol=TCP localport=18889
netsh advfirewall firewall add rule name="Open Port 18890" dir=in action=allow protocol=TCP localport=18890
netsh advfirewall firewall add rule name="Open Port 18891" dir=in action=allow protocol=TCP localport=18891
netsh advfirewall firewall add rule name="Open Port 18892" dir=in action=allow protocol=TCP localport=18892
netsh advfirewall firewall add rule name="Open Port 18893" dir=in action=allow protocol=TCP localport=18893
netsh advfirewall firewall add rule name="Open Port 28891" dir=in action=allow protocol=TCP localport=28891


Code un-sync with hothatch:

1. carrier image push function
2. AWS S3 image operation
3. enable security socket
4. configure carrier_rw timeout value
5. disk serial number mapping
6. Offlin packer
7. Linux launcher
8. Multi language message support in services. 
9. Carrier Checksum filter