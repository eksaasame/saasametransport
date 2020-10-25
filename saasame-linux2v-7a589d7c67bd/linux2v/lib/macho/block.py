import re
import os
import shlex
import shutil
from functional import seq
from pathlib import Path
from subprocess import Popen, PIPE
import struct
import uuid
import time
import zlib

LBA_SIZE = 512
PRIMARY_GPT_LBA = 1
OFFSET_CRC32_OF_HEADER = 16
GPT_HEADER_FORMAT = '<8sIIIIQQQQ16sQIII420x'
GUID_PARTITION_ENTRY_FORMAT = '<16s16sQQQ72s'
RESCAN_SCSI_SLEEP_SECONDS = 0.5

class gpt_part_entry:
	def __init__(self, part_entry):
		self.PartitionType = str(uuid.UUID(bytes_le=part_entry[0])).upper()
		self.PartitionGuid = str(uuid.UUID(bytes_le=part_entry[1])).upper()
		self.StartLBA = part_entry[2]
		self.LastLBA = part_entry[3]
		self.AttributeFlags = part_entry[4]
		self.Name=part_entry[5].decode("utf-8", "ignore")

class mbr_part_entry:
	def __init__(self, data):
		self.BootableFlag = struct.unpack("<c", data[:1])[0]
		self.StartCHS0 = struct.unpack("<B", data[1:2])[0]
		self.StartCHS1 = struct.unpack("<B", data[2:3])[0]
		self.StartCHS2 = struct.unpack("<B", data[3:4])[0]
		self.PartitionType = struct.unpack("<c", data[4:5])[0]
		self.EndCHS0 = struct.unpack("<B", data[5:6])[0]
		self.EndCHS1 = struct.unpack("<B", data[6:7])[0]
		self.EndCHS2 = struct.unpack("<B", data[7:8])[0]
		self.StartLBA = struct.unpack("<I", data[8:12])[0]
		self.SizeInSectors = struct.unpack("<i", data[12:16])[0]

class mbr_partition_table:
	def __init__(self, data):
		self.DiskSignature0 = struct.unpack("<B", data[:1])[0]
		self.DiskSignature1 = struct.unpack("<B", data[1:2])[0]
		self.DiskSignature2 = struct.unpack("<B", data[2:3])[0]
		self.DiskSignature3 = struct.unpack("<B", data[3:4])[0]
		self.Unused = struct.unpack("<H", data[4:6])[0]
		self.Entry0 = mbr_part_entry(data[6:22])
		self.Entry1 = mbr_part_entry(data[22:38])
		self.Entry2 = mbr_part_entry(data[38:54])
		self.Entry3 = mbr_part_entry(data[54:70])
		self.Signature = struct.unpack("<H", data[70:72])[0]

class block:
	def __init__(self, name, path, size, major, minior):
		self.name = name
		self.path = path
		self.size = size
		self.major = major
		self.minior = minior

	def __str__(self):
		return str('{}({}:{}) = name : {}, path : {}, size : {} '.format(self.__class__, self.major, self.minior, self.name, self.path, self.size))
		
	def __repr__(self):
		return '{}({}:{}) = name : {}, path : {}, size : {}'.format(self.__class__, self.major, self.minior, self.name, self.path, self.size)
	
	@staticmethod
	def _read_file_contect(file_path):
		with(Path(file_path)).open(mode="r") as f:
			return f.read()
	@staticmethod
	def _run(cmd):
		args = shlex.split(cmd)
		try:
			process = Popen(args, stdout=PIPE, stderr=PIPE)
			communication = process.communicate()
			output = communication[0]
			retcode = process.returncode
			if retcode < 0:
				msg = "[{}], {}".format(-retcode, output)
				#log.warning(msg)
				print("warning {}".format(msg))
			return output.decode('utf-8')
		except OSError as e:
			print("error {}".format(e))
			#log.error(e)

