# saasametransport
Saasame Transport is a fully-automated pure software tool for workload protection and migration in Hybrid Cloud. It handles both physical and virtual, windows and linux workloads. It protects source workloads with OS-level windows or linux agents, or hyperviser-level agentless mode for VMware VMs (virtual machines). The supported hybrid cloud environments can be any of the two clouds following as a source/target pair: AWS (Amazon Web Services), Microsoft Azure, Openstack, Alibaba Cloud, VMware (vCenter and vSphere) and On-premises. Both Global and China versions are supported for AWS, Azure and Aliababa. It can be integrated with any cloud platform or business applications via complete offering of REST API. It includes a management portal to break down the end-to-end workload protection and migration process to four easy steps: select a target cloud, select source workloads, configure replication and trigger recovery, to support the three main enterprise use cases:
1) Planned migration - source environment is shut down in coordinated fashion to move the latest workloads (data and binary) to the target cloud with virtually no data loss
2) Disaster Recovery - when any type of disaster taking place at the source, the workloads in the target cloud would be up and running in minutes based on the most recent snapshots
3) Development & Testing - workloads are brought up based on the most recent snapshots for Dev/Test or DR rehearsal purpose, replication still on-going from source to target cloud to support multiple tests.

More advanced features are also included in this code base as well. In addition to on-line replication, users can export source workloads to storage device and import to the target cloud as "offline" alternative, specifically designed for enterprises to move large number of workloads or enforce strict data center security. To further enhance the availability of target cloud, both parallel and cascading replication are supported, so users can select which target cloud to recover workloads based on disaster situation and cost consideration. 

Saasame Transport is a block-level tool, i.e. it moves disks from source to the target cloud. The key is to convert the source disks to be bootable in the target cloud, which includes automation to re-configure disk format, inject target drivers and other necessary changes. Launching recovered instances in the target cloud are automated as well, so the notoriously complicated process is simplied to a few clicks in the web-based management portal. Core modules such as the host agents and transport servers - replication and recovery code - are impemented in C++ and available in this repository. 

The web-based management portal and REST API interface are NOT included in this open source repository. It implements business logic of the use cases described above and exposes the full features of the host agents and Transport server code in this repository. Feel free to reach out (ek@saasame.com) to get more detail.
