-- phpMyAdmin SQL Dump
-- version 4.0.10.20
-- https://www.phpmyadmin.net
--
-- Host: 127.0.0.1
-- Generation Time: Jan 22, 2019 at 09:17 AM
-- Server version: 10.3.12-MariaDB
-- PHP Version: 7.3.1

SET SQL_MODE = "NO_AUTO_VALUE_ON_ZERO";
SET time_zone = "+00:00";


/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8 */;

--
-- Database: `management`
--

-- --------------------------------------------------------

--
-- Table structure for table `_ACCOUNT`
--

CREATE TABLE IF NOT EXISTS `_ACCOUNT` (
  `_ID` int(4) NOT NULL AUTO_INCREMENT,
  `_ACCT_UUID` varchar(36) COLLATE utf8_unicode_ci NOT NULL,
  `_ACCT_DATA` text COLLATE utf8_unicode_ci NOT NULL,
  `_TIMESTAMP` timestamp NOT NULL DEFAULT current_timestamp(),
  `_STATUS` varchar(1) COLLATE utf8_unicode_ci NOT NULL,
  PRIMARY KEY (`_ID`),
  UNIQUE KEY `_UUID` (`_ACCT_UUID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci AUTO_INCREMENT=1 ;

-- --------------------------------------------------------

--
-- Table structure for table `_ACCOUNT_REGN`
--

CREATE TABLE IF NOT EXISTS `_ACCOUNT_REGN` (
  `_ID` int(4) NOT NULL AUTO_INCREMENT,
  `_ACCT_UUID` varchar(36) COLLATE utf8_unicode_ci NOT NULL,
  `_REGN_UUID` varchar(36) COLLATE utf8_unicode_ci NOT NULL,
  `_REGN_NAME` varchar(25) COLLATE utf8_unicode_ci NOT NULL,
  `_REGN_DATA` text COLLATE utf8_unicode_ci NOT NULL,
  `_TIMESTAMP` timestamp NOT NULL DEFAULT current_timestamp(),
  `_STATUS` varchar(1) COLLATE utf8_unicode_ci NOT NULL,
  PRIMARY KEY (`_ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci AUTO_INCREMENT=1 ;

-- --------------------------------------------------------

--
-- Table structure for table `_CLOUD_DISK`
--

CREATE TABLE IF NOT EXISTS `_CLOUD_DISK` (
  `_ID` int(5) NOT NULL AUTO_INCREMENT,
  `_OPEN_DISK_UUID` varchar(255) COLLATE utf8_unicode_ci NOT NULL,
  `_OPEN_SERV_UUID` varchar(255) COLLATE utf8_unicode_ci NOT NULL,
  `_OPEN_DISK_ID` varchar(255) COLLATE utf8_unicode_ci NOT NULL,
  `_DEVICE_PATH` varchar(10) COLLATE utf8_unicode_ci NOT NULL,
  `_REPL_UUID` varchar(36) COLLATE utf8_unicode_ci NOT NULL,
  `_REPL_DISK_UUID` varchar(36) COLLATE utf8_unicode_ci NOT NULL,
  `_TIMESTAMP` datetime NOT NULL,
  `_STATUS` varchar(1) COLLATE utf8_unicode_ci NOT NULL,
  PRIMARY KEY (`_ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci AUTO_INCREMENT=1 ;

-- --------------------------------------------------------

--
-- Table structure for table `_CLOUD_MGMT`
--

CREATE TABLE IF NOT EXISTS `_CLOUD_MGMT` (
  `_ID` int(5) NOT NULL AUTO_INCREMENT,
  `_ACCT_UUID` varchar(36) COLLATE utf8_unicode_ci NOT NULL,
  `_REGN_UUID` varchar(36) COLLATE utf8_unicode_ci NOT NULL,
  `_CLUSTER_UUID` varchar(36) COLLATE utf8_unicode_ci NOT NULL,
  `_PROJECT_NAME` text COLLATE utf8_unicode_ci NOT NULL,
  `_CLUSTER_USER` text COLLATE utf8_unicode_ci NOT NULL,
  `_CLUSTER_PASS` text COLLATE utf8_unicode_ci NOT NULL,
  `_CLUSTER_ADDR` varchar(50) COLLATE utf8_unicode_ci NOT NULL,
  `_AUTH_TOKEN` text COLLATE utf8_unicode_ci NOT NULL,
  `_TIMESTAMP` datetime NOT NULL,
  `_STATUS` varchar(1) COLLATE utf8_unicode_ci NOT NULL,
  `_CLOUD_TYPE` int(4) NOT NULL DEFAULT 1,
  PRIMARY KEY (`_ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci AUTO_INCREMENT=1 ;

-- --------------------------------------------------------

--
-- Table structure for table `_JOBS_HISTORY`
--

CREATE TABLE IF NOT EXISTS `_JOBS_HISTORY` (
  `_ID` int(10) NOT NULL AUTO_INCREMENT,
  `_JOBS_UUID` varchar(36) COLLATE utf8_unicode_ci NOT NULL,
  `_HIST_JSON` text COLLATE utf8_unicode_ci NOT NULL,
  `_HIST_TIME` varchar(36) COLLATE utf8_unicode_ci NOT NULL DEFAULT '1970-01-01 00:00:00',
  `_SYNC_TIME` datetime NOT NULL,
  `_IS_DISPLAY` varchar(1) COLLATE utf8_unicode_ci NOT NULL DEFAULT 'Y',
  PRIMARY KEY (`_ID`),
  KEY `_HIST_TIME` (`_HIST_TIME`),
  KEY `_SYNC_TIME` (`_SYNC_TIME`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci AUTO_INCREMENT=1 ;

-- --------------------------------------------------------

--
-- Table structure for table `_REPLICA`
--

CREATE TABLE IF NOT EXISTS `_REPLICA` (
  `_ID` int(5) NOT NULL AUTO_INCREMENT,
  `_ACCT_UUID` varchar(36) COLLATE utf8_unicode_ci NOT NULL,
  `_REGN_UUID` varchar(36) COLLATE utf8_unicode_ci NOT NULL,
  `_REPL_UUID` varchar(36) COLLATE utf8_unicode_ci NOT NULL,
  `_CLUSTER_UUID` varchar(36) COLLATE utf8_unicode_ci NOT NULL,
  `_MGMT_ADDR` varchar(50) COLLATE utf8_unicode_ci NOT NULL,
  `_PACK_UUID` varchar(36) COLLATE utf8_unicode_ci NOT NULL,
  `_CONN_UUID` tinytext COLLATE utf8_unicode_ci NOT NULL,
  `_REPL_TYPE` varchar(36) COLLATE utf8_unicode_ci NOT NULL,
  `_JOBS_JSON` text COLLATE utf8_unicode_ci NOT NULL,
  `_WINPE_JOB` varchar(1) COLLATE utf8_unicode_ci NOT NULL DEFAULT 'N',
  `_REPL_HIST_JSON` text COLLATE utf8_unicode_ci NOT NULL,
  `_REPL_CREATE_TIME` datetime NOT NULL,
  `_REPL_DELETE_TIME` datetime NOT NULL,
  `_MODIFY_BY` varchar(50) COLLATE utf8_unicode_ci NOT NULL,
  `_STATUS` varchar(1) COLLATE utf8_unicode_ci NOT NULL,
  PRIMARY KEY (`_ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci AUTO_INCREMENT=1 ;

-- --------------------------------------------------------

--
-- Table structure for table `_REPLICA_DISK`
--

CREATE TABLE IF NOT EXISTS `_REPLICA_DISK` (
  `_ID` int(10) NOT NULL AUTO_INCREMENT,
  `_DISK_UUID` varchar(36) COLLATE utf8_unicode_ci NOT NULL,
  `_REPL_UUID` varchar(36) COLLATE utf8_unicode_ci NOT NULL,
  `_HOST_UUID` varchar(36) COLLATE utf8_unicode_ci NOT NULL,
  `_PACK_URI` text COLLATE utf8_unicode_ci NOT NULL,
  `_DISK_SIZE` bigint(15) NOT NULL DEFAULT 0,
  `_SCSI_ADDR` varchar(50) COLLATE utf8_unicode_ci NOT NULL,
  `_PURGE_DATA` varchar(1) COLLATE utf8_unicode_ci NOT NULL DEFAULT 'Y',
  `_OPEN_DISK` varchar(255) COLLATE utf8_unicode_ci NOT NULL DEFAULT '00000000-0000-0000-0000-000000000000',
  `_CBT_INFO` text COLLATE utf8_unicode_ci NOT NULL,
  `_TIMESTAMP` datetime NOT NULL,
  `_STATUS` varchar(1) COLLATE utf8_unicode_ci NOT NULL,
  PRIMARY KEY (`_ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci AUTO_INCREMENT=1 ;

-- --------------------------------------------------------

--
-- Table structure for table `_REPLICA_SNAP`
--

CREATE TABLE IF NOT EXISTS `_REPLICA_SNAP` (
  `_ID` int(10) NOT NULL AUTO_INCREMENT,
  `_REPL_UUID` varchar(36) COLLATE utf8_unicode_ci NOT NULL,
  `_DISK_UUID` varchar(36) COLLATE utf8_unicode_ci NOT NULL,
  `_SNAP_UUID` varchar(36) COLLATE utf8_unicode_ci NOT NULL,
  `_SNAP_NAME` text COLLATE utf8_unicode_ci NOT NULL,
  `_ORIGINAL_SIZE` bigint(15) NOT NULL DEFAULT 0,
  `_BACKUP_SIZE` bigint(15) NOT NULL DEFAULT 0,
  `_PROGRESS_SIZE` bigint(15) NOT NULL DEFAULT 0,
  `_OFFSET_SIZE` bigint(15) NOT NULL DEFAULT 0,
  `_LOADER_DATA` bigint(15) NOT NULL DEFAULT 0,
  `_TRANSPORT_DATA` bigint(15) NOT NULL DEFAULT 0,
  `_LOADER_TRIG` varchar(1) COLLATE utf8_unicode_ci NOT NULL DEFAULT 'N',
  `_SNAP_TIME` varchar(30) COLLATE utf8_unicode_ci NOT NULL,
  `_SNAP_OPEN` varchar(1) COLLATE utf8_unicode_ci NOT NULL DEFAULT 'N',
  `_CBT_INFO` text COLLATE utf8_unicode_ci NOT NULL,
  `_SNAP_OPTIONS` text COLLATE utf8_unicode_ci NOT NULL,
  `_TIMESTAMP` datetime NOT NULL,
  `_STATUS` varchar(1) COLLATE utf8_unicode_ci NOT NULL,
  PRIMARY KEY (`_ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci AUTO_INCREMENT=1 ;

-- --------------------------------------------------------

--
-- Table structure for table `_SERVER`
--

CREATE TABLE IF NOT EXISTS `_SERVER` (
  `_ID` int(5) NOT NULL AUTO_INCREMENT,
  `_ACCT_UUID` varchar(36) COLLATE utf8_unicode_ci NOT NULL,
  `_REGN_UUID` varchar(36) COLLATE utf8_unicode_ci NOT NULL,
  `_OPEN_UUID` varchar(36) COLLATE utf8_unicode_ci NOT NULL,
  `_HOST_UUID` text CHARACTER SET utf8 NOT NULL,
  `_SERV_UUID` varchar(36) COLLATE utf8_unicode_ci NOT NULL,
  `_SERV_ADDR` text COLLATE utf8_unicode_ci NOT NULL,
  `_HOST_NAME` varchar(255) COLLATE utf8_unicode_ci NOT NULL,
  `_SERV_INFO` text COLLATE utf8_unicode_ci NOT NULL,
  `_SERV_MISC` text COLLATE utf8_unicode_ci NOT NULL,
  `_SERV_TYPE` varchar(36) COLLATE utf8_unicode_ci NOT NULL,
  `_SYST_TYPE` varchar(10) COLLATE utf8_unicode_ci NOT NULL,
  `_LOCATION` varchar(15) COLLATE utf8_unicode_ci NOT NULL,
  `_TIMESTAMP` datetime NOT NULL,
  `_STATUS` varchar(1) COLLATE utf8_unicode_ci NOT NULL,
  PRIMARY KEY (`_ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci AUTO_INCREMENT=1 ;

-- --------------------------------------------------------

--
-- Table structure for table `_SERVER_CONN`
--

CREATE TABLE IF NOT EXISTS `_SERVER_CONN` (
  `_ID` int(5) NOT NULL AUTO_INCREMENT,
  `_ACCT_UUID` varchar(36) COLLATE utf8_unicode_ci NOT NULL,
  `_SCHD_UUID` varchar(36) COLLATE utf8_unicode_ci NOT NULL,
  `_CARR_UUID` varchar(36) COLLATE utf8_unicode_ci NOT NULL,
  `_LOAD_UUID` varchar(36) COLLATE utf8_unicode_ci NOT NULL,
  `_LAUN_UUID` varchar(36) COLLATE utf8_unicode_ci NOT NULL,
  `_CLUSTER_UUID` varchar(36) COLLATE utf8_unicode_ci NOT NULL,
  `_CONN_UUID` varchar(36) COLLATE utf8_unicode_ci NOT NULL,
  `_CONN_DATA` text COLLATE utf8_unicode_ci NOT NULL,
  `_CONN_TYPE` varchar(36) COLLATE utf8_unicode_ci NOT NULL,
  `_MGMT_ADDR` varchar(50) COLLATE utf8_unicode_ci NOT NULL,
  `_TIMESTAMP` datetime NOT NULL,
  `_STATUS` varchar(1) COLLATE utf8_unicode_ci NOT NULL,
  PRIMARY KEY (`_ID`),
  UNIQUE KEY `_ID` (`_ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci AUTO_INCREMENT=1 ;

-- --------------------------------------------------------

--
-- Table structure for table `_SERVER_DISK`
--

CREATE TABLE IF NOT EXISTS `_SERVER_DISK` (
  `_ID` int(10) NOT NULL AUTO_INCREMENT,
  `_HOST_UUID` varchar(36) COLLATE utf8_unicode_ci NOT NULL,
  `_DISK_UUID` varchar(36) COLLATE utf8_unicode_ci NOT NULL,
  `_DISK_NAME` text COLLATE utf8_unicode_ci NOT NULL,
  `_DISK_SIZE` bigint(15) NOT NULL DEFAULT 0,
  `_DISK_URI` text COLLATE utf8_unicode_ci NOT NULL,
  `_TIMESTAMP` datetime NOT NULL,
  `_STATUS` varchar(1) COLLATE utf8_unicode_ci NOT NULL,
  UNIQUE KEY `_ID` (`_ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci AUTO_INCREMENT=1 ;

-- --------------------------------------------------------

--
-- Table structure for table `_SERVER_HOST`
--

CREATE TABLE IF NOT EXISTS `_SERVER_HOST` (
  `_ID` int(5) NOT NULL AUTO_INCREMENT,
  `_ACCT_UUID` varchar(36) COLLATE utf8_unicode_ci NOT NULL,
  `_REGN_UUID` varchar(36) COLLATE utf8_unicode_ci NOT NULL,
  `_HOST_UUID` varchar(36) COLLATE utf8_unicode_ci NOT NULL,
  `_HOST_ADDR` text COLLATE utf8_unicode_ci NOT NULL,
  `_HOST_NAME` varchar(100) COLLATE utf8_unicode_ci NOT NULL,
  `_HOST_INFO` text COLLATE utf8_unicode_ci NOT NULL,
  `_SERV_UUID` varchar(36) COLLATE utf8_unicode_ci NOT NULL,
  `_SERV_TYPE` varchar(15) COLLATE utf8_unicode_ci NOT NULL,
  `_TIMESTAMP` datetime NOT NULL,
  `_STATUS` varchar(1) COLLATE utf8_unicode_ci NOT NULL,
  PRIMARY KEY (`_ID`),
  UNIQUE KEY `_ID` (`_ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci AUTO_INCREMENT=1 ;

-- --------------------------------------------------------

--
-- Table structure for table `_SERVICE`
--

CREATE TABLE IF NOT EXISTS `_SERVICE` (
  `_ID` int(5) NOT NULL AUTO_INCREMENT,
  `_ACCT_UUID` varchar(36) COLLATE utf8_unicode_ci NOT NULL,
  `_REGN_UUID` varchar(36) COLLATE utf8_unicode_ci NOT NULL,
  `_SERV_UUID` varchar(36) COLLATE utf8_unicode_ci NOT NULL,
  `_REPL_UUID` varchar(36) COLLATE utf8_unicode_ci NOT NULL,
  `_SNAP_JSON` text COLLATE utf8_unicode_ci NOT NULL,
  `_PACK_UUID` varchar(36) COLLATE utf8_unicode_ci NOT NULL,
  `_CONN_UUID` varchar(36) COLLATE utf8_unicode_ci NOT NULL,
  `_CLUSTER_UUID` varchar(36) COLLATE utf8_unicode_ci NOT NULL,
  `_FLAVOR_ID` varchar(36) COLLATE utf8_unicode_ci NOT NULL,
  `_NETWORK_UUID` varchar(255) COLLATE utf8_unicode_ci NOT NULL,
  `_SGROUP_UUID` text COLLATE utf8_unicode_ci NOT NULL,
  `_NOVA_VM_UUID` varchar(255) COLLATE utf8_unicode_ci NOT NULL,
  `_IMAGE_ID` text COLLATE utf8_unicode_ci DEFAULT NULL,
  `_TASK_ID` text COLLATE utf8_unicode_ci DEFAULT NULL,
  `_ADMIN_PASS` varchar(36) COLLATE utf8_unicode_ci NOT NULL,
  `_SERV_HIST_JSON` text COLLATE utf8_unicode_ci NOT NULL,
  `_JOBS_JSON` text COLLATE utf8_unicode_ci NOT NULL,
  `_OS_TYPE` varchar(5) COLLATE utf8_unicode_ci NOT NULL,
  `_TIMESTAMP` datetime NOT NULL,
  `_STATUS` varchar(1) COLLATE utf8_unicode_ci NOT NULL,
  PRIMARY KEY (`_ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci AUTO_INCREMENT=1 ;

-- --------------------------------------------------------

--
-- Table structure for table `_SERVICE_DISK`
--

CREATE TABLE IF NOT EXISTS `_SERVICE_DISK` (
  `_ID` int(10) NOT NULL AUTO_INCREMENT,
  `_DISK_UUID` varchar(36) COLLATE utf8_unicode_ci NOT NULL,
  `_SERV_UUID` varchar(36) COLLATE utf8_unicode_ci NOT NULL,
  `_HOST_UUID` varchar(36) COLLATE utf8_unicode_ci NOT NULL,
  `_DISK_SIZE` bigint(20) NOT NULL,
  `_SCSI_ADDR` varchar(50) COLLATE utf8_unicode_ci NOT NULL,
  `_OPEN_DISK` varchar(255) COLLATE utf8_unicode_ci NOT NULL,
  `_SNAP_UUID` varchar(255) COLLATE utf8_unicode_ci NOT NULL,
  `_TIMESTAMP` datetime NOT NULL,
  `_STATUS` varchar(1) COLLATE utf8_unicode_ci NOT NULL,
  PRIMARY KEY (`_ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci AUTO_INCREMENT=1 ;

-- --------------------------------------------------------

--
-- Table structure for table `_SERVICE_PLAN`
--

CREATE TABLE IF NOT EXISTS `_SERVICE_PLAN` (
  `_ID` int(5) NOT NULL AUTO_INCREMENT,
  `_ACCT_UUID` varchar(36) COLLATE utf8_unicode_ci NOT NULL,
  `_REGN_UUID` varchar(36) COLLATE utf8_unicode_ci NOT NULL,
  `_REPL_UUID` varchar(36) COLLATE utf8_unicode_ci NOT NULL,
  `_PLAN_UUID` varchar(36) COLLATE utf8_unicode_ci NOT NULL,
  `_PLAN_JSON` text COLLATE utf8_unicode_ci NOT NULL,
  `_TIMESTAMP` datetime NOT NULL,
  `_STATUS` varchar(1) COLLATE utf8_unicode_ci NOT NULL DEFAULT 'Y',
  PRIMARY KEY (`_ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci AUTO_INCREMENT=1 ;

-- --------------------------------------------------------

--
-- Table structure for table `_SYS_CLOUD_TYPE`
--

CREATE TABLE IF NOT EXISTS `_SYS_CLOUD_TYPE` (
  `_ID` int(4) NOT NULL AUTO_INCREMENT,
  `_CLOUD_TYPE` varchar(256) NOT NULL,
  PRIMARY KEY (`_ID`)
) ENGINE=InnoDB  DEFAULT CHARSET=latin1 ROW_FORMAT=COMPACT AUTO_INCREMENT=8 ;

--
-- Dumping data for table `_SYS_CLOUD_TYPE`
--

INSERT INTO `_SYS_CLOUD_TYPE` (`_ID`, `_CLOUD_TYPE`) VALUES
(1, 'OPENSTACK'),
(2, 'AWS'),
(3, 'Azure'),
(4, 'Aliyun'),
(5, 'Ctyun'),
(6, 'Tencent'),
(7, 'VMWare');

-- --------------------------------------------------------

--
-- Table structure for table `_SYS_CODES`
--

CREATE TABLE IF NOT EXISTS `_SYS_CODES` (
  `_CODE` varchar(3) COLLATE utf8_unicode_ci NOT NULL,
  `_CODE_MSG` varchar(50) COLLATE utf8_unicode_ci NOT NULL,
  `_STATUS` varchar(1) COLLATE utf8_unicode_ci NOT NULL DEFAULT 'Y',
  PRIMARY KEY (`_CODE`),
  UNIQUE KEY `_CODE` (`_CODE`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

-- --------------------------------------------------------

--
-- Table structure for table `_SYS_DEST_REF`
--

CREATE TABLE IF NOT EXISTS `_SYS_DEST_REF` (
  `_ID` int(3) NOT NULL AUTO_INCREMENT,
  `_REF_TYPE` varchar(25) COLLATE utf8_unicode_ci NOT NULL,
  `_REF_UUID` varchar(12) COLLATE utf8_unicode_ci NOT NULL,
  `_STATUS` varchar(1) COLLATE utf8_unicode_ci NOT NULL,
  PRIMARY KEY (`_ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci AUTO_INCREMENT=1 ;

-- --------------------------------------------------------

--
-- Table structure for table `_SYS_RESTFUL_REF`
--

CREATE TABLE IF NOT EXISTS `_SYS_RESTFUL_REF` (
  `_ID` int(10) NOT NULL AUTO_INCREMENT,
  `_URI_NAME` varchar(50) COLLATE utf8_unicode_ci NOT NULL,
  `_REF_NAME` varchar(50) COLLATE utf8_unicode_ci NOT NULL,
  `_STATUS` varchar(1) COLLATE utf8_unicode_ci NOT NULL,
  PRIMARY KEY (`_ID`),
  UNIQUE KEY `_REF_NAME` (`_REF_NAME`),
  UNIQUE KEY `_URI_NAME` (`_URI_NAME`),
  UNIQUE KEY `_ID` (`_ID`)
) ENGINE=InnoDB  DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci ROW_FORMAT=COMPACT AUTO_INCREMENT=13 ;

--
-- Dumping data for table `_SYS_RESTFUL_REF`
--

INSERT INTO `_SYS_RESTFUL_REF` (`_ID`, `_URI_NAME`, `_REF_NAME`, `_STATUS`) VALUES
(1, 'GetVirtualMachineStatus', '_GetVirtualMachineStatus.php', 'Y'),
(2, 'IdentificationRegistration', '_IdentificationRegistration.php', 'Y'),
(3, 'AmazonWebServices', '_AmazonWebServices.php', 'Y'),
(4, 'ServiceManagement', '_ServiceManagement.php', 'Y'),
(5, 'ReplicaManagement', '_ReplicaManagement.php', 'Y'),
(6, 'HotHatchManagement', '_HotHatchManagement.php', 'Y'),
(7, 'OpenStackManagement', '_OpenStackManagement.php', 'Y'),
(8, 'AzureWebServices', '_AzureWebServices.php', 'Y'),
(9, 'AliyunWebServices', '_AliyunWebServices.php', 'Y'),
(10, 'CtyunWebServices', '_CtyunWebServices.php', 'Y'),
(11, 'TencentWebServices', '_TencentWebServices.php', 'Y'),
(12, 'VMWareWebServices', '_VMWareWebServices.php', 'Y');

-- --------------------------------------------------------

--
-- Table structure for table `_SYS_SYNC_SERVER`
--

CREATE TABLE IF NOT EXISTS `_SYS_SYNC_SERVER` (
  `_ID` int(1) NOT NULL AUTO_INCREMENT,
  `_SYNC_UUID` varchar(36) COLLATE utf8_unicode_ci NOT NULL,
  `_SYNC_TOKEN` varchar(36) COLLATE utf8_unicode_ci NOT NULL,
  `_TIMESTAMP` datetime NOT NULL,
  `_STATUS` varchar(1) COLLATE utf8_unicode_ci NOT NULL,
  PRIMARY KEY (`_ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci AUTO_INCREMENT=1 ;

-- --------------------------------------------------------

--
-- Table structure for table `_SYS_SYNC_TARGET`
--

CREATE TABLE IF NOT EXISTS `_SYS_SYNC_TARGET` (
  `_ID` int(1) NOT NULL AUTO_INCREMENT,
  `_SYNC_UUID` varchar(36) COLLATE utf8_unicode_ci NOT NULL,
  `_SYNC_SERVER` varchar(36) COLLATE utf8_unicode_ci NOT NULL,
  `_SYNC_ADDR` varchar(50) COLLATE utf8_unicode_ci NOT NULL,
  `_SYNC_TOKEN` varchar(36) COLLATE utf8_unicode_ci NOT NULL,
  `_TIMESTAMP` datetime NOT NULL,
  `_STATUS` varchar(1) COLLATE utf8_unicode_ci NOT NULL,
  PRIMARY KEY (`_ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci AUTO_INCREMENT=1 ;

/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