class part(block):
	def __init__(self, name, start, size, major, minior, number):
		super().__init__(name, '/dev/{}'.format(name), size, major, minior)
		self.volumes=[]
		self.number = number
		self.partition_style = "mbr"
		self.start = start
		self.mbr_bootable_flag = b'\x00'
		self.mbr_partition_type = b'\x00'
		self.gpt_partition_type = '00000000-0000-0000-0000-000000000000'
		self.gpt_attribute_flags = 0
		self.gpt_partition_guid = '00000000-0000-0000-0000-000000000000'
		self.gpt_partition_name = ""
		self.fs=""
		self.is_current_system_volume = False
		self.mount_point=""
		output = super()._run('lsblk -P -p -o NAME,FSTYPE,MOUNTPOINT {}'.format(self.path))
		for line in output.split("\n"):
			dev_name = None
			for keyvar in line.split(" "):
				name, var = keyvar.partition("=")[::2]
				if name=="NAME":
					dev_name=var.replace("\"","")
				if dev_name == self.path:
					if name=="FSTYPE":
						self.fs = var.replace("\"","")
					elif name=="MOUNTPOINT":
						self.mount_point= var.replace("\"","")
						if var=="\"/boot\"" or var == "\"/boot/efi\"" or var=="\"/\"":
							self.is_current_system_volume = True
		for swap in super()._read_file_contect('/proc/swaps').split("\n"):
			if swap.startswith(self.path):
				self.is_current_system_volume = True
				break

	def set_mbr_part_attributes( self, mbr_entry ):
		self.partition_style = "mbr"
		self.mbr_bootable_flag = mbr_entry.BootableFlag
		self.mbr_partition_type = mbr_entry.PartitionType

	def set_gpt_part_attributes( self, gpt_entry ):
		self.partition_style = "gpt"
		self.gpt_partition_name = gpt_entry.Name
		self.gpt_partition_type = gpt_entry.PartitionType
		self.gpt_attribute_flags = gpt_entry.AttributeFlags
		self.gpt_partition_guid = gpt_entry.PartitionGuid

	def __str__(self):
		if self.partition_style == "mbr" :
			return str('{}({}:{}) = name : {}, number: {}, path : {}, start : {}, size : {}, mbr_bootable_flag : {}, mbr_partition_type : {}, fs: {}, current_system_volume: {}, volumes: {}, mount_point: {}'.format(self.__class__, self.major, self.minior, self.name, self.number, self.path, self.start, self.size, self.mbr_bootable_flag, self.mbr_partition_type,self.fs,self.is_current_system_volume, self.volumes, self.mount_point))
		else:
			return str('{}({}:{}) = name : {}, number: {}, path : {}, start : {}, size : {}, gpt_partition_type : {}, gpt_attribute_flags : {}, gpt_partition_guid: {}, gpt_partition_name : {}, fs: {}, current_system_volume: {}, volumes: {}, mount_point: {}'.format(self.__class__, self.major, self.minior, self.name, self.number, self.path, self.start, self.size, self.gpt_partition_type, self.gpt_attribute_flags, self.gpt_partition_guid, self.gpt_partition_name,self.fs,self.is_current_system_volume, self.volumes, self.mount_point))

	def __repr__(self):
		if self.partition_style == "mbr" :
			return '{}({}:{}) = name : {}, number: {}, path : {}, start : {}, size : {}, mbr_bootable_flag : {}, mbr_partition_type : {}, fs: {}, current_system_volume: {}, volumes: {}, mount_point: {}'.format(self.__class__, self.major, self.minior, self.name, self.number, self.path, self.start, self.size, self.mbr_bootable_flag, self.mbr_partition_type,self.fs,self.is_current_system_volume, self.volumes, self.mount_point)
		else:
			return '{}({}:{}) = name : {}, number: {}, path : {}, start : {}, size : {}, gpt_partition_type : {}, gpt_attribute_flags : {}, gpt_partition_guid: {}, gpt_partition_name : {}, fs: {}, current_system_volume: {}, volumes: {}, mount_point: {}'.format(self.__class__, self.major, self.minior, self.name, self.number,  self.path, self.start, self.size, self.gpt_partition_type, self.gpt_attribute_flags, self.gpt_partition_guid, self.gpt_partition_name,self.fs,self.is_current_system_volume, self.volumes, self.mount_point)

	def is_bootable(self):
		return self.mbr_bootable_flag == b'\x80' or uuid.UUID(self.gpt_partition_type) == uuid.UUID('21686148-6449-6E6F-744E-656564454649')

	def is_efi_system_partition(self):
		return uuid.UUID(self.gpt_partition_type) == uuid.UUID('C12A7328-F81F-11D2-BA4B-00A0C93EC93B') 
		
	def is_swap_partition(self):
		return self.mbr_partition_type == b'\x82' or uuid.UUID(self.gpt_partition_type) == uuid.UUID('0657FD6D-A4AB-43C4-84E5-0933C84B4F4F')

	def is_lvm_partition(self):
		return self.mbr_partition_type == b'\x8e' or uuid.UUID(self.gpt_partition_type) == uuid.UUID('E6D6D379-F507-44C2-A23C-238F2A3DF928')

	def is_extended_partition(self):
		return self.mbr_partition_type == b'\x05' or self.mbr_partition_type == b'\x0f'

class volume(block):
	def __init__(self, name, size, major, minior):
		block_path=Path('/sys/dev/block/{}:{}'.format(major, minior))
		size = int(super()._read_file_contect(block_path / 'size').split("\n")[0])
		name = super()._read_file_contect(block_path / 'dm/name').split("\n")[0]
		super().__init__(name, '/dev/mapper/{}'.format(name), size, major, minior)
		self.uuid = super()._read_file_contect(block_path / 'dm/uuid').split("\n")[0]
		self.slaves = []
		for folder in Path(block_path/ 'slaves').iterdir():
			self.slaves.append(folder.name)
		self.fs=""
		self.is_current_system_volume = False
		self.mount_point=""
		output = super()._run('lsblk -P -p -o NAME,FSTYPE,MOUNTPOINT {}'.format(self.path))
		for line in output.split("\n"):
			dev_name = None
			for keyvar in line.split(" "):
				name, var = keyvar.partition("=")[::2]
				if name=="NAME":
					dev_name=var.replace("\"","")
				if dev_name == self.path:
					if name=="FSTYPE":
						self.fs = var.replace("\"","")
					elif name=="MOUNTPOINT":
						self.mount_point= var.replace("\"","")
						if var=="\"/boot\"" or var == "\"/boot/efi\"" or var=="\"/\"":
							self.is_current_system_volume = True
		for swap in super()._read_file_contect('/proc/swaps').split("\n"):
			if swap.startswith(self.path):
				self.is_current_system_volume = True
				break

	def __str__(self):
		return str('{}({}:{}) = name : {}, path : {}, size : {}, uuid : {}, slaves: {}, fs: {}, current_system_volume: {}, mount_point: {} '.format(self.__class__, self.major, self.minior, self.name, self.path, self.size,self.uuid,self.slaves,self.fs,self.is_current_system_volume, self.mount_point))

	def __repr__(self):
		return '{}({}:{}) = name : {}, path : {}, size : {}, uuid : {}, slaves: {}, fs: {}, current_system_volume: {} , mount_point: {}'.format(self.__class__, self.major, self.minior, self.name, self.path, self.size,self.uuid,self.slaves,self.fs,self.is_current_system_volume, self.mount_point)
	
class disk(block):
	@staticmethod
	def __make_nop(byte):
		nop_code = 0x00
		pk_nop_code = struct.pack('=B', nop_code)
		nop = pk_nop_code*byte
		return nop

	@staticmethod
	def __unsigned32(n):
		return n&0xFFFFFFFF

	@staticmethod
	def __calc_header_crc32(fbuf, header_size):
		nop = disk.__make_nop(4)
		clean_header = fbuf[:OFFSET_CRC32_OF_HEADER] + nop + fbuf[OFFSET_CRC32_OF_HEADER+4:header_size]
		crc32_header_value = disk.__unsigned32(zlib.crc32(clean_header))
		return crc32_header_value
	
	@staticmethod
	def __get_lba(fhandle, entry_number, count):
		fhandle.seek(LBA_SIZE*entry_number)
		fbuf = fhandle.read(LBA_SIZE*count)
		return fbuf
	
	@staticmethod
	def __get_gpt_header(fhandle, fbuf, lba):
		fbuf = disk.__get_lba(fhandle, lba, 1)
		gpt_header = struct.unpack(GPT_HEADER_FORMAT, fbuf)
		crc32_header_value = disk.__calc_header_crc32(fbuf, gpt_header[2])
		return gpt_header, crc32_header_value, fbuf

	# get gpt partition entry
	@staticmethod
	def __get_gpt_part_entry(fbuf, offset, size):
		return struct.unpack(GUID_PARTITION_ENTRY_FORMAT, fbuf[offset:offset+size])

	@staticmethod
	def __get_gpt_part_table_area(f, gpt_header):
		part_entry_start_lba = gpt_header[10]
		first_use_lba_for_partitions = gpt_header[7]
		fbuf = disk.__get_lba(f, part_entry_start_lba, first_use_lba_for_partitions - part_entry_start_lba)
		return fbuf

	def __init__(self, name, size, major, minior):
		super().__init__(name, '/dev/{}'.format(name), size, major, minior)
		self.volumes=[]
		self.serial = ""
		self.partition_style = "raw"
		self.scsi_port = -1
		self.scsi_bus = -1
		self.scsi_target_id = -1
		self.scsi_lun = -1
		self.model = ""
		self.rev = ""
		self.vendor = ""
		self.id =""
		block_path=Path('/sys/dev/block/{}:{}'.format(major, minior))
		self.size = int(super()._read_file_contect(block_path / 'size').split("\n")[0])
		if os.path.islink(block_path / 'device'):
			link = os.path.realpath(block_path / 'device')
			elements = link.split('/')
			scsi = elements[len(elements)-1].split(':')
			if len(scsi) == 4 :
				self.scsi_port = int(scsi[0])
				self.scsi_bus = int(scsi[1])
				self.scsi_target_id = int(scsi[2])
				self.scsi_lun = int(scsi[3])
		if Path(block_path / 'serial').exists():
			self.serial = super()._read_file_contect(block_path / 'serial').split("\n")[0]
		if Path(block_path / 'device/vendor').exists():
			self.vendor = super()._read_file_contect(block_path / 'device/vendor').split("\n")[0]
		if Path(block_path / 'device/rev').exists():
			self.rev = super()._read_file_contect(block_path / 'device/rev').split("\n")[0]
		if Path(block_path / 'device/model').exists():
			self.model = super()._read_file_contect(block_path / 'device/model').split("\n")[0]
		self.parts = []
		for dir in block_path.iterdir():
			if dir.name.startswith(name):
				start = super()._read_file_contect(dir / 'start').split("\n")[0]
				dev = super()._read_file_contect(dir / 'dev').split("\n")[0].split(":")
				size = super()._read_file_contect(dir / 'size').split("\n")[0]
				number = int(dir.name.replace( name, ""))
				self.parts.append(part(dir.name, int(start), int(size), dev[0], dev[1], number))
		self.parts = sorted(self.parts, key=lambda part: part.start)
		try:
			f = open(self.path, 'rb')
			data = f.read(512)
			part_table = mbr_partition_table(data[440:])
			if part_table.Signature == 0xaa55:
				if part_table.Entry0.PartitionType == b'\xee':
					self.partition_style = "gpt"
					fbuf = ''
					gpt_header, crc32_header_value, gpt_buf = disk.__get_gpt_header(f, fbuf, PRIMARY_GPT_LBA)
					signature = gpt_header[0]
					revision = gpt_header[1]
					headersize = gpt_header[2]
					crc32_header = gpt_header[3]
					reserved = gpt_header[4]
					currentlba = gpt_header[5]
					backuplba = gpt_header[6]
					first_use_lba_for_partitions = gpt_header[7]
					last_use_lba_for_partitions = gpt_header[8]
					disk_guid = uuid.UUID(bytes_le=gpt_header[9])
					part_entry_start_lba = gpt_header[10]
					num_of_part_entry = gpt_header[11]
					size_of_part_entry = gpt_header[12]
					crc32_of_partition_array = gpt_header[13]

					self.id = str(disk_guid).lower()
					partition_table = disk.__get_gpt_part_table_area(f, gpt_header)
					crc32_part_value = disk.__unsigned32(zlib.crc32(partition_table))
					part_list = []
					for part_entry_num in range(0, num_of_part_entry):
						part_entry = disk.__get_gpt_part_entry(partition_table, size_of_part_entry * part_entry_num, size_of_part_entry)
						# first LBA, last LBA
						if part_entry[2] == 0 or part_entry[3] == 0:
							continue
						part_list.append(gpt_part_entry(part_entry))
					for gpt_part in part_list:
						for p in self.parts:
							if p.start == gpt_part.StartLBA:
								p.set_gpt_part_attributes(gpt_part)
								break
				else:
					self.partition_style = "mbr"
					self.id = "0x{0:02x}{1:02x}{2:02x}{3:02x}\n".format(part_table.DiskSignature3, part_table.DiskSignature2, part_table.DiskSignature1, part_table.DiskSignature0)
					part_entries = []
					part_entries.append(part_table.Entry0)
					part_entries.append(part_table.Entry1)
					part_entries.append(part_table.Entry2)
					part_entries.append(part_table.Entry3)
					for entry in part_entries:
						if entry.PartitionType == b'\x05' or entry.PartitionType == b'\x0f':
							start = entry.StartLBA
							while True :
								f.seek(LBA_SIZE*start)
								buf = f.read(LBA_SIZE)
								ext_part_table = mbr_partition_table(buf[440:])
								if ext_part_table.Signature != 0xaa55:
									break
								if ext_part_table.Entry0.PartitionType == b'\x05' or ext_part_table.Entry0.PartitionType == b'\x0f':
									start = entry.StartLBA + ext_part_table.Entry0.StartLBA
									continue
								elif ext_part_table.Entry0.StartLBA != 0 and ext_part_table.Entry0.SizeInSectors != 0 :
									ext_part_table.Entry0.StartLBA = start + ext_part_table.Entry0.StartLBA
									part_entries.append(ext_part_table.Entry0)
								else:
									break
								if ext_part_table.Entry1.PartitionType == b'\x05' or ext_part_table.Entry1.PartitionType == b'\x0f':
									start = entry.StartLBA + ext_part_table.Entry1.StartLBA
									continue
								elif ext_part_table.Entry1.StartLBA != 0 and ext_part_table.Entry1.SizeInSectors != 0 :
									ext_part_table.Entry1.StartLBA = start + ext_part_table.Entry1.StartLBA
									part_entries.append(ext_part_table.Entry1)
								else:
									break
					for p in self.parts:
						for entry in part_entries:
							if p.start == entry.StartLBA :
								p.set_mbr_part_attributes(entry)
								break
		except IOError:
			print ('[+] WARNING!! Can not open disk image.')

	def __str__(self):
		return str('{}({}:{}) = name : {}, path : {}, size : {}, serial : {}, partition_style : {}, vendor : {}, model : {}, rev : {}, scsi addr : {}:{}:{}:{}, id: {}, partitions: {}, volumes: {}'.format(self.__class__, self.major, self.minior, self.name, self.path, self.size, self.serial, self.partition_style, self.vendor, self.model, self.rev, self.scsi_port, self.scsi_bus, self.scsi_target_id, self.scsi_lun, self.id, self.parts, self.volumes ))

	def __repr__(self):
		return '{}({}:{}) = name : {}, path : {}, size : {}, serial : {}, partition_style : {}, vendor : {}, model : {}, rev : {}, scsi addr : {}:{}:{}:{}, id: {}, partitions: {}, volumes: {}'.format(self.__class__,self.major,self.minior, self.name, self.path, self.size, self.serial, self.partition_style, self.vendor, self.model, self.rev, self.scsi_port, self.scsi_bus, self.scsi_target_id, self.scsi_lun, self.id, self.parts, self.volumes)

	def is_gpt_partition_style(self):
		return self.partition_style == "gpt"

	def is_mbr_partition_style(self):
		return self.partition_style == "mbr"

	def is_raw_partition_style(self):
		return self.partition_style == "raw"

class storage:
	def __init__(self):
		self.disks=[]
		self.parts=[]
		self.volumes=[]
		self.rescan()
	
	@staticmethod
	def rescan_a_scsi_host(host_path):
		# https://blogs.it.ox.ac.uk/oxcloud/2013/03/25/rescanning-your-scsi-bus-to-see-new-storage/
		# Which should return a line like
		# /sys/class/scsi_host/host0/proc_name:mptspi
		# where host0 is the relevant field.
		# use this to rescan the bus with the following command
		scan_path = host_path / "scan"
		cmd = "echo \"- - -\" > {}".format(scan_path)
		os.system(cmd)
	
	@staticmethod
	def rescan_scsi_hosts():
		for host_path in Path("/sys/class/scsi_host/").iterdir():
			storage.rescan_a_scsi_host(host_path)
			time.sleep(RESCAN_SCSI_SLEEP_SECONDS)
		
	def rescan(self):
		self.disks.clear()
		self.parts.clear()
		self.volumes.clear()
		for line in block._read_file_contect(Path('/proc/partitions')).split("\n"):
			elements = line.split()
			if elements and len(elements) == 4 and elements[0] != 'major' :
				block_path=Path('/sys/dev/block/{}:{}'.format(int(elements[0]), int(elements[1])))
				if Path(block_path / 'device').exists():
					if elements[3].startswith('fd') or elements[3].startswith('sr'):
						continue
					self.disks.append(disk(elements[3], 0, int(elements[0]), int(elements[1])))
				elif Path(block_path / 'start').exists():
					continue
				elif Path(block_path / 'dm').exists():
					available=True
					for folder in Path(block_path/ 'slaves').iterdir():
						if not os.path.exists( '/dev/{}'.format(folder.name)):
							available=False
					if available:
						self.volumes.append(volume(elements[3], 0, int(elements[0]), int(elements[1])))
				
		for d in self.disks:
			for p in d.parts:
				self.parts.append(p)
				for v in self.volumes:
					for s in v.slaves:
						if s == p.name:
							p.volumes.append(v)
							d.volumes.append(v)
							break

	def prepare_bios_grub_boot_on_gpt_disk(self, disk_devices):
		gpt_disk = None
		for d in self.disks:
			found = False
			for dev in disk_devices:
				if dev == d.path :
					found = True
			if not found:
				continue
			if d.is_gpt_partition_style() : 
				has_boot_part = False
				has_efi_sys_part = False
				for p in d.parts :
					if p.is_efi_system_partition() :
						has_efi_sys_part = True
					if p.is_bootable() :
						has_boot_part = True
				if has_efi_sys_part == True and has_boot_part == False :
					gpt_disk = d
					break

		if gpt_disk :
			block._run("parted {} print Fix Fix".format(gpt_disk.path))
			free_part_number = 1
			parts = sorted(gpt_disk.parts, key=lambda part: part.number)
			for p in parts :
				if free_part_number == p.number :
					free_part_number = free_part_number + 1
			prev_end = 0
			free_ranges = dict()
			parts = sorted(gpt_disk.parts, key=lambda part: part.start)
			for p in parts :
				if prev_end == 0 :
					prev_end = p.start + p.size
				else:
					free_size = p.start - prev_end
					if prev_end < p.start and free_size > 4096 :
						free_ranges[prev_end] = p.start - prev_end
					prev_end = p.start + p.size
			if len(free_ranges) :
				for start, size in free_ranges.items():
					if size >= 4096 :
						block._run("sgdisk --new {}:{}:+2M --typecode={}:ef02 --change-name={}:\'BIOS boot partition\' {}".format(free_part_number,start,free_part_number,free_part_number,gpt_disk.path))
						block._run("parted {} set {} bios_grub on".format(gpt_disk.path, free_part_number))
			else:
				for p in gpt_disk.parts :
					if p.is_efi_system_partition() :
						block._run("fatresize {} -s {} -q".format( p.path, (p.size * LBA_SIZE) - (3 * 1024 * 1024)))
						self.rescan()
						for g in self.disks :
							if g.path == gpt_disk.path :
								prev_end = 0
								parts = sorted(g.parts, key=lambda part: part.start)
								for np in parts :
									if prev_end == 0 :
										prev_end = np.start + np.size
									else:
										free_size = np.start - prev_end
										if prev_end < np.start and free_size > 4096 :
											free_ranges[prev_end] = np.start - prev_end
										prev_end = np.start + np.size
								if len(free_ranges) == 0  and prev_end < p.start + p.size :
									free_ranges[prev_end] = p.start + p.size - prev_end
								if len(free_ranges) :
									for start, size in free_ranges.items():
										if size >= 4096 :
											block._run("sgdisk --new {}:{}:+2M --typecode={}:ef02 --change-name={}:\'BIOS boot partition\' {}".format(free_part_number,start,free_part_number,free_part_number,gpt_disk.path))
											block._run("parted {} set {} bios_grub on".format(gpt_disk.path, free_part_number))

def copytree(src, dst, symlinks=False, ignore=None):
	for item in os.listdir(src):
		s = os.path.join(src, item)
		d = os.path.join(dst, item)
		if os.path.isdir(s):
			shutil.copytree(s, d, symlinks, ignore)
		else:
			shutil.copy2(s, d)
				
def main():
	storage.rescan_scsi_hosts()
	stg = storage()
	for d in stg.disks:
		if d.path == "/dev/vdc" :
			for p in d.parts:
				print( "{} : {} - {}".format(p.name, p.start, p.size))
			parts = sorted(d.parts, key=lambda part: part.size)
			for p in parts:
				print( "{} : {} - {}".format(p.name, p.start, p.size))
			parts = sorted(d.parts, key=lambda part: part.number)
			for p in parts:
				print( "{} : {} - {}".format(p.name, p.start, p.size))
			parts = sorted(d.parts, key=lambda part: part.start)
			for p in parts:
				print( "{} : {} - {}".format(p.name, p.start, p.size))
	"""
	grub_lib= Path("/mnt")
	if os.path.exists(grub_lib):
		if not os.path.exists(grub_lib / "i386-pc"):
			os.makedirs(str(grub_lib / "i386-pc"), exist_ok=True)
			copytree("/usr/lib/grub/i386-pc", grub_lib / "i386-pc")
		if not os.path.exists(grub_lib/ "x86_64-efi"):
			os.makedirs(str(grub_lib / "x86_64-efi"), exist_ok=True)
			copytree(Path("/usr/lib/grub/x86_64-efi"), grub_lib/ "x86_64-efi" )
	storage.rescan_scsi_hosts()
	stg = storage()
	stg.prepare_bios_grub_boot_on_gpt_disk({"/dev/vdh"})
	stg.rescan()
	for p in stg.parts:
		print( "{} : {} - {}".format(p.name, p.start, p.size))
	for p in stg.parts:
		print( "{}".format(p))
		
	for d in stg.disks:
		print( "{}".format(d))
		
	for v in stg.volumes:
		print( "{}".format(v))
	
	for p in stg.parts:
		print( "{}".format(p))
		if p.is_bootable() :
			print("{} is bootable".format( p.name))
		elif p.is_efi_system_partition() :
			print("{} is efi system partition".format(p.name))
		elif p.is_swap_partition() :
			print("{} is swap partition".format( p.name))
		elif p.is_extended_partition() :
			print("{} is extended partition".format(p.name))
		elif p.is_lvm_partition() :
			print("{} is lvm partition".format(p.name))
	"""
if __name__ == '__main__':
	main()