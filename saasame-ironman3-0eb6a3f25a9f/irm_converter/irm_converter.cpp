
#include "irm_converter.h"
#include "irm_disk.h"
#include <codecvt>
#include "reg_edit_ex.h"
#include "bcd_edit.h"
#include "boot_ini.h"
#include "irm_conv.h"
#include "..\gen-cpp\universal_disk_rw.h"
#include "..\gen-cpp\saasame_constants.h"
#include "..\irm_host_mgmt\vhdx.h"
#include "json_spirit.h"

using namespace macho;
using namespace macho::windows;
using namespace json_spirit;
#define BUFSIZE 512
#define BUFSIZE2 1024
#define GUID_FMT                            _T("{%.8x-%.4x-%.4x-%.2x%.2x-%.2x%.2x%.2x%.2x%.2x%.2x}")
#define GUID_PRINTF( X )                                    \
    (X).Data1, (X).Data2, (X).Data3,                        \
    (X).Data4[0], (X).Data4[1], (X).Data4[2], (X).Data4[3], \
    (X).Data4[4], (X).Data4[5], (X).Data4[6], (X).Data4[7]

#define NTSIGNATURE(ptr) ((LPVOID)((BYTE *)(ptr) + ((PIMAGE_DOS_HEADER)(ptr))->e_lfanew))
#define SIZE_OF_NT_SIGNATURE (sizeof(DWORD))
#define PEFHDROFFSET(ptr) ((LPVOID)((BYTE *)(ptr)+((PIMAGE_DOS_HEADER)(ptr))->e_lfanew+SIZE_OF_NT_SIGNATURE))
#define OPTHDROFFSET(ptr) ((LPVOID)((BYTE *)(ptr)+((PIMAGE_DOS_HEADER)(ptr))->e_lfanew+SIZE_OF_NT_SIGNATURE+sizeof(IMAGE_FILE_HEADER)))
#define SECHDROFFSET(ptr) ((LPVOID)((BYTE *)(ptr)+((PIMAGE_DOS_HEADER)(ptr))->e_lfanew+SIZE_OF_NT_SIGNATURE+sizeof(IMAGE_FILE_HEADER)+sizeof(IMAGE_OPTIONAL_HEADER)))
#define RVATOVA(base,offset) ((LPVOID)((DWORD)(base)+(DWORD)(offset)))
#define VATORVA(base,offset) ((LPVOID)((DWORD)(offset)-(DWORD)(base)))

#define REG_HIVE_PATH                       _T("\\system32\\config")  
#define PENDING_UPDATE_FILE                 _T("\\Winsxs\\pending.xml")
#define PENDING_UPDATE_REG_KEY              _T("\\Microsoft\\Windows\\CurrentVersion\\WindowsUpdate\\Auto Update\\RebootRequired")
#define PENDING_FILE_RENAME_REG_KEY         _T("\\CurrentControlSet\\Control\\Session Manager\\PendingFileRenameOperations")
#define COMPONENT_BASE_SERVICING_REG_KEY    _T("\\Microsoft\\Windows\\CurrentVersion\\Component Based Servicing\\RebootPending")

const struct _TIMEZONE {
    int    index;
    TCHAR* timezone;
    TCHAR* display;
} timezones_info[] = {
    { 0, _T("Dateline Standard Time"), _T("(GMT-12:00) International Date Line West") },
    { 1, _T("Samoa Standard Time"), _T("(GMT-11:00) Midway Island, Samoa") },
    { 2, _T("Hawaiian Standard Time"), _T("(GMT-10:00) Hawaii") },
    { 3, _T("Alaskan Standard Time"), _T("(GMT-09:00) Alaska") },
    { 4, _T("Pacific Standard Time"), _T("(GMT-08:00) Pacific Time (US and Canada); Tijuana") },
    { 10, _T("Mountain Standard Time"), _T("(GMT-07:00) Mountain Time (US and Canada)") },
    { 13, _T("Mexico Standard Time 2"), _T("(GMT-07:00) Chihuahua, La Paz, Mazatlan") },
    { 15, _T("U.S. Mountain Standard Time"), _T("(GMT-07:00) Arizona") },
    { 20, _T("Central Standard Time"), _T("(GMT-06:00) Central Time (US and Canada)") },
    { 25, _T("Canada Central Standard Time"), _T("(GMT-06:00) Saskatchewan") },
    { 30, _T("Mexico Standard Time"), _T("(GMT-06:00) Guadalajara, Mexico City, Monterrey") },
    { 33, _T("Central America Standard Time"), _T("(GMT-06:00) Central America") },
    { 35, _T("Eastern Standard Time"), _T("(GMT-05:00) Eastern Time (US and Canada)") },
    { 40, _T("U.S. Eastern Standard Time"), _T("(GMT-05:00) Indiana (East)") },
    { 45, _T("S.A. Pacific Standard Time"), _T("(GMT-05:00) Bogota, Lima, Quito") },
    { 50, _T("Atlantic Standard Time"), _T("(GMT-04:00) Atlantic Time (Canada)") },
    { 55, _T("S.A. Western Standard Time"), _T("(GMT-04:00) Caracas, La Paz") },
    { 56, _T("Pacific S.A. Standard Time"), _T("(GMT-04:00) Santiago") },
    { 60, _T("Newfoundland and Labrador Standard Time"), _T("(GMT-03:30) Newfoundland and Labrador") },
    { 65, _T("E. South America Standard Time"), _T("(GMT-03:00) Brasilia") },
    { 70, _T("S.A. Eastern Standard Time"), _T("(GMT-03:00) Buenos Aires, Georgetown") },
    { 73, _T("Greenland Standard Time"), _T("(GMT-03:00) Greenland") },
    { 75, _T("Mid-Atlantic Standard Time"), _T("(GMT-02:00) Mid-Atlantic") },
    { 80, _T("Azores Standard Time"), _T("(GMT-01:00) Azores") },
    { 83, _T("Cape Verde Standard Time"), _T("(GMT-01:00) Cape Verde Islands") },
    { 85, _T("GMT Standard Time"), _T("(GMT) Greenwich Mean Time : Dublin, Edinburgh, Lisbon, London") },
    { 90, _T("Greenwich Standard Time"), _T("(GMT) Casablanca, Monrovia") },
    { 95, _T("Central Europe Standard Time"), _T("(GMT+01:00) Belgrade, Bratislava, Budapest, Ljubljana, Prague") },
    { 100, _T("Central European Standard Time"), _T("(GMT+01:00) Sarajevo, Skopje, Warsaw, Zagreb") },
    { 105, _T("Romance Standard Time"), _T("(GMT+01:00) Brussels, Copenhagen, Madrid, Paris") },
    { 110, _T("W. Europe Standard Time"), _T("(GMT+01:00) Amsterdam, Berlin, Bern, Rome, Stockholm, Vienna") },
    { 113, _T("W. Central Africa Standard Time"), _T("(GMT+01:00) West Central Africa") },
    { 115, _T("E. Europe Standard Time"), _T("(GMT+02:00) Bucharest") },
    { 120, _T("Egypt Standard Time"), _T("(GMT+02:00) Cairo") },
    { 125, _T("FLE Standard Time"), _T("(GMT+02:00) Helsinki, Kyiv, Riga, Sofia, Tallinn, Vilnius") },
    { 130, _T("GTB Standard Time"), _T("(GMT+02:00) Athens, Istanbul, Minsk") },
    { 135, _T("Israel Standard Time"), _T("(GMT+02:00) Jerusalem") },
    { 140, _T("South Africa Standard Time"), _T("(GMT+02:00) Harare, Pretoria") },
    { 145, _T("Russian Standard Time"), _T("(GMT+03:00) Moscow, St. Petersburg, Volgograd") },
    { 150, _T("Arab Standard Time"), _T("(GMT+03:00) Kuwait, Riyadh") },
    { 155, _T("E. Africa Standard Time"), _T("(GMT+03:00) Nairobi") },
    { 158, _T("Arabic Standard Time"), _T("(GMT+03:00) Baghdad") },
    { 160, _T("Iran Standard Time"), _T("(GMT+03:30) Tehran") },
    { 165, _T("Arabian Standard Time"), _T("(GMT+04:00) Abu Dhabi, Muscat") },
    { 170, _T("Caucasus Standard Time"), _T("(GMT+04:00) Baku, Tbilisi, Yerevan") },
    { 175, _T("Transitional Islamic State of Afghanistan Standard Time"), _T("(GMT+04:30) Kabul") },
    { 180, _T("Ekaterinburg Standard Time"), _T("(GMT+05:00) Ekaterinburg") },
    { 185, _T("West Asia Standard Time"), _T("(GMT+05:00) Islamabad, Karachi, Tashkent") },
    { 190, _T("India Standard Time"), _T("(GMT+05:30) Chennai, Kolkata, Mumbai, New Delhi") },
    { 193, _T("Nepal Standard Time"), _T("(GMT+05:45) Kathmandu") },
    { 195, _T("Central Asia Standard Time"), _T("(GMT+06:00) Astana, Dhaka") },
    { 200, _T("Sri Lanka Standard Time"), _T("(GMT+06:00) Sri Jayawardenepura") },
    { 201, _T("N. Central Asia Standard Time"), _T("(GMT+06:00) Almaty, Novosibirsk") },
    { 203, _T("Myanmar Standard Time"), _T("(GMT+06:30) Yangon (Rangoon)") },
    { 205, _T("S.E. Asia Standard Time"), _T("(GMT+07:00) Bangkok, Hanoi, Jakarta") },
    { 207, _T("North Asia Standard Time"), _T("(GMT+07:00) Krasnoyarsk") },
    { 210, _T("China Standard Time"), _T("(GMT+08:00) Beijing, Chongqing, Hong Kong SAR, Urumqi") },
    { 215, _T("Singapore Standard Time"), _T("(GMT+08:00) Kuala Lumpur, Singapore") },
    { 220, _T("Taipei Standard Time"), _T("(GMT+08:00) Taipei") },
    { 225, _T("W. Australia Standard Time"), _T("(GMT+08:00) Perth") },
    { 227, _T("North Asia East Standard Time"), _T("(GMT+08:00) Irkutsk, Ulan Bator") },
    { 230, _T("Korea Standard Time"), _T("(GMT+09:00) Seoul") },
    { 235, _T("Tokyo Standard Time"), _T("(GMT+09:00) Osaka, Sapporo, Tokyo") },
    { 240, _T("Yakutsk Standard Time"), _T("(GMT+09:00) Yakutsk") },
    { 245, _T("A.U.S. Central Standard Time"), _T("(GMT+09:30) Darwin") },
    { 250, _T("Cen. Australia Standard Time"), _T("(GMT+09:30) Adelaide") },
    { 255, _T("A.U.S. Eastern Standard Time"), _T("(GMT+10:00) Canberra, Melbourne, Sydney") },
    { 260, _T("E. Australia Standard Time"), _T("(GMT+10:00) Brisbane") },
    { 265, _T("Tasmania Standard Time"), _T("(GMT+10:00) Hobart") },
    { 270, _T("Vladivostok Standard Time"), _T("(GMT+10:00) Vladivostok") },
    { 275, _T("West Pacific Standard Time"), _T("(GMT+10:00) Guam, Port Moresby") },
    { 280, _T("Central Pacific Standard Time"), _T("(GMT+11:00) Magadan, Solomon Islands, New Caledonia") },
    { 285, _T("Fiji Islands Standard Time"), _T("(GMT+12:00) Fiji Islands, Kamchatka, Marshall Islands") },
    { 290, _T("New Zealand Standard Time"), _T("(GMT+12:00) Auckland, Wellington") },
    { 300, _T("Tonga Standard Time"), _T("(GMT+13:00) Nuku'alofa") },
};

static std::vector<std::wstring> _reg_contexts = {
    _T("[HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\MountMgr]"),
    _T("\"NoAutoMount\"=dword:00000000"),
    _T("[HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Enum\\PCIIDE\\IDEChannel]"),
    _T("[HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Enum\\PCIIDE\\IDEChannel\\4&80d05ed&0&0]"),
    _T("\"DeviceDesc\"=\"@mshdc.inf,%idechannel.devicedesc%;IDE Channel\""),
    _T("\"LocationInformation\"=\"Channel 0\""),
    _T("\"Capabilities\"=dword:00000000"),
    _T("\"HardwareID\"=hex(7):49,00,6e,00,74,00,65,00,6c,00,2d,00,50,00,49,00,49,00,58,\\"),
    _T("00,33,00,00,00,49,00,6e,00,74,00,65,00,72,00,6e,00,61,00,6c,00,5f,00,49,00,\\"),
    _T("44,00,45,00,5f,00,43,00,68,00,61,00,6e,00,6e,00,65,00,6c,00,00,00,00,00"),
    _T("\"CompatibleIDs\"=hex(7):2a,00,50,00,4e,00,50,00,30,00,36,00,30,00,30,00,00,00,\\"),
    _T("00,00"),
    _T("\"ContainerID\"=\"{00000000-0000-0000-ffff-ffffffffffff}\""),
    _T("\"Service\"=\"atapi\""),
    _T("\"ClassGUID\"=\"{4d36e96a-e325-11ce-bfc1-08002be10318}\""),
    _T("\"ConfigFlags\"=dword:00000000"),
    _T("\"ParentIdPrefix\"=\"5&158eda0f&0\""),
    _T("\"Driver\"=\"{4d36e96a-e325-11ce-bfc1-08002be10318}\\0001\""),
    _T("\"FriendlyName\"=\"ATA Channel 0\""),
    _T("\"Class\"=\"hdc\""),
    _T("\"Mfg\"=\"@mshdc.inf,%ms-drivers%;(Standard IDE ATA/ATAPI controllers)\""),
    _T("[HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Enum\\PCIIDE\\IDEChannel\\4&80d05ed&0&1]"),
    _T("\"DeviceDesc\"=\"@mshdc.inf,%idechannel.devicedesc%;IDE Channel\""),
    _T("\"LocationInformation\"=\"Channel 1\""),
    _T("\"Capabilities\"=dword:00000000"),
    _T("\"HardwareID\"=hex(7):49,00,6e,00,74,00,65,00,6c,00,2d,00,50,00,49,00,49,00,58,\\"),
    _T("00,33,00,00,00,49,00,6e,00,74,00,65,00,72,00,6e,00,61,00,6c,00,5f,00,49,00,\\"),
    _T("44,00,45,00,5f,00,43,00,68,00,61,00,6e,00,6e,00,65,00,6c,00,00,00,00,00"),
    _T("\"CompatibleIDs\"=hex(7):2a,00,50,00,4e,00,50,00,30,00,36,00,30,00,30,00,00,00,\\"),
    _T("00,00"),
    _T("\"ContainerID\"=\"{00000000-0000-0000-ffff-ffffffffffff}\""),
    _T("\"Service\"=\"atapi\""),
    _T("\"ClassGUID\"=\"{4d36e96a-e325-11ce-bfc1-08002be10318}\""),
    _T("\"ConfigFlags\"=dword:00000000"),
    _T("\"ParentIdPrefix\"=\"5&2d5296a&0\""),
    _T("\"Driver\"=\"{4d36e96a-e325-11ce-bfc1-08002be10318}\\\\0002\""),
    _T("\"FriendlyName\"=\"ATA Channel 1\""),
    _T("\"Class\"=\"hdc\""),
    _T("\"Mfg\"=\"@mshdc.inf,%ms-drivers%;(Standard IDE ATA/ATAPI controllers)\""),
    _T("[HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\atapi]"),
    _T("\"Start\"=dword:00000000"),
    _T("[HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\intelide]"),
    _T("\"Start\"=dword:00000000"),
    _T("[HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\pciide]"),
    _T("\"Start\"=dword:00000004")
};

//This structure can be used to parser binary element of the BCD boot entry 11000001 & 21000001.
typedef struct _BCD_DEVICE_ELEMENT
{
    DWORD     Reserved1[4];
    ULONGLONG DeviceType; // must be 0x06
    ULONGLONG Signature;  // must be 0x48
    union
    {
        ULONGLONG  Offset[2];
        GUID	   PartitionGuid;
    };
    DWORD   Reserved2;
    BOOL    IsMBRDisk;
    union
    {
        DWORD MBRSignature[4];
        GUID  DiskGuid;
    };
    BYTE    Reserved3[16];
}BCD_DEVICE_ELEMENT, *PBCD_DEVICE_ELEMENT;

typedef struct _MS_DIGITAL_PRODUCTID
{
    unsigned int uiSize;
    BYTE bUnknown1[4];
    char szProductId[28];
    char szEditionId[16];
    BYTE bUnknown2[112];
} MS_DIGITAL_PRODUCTID, *PMS_DIGITAL_PRODUCTID;

typedef struct _MS_DIGITAL_PRODUCTID4
{
    unsigned int uiSize;
    BYTE bUnknown1[4];
    WCHAR szAdvancedPid[64];
    WCHAR szActivationId[72];
    WCHAR szEditionType[256];
    BYTE bUnknown2[96];
    WCHAR szEditionId[64];
    WCHAR szKeyType[64];
    WCHAR szEULA[64];
}MS_DIGITAL_PRODUCTID4, *PMS_DIGITAL_PRODUCTID4;

static stdstring decode_ms_key(const LPBYTE lpBuf, const DWORD rpkOffset){

    INT32 i, j;
    DWORD  dwAccumulator;
    TCHAR  szPossibleChars[] = _T("BCDFGHJKMPQRTVWXY2346789");
    stdstring szMSKey;
    i = 28;
    szMSKey.resize(i + 1);

    do
    {
        dwAccumulator = 0;
        j = 14;

        do
        {
#if 0
            dwAccumulator = dwAccumulator << 8;
            dwAccumulator = lpBuf[j + rpkOffset] + dwAccumulator;
            lpBuf[j + rpkOffset] = (dwAccumulator / 24) & 0xFF;
#else            
            dwAccumulator = (dwAccumulator << 8) ^ lpBuf[j + rpkOffset];
            lpBuf[j + rpkOffset] = (BYTE)(dwAccumulator / 24);
#endif
            dwAccumulator = dwAccumulator % 24;
            j--;
        } while (j >= 0);

        szMSKey[i] = szPossibleChars[dwAccumulator];
        i--;

        if ((((29 - i) % 6) == 0) && (i != -1))
        {
            szMSKey[i] = _T('-');
            i--;
        }

    } while (i >= 0);

    return szMSKey;
}

DWORD image_file_type(LPVOID lpFile){

    /* DOS file signature comes first. */
    if (*(USHORT *)lpFile == IMAGE_DOS_SIGNATURE){
        /* Determine location of PE File header from
        DOS header. */
        if (LOWORD(*(DWORD *)NTSIGNATURE(lpFile)) ==
            IMAGE_OS2_SIGNATURE ||
            LOWORD(*(DWORD *)NTSIGNATURE(lpFile)) ==
            IMAGE_OS2_SIGNATURE_LE)
            return (DWORD)LOWORD(*(DWORD *)NTSIGNATURE(lpFile));

        else if (*(DWORD *)NTSIGNATURE(lpFile) ==
            IMAGE_NT_SIGNATURE)
            return IMAGE_NT_SIGNATURE;

        else
            return IMAGE_DOS_SIGNATURE;
    }
    else
        /* unknown file type */
        return 0;
}

LONG get_pe_machine_type(std::wstring txtFilePath, DWORD &MachineType){

    DWORD dwRet = ERROR_INVALID_PARAMETER;
    if (txtFilePath.length() > 0){
        HANDLE hFile;
        HANDLE hFileMapping;
        LPVOID lpFileBase;
        PIMAGE_DOS_HEADER  pDosHeader;
        PIMAGE_FILE_HEADER pFileHeader;

        hFile = CreateFile(txtFilePath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

        if (hFile == INVALID_HANDLE_VALUE){
            //printf("Couldn't open file with CreateFile()\n");
            dwRet = GetLastError();
        }
        else{
            hFileMapping = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
            if (hFileMapping == 0){
                dwRet = GetLastError();
                //("Couldn't open file mapping with CreateFileMapping()\n"); 
            }
            else{
                lpFileBase = MapViewOfFile(hFileMapping, FILE_MAP_READ, 0, 0, 0);
                if (lpFileBase == 0){
                    dwRet = GetLastError();
                    //("Couldn't map view of file with MapViewOfFile()\n");
                }
                else{
                    pDosHeader = (PIMAGE_DOS_HEADER)lpFileBase;

                    if (IMAGE_NT_SIGNATURE == image_file_type(lpFileBase)){
                        pFileHeader = (PIMAGE_FILE_HEADER)PEFHDROFFSET(lpFileBase);
                        MachineType = pFileHeader->Machine;
                        dwRet = ERROR_SUCCESS;
                    }

                    UnmapViewOfFile(lpFileBase);
                }
                CloseHandle(hFileMapping);
            }
            CloseHandle(hFile);
        }
    }
    return dwRet;
}

struct mbr_partition{
    typedef boost::shared_ptr<mbr_partition> ptr;
    typedef std::vector<ptr> vtr;
    mbr_partition() : start(0), length(0), type(0), boot_indicator(FALSE), recognized(FALSE){}
    struct compare {
        bool operator() (mbr_partition::ptr const & lhs, mbr_partition::ptr const & rhs) const {
            return (*lhs).start < (*rhs).start;
        }
    };
    static mbr_partition::vtr get(boost::shared_ptr<DRIVE_LAYOUT_INFORMATION_EX> ppdli){
        mbr_partition::vtr results;
        PARTITION_INFORMATION_EX *ppi = ppdli->PartitionEntry;;

        for (int i = 0; i < ppdli->PartitionCount; i++){
            if (ppi[i].StartingOffset.QuadPart > 0){
                mbr_partition::ptr  p = mbr_partition::ptr(new mbr_partition());
                p->type = ppi[i].Mbr.PartitionType;
                p->boot_indicator = ppi[i].Mbr.BootIndicator;
                p->recognized = ppi[i].Mbr.RecognizedPartition;
                p->start = ppi[i].StartingOffset.QuadPart;
                p->length = ppi[i].PartitionLength.QuadPart;
                results.push_back(p);
            }
        }
        std::sort(results.begin(), results.end(), mbr_partition::compare());
        return results;
    }
    uint64_t                 start;
    uint64_t                 length;
    BYTE                     type;
    BOOL                     boot_indicator;
    BOOL                     recognized;
};

struct gpt_partition{
    typedef boost::shared_ptr<gpt_partition> ptr;
    typedef std::vector<ptr> vtr;
    gpt_partition() : start(0), length(0), attributes(0){}
    struct compare {
        bool operator() (gpt_partition::ptr const & lhs, gpt_partition::ptr const & rhs) const {
            return (*lhs).start < (*rhs).start;
        }
    };

    static gpt_partition::vtr get(PGPT_PARTITION_ENTRY pGptPartitionEntries, int count, int size_of_lba){
        gpt_partition::vtr results;
        for (int i = 0; i < count; i++){
            if (macho::guid_(pGptPartitionEntries[i].PartitionTypeGuid) != macho::guid_(GUID_NULL)){
                gpt_partition::ptr  p = gpt_partition::ptr(new gpt_partition());
                p->type = pGptPartitionEntries[i].PartitionTypeGuid;
                p->unique = pGptPartitionEntries[i].UniquePartitionGuid;
                p->start = pGptPartitionEntries[i].StartingLBA * size_of_lba;
                p->length = (pGptPartitionEntries[i].EndingLBA - pGptPartitionEntries[i].StartingLBA) * size_of_lba;
                p->attributes = pGptPartitionEntries[i].Attributes;
                p->name = pGptPartitionEntries[i].PartitionName;
                results.push_back(p);
            }
        }
        std::sort(results.begin(), results.end(), gpt_partition::compare());
        return results;
    }

    static gpt_partition::vtr get(boost::shared_ptr<DRIVE_LAYOUT_INFORMATION_EX> ppdli){
        gpt_partition::vtr results;
        PARTITION_INFORMATION_EX *ppi = ppdli->PartitionEntry;;

        for (int i = 0; i < ppdli->PartitionCount; i++){
            gpt_partition::ptr  p = gpt_partition::ptr(new gpt_partition());
            p->type = ppi[i].Gpt.PartitionType;
            p->unique = ppi[i].Gpt.PartitionId;
            p->start = ppi[i].StartingOffset.QuadPart;
            p->length = ppi[i].PartitionLength.QuadPart;
            p->attributes = ppi[i].Gpt.Attributes;
            p->name = ppi[i].Gpt.Name;
            results.push_back(p);
        }
        std::sort(results.begin(), results.end(), gpt_partition::compare());
        return results;
    }

    static bool save(boost::filesystem::path file, macho::guid_ disk_guid, gpt_partition::vtr partitions){
        try{
            mObject storage;
            storage["disk_guid"] = (std::string) disk_guid; 
            mArray _partitions;
            foreach(gpt_partition::ptr &p, partitions){
                mObject _p;
                _p["type"] = (std::string)p->type;
                _p["unique"] = (std::string)p->unique;
                _p["start"] = p->start;
                _p["length"] = p->length;
                _p["attributes"] = p->attributes;
                _p["name"] = macho::stringutils::convert_unicode_to_utf8(p->name);
                _partitions.push_back(_p);
            }
            storage["partitions"] = _partitions;
            std::ofstream output(file.string(), std::ios::out | std::ios::trunc);
            std::locale loc(std::locale::classic(), new std::codecvt_utf8<wchar_t>);
            output.imbue(loc);
            write(storage, output, json_spirit::pretty_print | json_spirit::raw_utf8);
            return true;
        }
        catch (boost::exception& ex){
            LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_ansi_to_unicode(boost::exception_detail::get_diagnostic_information(ex, "can't output storage info.")));
        }
        catch (...){
        }
        return false;
    }

    static bool load(boost::filesystem::path file, macho::guid_& disk_guid, gpt_partition::vtr& partitions){
        if (boost::filesystem::exists(file)){
            try{
                std::ifstream is(file.string());
                mValue storage;
                read(is, storage);
                disk_guid = find_value_string(storage.get_obj(), "disk_guid");
                mArray _partitions = find_value(storage.get_obj(), "partitions").get_array();
                foreach(mValue &p, _partitions){
                    gpt_partition::ptr _p = gpt_partition::ptr(new gpt_partition());
                    _p->type = find_value_string(p.get_obj(), "type");
                    _p->unique = find_value_string(p.get_obj(), "unique");
                    _p->start = find_value(p.get_obj(), "start").get_int64();
                    _p->length = find_value(p.get_obj(), "length").get_int64();
                    _p->attributes = find_value(p.get_obj(), "attributes").get_int64();
                    _p->name = macho::stringutils::convert_utf8_to_unicode(find_value(p.get_obj(), "name").get_str());
                    partitions.push_back(_p);
                }
                return true;
            }
            catch (boost::exception& ex){
                LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_ansi_to_unicode(boost::exception_detail::get_diagnostic_information(ex, "can't load storage info.")));
            }
            catch (...){
            }
        }
        return false;
    }

    static const mValue& find_value(const mObject& obj, const std::string& name){
        mObject::const_iterator i = obj.find(name);
        assert(i != obj.end());
        assert(i->first == name);
        return i->second;
    }

    static const std::string find_value_string(const mObject& obj, const std::string& name){
        mObject::const_iterator i = obj.find(name);
        if (i != obj.end()){
            return i->second.get_str();
        }
        return "";
    }
    macho::guid_             type;
    macho::guid_             unique;
    uint64_t                 start;
    uint64_t                 length;
    uint64_t                 attributes;
    std::wstring             name;
};

os_image_info irm_conv::get_offline_os_info(boost::filesystem::path win_dir){

    os_image_info image;
    image.win_dir = win_dir;
    macho::windows::file_version_info ntoskrnl = macho::windows::file_version_info::get_file_version_info(boost::filesystem::path(win_dir / L"system32\\NTOSKRNL.EXE").wstring());

    DWORD dwRet = get_pe_machine_type(boost::filesystem::path(win_dir / L"system32\\NTOSKRNL.EXE").wstring(), image.machine_type);

    if (ERROR_SUCCESS != dwRet){
        BOOST_THROW_EXCEPTION_BASE(irm_conv::exception, dwRet, boost::str(boost::wformat(L"get_pe_machine_type error (%d).") % dwRet));
    }
    else{
        if ((image.machine_type == IMAGE_FILE_MACHINE_I386) ||
            (image.machine_type == IMAGE_FILE_MACHINE_IA64) || // Break IA64 support here.
            (image.machine_type == IMAGE_FILE_MACHINE_AMD64)){
            switch (image.machine_type)
            {
            case IMAGE_FILE_MACHINE_I386:
                image.architecture = _T("x86");
                break;
            case IMAGE_FILE_MACHINE_IA64:
                image.architecture = _T("ia64");
                BOOST_THROW_EXCEPTION_BASE(irm_conv::exception, ERROR_INVALID_TARGET, boost::str(boost::wformat(L"Can't support to convert the OS architecutre (%s) image.") % image.architecture ));
                break;
            case IMAGE_FILE_MACHINE_AMD64:
                image.architecture = _T("amd64");
                break;
            }
        }
    }

    reg_hive_edit::ptr system_hive_edit_ptr = reg_hive_edit::ptr(new reg_hive_edit( win_dir / L"System32\\config" ));
    image.system_hive_edit_ptr = system_hive_edit_ptr;
    macho::windows::registry reg(*system_hive_edit_ptr.get(), macho::windows::REGISTRY_FLAGS_ENUM::REGISTRY_READONLY);
    if (!reg.open(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"))){
        BOOST_THROW_EXCEPTION_BASE(irm_conv::exception, ERROR_INVALID_TARGET, L" Can't open the registry \"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\"");
    }
    else{
        if (reg[_T("SystemRoot")].is_string())
            image.system_root = reg[_T("SystemRoot")].wstring();
        image.product_name = reg[_T("ProductName")].wstring();
        image.full_name = reg[_T("RegisteredOwner")].wstring();
        image.orgname = reg[_T("RegisteredOrganization")].wstring();

        if (reg[_T("CurrentMajorVersionNumber")].exists() && reg[_T("CurrentMinorVersionNumber")].exists()){
            image.major_version = (DWORD)reg[_T("CurrentMajorVersionNumber")];
            image.minor_version = (DWORD)reg[_T("CurrentMinorVersionNumber")];
        }
        else if (reg[_T("CurrentVersion")].is_string()){
            std::vector<std::wstring> tmptxtArray;
            image.version = reg[_T("CurrentVersion")].wstring();
            tmptxtArray = macho::stringutils::tokenize(image.version, L".");
            if (tmptxtArray.size() > 1){
                image.major_version = _ttol(tmptxtArray[0].c_str());
                image.minor_version = _ttol(tmptxtArray[1].c_str());
            }
        }

        if (reg[_T("CSDVersion")].exists() && reg[_T("CSDVersion")].is_string()){
            image.csd_version = reg[_T("CSDVersion")].wstring();
        }

        if (reg[_T("CSDBuildNumber")].exists() && reg[_T("CSDBuildNumber")].is_string()){
            image.csd_build_number = reg[_T("CSDBuildNumber")].wstring();
        }

        if (reg[_T("DigitalProductId4")].exists()){
            PMS_DIGITAL_PRODUCTID4 pDigitalPid4 = (PMS_DIGITAL_PRODUCTID4)(LPBYTE)reg[_T("DigitalProductId4")];
            image.is_eval_key = 0 == _wcsicmp(pDigitalPid4->szEULA, L"EVAL");
            LOG(LOG_LEVEL_INFO, L"License Info ( DigitalProductId4 ) : EditionType ( %s ). KeyType ( %s )", pDigitalPid4->szEditionType, pDigitalPid4->szKeyType);
        }

        if (reg[_T("DigitalProductId")].exists()){
            PMS_DIGITAL_PRODUCTID pDigitalPid = (PMS_DIGITAL_PRODUCTID)(LPBYTE)reg[_T("DigitalProductId")];

            std::wstring licKey = decode_ms_key(pDigitalPid->bUnknown2, 0);
            image.lic = licKey;
#if _DEBUG
            LOG(LOG_LEVEL_INFO, _T("License Info : OS License Key ( %s )."), licKey.c_str());
#endif
            LOG(LOG_LEVEL_INFO, _T("License Info ( DigitalProductId ) : EditionId ( %s ). ProductId ( %s )"), stringutils::convert_ansi_to_unicode(std::string(pDigitalPid->szEditionId)).c_str(), stringutils::convert_ansi_to_unicode(std::string(pDigitalPid->szProductId)).c_str());
        }

        reg.close();

        if (!reg.open(_T("SYSTEM\\CurrentControlSet\\Control\\ProductOptions"))){
            BOOST_THROW_EXCEPTION_BASE(irm_conv::exception, ERROR_INVALID_TARGET, L" Can't open the registry \"SYSTEM\\CurrentControlSet\\Control\\ProductOptions\"");
        }
        else{
            if (reg[_T("ProductType")].exists()){
                if (0 == _tcsicmp(reg[_T("ProductType")].wstring().c_str(), _T("LanmanNT")))
                    image.product_type = VER_NT_DOMAIN_CONTROLLER;
                else if (0 == _tcsicmp(reg[_T("ProductType")].wstring().c_str(), _T("WinNT")))
                    image.product_type = VER_NT_WORKSTATION;
                else if (0 == _tcsicmp(reg[_T("ProductType")].wstring().c_str(), _T("ServerNT")))
                    image.product_type = VER_NT_SERVER;
            }

            if (reg[_T("ProductSuite")].exists()){
                for (size_t i = 0; i < reg[_T("ProductSuite")].get_multi_count(); ++i) {
                    if (0 == _tcsicmp(reg[_T("ProductSuite")].get_multi_at(i), _T("Small Business")))
                        image.suite_mask |= VER_SUITE_SMALLBUSINESS;
                    else if (0 == _tcsicmp(reg[_T("ProductSuite")].get_multi_at(i), _T("Enterprise")))
                        image.suite_mask |= VER_SUITE_ENTERPRISE;
                    else if (0 == _tcsicmp(reg[_T("ProductSuite")].get_multi_at(i), _T("BackOffice")))
                        image.suite_mask |= VER_SUITE_BACKOFFICE;
                    else if (0 == _tcsicmp(reg[_T("ProductSuite")].get_multi_at(i), _T("CommunicationServer")))
                        image.suite_mask |= VER_SUITE_COMMUNICATIONS;
                    else if (0 == _tcsicmp(reg[_T("ProductSuite")].get_multi_at(i), _T("Terminal Server")))
                        image.suite_mask |= VER_SUITE_TERMINAL;
                    else if (0 == _tcsicmp(reg[_T("ProductSuite")].get_multi_at(i), _T("Small Business(Restricted)")))
                        image.suite_mask |= VER_SUITE_SMALLBUSINESS_RESTRICTED;
                    else if (0 == _tcsicmp(reg[_T("ProductSuite")].get_multi_at(i), _T("EmbeddedNT")))
                        image.suite_mask |= VER_SUITE_EMBEDDEDNT;
                    else if (0 == _tcsicmp(reg[_T("ProductSuite")].get_multi_at(i), _T("DataCenter")))
                        image.suite_mask |= VER_SUITE_DATACENTER;
                    else if (0 == _tcsicmp(reg[_T("ProductSuite")].get_multi_at(i), _T("SINGLEUSERTS")))
                        image.suite_mask |= VER_SUITE_SINGLEUSERTS;
                    else if (0 == _tcsicmp(reg[_T("ProductSuite")].get_multi_at(i), _T("Personal")))
                        image.suite_mask |= VER_SUITE_PERSONAL;
                    else if (0 == _tcsicmp(reg[_T("ProductSuite")].get_multi_at(i), _T("Blade"))) // 0x00000400 (VER_SUITE_SERVERAPPLIANCE)
                        image.suite_mask |= VER_SUITE_BLADE;
                    else if (0 == _tcsicmp(reg[_T("ProductSuite")].get_multi_at(i), _T("Embedded(Restricted)")))
                        image.suite_mask |= VER_SUITE_EMBEDDED_RESTRICTED;
                    else if (0 == _tcsicmp(reg[_T("ProductSuite")].get_multi_at(i), _T("Security Appliance")))
                        image.suite_mask |= VER_SUITE_SECURITY_APPLIANCE;
                    else if (0 == _tcsicmp(reg[_T("ProductSuite")].get_multi_at(i), _T("Storage Server")))
                        image.suite_mask |= VER_SUITE_STORAGE_SERVER;
                    else if (0 == _tcsicmp(reg[_T("ProductSuite")].get_multi_at(i), _T("Compute Server")))
                        image.suite_mask |= VER_SUITE_COMPUTE_SERVER;
                }
            }
            reg.close();
        }

        if (macho::windows::environment::is_running_as_local_system()){
            if (reg.open(_T("Security\\Policy\\PolPrDmN"))){
                LPBYTE lpByte = reg[_T("")];
                image.work_group = (LPWSTR)&lpByte[8];
                reg.close();
            }
        }

        if (reg.open(_T("SYSTEM\\CurrentControlSet\\Control\\ComputerName\\ComputerName"))){
            image.computer_name = reg[_T("ComputerName")].wstring();
            reg.close();
        }

        if (reg.open(_T("SYSTEM\\CurrentControlSet\\services\\Tcpip\\Parameters"))){
            image.domain = reg[_T("Domain")].wstring();
            reg.close();
        }

        if (reg.open(_T("SYSTEM\\CurrentControlSet\\Control\\TimeZoneInformation"))){
            if (reg[_T("TimeZoneKeyName")].exists())
                image.time_zone = reg[_T("TimeZoneKeyName")].wstring();
            else
                image.time_zone = reg[_T("StandardName")].wstring();
            reg.close();
        }

        if (reg.open(_T("SYSTEM\\CurrentControlSet\\Services\\LicenseInfo\\FilePrint"))){
            if (reg[_T("ConcurrentLimit")].exists())
                image.concurrent_limit = reg[_T("ConcurrentLimit")];
            if (reg[_T("Mode")].exists())
                image.mode = reg[_T("Mode")];
            reg.close();
        }

        if (reg.open(_T("SYSTEM\\CurrentControlSet\\Enum\\Root\\PCI_HAL\\0000"), HKEY_LOCAL_MACHINE)){
            if (reg[_T("HardwareID")].exists() && reg[_T("HardwareID")].get_multi_count())
                image.system_hal = reg[_T("HardwareID")].get_multi_at(0);
            reg.close();
        }

        if (!image.system_hal.length() && reg.open(_T("SYSTEM\\CurrentControlSet\\Enum\\Root\\ACPI_HAL\\0000"), HKEY_LOCAL_MACHINE)){
            if (reg[_T("HardwareID")].exists() && reg[_T("HardwareID")].get_multi_count())
                image.system_hal = reg[_T("HardwareID")].get_multi_at(0);
            reg.close();
        }

        if (!reg.open(L"Software" PENDING_UPDATE_REG_KEY, HKEY_LOCAL_MACHINE)){
            LOG(LOG_LEVEL_INFO, _T("registry key (%s) does not exist."), PENDING_UPDATE_REG_KEY);
            if (image.major_version >= 6){
                // Windows 2008 or newer
                // Check if C:\Windows\Winsxs\pending.xml exists
                try{
                    if (boost::filesystem::exists(win_dir / PENDING_UPDATE_FILE)){
                        LOG(LOG_LEVEL_NOTICE, _T("A Windows update is pending. File (%s\\%s) exists."), win_dir.wstring().c_str(), PENDING_UPDATE_FILE);
                        image.is_pending_windows_update = true;
                    }
                    else if (reg.open(L"Software" COMPONENT_BASE_SERVICING_REG_KEY, HKEY_LOCAL_MACHINE)){
                        LOG(LOG_LEVEL_NOTICE, _T("A Windows update is pending. Registry key (%s) exists."), COMPONENT_BASE_SERVICING_REG_KEY);
                        image.is_pending_windows_update = true;
                    }
                    else{
                        LOG(LOG_LEVEL_INFO, _T("A Windows update is not pending. File (%s\\%s) or Registry key (%s) does not exist."), win_dir.wstring().c_str(), PENDING_UPDATE_FILE, COMPONENT_BASE_SERVICING_REG_KEY);
                    }
                }
                catch (const boost::filesystem::filesystem_error& ex){
                    LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_ansi_to_unicode(ex.what()).c_str());
                }
            }
            else {
                // Windows 2003
                if (reg.open(L"System" PENDING_FILE_RENAME_REG_KEY, HKEY_LOCAL_MACHINE ) && reg.count() > 0){
                    LOG(LOG_LEVEL_NOTICE, _T("A Windows update is pending. Registry key (%s) exists and doesn't empty."), PENDING_FILE_RENAME_REG_KEY);
                    image.is_pending_windows_update = true;
                }
                else{
                    LOG(LOG_LEVEL_INFO, _T("A Windows update is not pending. Registry key (%s) is empty."), PENDING_FILE_RENAME_REG_KEY);
                }
            }
        }
        else{
            LOG(LOG_LEVEL_NOTICE, _T("A Windows update is pending. Registry key (%s) exists."), PENDING_UPDATE_REG_KEY);
            image.is_pending_windows_update = true;
        }
    }
    return image;
}

bool irm_conv::set_privileges(){
    bool result;
    if (!(result = macho::windows::environment::set_token_privilege(SE_BACKUP_NAME, true))){
        LOG(LOG_LEVEL_ERROR, L"Failed to set token privilege (SE_BACKUP_NAME). ");
    }
    else if (!(result = macho::windows::environment::set_token_privilege(SE_RESTORE_NAME, true))){
        LOG(LOG_LEVEL_ERROR, L"Failed to set token privilege (SE_RESTORE_NAME). ");
    }
    else if (!(result=macho::windows::environment::set_token_privilege(SE_AUDIT_NAME, true))){
        LOG(LOG_LEVEL_ERROR, L"Failed to set token privilege (SE_AUDIT_NAME). ");
    }
    else if (!(result = macho::windows::environment::set_token_privilege(SE_CHANGE_NOTIFY_NAME, true))){
        LOG(LOG_LEVEL_ERROR, L"Failed to set token privilege (SE_CHANGE_NOTIFY_NAME). ");
    }
    else if (!(result = macho::windows::environment::set_token_privilege(SE_CREATE_GLOBAL_NAME, true))){
        LOG(LOG_LEVEL_ERROR, L"Failed to set token privilege (SE_CREATE_GLOBAL_NAME). ");
    }
    else if (!(result = macho::windows::environment::set_token_privilege(SE_CREATE_PERMANENT_NAME, true))){
        LOG(LOG_LEVEL_ERROR, L"Failed to set token privilege (SE_CREATE_PERMANENT_NAME). ");
    }
    else if (!(result = macho::windows::environment::set_token_privilege(SE_CREATE_SYMBOLIC_LINK_NAME, true))){
        LOG(LOG_LEVEL_ERROR, L"Failed to set token privilege (SE_CREATE_SYMBOLIC_LINK_NAME). ");
    }
    else if (!(result = macho::windows::environment::set_token_privilege(SE_CREATE_TOKEN_NAME, true))){
        LOG(LOG_LEVEL_ERROR, L"Failed to set token privilege (SE_CREATE_TOKEN_NAME). ");
    }
    else if (!(result = macho::windows::environment::set_token_privilege(SE_DEBUG_NAME, true))){
        LOG(LOG_LEVEL_ERROR, L"Failed to set token privilege (SE_DEBUG_NAME). ");
    }
    else if (!(result = macho::windows::environment::set_token_privilege(SE_IMPERSONATE_NAME, true))){
        LOG(LOG_LEVEL_ERROR, L"Failed to set token privilege (SE_IMPERSONATE_NAME). ");
    }
    else if (!(result = macho::windows::environment::set_token_privilege(SE_INC_BASE_PRIORITY_NAME, true))){
        LOG(LOG_LEVEL_ERROR, L"Failed to set token privilege (SE_INC_BASE_PRIORITY_NAME). ");
    }
    else if (!(result = macho::windows::environment::set_token_privilege(SE_INC_WORKING_SET_NAME, true))){
        LOG(LOG_LEVEL_ERROR, L"Failed to set token privilege (SE_INC_WORKING_SET_NAME). ");
    }
    else if (!(result = macho::windows::environment::set_token_privilege(SE_LOAD_DRIVER_NAME, true))){
        LOG(LOG_LEVEL_ERROR, L"Failed to set token privilege (SE_LOAD_DRIVER_NAME). ");
    }
    else if (!(result = macho::windows::environment::set_token_privilege(SE_MANAGE_VOLUME_NAME, true))){
        LOG(LOG_LEVEL_ERROR, L"Failed to set token privilege (SE_MANAGE_VOLUME_NAME). ");
    }
    else if (!(result = macho::windows::environment::set_token_privilege(SE_PROF_SINGLE_PROCESS_NAME, true))){
        LOG(LOG_LEVEL_ERROR, L"Failed to set token privilege (SE_PROF_SINGLE_PROCESS_NAME). ");
    }
    else if (!(result = macho::windows::environment::set_token_privilege(SE_RELABEL_NAME, true))){
        LOG(LOG_LEVEL_ERROR, L"Failed to set token privilege (SE_RELABEL_NAME). ");
    }
    else if (!(result = macho::windows::environment::set_token_privilege(SE_REMOTE_SHUTDOWN_NAME, true))){
        LOG(LOG_LEVEL_ERROR, L"Failed to set token privilege (SE_REMOTE_SHUTDOWN_NAME). ");
    }
    else if (!(result = macho::windows::environment::set_token_privilege(SE_SECURITY_NAME, true))){
        LOG(LOG_LEVEL_ERROR, L"Failed to set token privilege (SE_SECURITY_NAME). ");
    }
    else if (!(result = macho::windows::environment::set_token_privilege(SE_SYNC_AGENT_NAME, true))){
        LOG(LOG_LEVEL_ERROR, L"Failed to set token privilege (SE_SYNC_AGENT_NAME). ");
    }
    else if (!(result = macho::windows::environment::set_token_privilege(SE_SYSTEM_PROFILE_NAME, true))){
        LOG(LOG_LEVEL_ERROR, L"Failed to set token privilege (SE_SYSTEM_PROFILE_NAME). ");
    }
    else if (!(result = macho::windows::environment::set_token_privilege(SE_SYSTEMTIME_NAME, true))){
        LOG(LOG_LEVEL_ERROR, L"Failed to set token privilege (SE_SYSTEMTIME_NAME). ");
    }
    else if (!(result = macho::windows::environment::set_token_privilege(SE_TAKE_OWNERSHIP_NAME, true))){
        LOG(LOG_LEVEL_ERROR, L"Failed to set token privilege (SE_TAKE_OWNERSHIP_NAME). ");
    }
    else if (!(result = macho::windows::environment::set_token_privilege(SE_TCB_NAME, true))){
        LOG(LOG_LEVEL_ERROR, L"Failed to set token privilege (SE_TCB_NAME). ");
    }
    else if (!(result = macho::windows::environment::set_token_privilege(SE_TIME_ZONE_NAME, true))){
        LOG(LOG_LEVEL_ERROR, L"Failed to set token privilege (SE_TIME_ZONE_NAME). ");
    }
    else if (!(result = macho::windows::environment::set_token_privilege(SE_TRUSTED_CREDMAN_ACCESS_NAME, true))){
        LOG(LOG_LEVEL_ERROR, L"Failed to set token privilege (SE_TRUSTED_CREDMAN_ACCESS_NAME). ");
    }
    else if (!(result = macho::windows::environment::set_token_privilege(SE_UNSOLICITED_INPUT_NAME, true))){
        LOG(LOG_LEVEL_ERROR, L"Failed to set token privilege (SE_UNSOLICITED_INPUT_NAME). ");
    }
    else if (!(result = macho::windows::environment::set_token_privilege(SE_ASSIGNPRIMARYTOKEN_NAME, true))){
        LOG(LOG_LEVEL_ERROR, L"Failed to set token privilege (SE_ASSIGNPRIMARYTOKEN_NAME). ");
    }
    return result;
}

bool irm_conv::get_win_dir_ini(__in const boost::filesystem::path boot_ini_file, __in macho::windows::storage::disk::ptr d, __out windows_element &system_device_element){
    TCHAR buf[64] = { 0 };
    bool result = false;
    DWORD ret = GetPrivateProfileString(L"boot loader", L"default", L"", buf, sizeof(buf), boot_ini_file.wstring().c_str());
    if (ret){
        std::wstring default_boot_entry = buf;
        transform(default_boot_entry.begin(), default_boot_entry.end(), default_boot_entry.begin(), tolower); 
        size_t pos = default_boot_entry.find(_T("partition("));
        std::wstring partition = default_boot_entry.substr(pos + 10, default_boot_entry.npos);
        pos = partition.find(_T(")"));
        partition = partition.substr(0, pos);
        DWORD partition_number = _ttol(partition.c_str());
        pos = default_boot_entry.find(_T("\\"));
        system_device_element.windows_dir = default_boot_entry.substr(pos+1, default_boot_entry.npos);
        macho::windows::storage::partition::vtr partitions = d->get_partitions();
        foreach(macho::windows::storage::partition::ptr p, partitions){
            if (p->partition_number() == partition_number){
                system_device_element.is_mbr_disk = true;
                system_device_element.mbr_signature = d->signature();
                system_device_element.partition_offset = p->offset();
                result = true;
                break;
            }
        }
    }
    return result;
}

bool irm_conv::get_win_dir(__in const boost::filesystem::path bcd_folder, __out windows_element &system_device_element){

    std::wstring default_id;
    reg_hive_edit::ptr boot_hive_edit_ptr = reg_hive_edit::ptr(new reg_hive_edit(bcd_folder));
    macho::windows::registry reg(*boot_hive_edit_ptr.get(), macho::windows::REGISTRY_FLAGS_ENUM::REGISTRY_READONLY);
    if (reg.open(L"BCD\\Objects\\{9dea862c-5cdd-4e70-acc1-f32b344d4795}\\Elements\\23000003")){
        if (reg[_T("Element")].exists() && reg[_T("Element")].is_string())
            default_id = reg[_T("Element")].wstring();
        reg.close();
    }
    else if (reg.open(L"BCD\\Objects\\{9dea862c-5cdd-4e70-acc1-f32b344d4795}\\Elements\\24000001")){
        if (reg[_T("Element")].exists() && reg[_T("Element")].is_multi_sz() && reg[_T("Element")].get_multi_count())
            default_id = reg[_T("Element")].get_multi_at(0);
        reg.close();
    }
    if (default_id.length()){
        if (reg.open(boost::str(boost::wformat(L"BCD\\Objects\\%s\\Elements")%default_id)) &&
            reg.subkey(_T("21000001"))[_T("Element")].exists() &&
            reg.subkey(_T("22000002"))[_T("Element")].exists()){
            PBCD_DEVICE_ELEMENT lpDevElem = (PBCD_DEVICE_ELEMENT)((LPBYTE)reg.subkey(_T("21000001"))[_T("Element")]);
            system_device_element.windows_dir = reg.subkey(_T("22000002"))[_T("Element")].wstring();
            if (system_device_element.windows_dir.at(0) == L'\\')
                system_device_element.windows_dir = system_device_element.windows_dir.substr(1, -1);
            if (lpDevElem != NULL){
                if ((lpDevElem->DeviceType == 0x06) && (lpDevElem->Signature == 0x48)){ // This is bcd partition device element format. 
                    system_device_element.is_mbr_disk = lpDevElem->IsMBRDisk ? true : false;
                    if (!lpDevElem->IsMBRDisk){
                        system_device_element.disk_guid = lpDevElem->DiskGuid;
                        system_device_element.partition_guid = lpDevElem->PartitionGuid;
                    }
                    else{
                        system_device_element.partition_offset = lpDevElem->Offset[0];
                        system_device_element.mbr_signature = lpDevElem->MBRSignature[0];
                    }
                    return true;
                }
                else if ((lpDevElem->DeviceType == 0x05) && (lpDevElem->Signature == 0x48)){
                    system_device_element.is_boot_volume = true;
                    return true;
                }
                else if ((lpDevElem->DeviceType == 0x08) && (lpDevElem->Signature == 0x1E)){
                    system_device_element.is_template = true;
                    LOG(LOG_LEVEL_RECORD, L"Recovery from windows template.");       
                    return true;
                }
            }
            reg.close();
        }
    }
    return false;
}

bool irm_conv::set_bcd_boot_device_entry(__in const boost::filesystem::path bcd_folder, __in const windows_element &system_device_element){
    std::wstring default_id;
    reg_hive_edit::ptr boot_hive_edit_ptr = reg_hive_edit::ptr(new reg_hive_edit(bcd_folder));
    macho::windows::registry reg(*boot_hive_edit_ptr.get());
    if (reg.open(L"BCD\\Objects\\{9dea862c-5cdd-4e70-acc1-f32b344d4795}\\Elements\\23000003")){
        if (reg[_T("Element")].exists() && reg[_T("Element")].is_string())
            default_id = reg[_T("Element")].wstring();
        reg.close();
    }
    else if (reg.open(L"BCD\\Objects\\{9dea862c-5cdd-4e70-acc1-f32b344d4795}\\Elements\\24000001")){
        if (reg[_T("Element")].exists() && reg[_T("Element")].is_multi_sz() && reg[_T("Element")].get_multi_count())
            default_id = reg[_T("Element")].get_multi_at(0);
        reg.close();
    }

    if (default_id.length()){
        if (reg.open(boost::str(boost::wformat(L"BCD\\Objects\\%s\\Elements") % default_id)) &&
            reg.subkey(_T("21000001"))[_T("Element")].exists() &&
            reg.subkey(_T("11000001"))[_T("Element")].exists()){
            BCD_DEVICE_ELEMENT DevElem;
            memset(&DevElem, 0, sizeof(BCD_DEVICE_ELEMENT));
            DevElem.DeviceType = 0x06;
            DevElem.Signature = 0x48;
            DevElem.IsMBRDisk = system_device_element.is_mbr_disk ? TRUE : FALSE;
            if (DevElem.IsMBRDisk){
                DevElem.MBRSignature[0] = system_device_element.mbr_signature;
                DevElem.Offset[0] = system_device_element.partition_offset;
            }
            else{
                DevElem.DiskGuid = system_device_element.disk_guid;
                DevElem.PartitionGuid = system_device_element.partition_guid;
            }
            reg.subkey(_T("11000001"))[_T("Element")].set_binary((LPBYTE)&DevElem, sizeof(BCD_DEVICE_ELEMENT));
            reg.subkey(_T("21000001"))[_T("Element")].set_binary((LPBYTE)&DevElem, sizeof(BCD_DEVICE_ELEMENT));
            reg.close();
            return true;
        }
    }
    return false;
}

bool irm_conv::fix_disk_geometry_issue(DWORD disk_number, DISK_GEOMETRY &geometry){

    PBYTE                      buffer = NULL;
    PLEGACY_MBR                pLegacyMBR = NULL;
    PGPT_PARTITIONTABLE_HEADER pGptPartitonHeader = NULL;
    DWORD                      dwBytesRead;
    DWORD                      cbReturned;
    BOOL                       bResult = FALSE;
    CHS_ADDRESS                chs;
    size_t                     index;

    SecureZeroMemory(&chs, sizeof(chs));
    macho::windows::auto_file_handle handle = CreateFile(boost::str(boost::wformat(L"\\\\.\\PhysicalDrive%d") % disk_number).c_str(),
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_NO_BUFFERING,
        NULL);

    if (handle.is_valid()){
        buffer = (PBYTE)malloc(BUFSIZE2);
        if (buffer){
            SecureZeroMemory(buffer, BUFSIZE2);
            if (ReadFile(handle, buffer, BUFSIZE2, &dwBytesRead, NULL)){
                pLegacyMBR = (PLEGACY_MBR)buffer;
                pGptPartitonHeader = (PGPT_PARTITIONTABLE_HEADER)&buffer[512];
                if ((pLegacyMBR->Signature == 0xAA55) && (pLegacyMBR->PartitionRecord[0].PartitionType == 0xEE) && (pGptPartitonHeader->Signature == 0x5452415020494645)){
                    LOG(LOG_LEVEL_INFO, L"Partition Style is GPT. ");
                    bResult = TRUE;
                }
                else{
                    for (index = 0; index < 4; index++){
                        if (pLegacyMBR->PartitionRecord[index].BootIndicator == 0x80){
                            CHS_ADDRESS                originalchs;
                            SecureZeroMemory(&originalchs, sizeof(originalchs));
                            get_mbr_partition_chs_address(pLegacyMBR->PartitionRecord[index].StartCHS, &originalchs);
                            // We need to recalculate and reset the CHS address of boot partition when the physical geometry was changed.
                            calculate_chs_address(pLegacyMBR->PartitionRecord[index].StartingLBA, &geometry, &chs);
                            set_mbr_partition_chs_address(pLegacyMBR->PartitionRecord[index].StartCHS, &chs);
                            calculate_chs_address(pLegacyMBR->PartitionRecord[index].StartingLBA + pLegacyMBR->PartitionRecord[index].SizeInLBA, &geometry, &chs);
                            set_mbr_partition_chs_address(pLegacyMBR->PartitionRecord[index].EndCHS, &chs);
                        }
                    }
                    SetFilePointer(handle, 0, NULL, 0);
                    if (bResult = WriteFile(handle, buffer, dwBytesRead, &cbReturned, NULL))
                        LOG(LOG_LEVEL_INFO, L"Succeeded to update the MBR partition records!");
                    else
                        LOG(LOG_LEVEL_ERROR, L"Failed to update the MBR partition records. (%d)", GetLastError());
                }
            }
            free(buffer);
        }
    }
    return bResult ? true : false;
}
        
bool irm_conv::fix_mbr_disk_signature_issue(__in DWORD disk_number, DWORD signature){

    PBYTE                      buffer = NULL;
    PLEGACY_MBR                pLegacyMBR = NULL;
    PGPT_PARTITIONTABLE_HEADER pGptPartitonHeader = NULL;
    DWORD                      dwBytesRead;
    DWORD                      cbReturned;
    BOOL                       bResult = FALSE;

    macho::windows::auto_file_handle handle = CreateFile(boost::str(boost::wformat(L"\\\\.\\PhysicalDrive%d") % disk_number).c_str(),
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_NO_BUFFERING,
        NULL);

    if (handle.is_valid()){
        buffer = (PBYTE)malloc(BUFSIZE2);
        if (buffer){
            SecureZeroMemory(buffer, BUFSIZE2);
            if (ReadFile(handle, buffer, BUFSIZE2, &dwBytesRead, NULL)){
                pLegacyMBR = (PLEGACY_MBR)buffer;
                pGptPartitonHeader = (PGPT_PARTITIONTABLE_HEADER)&buffer[512];
                if ((pLegacyMBR->Signature == 0xAA55) && (pLegacyMBR->PartitionRecord[0].PartitionType == 0xEE) && (pGptPartitonHeader->Signature == 0x5452415020494645)){
                    LOG(LOG_LEVEL_INFO, L"Partition Style is GPT. ");
                    bResult = TRUE;
                }
                else if (pLegacyMBR->UniqueMBRSignature == signature){
                    LOG(LOG_LEVEL_INFO, L"MBR Signature is not changed. ");
                    bResult = TRUE;
                }
                else{
                    pLegacyMBR->UniqueMBRSignature = signature;
                    SetFilePointer(handle, 0, NULL, 0);
                    if (bResult = WriteFile(handle, buffer, dwBytesRead, &cbReturned, NULL))
                        LOG(LOG_LEVEL_INFO, L"Succeeded to update the MBR signature!");
                    else
                        LOG(LOG_LEVEL_ERROR, L"Failed to update the MBR signature. (%d)", GetLastError());
                }
            }
            free(buffer);
        }
    }
    return bResult ? true : false;
}

bool irm_conv::fix_boot_volume_geometry_issue(__in const boost::filesystem::path boot_volume, DISK_GEOMETRY &geometry){
    
    BOOL                   bResult = FALSE;
    std::wstring           path;
    if (irm_conv::is_drive_letter(boot_volume.wstring()))
        path = boost::str(boost::wformat(L"\\\\.\\%1%:") % boot_volume.wstring()[0]);
    else
        path = boot_volume.wstring();

    macho::windows::auto_file_handle hVolumeHandle = CreateFileW(path.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_NO_BUFFERING,
        NULL);

    if (hVolumeHandle.is_valid()){

        LARGE_INTEGER          OffSet ;
        DWORD                  dwBytesRead;
        BYTE                   chBuffer[BUFSIZE];
        DWORD                  cbReturned;
        PPARTITION_BOOT_SECTOR pPartitionBootSector = NULL;

        SecureZeroMemory(chBuffer, sizeof(chBuffer));
        
        //OffSet.QuadPart = 0 * (geometry.BytesPerSector > 0 ? geometry.BytesPerSector : 512);
        OffSet.QuadPart = 0;
        FlushFileBuffers(hVolumeHandle);
        SetFilePointerEx(hVolumeHandle, OffSet, NULL, 0);

        if (!(bResult = ReadFile(hVolumeHandle, chBuffer, BUFSIZE, &dwBytesRead, NULL))){
            LOG(LOG_LEVEL_ERROR, _T("ReadFile Error (%d)"), GetLastError());
        }
        else{
            pPartitionBootSector = (PPARTITION_BOOT_SECTOR)&chBuffer;

            // We only apply the change on windows file system partiton.

            if ((pPartitionBootSector->EndofSectorMarker == 0xAA55) &&
                (((pPartitionBootSector->OEM_Identifier[0] == 'N') && (pPartitionBootSector->OEM_Identifier[1] == 'T') && (pPartitionBootSector->OEM_Identifier[2] == 'F') && (pPartitionBootSector->OEM_Identifier[3] == 'S')) // NTFS
                || ((chBuffer[54] == 'F') && (chBuffer[55] == 'A') && (chBuffer[56] == 'T')) // FAT
                || ((chBuffer[82] == 'F') && (chBuffer[83] == 'A') && (chBuffer[84] == 'T')))) // FAT32
            {
                if ((geometry.SectorsPerTrack != pPartitionBootSector->BPB.SectorsPerTrack) || (geometry.TracksPerCylinder != pPartitionBootSector->BPB.TracksPerCylinder)){
                    pPartitionBootSector->BPB.SectorsPerTrack = (WORD)geometry.SectorsPerTrack;
                    pPartitionBootSector->BPB.TracksPerCylinder = (WORD)geometry.TracksPerCylinder;
                    if (DeviceIoControl(hVolumeHandle, FSCTL_LOCK_VOLUME, NULL, 0, NULL, 0, &cbReturned, NULL)){
                        if (DeviceIoControl(hVolumeHandle, FSCTL_DISMOUNT_VOLUME, NULL, 0, NULL, 0, &cbReturned, NULL)){
                            SetFilePointerEx(hVolumeHandle, OffSet, NULL, 0);

                            if (!(bResult = WriteFile(hVolumeHandle, chBuffer, BUFSIZE, &dwBytesRead, NULL))){
                                LOG(LOG_LEVEL_ERROR, _T("WriteFile Error (%d)"), GetLastError());
                            }
                        }
                        else{
                            LOG(LOG_LEVEL_ERROR, _T("FSCTL_DISMOUNT_VOLUME Error (%d)"), GetLastError() );
                        }

                        DeviceIoControl(hVolumeHandle, FSCTL_UNLOCK_VOLUME, NULL, 0, NULL, 0, &cbReturned, NULL);
                    }
                    else{
                        LOG(LOG_LEVEL_ERROR, _T("FSCTL_LOCK_VOLUME Error (%d)"), GetLastError() );
                    }
                }
                else{
                    bResult = TRUE;
                }
            }
            else{
                LOG(LOG_LEVEL_ERROR, _T("Unknow partition file system type"));
            }
        }
    }
    return bResult ? true : false;
}

void irm_conv::set_active_directory_recovery(const os_image_info &image, bool is_sysvol_authoritative_restore){
    if (image.is_dc()){
        bool is_ntfrs_mode = false;
        registry reg(*image.system_hive_edit_ptr.get());
        if ( reg.open(L"System\\CurrentControlSet\\Services\\NTDS\\Parameters" ) )
            reg[_T("Database restored from backup")] = (DWORD)0x01;
        if (reg.open(L"System\\CurrentControlSet\\Services\\NtFrs"))
            is_ntfrs_mode = ((DWORD)reg[_T("Start")] == 0x02) && reg.subkey(_T("Parameters\\Backup/Restore\\Process at Startup")).exists();
        if (is_ntfrs_mode){
            if (reg.open(L"System\\CurrentControlSet\\Services\\NtFrs\\Parameters\\Backup/Restore\\Process at Startup"))
                reg[_T("BurFlags")] = (DWORD)is_sysvol_authoritative_restore ? 0x000000D4 : 0x000000CA;
        }
        else if (image.major_version >= 6){
            bool is_dfsr_mode = false;
            if (reg.open(L"System\\CurrentControlSet\\Services\\DFSR")){
                if ( is_dfsr_mode = ((DWORD)reg[_T("Start")] == 0x02) )
                    reg.subkey(_T("Restore"), true)[_T("SYSVOL")] = is_sysvol_authoritative_restore ? _T("authoritative") : _T("non-authoritative");
            }

            if (is_dfsr_mode){
                if (reg.open(L"System\\CurrentControlSet\\Control\\BackupRestore")){
                    reg.subkey(_T("SystemStateRestore"), true)[_T("LastRestoreId")] = (std::wstring)macho::guid_::create();
                }
            }
        }
    }
}

void irm_conv::set_machine_password_change(__in const os_image_info &image, __in bool is_disable){
    if (!image.is_dc()){
        registry reg(*image.system_hive_edit_ptr.get());
        if (reg.open(L"System\\CurrentControlSet\\Services\\Netlogon\\Parameters"))
            reg[_T("DisablePasswordChange")] = (DWORD)(is_disable ? 0x1 : 0x0);
    }
}

void irm_conv::disable_unexpected_shutdown_message(__in const os_image_info &image){
    registry reg(*image.system_hive_edit_ptr.get());
    if (reg.open(L"Software\\Microsoft\\Windows\\CurrentVersion\\Reliability"))
        reg[_T("LastAliveStamp")].delete_value();
}

bool irm_conv::add_windows_service(const os_image_info &image, const std::wstring short_service_name, const std::wstring display_name, const std::wstring image_path){
    registry reg(*image.system_hive_edit_ptr.get(), macho::windows::REGISTRY_CREATE);
    std::wstring key_path = boost::str(boost::wformat(L"System\\CurrentControlSet\\services\\%s") % short_service_name);
    if (reg.open(key_path)){
        reg[_T("DisplayName")] = display_name;
        reg[_T("ObjectName")] = _T("LocalSystem");
        reg[_T("ErrorControl")] = (DWORD)1;
        reg[_T("Start")] = (DWORD)2;
        reg[_T("Type")] = (DWORD)16;
        reg[_T("ImagePath")].set_expand_sz(image_path.c_str());
    }
    else{
        LOG(LOG_LEVEL_ERROR, L"Can't open the registry key path (%s)", key_path.c_str());
        return false;
    }
    std::wstring safe_boot_key_path = boost::str(boost::wformat(L"System\\CurrentControlSet\\Control\\SafeBoot\\Network\\%s") % short_service_name);
    if (reg.open(safe_boot_key_path))
        reg[_T("")] = _T("Service");
    else{
        LOG(LOG_LEVEL_ERROR, L"Can't open the registry key path (%s)", safe_boot_key_path.c_str());
        return false;
    }
    return true;
}

bool irm_conv::clone_key_entry(macho::windows::reg_edit_base& reg, HKEY key, std::wstring source_path, std::wstring target_path, uint32_t level){
    registry source_reg(reg, REGISTRY_READONLY);
    registry target_reg(reg, REGISTRY_CREATE);
    bool result = false;
    if (!(result = source_reg.open(key, source_path))){
        LOG(LOG_LEVEL_ERROR, L"Can't open the source registry key path (%s)", source_path.c_str());
    }
    else if (!(result = target_reg.open(key, target_path))){
        LOG(LOG_LEVEL_ERROR, L"Can't open/create the target registry key path (%s)", target_path.c_str());
    }
    else{
        if (level > 0){
            level--;
            source_reg.refresh_subkeys();
            for (int i = 0; i < source_reg.subkeys_count(); i++){
                string_array_w sub_keys = macho::stringutils::tokenize(source_reg.subkey(i).key_path(), L"\\/");
                if (!(result = clone_key_entry(reg, key, source_reg.subkey(i).key_path(), boost::str(boost::wformat(L"%s\\%s") % target_path % sub_keys[sub_keys.size() - 1]), level)))
                    break;
            }
        }
        if (result){
            for (int i = 0; i < source_reg.count(); i++){
                size_t ulBinSize;
                std::auto_ptr<BYTE> lpBin;
                switch (source_reg[i].type()){
                case REG_NONE:
                    target_reg[source_reg[i].name()];
                    break;
                case REG_SZ:
                    target_reg[source_reg[i].name()] = source_reg[i];
                    break;
                case REG_EXPAND_SZ:
                    target_reg[source_reg[i].name()].set_expand_sz(source_reg[i].wstring().c_str());
                    break;
                case REG_MULTI_SZ:
                    for (int iMultiCount = 0; iMultiCount < source_reg[i].get_multi_count(); iMultiCount++)
                        target_reg[source_reg[i].name()].add_multi(source_reg[i].get_multi_at(iMultiCount));
                    break;
                case REG_BINARY:
                    ulBinSize = source_reg[i].get_binary_length();
                    lpBin = std::auto_ptr<BYTE>(new BYTE[ulBinSize]);
                    if (NULL != lpBin.get()){
                        source_reg[i].get_binary(lpBin.get(), ulBinSize);
                        target_reg[source_reg[i].name()].set_binary(lpBin.get(), ulBinSize);
                    }
                    break;
                case REG_DWORD:
                    target_reg[source_reg[i].name()] = source_reg[i];
                    break;
                default:
                    break;
                }
            }
        }
    }
    return result;
}

bool irm_conv::add_credential_provider(__in const os_image_info &image, __in macho::guid_ clsid, __in const std::wstring name, __in const std::wstring dll){
    if (image.major_version >= 6){
        std::vector<std::wstring> reg_contexts;
        reg_edit_ex reg(*image.system_hive_edit_ptr.get());
        reg_contexts.push_back(L"[HKEY_LOCAL_MACHINE\\Software\\Microsoft\\Windows\\CurrentVersion\\Authentication\\Credential Providers]");
        reg_contexts.push_back(L"\"ProhibitFallbacks\"=dword:1");
        reg_contexts.push_back(boost::str(boost::wformat(L"[HKEY_LOCAL_MACHINE\\Software\\Microsoft\\Windows\\CurrentVersion\\Authentication\\Credential Providers\\{%s}]")%clsid.wstring()));
        reg_contexts.push_back(boost::str(boost::wformat(L"@=\"%s\"") % name));
        reg_contexts.push_back(boost::str(boost::wformat(L"[HKEY_LOCAL_MACHINE\\Software\\Classes\\CLSID\\{%s}]") % clsid.wstring()));
        reg_contexts.push_back(boost::str(boost::wformat(L"@=\"%s\"") % name));
        reg_contexts.push_back(boost::str(boost::wformat(L"[HKEY_LOCAL_MACHINE\\Software\\Classes\\CLSID\\{%s}\\InprocServer32]") % clsid.wstring()));
        reg_contexts.push_back(boost::str(boost::wformat(L"@=\"%s\"") % dll));
        reg_contexts.push_back(_T("\"ThreadingModel\"=\"Apartment\""));
        return reg.import(reg_contexts, true) == ERROR_SUCCESS;
    }
    return false;
}

void irm_conv::windows_service_control(__in const os_image_info &image, const std::map<std::wstring, int>& services){
    registry reg(*image.system_hive_edit_ptr.get());
    typedef  std::map<std::wstring, int> service_map;
    if (reg.open(L"System\\CurrentControlSet\\services")){
        foreach(service_map::value_type s, services){
            if (reg.subkey(s.first).exists())
                reg.subkey(s.first)[L"Start"] = s.second;
        }
    }
}

bool irm_conv::copy_directory(boost::filesystem::path const & source, boost::filesystem::path const & destination){
    namespace fs = boost::filesystem;
    try
    {
        // Check whether the function call is valid
        if (
            !fs::exists(source) ||
            !fs::is_directory(source)
            )
        {
            std::cerr << "Source directory " << source.string()
                << " does not exist or is not a directory." << '\n'
                ;
            LOG(LOG_LEVEL_ERROR, L"Source directory %s does not exist or is not a directory.", source.wstring().c_str());
            return false;
        }
        if (fs::exists(destination))
        {
            std::clog << "Destination directory " << destination.string()
                << " already exists." << '\n'
                ;
            LOG(LOG_LEVEL_INFO, L"Destination directory %s already exists.", destination.wstring().c_str());
        }
        // Create the destination directory
        else if (!fs::create_directories(destination))
        {
            std::cerr << "Unable to create destination directory"
                << destination.string() << '\n'
                ;
            LOG(LOG_LEVEL_ERROR, L"Unable to create destination directory %s", destination.wstring().c_str());
            return false;
        }
    }
    catch (fs::filesystem_error const & e)
    {
        std::cerr << e.what() << '\n';
        LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_ansi_to_unicode(e.what()));
        return false;
    }
    // Iterate through the source directory
    for (
        fs::directory_iterator file(source);
        file != fs::directory_iterator(); ++file
        )
    {
        try
        {
            fs::path current(file->path());
            if (fs::is_directory(current))
            {
                // Found directory: Recursion
                if (
                    !copy_directory(
                    current,
                    destination / current.filename()
                    )
                    )
                {
                    return false;
                }
            }
            else
            {
                // Found file: Copy
                fs::copy_file(
                    current,
                    destination / current.filename()
                    );
            }
        }
        catch (fs::filesystem_error const & e)
        {
            std::cerr << e.what() << '\n';
            LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_ansi_to_unicode(e.what()));
            return false;
        }
    }
    return true;
}

bool irm_conv::create_hybrid_mbr(const int disk_number, std::string& gpt){
    std::wstring path = boost::str(boost::wformat(L"\\\\.\\PhysicalDrive%d") % disk_number);
    macho::windows::auto_file_handle hDevice = CreateFile(path.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if (hDevice.is_valid()){
        boost::shared_ptr<DRIVE_LAYOUT_INFORMATION_EX> pdli = get_drive_layout(hDevice);
        if (pdli && pdli->PartitionStyle == PARTITION_STYLE_GPT){
            gpt_partition::vtr gpt_parts = gpt_partition::get(pdli);
            gpt_partition::ptr boot_part, msft_reserved;
            int count = 0, msft_reserved_partition_number = 0;
            foreach(gpt_partition::ptr p, gpt_parts){
                if (p->type == PARTITION_SYSTEM_GUID)
                    boot_part = p;
                else if (p->type == PARTITION_MSFT_RESERVED_GUID){
                    msft_reserved = p;
                    msft_reserved_partition_number = count;
                }
                count++;
            }

            if (!(gpt_parts.size() <= 4 || (boot_part && msft_reserved && msft_reserved->start > boot_part->start && msft_reserved_partition_number < 4))){
                LOG(LOG_LEVEL_ERROR, _T("Can't support to create hybrid mbr for this disk."));
            }
            else{
                universal_disk_rw::ptr                         io = universal_disk_rw::ptr(new  general_io_rw(hDevice, path));
                std::string                                    buf;
                DWORD                                          bytes;
                uint32_t                                       bytes_be_written = 0;
                uint32_t                                       bytes_to_read = 0;
                if (io->read(0, BUFSIZE * 34, gpt)){
                    PLEGACY_MBR                     pLegacyMBR = (PLEGACY_MBR)gpt.c_str();
                    pLegacyMBR->Signature = 0x0;
                    if (!io->write(0, gpt, bytes_be_written)){
                        LOG(LOG_LEVEL_ERROR, L"Failed to clear partition table info.");
                    }
                    else if (!DeviceIoControl(hDevice, IOCTL_DISK_UPDATE_PROPERTIES, NULL, 0, NULL, 0, &bytes, NULL)){
                        LOG(LOG_LEVEL_ERROR, _T("IOCTL_DISK_UPDATE_PROPERTIES Error (%d)"), GetLastError());
                    }
                    else{
                        DWORD                                          i;
                        PARTITION_INFORMATION_EX *	                   newppi = NULL;
                        CREATE_DISK	                                   createDisk;
                        BYTE                                           chBuffer[BUFSIZE];
                        PPARTITION_BOOT_SECTOR                         pPartitionBootSector = NULL;
                        DWORD                                          dwBufSize = sizeof(DRIVE_LAYOUT_INFORMATION_EX) + sizeof(PARTITION_INFORMATION_EX) * gpt_parts.size();
                        boost::shared_ptr<DRIVE_LAYOUT_INFORMATION_EX> new_pdli = boost::shared_ptr<DRIVE_LAYOUT_INFORMATION_EX>((DRIVE_LAYOUT_INFORMATION_EX*)new BYTE[dwBufSize]);
                        memset(new_pdli.get(), 0, dwBufSize);
                        SecureZeroMemory(&chBuffer, BUFSIZE);
                        memset(&createDisk, 0, sizeof(createDisk));

                        Sleep(500); // Waiting for the IOCTL_DISK_UPDATE_PROPERTIES IOCTL to remove volumes object.
                        createDisk.PartitionStyle = PARTITION_STYLE_MBR;
                        if (!DeviceIoControl(hDevice, IOCTL_DISK_CREATE_DISK, &createDisk, sizeof(createDisk), NULL, 0, &bytes, NULL)){
                            LOG(LOG_LEVEL_ERROR, _T("IOCTL_DISK_CREATE_DISK Error (%d)"), GetLastError());
                        }
                        else if (!DeviceIoControl(hDevice, IOCTL_DISK_GET_DRIVE_LAYOUT_EX, NULL, 0, new_pdli.get(), dwBufSize, &bytes, NULL)){
                            LOG(LOG_LEVEL_ERROR, _T("IOCTL_DISK_GET_DRIVE_LAYOUT_EX Error (%d)"), GetLastError());
                        }
                        else{
                            new_pdli->PartitionCount = gpt_parts.size();
                            for (i = 0, newppi = new_pdli->PartitionEntry; i < new_pdli->PartitionCount; ++i){
                                if (io->read(gpt_parts[i]->start, BUFSIZE, chBuffer, bytes_to_read)){
                                    pPartitionBootSector = (PPARTITION_BOOT_SECTOR)&chBuffer;
                                    newppi[i].RewritePartition = TRUE;
                                    newppi[i].PartitionNumber = i + 1;
                                    newppi[i].StartingOffset.QuadPart = gpt_parts[i]->start;
                                    newppi[i].PartitionLength.QuadPart = gpt_parts[i]->length;
                                    newppi[i].PartitionStyle = PARTITION_STYLE_MBR;
                                    newppi[i].Mbr.PartitionType = 0xFB;
                                    newppi[i].Mbr.RecognizedPartition = TRUE;
                                    newppi[i].Mbr.BootIndicator = FALSE;

                                    if (pPartitionBootSector->EndofSectorMarker == 0xAA55){
                                        if ((pPartitionBootSector->OEM_Identifier[0] == 'N') && (pPartitionBootSector->OEM_Identifier[1] == 'T') && (pPartitionBootSector->OEM_Identifier[2] == 'F') && (pPartitionBootSector->OEM_Identifier[3] == 'S')){ // NTFS
                                            newppi[i].Mbr.PartitionType = PARTITION_IFS;
                                        }
                                        else if ((chBuffer[54] == 'F') && (chBuffer[55] == 'A') && (chBuffer[56] == 'T')){ // FAT
                                            newppi[i].Mbr.PartitionType = PARTITION_HUGE;
                                        }
                                        else if ((chBuffer[82] == 'F') && (chBuffer[83] == 'A') && (chBuffer[84] == 'T')){ // FAT32
                                            newppi[i].Mbr.PartitionType = PARTITION_FAT32;
                                        }
                                    }

                                    if (gpt_parts[i]->type == PARTITION_SYSTEM_GUID){
                                        newppi[i].Mbr.BootIndicator = TRUE;
                                    }
                                    else if (gpt_parts[i]->type == PARTITION_LDM_DATA_GUID ||
                                        gpt_parts[i]->type == PARTITION_LDM_METADATA_GUID){
                                        newppi[i].Mbr.PartitionType = PARTITION_LDM;
                                    }
                                    else if (new_pdli->PartitionCount > 4 && gpt_parts[i]->type == PARTITION_MSFT_RESERVED_GUID){
                                        newppi[i].Mbr.PartitionType = PARTITION_EXTENDED;
                                        newppi[i].PartitionLength.QuadPart = (gpt_parts[gpt_parts.size() - 1]->start + gpt_parts[gpt_parts.size() - 1]->length) - gpt_parts[i]->start;
                                    }
                                }
                            }
                            if (!DeviceIoControl(hDevice, IOCTL_DISK_SET_DRIVE_LAYOUT_EX, new_pdli.get(), dwBufSize, NULL, 0, &bytes, NULL)){
                                LOG(LOG_LEVEL_ERROR, _T("IOCTL_DISK_SET_DRIVE_LAYOUT_EX Error (%d)"), GetLastError());
                            }
                            else{
                                BYTE buff[BUFSIZE];
                                if (io->read(0, BUFSIZE, buff, bytes_to_read)){
                                    pLegacyMBR = (PLEGACY_MBR)buff;
                                    memcpy_s(buff, 440, DISK_MBRDATA, 440);
                                    if (!io->write(0, buff, BUFSIZE, bytes_be_written)){}
                                    else if (DeviceIoControl(hDevice, IOCTL_DISK_UPDATE_PROPERTIES, NULL, 0, NULL, 0, &bytes, NULL)){
                                        return true;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return false;
}

bool irm_conv::remove_hybrid_mbr(__in const int disk_number, __in std::string gpt){
    std::wstring path = boost::str(boost::wformat(L"\\\\.\\PhysicalDrive%d") % disk_number);
    macho::windows::auto_file_handle hDevice = CreateFile(path.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if (hDevice.is_valid()){
        boost::shared_ptr<DRIVE_LAYOUT_INFORMATION_EX> pdli = get_drive_layout(hDevice);
        if (pdli && pdli->PartitionStyle == PARTITION_STYLE_MBR){
            universal_disk_rw::ptr                         io = universal_disk_rw::ptr(new  general_io_rw(hDevice, path));
            std::string                                    buf;
            DWORD                                          bytes;
            uint32_t                                       bytes_be_written = 0;
            uint32_t                                       bytes_to_read = 0;
            PLEGACY_MBR                                    pLegacyMBR = (PLEGACY_MBR)gpt.c_str();
            PGPT_PARTITIONTABLE_HEADER                     pGptPartitonHeader = (PGPT_PARTITIONTABLE_HEADER)&gpt[BUFSIZE];
            PGPT_PARTITION_ENTRY                           pGptPartitionEntries = (PGPT_PARTITION_ENTRY)&gpt[BUFSIZE * pGptPartitonHeader->PartitionEntryLBA];
            mbr_partition::vtr mbr_parts = mbr_partition::get(pdli);
            gpt_partition::vtr gpt_parts = gpt_partition::get(pGptPartitionEntries, pGptPartitonHeader->NumberOfPartitionEntries, 512);
            if (mbr_parts.size() > gpt_parts.size()){
                LOG(LOG_LEVEL_ERROR, _T("Failed to remove hybrid mbr. The MBR partition number is more than GPT partition number."));
                return false;
            }
            else if (mbr_parts.size() < gpt_parts.size()){
                LOG(LOG_LEVEL_ERROR, _T("Failed to remove hybrid mbr. The MBR partition number is less than GPT partition number."));
                return false;
            }
            for (size_t i = 0; i < gpt_parts.size(); i++){
                if (mbr_parts[i]->boot_indicator){
                    if (gpt_parts[i]->type != PARTITION_SYSTEM_GUID){
                        LOG(LOG_LEVEL_ERROR, _T("Failed to remove hybrid mbr. The partiton layout is changed (boot partition)."));
                        return false;
                    }
                }
                else if (mbr_parts[i]->type == PARTITION_LDM){
                    if (!(gpt_parts[i]->type == PARTITION_LDM_DATA_GUID ||
                        gpt_parts[i]->type == PARTITION_LDM_METADATA_GUID)){
                        LOG(LOG_LEVEL_ERROR, _T("Failed to remove hybrid mbr. The partiton layout is changed (LDM partition)."));
                        return false;
                    }
                }
                else if (mbr_parts[i]->type == PARTITION_EXTENDED){
                    if (gpt_parts[i]->type != PARTITION_MSFT_RESERVED_GUID){
                        LOG(LOG_LEVEL_ERROR, _T("Failed to remove hybrid mbr. The partiton layout is changed (MSFT Reserved partition)."));
                        return false;
                    }
                }
            }

            pLegacyMBR->Signature = 0x0;
            if (!io->write(0, gpt, bytes_be_written)){
                LOG(LOG_LEVEL_ERROR, L"Failed to clear partition table info.");
            }
            else if (!DeviceIoControl(hDevice, IOCTL_DISK_UPDATE_PROPERTIES, NULL, 0, NULL, 0, &bytes, NULL)){
                LOG(LOG_LEVEL_ERROR, _T("IOCTL_DISK_UPDATE_PROPERTIES Error (%d)"), GetLastError());
            }
            else{
                DWORD                                          i;
                PARTITION_INFORMATION_EX *	                   newppi = NULL;
                CREATE_DISK	                                   createDisk;
                BYTE                                           chBuffer[BUFSIZE];
                DWORD                                          dwBufSize = sizeof(DRIVE_LAYOUT_INFORMATION_EX) + sizeof(PARTITION_INFORMATION_EX) * mbr_parts.size();
                boost::shared_ptr<DRIVE_LAYOUT_INFORMATION_EX> new_pdli = boost::shared_ptr<DRIVE_LAYOUT_INFORMATION_EX>((DRIVE_LAYOUT_INFORMATION_EX*)new BYTE[dwBufSize]);
                
                memset(new_pdli.get(), 0, dwBufSize);
                SecureZeroMemory(&chBuffer, BUFSIZE);
                memset(&createDisk, 0, sizeof(createDisk));

                Sleep(500); // Waiting for the IOCTL_DISK_UPDATE_PROPERTIES IOCTL to remove volumes object.
                createDisk.PartitionStyle = PARTITION_STYLE_GPT;
                createDisk.Gpt.DiskId = pGptPartitonHeader->DiskGUID;
                createDisk.Gpt.MaxPartitionCount = pGptPartitonHeader->NumberOfPartitionEntries;
                if (!DeviceIoControl(hDevice, IOCTL_DISK_CREATE_DISK, &createDisk, sizeof(createDisk), NULL, 0, &bytes, NULL)){
                    LOG(LOG_LEVEL_ERROR, _T("IOCTL_DISK_CREATE_DISK Error (%d)"), GetLastError());
                }
                else if (!DeviceIoControl(hDevice, IOCTL_DISK_GET_DRIVE_LAYOUT_EX, NULL, 0, new_pdli.get(), dwBufSize, &bytes, NULL)){
                    LOG(LOG_LEVEL_ERROR, _T("IOCTL_DISK_GET_DRIVE_LAYOUT_EX Error (%d)"), GetLastError());
                }
                else{
                    new_pdli->PartitionCount = mbr_parts.size();
                    for (i = 0, newppi = new_pdli->PartitionEntry; i < mbr_parts.size(); ++i){
                        newppi[i].RewritePartition = TRUE;
                        newppi[i].PartitionNumber = i + 1;
                        newppi[i].StartingOffset.QuadPart = mbr_parts[i]->start;
                        newppi[i].PartitionLength.QuadPart = mbr_parts[i]->length;
                        newppi[i].PartitionStyle = PARTITION_STYLE_GPT;                 
                        if (i < gpt_parts.size()){      
                            newppi[i].Gpt.PartitionType = gpt_parts[i]->type;
                            newppi[i].Gpt.PartitionId = gpt_parts[i]->unique;
                            newppi[i].Gpt.Attributes = gpt_parts[i]->attributes;
                            wcsncpy_s(newppi[i].Gpt.Name, 36, gpt_parts[i]->name.c_str(), gpt_parts[i]->name.length());
                        }                     
                        if (mbr_parts[i]->type == PARTITION_EXTENDED){
                            if (i != mbr_parts.size() - 1){
                                newppi[i].PartitionLength.QuadPart = mbr_parts[i + 1]->start - mbr_parts[i]->start;
                            }
                        }
                    }
                    if (!DeviceIoControl(hDevice, IOCTL_DISK_SET_DRIVE_LAYOUT_EX, new_pdli.get(), dwBufSize, NULL, 0, &bytes, NULL)){
                        LOG(LOG_LEVEL_ERROR, _T("IOCTL_DISK_SET_DRIVE_LAYOUT_EX Error (%d)"), GetLastError());
                    }
                    else{
                        BYTE buff[BUFSIZE];
                        if (io->read(0, BUFSIZE, buff, bytes_to_read)){
                            pLegacyMBR = (PLEGACY_MBR)buff;
                            memcpy_s(buff, 440, DISK_MBRDATA, 440);
                            if (!io->write(0, buff, BUFSIZE, bytes_be_written)){}
                            else if (DeviceIoControl(hDevice, IOCTL_DISK_UPDATE_PROPERTIES, NULL, 0, NULL, 0, &bytes, NULL)){
                                return true;
                            }
                        }
                    }
                }
            }
        }
    }
    return false;
}

boost::shared_ptr<DRIVE_LAYOUT_INFORMATION_EX> irm_conv::get_drive_layout(__in const int disk_number){
    macho::windows::auto_file_handle hDiskHandle = CreateFile(boost::str(boost::wformat(L"\\\\.\\PhysicalDrive%d") % disk_number).c_str(),
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_NO_BUFFERING,
        NULL);
    if (hDiskHandle.is_valid())
        return get_drive_layout(hDiskHandle);
    else
        return NULL;
}

boost::shared_ptr<DRIVE_LAYOUT_INFORMATION_EX> irm_conv::get_drive_layout(HANDLE handle){

    DWORD                           dwBytesReturned;
    BOOL                            fResult = FALSE;
    DWORD                           dwRet = ERROR_SUCCESS;
    DWORD                           bufsize = sizeof(DRIVE_LAYOUT_INFORMATION_EX) + sizeof(PARTITION_INFORMATION_EX) * 3;
    boost::shared_ptr<DRIVE_LAYOUT_INFORMATION_EX> ppdli = boost::shared_ptr<DRIVE_LAYOUT_INFORMATION_EX>((DRIVE_LAYOUT_INFORMATION_EX*)new BYTE[bufsize]);

    if (ppdli){
        memset(ppdli.get(), 0, bufsize);
        while (!(fResult = DeviceIoControl(handle, IOCTL_DISK_GET_DRIVE_LAYOUT_EX,
            NULL, 0, ppdli.get(), bufsize,
            &dwBytesReturned, NULL))){
            dwRet = GetLastError();
            if (dwRet == ERROR_INSUFFICIENT_BUFFER){
                // Create enough space for four more partition table entries.
                bufsize += sizeof(PARTITION_INFORMATION_EX) * 4;
                ppdli = boost::shared_ptr<DRIVE_LAYOUT_INFORMATION_EX>((DRIVE_LAYOUT_INFORMATION_EX*)new BYTE[bufsize]);

                if (!ppdli){
                    break;
                }
                memset(ppdli.get(), 0, bufsize);
            }
            else{
                break;
            }
        }
        if (fResult)
            return ppdli;
    }
    return NULL;
}

bool  irm_conv::add_drivers(__in const boost::filesystem::path system_volume, __in const boost::filesystem::path path, bool force_unsigned){
    std::wstring cmd = boost::str(boost::wformat(L"%s\\dism.exe /Image:%s /Add-Driver /Driver:%s") % macho::windows::environment::get_system_directory() % system_volume.wstring() % path.wstring());
    if (boost::filesystem::is_directory(path))
        cmd.append(L" /Recurse");
    if (force_unsigned)
        cmd.append(L" /ForceUnsigned");
    std::wstring ret;
    bool result = exec_console_application_with_timeout(cmd, ret, INFINITE);
#ifdef _DEBUG
    LOG(result ? LOG_LEVEL_RECORD : LOG_LEVEL_ERROR, L"Execute command(%s) -> result: \n%s", cmd.c_str(), ret.c_str());
#else
    LOG(result ? LOG_LEVEL_TRACE : LOG_LEVEL_ERROR, L"Execute command(%s) -> result: \n%s", cmd.c_str(), ret.c_str());
#endif
    return result;
}

bool irm_conv::remove_drivers(__in const boost::filesystem::path system_volume, __in const std::vector<boost::filesystem::path> paths){
    bool result = false;
    if (paths.size()){
        std::wstring cmd = boost::str(boost::wformat(L"%s\\dism.exe /Image:%s /Remove-Driver") % macho::windows::environment::get_system_directory() % system_volume.wstring());
        foreach(boost::filesystem::path path, paths){
            if (boost::filesystem::is_directory(path))
                return false;
            else
                cmd.append(boost::str(boost::wformat(L" /Driver:%s") % path.wstring()));
        }
        std::wstring ret;
        result = exec_console_application_with_timeout(cmd, ret, INFINITE);
#ifdef _DEBUG
        LOG(result ? LOG_LEVEL_RECORD : LOG_LEVEL_ERROR, L"Execute command(%s) -> result: \n%s", cmd.c_str(), ret.c_str());
#else
        //LOG(result ? LOG_LEVEL_TRACE : LOG_LEVEL_ERROR, L"Execute command(%s) -> result: \n%s", cmd.c_str(), ret.c_str());
#endif
    }
    return result;
}

DWORD irm_conv::write_to_file(boost::filesystem::path file_path, LPCVOID buffer, DWORD number_of_bytes_to_write, DWORD mode){
    DWORD dwRetVal;
    DWORD  rc = ERROR_SUCCESS;
    SetFileAttributes(file_path.wstring().c_str(), FILE_ATTRIBUTE_NORMAL);
    macho::windows::auto_file_handle hFile = CreateFile((LPTSTR)file_path.wstring().c_str(), // file name 
        GENERIC_WRITE,        // open for write 
        0,                    // do not share 
        NULL,                 // default security 
        mode,               // create mode
        FILE_ATTRIBUTE_NORMAL,// normal file 
        NULL);
    if (hFile.is_valid()){
        SetFilePointer(hFile, 0, NULL, FILE_END);
        if (!WriteFile(hFile, buffer, number_of_bytes_to_write, &dwRetVal, NULL))
            rc = GetLastError();
    }
    else
        rc = GetLastError();
    return rc;
}

std::string  irm_conv::read_from_file(boost::filesystem::path file_path){
    std::fstream file(file_path.string(), std::ios::in | std::ios::binary);
    std::string result = std::string();
    if (file.is_open()){
        file.seekg(0, std::ios::end);
        result.resize(file.tellg());
        file.seekg(0, std::ios::beg);
        file.read(&result[0], result.size());
        file.close();
    }
    return result;
}

bool irm_conv::is_drive_letter(std::wstring volume_path){
    return temp_drive_letter::is_drive_letter(volume_path);
}

bool  irm_converter::initialize(__in const int disk_number){

    _stg = macho::windows::storage::get();
    _disk = _stg->get_disk(disk_number);
    macho::windows::storage::partition::vtr partitions;
    boost::filesystem::path boot_loader;
    if (_disk){
        if (_disk->is_offline()) {
            _disk->online();
            boost::this_thread::sleep(boost::posix_time::seconds(3));
            _disk = _stg->get_disk(disk_number);
        }
        
        if (_disk->is_read_only()){
            _disk->clear_read_only_flag();
            boost::this_thread::sleep(boost::posix_time::seconds(3));
            _disk = _stg->get_disk(disk_number);
        }
        int count = 3;
        while (count > 0 && (_disk->is_offline() || _disk->is_read_only() || _disk->partition_style() == storage::ST_PST_UNKNOWN)){
            boost::this_thread::sleep(boost::posix_time::seconds(3));
            _disk = _stg->get_disk(disk_number);
            count--;
        }
        //partitions = _stg->get_partitions(disk_number);
        //foreach(macho::windows::storage::partition::ptr p, partitions){
        //    p->set_attributes(false, p->no_default_drive_letter(), p->is_active(), false);
        //}

        if (_disk->is_offline()){
            LOG(LOG_LEVEL_ERROR, L"The disk (%d) is offline.", disk_number);
            return false;
        }
        else if (_disk->is_read_only()){
            LOG(LOG_LEVEL_ERROR, L"The disk (%d) is read_only.", disk_number);
            return false;
        }
        else if (_disk->partition_style() == storage::ST_PST_UNKNOWN){
            LOG(LOG_LEVEL_ERROR, L"The partition style of disk (%d) is unknown.", disk_number);
            return false;
        }
        else if (_disk->partition_style() == storage::ST_PST_GPT){
            mount_volumes();
            partitions = _stg->get_partitions(disk_number);
            foreach(macho::windows::storage::partition::ptr p, partitions){
                if (macho::guid_(p->gpt_type()) == macho::guid_(PARTITION_SYSTEM_GUID)){
                    _system_drive = temp_drive_letter::assign(p->disk_number(), p->partition_number());
                    break;
                }
            }
            if (!_system_drive){
                LOG(LOG_LEVEL_ERROR, L"Can't find the gpt system partition on disk (%d).", disk_number);
                return false;
            }
            else{
                _boot_volume = _system_drive->drive_letter();
                _boot_cfg = _boot_volume / L"EFI\\Microsoft\\BOOT\\BCD";
                _boot_cfg_folder = _boot_volume / L"EFI\\Microsoft\\BOOT";
                boot_loader = _boot_volume / L"EFI\\Microsoft\\BOOT\\bootmgr.efi";
                if (!(boost::filesystem::exists(_boot_cfg) && boost::filesystem::exists(boot_loader))){
                    LOG(LOG_LEVEL_ERROR, L"Can't find the boot file (%s) and (%s) on disk (%d).", _boot_cfg.wstring().c_str(), boot_loader.wstring().c_str(), disk_number);
                    return false;
                }
                else if (!irm_conv::get_win_dir(_boot_cfg_folder, _win_element)){
                    LOG(LOG_LEVEL_ERROR, L"Can't find the boot element form the BCD folder (%s).", _boot_cfg_folder.wstring().c_str());
                    return false;
                }
                else{
                    if (_win_element.is_template){
                        foreach(macho::windows::storage::partition::ptr p, partitions){
                            if (p->access_paths().size() && boost::filesystem::exists(boost::filesystem::path(p->access_paths()[0]) / _win_element.windows_dir)){
                                _win_element.partition_offset = p->offset();
                                _windows_volume = p->access_paths()[0];
                                break;
                            }
                        }
                    }
                    else{
                        foreach(macho::windows::storage::partition::ptr p, partitions){
                            if (macho::guid_(p->guid()) == macho::guid_(_win_element.partition_guid)){
                                _win_element.partition_offset = p->offset();
                                if (p->access_paths().size())
                                    _windows_volume = p->access_paths()[0];
                                break;
                            }
                        }
                    }
                    get_bcd_edit();
                }
            }
        }
        else if (_disk->partition_style() == storage::ST_PST_MBR){
            count = 3;
            while (count > 0 && _boot_volume.empty()){
                count--;
                mount_volumes();
                partitions = _stg->get_partitions(disk_number);
                foreach(macho::windows::storage::partition::ptr p, partitions){
                    if (p->is_active()){
                        if (p->access_paths().size()){
                            if (p->access_paths().size() == 1){
                                _system_drive = temp_drive_letter::assign(p->disk_number(), p->partition_number());
                                if (_system_drive) _boot_volume = _system_drive->drive_letter();
                            }
                            else
                                _boot_volume = p->access_paths()[0];
                        }
                        else{
                            if (count == 2)
                                _stg->rescan();
                            else
                                boost::this_thread::sleep(boost::posix_time::seconds(3));
                        }
                        break;
                    }
                }
            }
            if (!_boot_volume.wstring().length()){
                LOG(LOG_LEVEL_ERROR, L"Can't find the mbr boot partition on disk (%d).", disk_number);
            }
            else{
                _boot_cfg = _boot_volume / L"BOOT\\BCD";
                _boot_cfg_folder = _boot_volume / L"BOOT";
                boot_loader = _boot_volume / L"bootmgr";
                if (!(boost::filesystem::exists(_boot_cfg) && boost::filesystem::exists(boot_loader))){
                    LOG(LOG_LEVEL_WARNING, L"Can't find the boot file (%s) and (%s) on disk (%d).", _boot_cfg.wstring().c_str(), boot_loader.wstring().c_str(), disk_number);
                    _boot_cfg = _boot_volume / L"boot.ini";
                    _boot_cfg_folder.clear();
                    boot_loader = _boot_volume / L"ntldr";
                    if (!(boost::filesystem::exists(_boot_cfg) && boost::filesystem::exists(boot_loader))){
                        LOG(LOG_LEVEL_ERROR, L"Can't find the boot file (%s) and (%s) on disk (%d).", _boot_cfg.wstring().c_str(), boot_loader.wstring().c_str(), disk_number);
                        return false;
                    }
                    else if (!irm_conv::get_win_dir_ini(_boot_cfg, _disk, _win_element)){
                        LOG(LOG_LEVEL_ERROR, L"Can't find the boot element form the boot.ini file (%s).", _boot_cfg.wstring().c_str());
                        return false;
                    }
                    else{
                        _is_boot_ini_file = true;
                    }
                }
                else if (!irm_conv::get_win_dir(_boot_cfg_folder, _win_element)){
                    LOG(LOG_LEVEL_ERROR, L"Can't find the boot element form the BCD folder (%s).", _boot_cfg_folder.wstring().c_str());
                    return false;
                }

                if (_win_element.is_boot_volume){
                    _windows_volume = _boot_volume;
                }
                else if (_win_element.is_template){
                    foreach(macho::windows::storage::partition::ptr p, partitions){
                        if (p->access_paths().size() && boost::filesystem::exists(boost::filesystem::path(p->access_paths()[0]) / _win_element.windows_dir)){
                            _win_element.partition_offset = p->offset();
                            _windows_volume = p->access_paths()[0];
                            break;
                        }
                    }
                }
                else{
                    foreach(macho::windows::storage::partition::ptr p, partitions){
                        if (p->offset() == _win_element.partition_offset){
                            if (p->access_paths().size() && boost::filesystem::exists(boost::filesystem::path(p->access_paths()[0]) / _win_element.windows_dir)){
                                if (p->access_paths().size() == 1){
                                    _windows_drive = temp_drive_letter::assign(p->disk_number(), p->partition_number());
                                    if (_windows_drive) _windows_volume = _windows_drive->drive_letter();
                                }
                                else
                                    _windows_volume = p->access_paths()[0];
                            }
                            break;
                        }
                    }
                }
                if (!_is_boot_ini_file){
                    get_bcd_edit();
                }
            }
        }
    }
    if (!_windows_volume.wstring().length()){
        LOG(LOG_LEVEL_ERROR, L"Can't find the windows volume on disk (%d).", disk_number);
    }
    if (!_is_boot_ini_file && !_bcd){
        LOG(LOG_LEVEL_ERROR, L"Can't load the bcd config file on disk (%d).", disk_number);
    }
   
    return  (_is_boot_ini_file || _bcd) && _windows_volume.wstring().length() > 0;
}

bool irm_converter::gpt_to_mbr(){
    macho::windows::storage::partition::vtr partitions;
    std::string gpt;
    if (_disk){
        if (_disk->partition_style() == storage::ST_PST_GPT){
            boost::filesystem::remove_all(_boot_volume / L"Boot");
            boost::filesystem::remove(_boot_volume / L"bootmgr");
            if (!irm_conv::copy_directory(_windows_volume / _win_element.windows_dir / L"Boot\\PCAT", _boot_volume / L"Boot")){
                LOG(LOG_LEVEL_ERROR, L"Failed to copy the boot folder.");
                return false;
            }
            else if (!MoveFile(boost::filesystem::path(_boot_volume / L"Boot\\bootmgr").wstring().c_str(), boost::filesystem::path(_boot_volume / L"bootmgr").wstring().c_str())){
                LOG(LOG_LEVEL_ERROR, L"Failed to move the bootmgr file.");
                return false;
            }
            else if (!CopyFile(_boot_cfg.wstring().c_str(), boost::filesystem::path(_boot_volume / L"Boot\\BCD").wstring().c_str(), FALSE)){
                LOG(LOG_LEVEL_ERROR, L"Failed to copy the bcd file.");
                return false;
            }
            else{
                _bcd = NULL;
                irm_conv::flush_and_dismount_fs(_windows_volume.wstring());
                _system_drive = NULL;
                dismount_volumes();
                if (!irm_conv::create_hybrid_mbr(_disk->number(), gpt)){
                }
                else{
                    boost::this_thread::sleep(boost::posix_time::seconds(10));
                    _disk = _stg->get_disk(_disk->number());
                }
            }
            _boot_volume = L"";
            _windows_volume = L"";
        }

        if (gpt.length() && (_disk->partition_style() == storage::ST_PST_MBR)){
            int count = 3;
            while (count > 0 && _boot_volume.empty()){
                count--;
                mount_volumes();
                partitions = _stg->get_partitions(_disk->number());
                foreach(macho::windows::storage::partition::ptr p, partitions){
                    if (p->is_active()){
                        if (p->access_paths().size())
                            _boot_volume = p->access_paths()[0];
                        else{
                            if (count == 2)
                                _stg->rescan();
                            else
                                boost::this_thread::sleep(boost::posix_time::seconds(3));
                        }
                        break;
                    }
                }
            }
            if (!_boot_volume.wstring().length()){
                LOG(LOG_LEVEL_ERROR, L"Can't find the mbr boot partition on disk (%d).", _disk->number());
                return false;
            }
            else{
                boost::filesystem::path  boot_loader = _boot_volume / L"bootmgr";
                _boot_cfg = _boot_volume / L"BOOT\\BCD";
                _boot_cfg_folder = _boot_volume / L"BOOT";
                if (!(boost::filesystem::exists(_boot_cfg) && boost::filesystem::exists(boot_loader))){
                    LOG(LOG_LEVEL_ERROR, L"Can't find the boot file (%s) and (%s) on disk (%d).", _boot_cfg.wstring().c_str(), boot_loader.wstring().c_str(), _disk->number());
                    return false;
                }
                try{
                    bool is_virtual_disk = false;
                    std::vector<DWORD> disks;
                    DWORD err = win_vhdx_mgmt::get_all_attached_virtual_disk_physical_paths(disks);
                    foreach(DWORD d, disks){
                        if (d == _disk->number()){
                            is_virtual_disk = true;
                            break;
                        }
                    }

                    if ( gpt.size()){
                        _win_element.is_mbr_disk = true;
                        _win_element.mbr_signature = _disk->signature();
                        // Set boot device by registry edit
                        if ( is_virtual_disk )
                            irm_conv::set_bcd_boot_device_entry(_boot_cfg_folder, _win_element);
                    }

                    if (get_bcd_edit()){
                        foreach(macho::windows::storage::partition::ptr p, partitions){
                            if (p->offset() == _win_element.partition_offset){
                                if (p->access_paths().size()){
                                    _windows_volume = p->access_paths()[0];
                                    if (gpt.size()){
                                        if (!_bcd->set_boot_system_device(_windows_volume.wstring(), is_virtual_disk)){
                                            LOG(LOG_LEVEL_ERROR, L"Can't reset the bcd boot system partition.", _disk->number());
                                        }
                                        irm_conv::write_to_file(_boot_volume / L"gpt.backup", gpt);
                                    }
                                }
                                break;
                            }
                        }
                    }
                }
                catch (wmi_exception &e){
                    LOG(LOG_LEVEL_ERROR, L"%s", get_diagnostic_information(e).c_str());
                }
                catch (bcd_edit::exception &e){
                    LOG(LOG_LEVEL_ERROR, L"%s", get_diagnostic_information(e).c_str());
                }
                catch (macho::exception_base &e){
                    LOG(LOG_LEVEL_ERROR, L"%s", get_diagnostic_information(e).c_str());
                }
                catch (...){
                    LOG(LOG_LEVEL_ERROR, L"Unknown");
                }
            }
        }
    }
    return (_is_boot_ini_file || _bcd) && _disk && (_disk->partition_style() == storage::ST_PST_MBR) && _windows_volume.wstring().length() > 0;
}

bool irm_converter::remove_hybird_mbr(){
    macho::windows::storage::partition::vtr partitions;
    std::string gpt;
    bool _remove_hybird_mbr = false;
    if (_disk){
        if (_disk->partition_style() == storage::ST_PST_MBR){
            if (boost::filesystem::exists(_boot_volume / "gpt.backup") && boost::filesystem::exists(_boot_volume / "EFI\\Microsoft\\BOOT")){
                gpt = irm_conv::read_from_file(_boot_volume / "gpt.backup");
                _bcd = NULL;
                irm_conv::flush_and_dismount_fs(_windows_volume.wstring());
                _system_drive = NULL;
                dismount_volumes();
                if (!(_remove_hybird_mbr = irm_conv::remove_hybrid_mbr(_disk->number(), gpt))){
                }
                else{
                    boost::this_thread::sleep(boost::posix_time::seconds(10));
                    _disk = _stg->get_disk(_disk->number());
                }
                _boot_volume = L"";
                _windows_volume = L"";
            }
        }

        if (_remove_hybird_mbr && _disk->partition_style() == storage::ST_PST_GPT){
            int disk_number = _disk->number();
            mount_volumes();
            partitions = _stg->get_partitions(disk_number);
            foreach(macho::windows::storage::partition::ptr p, partitions){
                if (macho::guid_(p->gpt_type()) == macho::guid_(PARTITION_SYSTEM_GUID)){
                    _system_drive = temp_drive_letter::assign(p->disk_number(), p->partition_number());
                    break;
                }
            }
            if (!_system_drive){
                LOG(LOG_LEVEL_ERROR, L"Can't find the gpt system partition on disk (%d).", disk_number);
                return false;
            }
            else{
                _boot_volume = _system_drive->drive_letter();
                _boot_cfg = _boot_volume / L"EFI\\Microsoft\\BOOT\\BCD";
                _boot_cfg_folder = _boot_volume / L"EFI\\Microsoft\\BOOT";
                boost::filesystem::path boot_loader = _boot_volume / L"EFI\\Microsoft\\BOOT\\bootmgr.efi";
                if (!(boost::filesystem::exists(_boot_cfg) && boost::filesystem::exists(boot_loader))){
                    LOG(LOG_LEVEL_ERROR, L"Can't find the boot file (%s) and (%s) on disk (%d).", _boot_cfg.wstring().c_str(), boot_loader.wstring().c_str(), disk_number);
                    return false;
                }
                else if (!irm_conv::get_win_dir(_boot_cfg_folder, _win_element)){
                    LOG(LOG_LEVEL_ERROR, L"Can't find the boot element form the BCD folder (%s).", _boot_cfg_folder.wstring().c_str());
                    return false;
                }
                else{
                    if (_win_element.is_template){
                        foreach(macho::windows::storage::partition::ptr p, partitions){
                            if (p->access_paths().size() && boost::filesystem::exists(boost::filesystem::path(p->access_paths()[0]) / _win_element.windows_dir)){
                                _win_element.partition_offset = p->offset();
                                _windows_volume = p->access_paths()[0];
                                break;
                            }
                        }
                    }
                    else{
                        foreach(macho::windows::storage::partition::ptr p, partitions){
                            if (macho::guid_(p->guid()) == macho::guid_(_win_element.partition_guid)){
                                _win_element.partition_offset = p->offset();
                                if (p->access_paths().size())
                                    _windows_volume = p->access_paths()[0];
                                break;
                            }
                        }
                    }
                    get_bcd_edit();
                }
            }
        }
    }
    return (_is_boot_ini_file || _bcd) && _disk && (_disk->partition_style() == storage::ST_PST_GPT) && _windows_volume.wstring().length() > 0;
}

bool irm_converter::openstack(os_image_info &os, boost::filesystem::path &drv_dir, boost::filesystem::path &working_dir, std::string config){
    bool result = false;
    do{
        //Remove the StartOverride to make sure the OS can be boot up when the migration was occurred between "vioscsi" and "viostor" scsi adapter.
        static std::vector<std::wstring> reg_contexts = {
            _T("[-HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\vioscsi\\StartOverride]"),
            _T("[-HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\viostor\\StartOverride]")
        };
        reg_edit_ex reg(*os.system_hive_edit_ptr.get());
        if (result = (ERROR_SUCCESS == reg.import(reg_contexts, true))){
        }
    } while (false);

    if (result = boost::filesystem::exists(drv_dir / L"virtio-win")){
        boost::filesystem::path driver_folder = drv_dir / L"virtio-win";
        boost::filesystem::path os_folder = os.to_path();
        _bcd = NULL;
        macho::windows::operating_system _os = os;
        macho::windows::operating_system _current_os = macho::windows::environment::get_os_version();

        bool not_dism_inject_driver = _os.is_win2003() || _os.is_win2003r2() || _os.is_win2008() || _os.is_winvista() || (_os.major_version() > _current_os.major_version()) || (_os.major_version() == _current_os.major_version() && _os.minor_version() > _current_os.minor_version());

        if (not_dism_inject_driver){
            std::vector<boost::filesystem::path> oem_infs = macho::windows::environment::get_files(_windows_volume / _win_element.windows_dir / L"inf", _T("oem*.inf"));
            typedef std::map<std::wstring, boost::filesystem::path>    vio_map;
            vio_map vio_wins;
#ifdef VIOSCSI
            if (_os.is_winvista_or_later())
                vio_wins[_T("PCI\\VEN_1AF4&DEV_1004&SUBSYS_00081AF4&REV_00")] = driver_folder / "vioscsi" / os_folder / L"vioscsi.INF";
#endif
            vio_wins[_T("PCI\\VEN_1AF4&DEV_1001&SUBSYS_00021AF4&REV_00")] = driver_folder / "viostor" / os_folder / L"VIOSTOR.INF";
            foreach(vio_map::value_type &vio, vio_wins){
                bool is_found = false;
                foreach(boost::filesystem::path &inf, oem_infs){
                    macho::windows::setup_inf_file _inf;
                    if (result = _inf.load(inf.wstring())){
                        macho::windows::setup_inf_file::match_device_info::ptr match_dev = _inf.verify(_os, { vio.first }, {});
                        if (match_dev){
                            is_found = true;
                            break;
                        }
                    }
                }
                if (!is_found){
                    macho::windows::setup_inf_file _inf;
                    if (result = _inf.load(vio.second.wstring())){
                        macho::windows::setup_inf_file::match_device_info::ptr match_dev = _inf.verify(_os, { vio.first }, {});
                        if (match_dev){
                            setup_inf_file::setup_action::vtr _actions = _inf.get_setup_actions(match_dev, _windows_volume / _win_element.windows_dir, (*os.system_hive_edit_ptr.get()));
                            foreach(setup_inf_file::setup_action::ptr action, _actions){
                                if (!(result = action->install(*os.system_hive_edit_ptr.get())))
                                    break;
                            }
                        }
                    }
                }
                if (!result)
                    break;
            }
        }

        if (result){
            os.system_hive_edit_ptr = NULL;
            boost::filesystem::path agent_folder = boost::filesystem::path(_windows_volume / _win_element.windows_dir) / L"agent";
            if (boost::filesystem::exists(agent_folder)){
                std::vector<boost::filesystem::path> files = macho::windows::environment::get_files(agent_folder, L"*", true);
                foreach(boost::filesystem::path &f, files){
                    SetFileAttributesW(f.wstring().c_str(), FILE_ATTRIBUTE_NORMAL);
                }
                boost::filesystem::remove_all(agent_folder);
            }

            if (not_dism_inject_driver){
                if (!(result = irm_conv::copy_directory(driver_folder / "Balloon" / os_folder, agent_folder / L"Balloon"))){
                    LOG(LOG_LEVEL_ERROR, L"Failed to copy_directory (%s).", boost::filesystem::path(driver_folder / "Balloon" / os_folder).wstring().c_str());
                }
                else if (!(result = irm_conv::copy_directory(driver_folder / "NetKVM" / os_folder, agent_folder / L"NetKVM"))){
                    LOG(LOG_LEVEL_ERROR, L"Failed to copy_directory (%s).", boost::filesystem::path(driver_folder / "NetKVM" / os_folder).wstring().c_str());
                }
                else if (!(result = irm_conv::copy_directory(driver_folder / "qemupciserial", agent_folder / L"qemupciserial"))){
                    LOG(LOG_LEVEL_ERROR, L"Failed to copy_directory (%s).", boost::filesystem::path(driver_folder / "qemupciserial").wstring().c_str());
                }
                else if (!(result = irm_conv::copy_directory(driver_folder / "vioserial" / os_folder, agent_folder / L"vioserial"))){
                    LOG(LOG_LEVEL_ERROR, L"Failed to copy_directory (%s).", boost::filesystem::path(driver_folder / "vioserial" / os_folder).wstring().c_str());
                }
                else if (!(result = irm_conv::copy_directory(driver_folder / "viostor" / os_folder, agent_folder / L"viostor"))){
                    LOG(LOG_LEVEL_ERROR, L"Failed to copy_directory (%s).", boost::filesystem::path(driver_folder / "viostor" / os_folder).wstring().c_str());
                }
                else if (_os.is_winvista_or_later()){
                    if (!(result = irm_conv::copy_directory(driver_folder / "viorng" / os_folder, agent_folder / L"viorng"))){
                        LOG(LOG_LEVEL_ERROR, L"Failed to copy_directory (%s).", boost::filesystem::path(driver_folder / "viorng" / os_folder).wstring().c_str());
                    }
#ifdef VIOSCSI
                    else if (!(result = irm_conv::copy_directory(driver_folder / "vioscsi" / os_folder, agent_folder / L"vioscsi"))){
                        LOG(LOG_LEVEL_ERROR, L"Failed to copy_directory (%s).", boost::filesystem::path(driver_folder / "vioscsi" / os_folder).wstring().c_str());
                    }
#endif
                }
            }
            else{
                boost::filesystem::path tmp_dir = macho::windows::environment::create_temp_folder(L"tmp", drv_dir.wstring());
                if (tmp_dir.wstring().length()){
                    if (!(result = irm_conv::copy_directory(driver_folder / "Balloon" / os_folder, tmp_dir / L"Balloon"))){
                        LOG(LOG_LEVEL_ERROR, L"Failed to copy_directory (%s).", boost::filesystem::path(driver_folder / "Balloon" / os_folder).wstring().c_str());
                    }
                    else if (!(result = irm_conv::copy_directory(driver_folder / "NetKVM" / os_folder, tmp_dir / L"NetKVM"))){
                        LOG(LOG_LEVEL_ERROR, L"Failed to copy_directory (%s).", boost::filesystem::path(driver_folder / "NetKVM" / os_folder).wstring().c_str());
                    }
                    else if (!(result = irm_conv::copy_directory(driver_folder / "qemupciserial", tmp_dir / L"qemupciserial"))){
                        LOG(LOG_LEVEL_ERROR, L"Failed to copy_directory (%s).", boost::filesystem::path(driver_folder / "qemupciserial").wstring().c_str());
                    }
                    else if (!(result = irm_conv::copy_directory(driver_folder / "vioserial" / os_folder, tmp_dir / L"vioserial"))){
                        LOG(LOG_LEVEL_ERROR, L"Failed to copy_directory (%s).", boost::filesystem::path(driver_folder / "vioserial" / os_folder).wstring().c_str());
                    }
                    else if (!(result = irm_conv::copy_directory(driver_folder / "viostor" / os_folder, tmp_dir / L"viostor"))){
                        LOG(LOG_LEVEL_ERROR, L"Failed to copy_directory (%s).", boost::filesystem::path(driver_folder / "viostor" / os_folder).wstring().c_str());
                    }
                    else if (!(result = irm_conv::copy_directory(driver_folder / "viorng" / os_folder, tmp_dir / L"viorng"))){
                        LOG(LOG_LEVEL_ERROR, L"Failed to copy_directory (%s).", boost::filesystem::path(driver_folder / "viorng" / os_folder).wstring().c_str());
                    }
#ifdef VIOSCSI
                    else if (!(result = irm_conv::copy_directory(driver_folder / "vioscsi" / os_folder, tmp_dir / L"vioscsi"))){
                        LOG(LOG_LEVEL_ERROR, L"Failed to copy_directory (%s).", boost::filesystem::path(driver_folder / "vioscsi" / os_folder).wstring().c_str());
                    }
#endif
                    else{
                        result = irm_conv::add_drivers(_windows_volume, tmp_dir, true);
                    }
                    std::vector<boost::filesystem::path> files = macho::windows::environment::get_files(tmp_dir, L"*", true);
                    foreach(boost::filesystem::path &f, files){
                        SetFileAttributesW(f.wstring().c_str(), FILE_ATTRIBUTE_NORMAL);
                    }
                    boost::filesystem::remove_all(tmp_dir);
                }
                else{
                    LOG(LOG_LEVEL_ERROR, L"Failed to create temp dirver directory under (%s).", drv_dir.wstring().c_str());
                    result = false;
                }
            }

            if (result){
                boost::filesystem::path agent_source;
                if (_os.is_winvista_or_later())
                    agent_source = working_dir / (os.is_amd64() ? L"x64" : L"x86");
                else
                    agent_source = working_dir / (os.is_amd64() ? L"w2k3_x64" : L"w2k3_x86");

                if (!(result = irm_conv::copy_directory(agent_source, agent_folder))){
                    LOG(LOG_LEVEL_ERROR, L"Failed to copy_directory (%s).", agent_source.wstring().c_str());
                }
                else if (!config.empty()){
                    boost::filesystem::remove(agent_folder / L"agent.cfg");
                    result = ERROR_SUCCESS == irm_conv::write_to_file(agent_folder / L"agent.cfg", config);
                }

                if (result){
                    irm_conv::flush_and_dismount_fs(_boot_volume.wstring());
                    irm_conv::flush_and_dismount_fs(_windows_volume.wstring());
                }
            }
        }
    }
    return result;
}

bool irm_converter::xen(os_image_info &os, boost::filesystem::path &drv_dir, boost::filesystem::path &working_dir, std::string config){
    bool result = false;
    if (result = boost::filesystem::exists(drv_dir / L"XenTools")){
        macho::windows::operating_system _os = os;
        std::vector<std::wstring> reg_contexts = _reg_contexts;
        // To AWS - Enable RealTimeIsUniversal in TimeZoneInformation
        reg_contexts.push_back(_T("[HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\TimeZoneInformation]"));
        reg_contexts.push_back(_T("\"RealTimeIsUniversal\"=dword:00000001"));
        if (_os.is_win2003() || _os.is_win2003r2()){     
            reg_contexts.push_back(_T("[HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\CriticalDeviceDatabase\\pci#ven_8086&dev_7010]"));
            reg_contexts.push_back(_T("\"Service\"=\"intelide\""));
            reg_contexts.push_back(_T("\"ClassGUID\" = \"{4D36E96A-E325-11CE-BFC1-08002BE10318}\""));
        }
        reg_edit_ex reg(*os.system_hive_edit_ptr.get());
        if (result = (ERROR_SUCCESS == reg.import(reg_contexts, true))){
            extract_system_driver_file(os, L"atapi.sys");
            extract_system_driver_file(os, L"Intelide.sys");
            extract_system_driver_file(os, L"Pciide.sys");
            extract_system_driver_file(os, L"Pciidex.sys");

            boost::filesystem::path agent_folder = boost::filesystem::path(_windows_volume / _win_element.windows_dir) / L"agent";
            if (boost::filesystem::exists(agent_folder)){
                std::vector<boost::filesystem::path> files = macho::windows::environment::get_files(agent_folder, L"*", true);
                foreach(boost::filesystem::path &f, files){
                    SetFileAttributesW(f.wstring().c_str(), FILE_ATTRIBUTE_NORMAL);
                }
                boost::filesystem::remove_all(agent_folder);
            }
            boost::filesystem::path agent_source;
            if (_os.is_winvista_or_later())
                agent_source = working_dir / (os.is_amd64() ? L"x64" : L"x86");
            else
                agent_source = working_dir / (os.is_amd64() ? L"w2k3_x64" : L"w2k3_x86");
            if (_os.is_x86() || _os.is_win2003() || _os.is_win2003r2() || _os.is_win2008() || _os.is_winvista()){
                if ((result = irm_conv::copy_directory(agent_source, agent_folder)) &&
                    (result = irm_conv::copy_directory(drv_dir / L"XenTools" / (os.is_amd64() ? L"x64" : L"x86"), agent_folder)) &&
                    (result = irm_conv::copy_directory(drv_dir / L"XenTools" / L"batch", agent_folder))){
                    if (!config.empty()){
                        result = ERROR_SUCCESS == irm_conv::write_to_file(agent_folder / L"agent.cfg", config);
                    }
                }
            }
            else{
                if ((result = irm_conv::copy_directory(agent_source, agent_folder)) &&
                    (result = irm_conv::copy_directory(drv_dir / L"XenTools" / L"aws_pv_drivers", agent_folder))){
                    if (!config.empty()){
                        result = ERROR_SUCCESS == irm_conv::write_to_file(agent_folder / L"agent.cfg", config);
                    }
                }                
            }
            os.system_hive_edit_ptr = NULL;
            irm_conv::flush_and_dismount_fs(_boot_volume.wstring());
            irm_conv::flush_and_dismount_fs(_windows_volume.wstring());
        }
    }
    return result;
}

bool irm_converter::hyperv(os_image_info &os, boost::filesystem::path &drv_dir, boost::filesystem::path &working_dir, std::string config){
    bool result = false;
    if (result = boost::filesystem::exists(drv_dir / L"HyperV")){
        reg_edit_ex reg(*os.system_hive_edit_ptr.get());
        if (result = (ERROR_SUCCESS == reg.import(_reg_contexts, true))){
            irm_conv_device::vtr devices;
            registry _reg;
            macho::windows::operating_system _os = os;
            if (_os.is_win2003() || _os.is_win2003r2() || _os.is_win2000() || _os.is_winxp()){
                extract_system_driver_file(os, L"atapi.sys");
                extract_system_driver_file(os, L"Intelide.sys");
                extract_system_driver_file(os, L"Pciide.sys");
                extract_system_driver_file(os, L"Pciidex.sys");
                extract_system_driver_file(os, L"hidclass.sys");
                extract_system_driver_file(os, L"hidparse.sys"); 
                extract_system_driver_file(os, L"hidusb.sys");
            }
            try{    
                if (boost::filesystem::exists(drv_dir / L"HyperV" / L"support"))
                    result = any_to_any(os, drv_dir / L"HyperV" / L"support" / (os.is_amd64() ? L"amd64" : L"x86"), working_dir, devices, config);
                else
                    result = any_to_any(os, boost::filesystem::path(), working_dir, devices, config);
            }
            catch (const boost::filesystem::filesystem_error& ex){
                result = false;
                LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_ansi_to_unicode(ex.what()).c_str());
            }            
        }
    }
    return result;
}

bool irm_converter::vmware(os_image_info &os, boost::filesystem::path &drv_dir, boost::filesystem::path &working_dir, irm_conv_device::vtr& devices, std::string config){
    bool result = false;
    if (result = boost::filesystem::exists(drv_dir / L"VMWare")){
        macho::windows::operating_system _os = os;
        std::vector<std::wstring> reg_contexts;
        reg_contexts.push_back(_T("[@HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\QL2300]"));
        reg_contexts.push_back(_T("\"Start\"=dword:00000004"));
        if (_os.is_win2000() || _os.is_winxp()){}
        else if (_os.is_win2003() || _os.is_win2003r2() ){
            extract_system_driver_file(os, L"symmpi.sys");
            reg_contexts.push_back(_T("[HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\CriticalDeviceDatabase\\pci#ven_1000&dev_0030]"));
            reg_contexts.push_back(_T("\"Service\"=\"symmpi\""));
            reg_contexts.push_back(_T("\"ClassGUID\"=\"{4D36E97B-E325-11CE-BFC1-08002BE10318}\""));
            reg_contexts.push_back(_T("[HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\symmpi]"));
            reg_contexts.push_back(_T("\"ErrorControl\"=dword:00000001"));
            reg_contexts.push_back(_T("\"Group\"=\"SCSI miniport\""));
            reg_contexts.push_back(_T("\"Start\"=dword:00000000"));
            reg_contexts.push_back(_T("\"Type\"=dword:00000001"));
            reg_contexts.push_back(_T("\"ImagePath\"=hex(2):73,00,79,00,73,00,74,00,65,00,6d,00,33,00,32,00,5c,00,44,00,\\"));
            reg_contexts.push_back(_T("52,00,49,00,56,00,45,00,52,00,53,00,5c,00,73,00,79,00,6d,00,6d,00,70,00,69,\\"));
            reg_contexts.push_back(_T("00,2e,00,73,00,79,00,73,00,00,00"));
            reg_contexts.push_back(_T("\"Tag\"=dword:00000021"));
            reg_contexts.push_back(_T("[HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\symmpi\\Parameters]"));
            reg_contexts.push_back(_T("\"BusType\"=dword:00000001"));
            reg_contexts.push_back(_T("[HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\symmpi\\Parameters\\PnpInterface]"));
            reg_contexts.push_back(_T("\"5\"=dword:00000001"));
        }
        else{
            reg_contexts.push_back(_T("[@HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\LSI_SCSI]"));
            reg_contexts.push_back(_T("\"Start\"=dword:00000000"));
            reg_contexts.push_back(_T("[@HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\LSI_SAS]"));
            reg_contexts.push_back(_T("\"Start\"=dword:00000000"));
        }
        reg_edit_ex reg(*os.system_hive_edit_ptr.get());
        if (result = (ERROR_SUCCESS == reg.import(reg_contexts, true))){
            //if (!devices.size()){
            //    irm_conv_device device;
            //    if (_os.is_win2003() || _os.is_win2003r2() || _os.is_win2000() || _os.is_winxp()){
            //        device.description = L"LSI Logic PCI - X Ultra320 SCSI Host Adapter";
            //        device.hardware_ids.push_back(L"PCI\\VEN_1000&DEV_0030&SUBSYS_197615AD&REV_01");
            //        device.hardware_ids.push_back(L"PCI\\VEN_1000&DEV_0030&SUBSYS_197615AD");
            //        device.hardware_ids.push_back(L"PCI\\VEN_1000&DEV_0030&CC_010000");
            //        device.hardware_ids.push_back(L"PCI\\VEN_1000&DEV_0030&CC_0100");

            //        device.compatible_ids.push_back(L"PCI\\VEN_1000&DEV_0030&REV_01");
            //        device.compatible_ids.push_back(L"PCI\\VEN_1000&DEV_0030");
            //        device.compatible_ids.push_back(L"PCI\\VEN_1000&CC_010000");
            //        device.compatible_ids.push_back(L"PCI\\VEN_1000&CC_0100");
            //        device.compatible_ids.push_back(L"PCI\\VEN_1000");
            //        device.compatible_ids.push_back(L"PCI\\CC_010000");
            //        device.compatible_ids.push_back(L"PCI\\CC_0100");
            //        devices.push_back(device);
            //    }
            //    else{
            //        device.description = L"LSI Adapter, SAS 3000 series, 8-port with 1068";
            //        device.hardware_ids.push_back(L"PCI\\VEN_1000&DEV_0054&SUBSYS_197615AD&REV_01");
            //        device.hardware_ids.push_back(L"PCI\\VEN_1000&DEV_0054&SUBSYS_197615AD");
            //        device.hardware_ids.push_back(L"PCI\\VEN_1000&DEV_0054&CC_010700");
            //        device.hardware_ids.push_back(L"PCI\\VEN_1000&DEV_0054&CC_0107");

            //        device.compatible_ids.push_back(L"PCI\\VEN_1000&DEV_0054&REV_01");
            //        device.compatible_ids.push_back(L"PCI\\VEN_1000&DEV_0054");
            //        device.compatible_ids.push_back(L"PCI\\VEN_1000&CC_010700");
            //        device.compatible_ids.push_back(L"PCI\\VEN_1000&CC_0107");
            //        device.compatible_ids.push_back(L"PCI\\VEN_1000");
            //        device.compatible_ids.push_back(L"PCI\\CC_010700&DT_0");
            //        device.compatible_ids.push_back(L"PCI\\CC_010700");
            //        device.compatible_ids.push_back(L"PCI\\CC_0107&DT_0");
            //        device.compatible_ids.push_back(L"PCI\\CC_0107");
            //    }
            //}
            try{
                result = any_to_any(os, drv_dir / L"VMWare" / (os.is_amd64() ? L"amd64" : L"x86"), working_dir, devices, config);
            }
            catch (const boost::filesystem::filesystem_error& ex){
                result = false;
                LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_ansi_to_unicode(ex.what()).c_str());
            }
        }
    }
    return result;
}

bool irm_converter::qemu(os_image_info &os, boost::filesystem::path &drv_dir, boost::filesystem::path &working_dir, std::string config){
    bool result = false;
    reg_edit_ex reg(*os.system_hive_edit_ptr.get());
    if (result = (ERROR_SUCCESS == reg.import(_reg_contexts, true))){
        irm_conv_device::vtr devices;
        registry _reg;
        macho::windows::operating_system _os = os;
        if (_os.is_win2003() || _os.is_win2003r2() || _os.is_win2000() || _os.is_winxp()){
            extract_system_driver_file(os, L"atapi.sys");
            extract_system_driver_file(os, L"Intelide.sys");
            extract_system_driver_file(os, L"Pciide.sys");
            extract_system_driver_file(os, L"Pciidex.sys");
            extract_system_driver_file(os, L"hidclass.sys");
            extract_system_driver_file(os, L"hidparse.sys");
            extract_system_driver_file(os, L"hidusb.sys");
        }
        result = openstack(os, drv_dir, working_dir, config);
    }
    return result;
}

bool irm_converter::any_to_any(os_image_info &os, boost::filesystem::path &drv_dir, boost::filesystem::path &working_dir, irm_conv_device::vtr& devices, std::string config){
    bool result = true;
    boost::filesystem::path agent_folder = boost::filesystem::path(_windows_volume / _win_element.windows_dir) / L"agent";
    if (boost::filesystem::exists(agent_folder)){
        std::vector<boost::filesystem::path> files = macho::windows::environment::get_files(agent_folder, L"*", true);
        foreach(boost::filesystem::path &f, files){
            SetFileAttributesW(f.wstring().c_str(), FILE_ATTRIBUTE_NORMAL);
        }
        boost::filesystem::remove_all(agent_folder);
    }
    if (drv_dir.empty() || !boost::filesystem::exists(drv_dir))
        LOG(LOG_LEVEL_WARNING, L"skip the drivers injection.");
    else{
        if (devices.size()){
            macho::windows::operating_system _os = os;
            if (_os.is_win2003() || _os.is_win2003r2() || _os.is_win2008() || _os.is_winvista() || _os.is_win2000() || _os.is_winxp()){
                // Need to be improved for checking the installed device driver from driver store.... ( C:\Windows\System32\DriverStore\FileRepository or C:\Windows\INF)
                static macho::guid_ scsi_adapter(L"{4d36e97b-e325-11ce-bfc1-08002be10318}");
                static macho::guid_ hdc(L"{4d36e96a-e325-11ce-bfc1-08002be10318}");
                std::vector<boost::filesystem::path> inf_files = macho::windows::environment::get_files(drv_dir, L"*.inf", true);
                foreach(auto &device, devices){
                    foreach(auto file, inf_files){
                        macho::windows::setup_inf_file _inf;
                        if ((result = _inf.load(file.wstring())) &&
                            (macho::guid_(_inf.class_guid()) == scsi_adapter || macho::guid_(_inf.class_guid()) == hdc)){
                            macho::windows::setup_inf_file::match_device_info::ptr match_dev = _inf.verify(_os, device.hardware_ids, device.compatible_ids);
                            if (match_dev){
                                setup_inf_file::setup_action::vtr _actions = _inf.get_setup_actions(match_dev, _windows_volume / _win_element.windows_dir, (*os.system_hive_edit_ptr.get()));
                                foreach(setup_inf_file::setup_action::ptr action, _actions){
                                    if (!(result = action->install(*os.system_hive_edit_ptr.get())))
                                        break;
                                }
                            }
                        }
                        if (!result)
                            break;
                    }
                    if (!result)
                        break;
                }
                if (result){
                    result = irm_conv::copy_directory(drv_dir, agent_folder);
                }
            }
            else{
                os.system_hive_edit_ptr = NULL;
                result = irm_conv::add_drivers(_windows_volume, drv_dir);
            }
        }
        else {
            result = irm_conv::copy_directory(drv_dir, agent_folder);
        }
    }

    if (result){
        macho::windows::operating_system _os = os;
        boost::filesystem::path agent_source;
        if (_os.is_winvista_or_later())
            agent_source = working_dir / (os.is_amd64() ? L"x64" : L"x86");
        else
            agent_source = working_dir / (os.is_amd64() ? L"w2k3_x64" : L"w2k3_x86");
        if (!(result = irm_conv::copy_directory(agent_source, agent_folder))){
            LOG(LOG_LEVEL_ERROR, L"Failed to copy_directory (%s).", agent_source.wstring().c_str());
        }
        else if (!config.empty()){
            boost::filesystem::remove(agent_folder / L"agent.cfg");
            result = ERROR_SUCCESS == irm_conv::write_to_file(agent_folder / L"agent.cfg", config);
        }
        os.system_hive_edit_ptr = NULL;
        irm_conv::flush_and_dismount_fs(_boot_volume.wstring());
        irm_conv::flush_and_dismount_fs(_windows_volume.wstring());
    }
    return result;
}

bool irm_converter::convert(irm_conv_parameter parameter){
    
    bool result = false;
    boost::filesystem::path working_dir = macho::windows::environment::get_working_directory();
    boost::filesystem::path drv_dir;

    if (parameter.drivers_path.length() && boost::filesystem::exists(parameter.drivers_path))
        drv_dir = parameter.drivers_path;
    else 
        drv_dir = working_dir;

    LOG(LOG_LEVEL_RECORD, L"Current Driver Path: (%s)", drv_dir.wstring().c_str());
    if ( (_is_boot_ini_file || _bcd) && _windows_volume.wstring().length()){
        os_image_info os = irm_conv::get_offline_os_info(_windows_volume / _win_element.windows_dir);
        if (!os.computer_name.length()){
            LOG(LOG_LEVEL_ERROR, L"Can't get the computer name from disk image.");
        }
        else{
            _computer_name = os.computer_name;
            _is_pending_windows_update = os.is_pending_windows_update;
            _operating_system = os;
            bool is_force_normal_boot = (os.is_eval_key || _win_element.is_template) ? true : parameter.is_force_normal_boot;
            if (!is_force_normal_boot){
                if (_is_boot_ini_file)
                    result = boot_ini_edit::set_safe_mode(_boot_cfg.wstring(), true, os.is_dc());
                else
                    result = _bcd->set_safe_mode(true, os.is_dc());
            }
            if ((!is_force_normal_boot) && (!result)){
                LOG(LOG_LEVEL_ERROR, L"Failed to enable safe mode.");
            }
            else{
                if (_is_boot_ini_file){
                    if (parameter.is_enable_debug)
                        boot_ini_edit::set_debug_settings(_boot_cfg.wstring(), true, true);
                }
                else{
                    _bcd->set_boot_status_policy_option(true);
                    _bcd->set_detect_hal_option(true);
                    if (parameter.is_enable_debug)
                        _bcd->set_debug_settings(true, true);
                    
                }
                if (parameter.is_disable_machine_password_change)
                    irm_conv::set_machine_password_change(os, true);

                irm_conv::set_active_directory_recovery(os, parameter.is_sysvol_authoritative_restore);
                irm_conv::disable_unexpected_shutdown_message(os);
                irm_conv::windows_service_control(os, parameter.services);
                
                {//Avoid service crash issue when release reg object.
                    registry reg(*os.system_hive_edit_ptr.get(),macho::windows::REGISTRY_CREATE);
                    if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
                        if (is_force_normal_boot)
                            reg[_T("ForceNormalBoot")] = (DWORD)0x1;
                        else
                            reg[_T("ForceNormalBoot")] = (DWORD)0x0;

                        if ( _win_element.is_template )
                            reg[_T("DisableAutoReboot")] = (DWORD)0x1;
                        else
                            reg[_T("DisableAutoReboot")] = (DWORD)0x0;

                        if (reg[_T("Machine")].exists())
                            reg[_T("Machine")].delete_value();

                        if (reg[_T("MgmtAddr")].exists())
                            reg[_T("MgmtAddr")].delete_value();

                        if (reg[_T("SessionId")].exists())
                            reg[_T("SessionId")].delete_value();

                        if (reg[_T("AllowMultiple")].exists())
                            reg[_T("AllowMultiple")].delete_value();

                        if (reg[_T("CallBacks")].exists())
                            reg[_T("CallBacks")].delete_value();

                        if (reg[_T("CallBackTimeOut")].exists())
                            reg[_T("CallBackTimeOut")].delete_value();

                        if (parameter.callbacks.size()){
                            foreach(std::string callback, parameter.callbacks){
                                if (!callback.empty())
                                    reg[_T("CallBacks")].add_multi(stringutils::convert_utf8_to_unicode(callback));
                            }
                            reg[_T("CallBackTimeOut")] = parameter.callback_timeout;
                        }
                    }
                }

                if ((result = irm_conv::add_windows_service(os, L"irm_conv_agent", L"irm_conv_agent", L"%SystemRoot%\\agent\\irm_conv_agent.exe") )&& 
                    (result = irm_conv::clone_key_entry(*os.system_hive_edit_ptr.get(), HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\SafeBoot", L"SYSTEM\\CurrentControlSet\\Control\\SafeBoot_", 2)) &&
                    (result = irm_conv::clone_key_entry(*os.system_hive_edit_ptr.get(), HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Services", L"SYSTEM\\CurrentControlSet\\Services_", 1)) &&
                    (_win_element.is_template || (result = irm_conv::clone_key_entry(*os.system_hive_edit_ptr.get(), HKEY_LOCAL_MACHINE, L"SYSTEM\\MountedDevices", L"SYSTEM\\MountedDevices_", 0)))){
                   
                    {
                        registry reg(*os.system_hive_edit_ptr.get());
                        if (reg.open(L"SYSTEM\\CurrentControlSet\\Control\\Session Manager")){
                            if (reg[_T("PendingFileRenameOperations")].exists() && reg[_T("PendingFileRenameOperations")].is_multi_sz()){
                                reg[_T("_PendingFileRenameOperations")] = reg[_T("PendingFileRenameOperations")];
                                reg[_T("PendingFileRenameOperations")].clear_multi();
                                std::wstring agent_folder = boost::str(boost::wformat(L"\\??\\%s\\agent") % os.system_root);
                                for (int i = 0; i < reg[_T("_PendingFileRenameOperations")].get_multi_count(); i++){
                                    std::wstring value = reg[_T("_PendingFileRenameOperations")].get_multi_at(i);
                                    if (value.length() >= agent_folder.length() && 0 == _wcsnicmp(value.c_str(), agent_folder.c_str(), agent_folder.length()))
                                        continue;
                                    reg[_T("PendingFileRenameOperations")].add_multi(value.c_str());
                                }
                                reg[_T("_PendingFileRenameOperations")].delete_value();
                                if ( reg[_T("PendingFileRenameOperations")].get_multi_count() == 0 )
                                    reg[_T("PendingFileRenameOperations")].delete_value();
                            }
                            reg.close();
                        }
                        if (reg.open(_T("SYSTEM\\CurrentControlSet\\Services\\xenfilt")) && reg[_T("Start")].exists()){
                            if (reg.open(L"SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e96a-e325-11ce-bfc1-08002be10318}")){	
                                if (reg[_T("UpperFilters")].exists())
                                    reg[_T("UpperFilters")].remove_multi(L"XENFILT");
                                if (parameter.type == conversion::type::xen){
                                    reg[_T("UpperFilters")].add_multi(L"XENFILT");
                                }
                            }
                            if (reg.open(L"SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e97d-e325-11ce-bfc1-08002be10318}")){
                                if (reg[_T("UpperFilters")].exists())
                                    reg[_T("UpperFilters")].remove_multi(L"XENFILT");
                                if (parameter.type == conversion::type::xen){
                                    reg[_T("UpperFilters")].add_multi(L"XENFILT");
                                }
                            }
                        }
                    }
                    if (boost::filesystem::exists(working_dir / L"jobs\\jobs.hive")){
                        std::string reg_contexts = irm_conv::read_from_file(working_dir / L"jobs\\jobs.hive");
                        reg_edit_ex reg(*os.system_hive_edit_ptr.get());
                        LOG(LOG_LEVEL_RECORD, L"Try to import jobs.hive.");
                        if (!(ERROR_SUCCESS == reg.import(macho::stringutils::convert_ansi_to_unicode(reg_contexts), true))){
                            LOG(LOG_LEVEL_ERROR, L"Failed to import jobs.hive.");
                        }
                    }
                    switch (parameter.type){
                    case conversion::type::openstack:
                        result = openstack(os, drv_dir, working_dir, parameter.config);
                        break;
                    case conversion::type::xen:
                        result = xen(os, drv_dir, working_dir, parameter.config);
                        break;
                    case conversion::type::vmware:
                        result = vmware(os, drv_dir, working_dir, parameter.devices, parameter.config);
                        break;
                    case conversion::type::hyperv:
                        result = hyperv(os, drv_dir, working_dir, parameter.config);
                        break;
                    case conversion::type::qemu:
                        result = qemu(os, drv_dir, working_dir, parameter.config);
                        break;
                    case conversion::type::any_to_any:
                    default:
                        result = any_to_any(os, boost::filesystem::path(parameter.drivers_path)/os.to_path(), working_dir, parameter.devices, parameter.config);
                    }

                    if (result && !parameter.post_script.empty()){
                        boost::filesystem::path agent_folder = boost::filesystem::path(_windows_volume / _win_element.windows_dir) / L"agent";
                        archive::unzip::ptr unzip_ptr = archive::unzip::open(parameter.post_script);
                        if (unzip_ptr){
                            LOG(LOG_LEVEL_RECORD, L"Decompress post script.");
                            if (!unzip_ptr->decompress_archive(agent_folder / "PostScript"))
                                LOG(LOG_LEVEL_ERROR, L"Cannot decompress post script.");
                        }
                    }

                    if (result && (_disk->partition_style() == storage::ST_PST_MBR) && parameter.sectors_per_track != 0 && parameter.tracks_per_cylinder != 0){
                        DISK_GEOMETRY geometry;
                        memset(&geometry, 0, sizeof(DISK_GEOMETRY));
                        geometry.SectorsPerTrack = parameter.sectors_per_track;
                        geometry.TracksPerCylinder = parameter.tracks_per_cylinder;
                        if (result = irm_conv::fix_disk_geometry_issue(_disk->number(), geometry))
                            result = irm_conv::fix_boot_volume_geometry_issue(_boot_volume, geometry);
                    }
                    _system_drive = NULL;
                    _windows_drive = NULL;
                    dismount_volumes();
                    _disk->offline();
                    if (result && (_disk->partition_style() == storage::ST_PST_MBR) && _win_element.mbr_signature ){
                        _disk->clear_read_only_flag();
                        result = irm_conv::fix_mbr_disk_signature_issue(_disk->number(), _win_element.mbr_signature);
                    }
                }
            }
        }
    }
    LOG(LOG_LEVEL_RECORD, L"%s to convert the disk (%d).", result ? L"Succeeded" : L"Failed", _disk->number());
    return result;
}

bool irm_converter::extract_system_driver_file(os_image_info &os, std::wstring name){
    bool result = false;
    boost::filesystem::path drivers = boost::filesystem::path(_windows_volume / _win_element.windows_dir) / L"system32\\drivers";
    if (!(result = boost::filesystem::exists(drivers / name))){
        macho::windows::operating_system _os = os;
        if (_os.is_win2003() || _os.is_win2003r2() || _os.is_win2000() || _os.is_winxp()){
            boost::filesystem::path driver_cache = os.win_dir;
            if (_os.is_amd64())
                driver_cache /= "Driver Cache\\amd64";
            else if (_os.is_x86())
                driver_cache /= "Driver Cache\\i386";
            else if (_os.is_ia64())
                driver_cache /= "Driver Cache\\ia64";
            if (boost::filesystem::exists(driver_cache)){
                int i = 0;
                while (!result){
                    boost::filesystem::path cabfile = driver_cache;
                    if (i > 5){
                        break;
                    }
                    else if (0 == i){
                        cabfile /= "driver.cab";
                    }
                    else {
                        cabfile /= boost::str(boost::format("sp%d.cab") % i);
                    }
                    if (boost::filesystem::exists(cabfile)){
                        result = cabinet::uncompress::extract_file(cabfile, name, drivers);
                    }
                    i++;
                }
            }      
        }
        else{
            boost::filesystem::path repository = boost::filesystem::path(_windows_volume / _win_element.windows_dir) / L"system32\\DriverStore\\FileRepository";
            std::vector<boost::filesystem::path> files = macho::windows::environment::get_files(repository, name, true);
            if (files.size()){
                try{
                    boost::filesystem::copy_file(files[0], drivers / name);
                    result = true;
                }
                catch (const boost::filesystem::filesystem_error& ex){
                    LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_ansi_to_unicode(ex.what()).c_str());
                }
            }     
        }
    }
    return result;
}

bool irm_converter::get_bcd_edit(){
    _bcd = NULL;
    try{
        bcd_edit::ptr bcd = bcd_edit::ptr(new bcd_edit(_boot_cfg.wstring()));
        _bcd = bcd;
    }
    catch (wmi_exception &e){
        LOG(LOG_LEVEL_ERROR, L"%s", get_diagnostic_information(e).c_str());
    }
    catch (bcd_edit::exception &e){
        LOG(LOG_LEVEL_ERROR, L"%s", get_diagnostic_information(e).c_str());
    }
    catch (macho::exception_base &e){
        LOG(LOG_LEVEL_ERROR, L"%s", get_diagnostic_information(e).c_str());
    }
    catch (...){
        LOG(LOG_LEVEL_ERROR, L"Unknown");
    }
    if (!_bcd){
        _bcd = bcd_edit::ptr(new bcd_edit_cli(_boot_cfg.wstring()));
    }
    return (_bcd ? true : false);
}

windows_element::windows_element() : is_mbr_disk(false), is_boot_volume(false), mbr_signature(0), partition_guid(GUID_NULL), partition_offset(0){
    windows_dir = L"";
}

irm_conv_parameter::irm_conv_parameter() : is_sysvol_authoritative_restore(false), is_disable_machine_password_change(false), is_enable_debug(false), is_force_normal_boot(false), skip_system_injection(false), type(conversion::type::any_to_any), sectors_per_track(0), tracks_per_cylinder(0){
    drivers_path = L"";
    config = "";
    callback_timeout = 30;
}

irm_conv_parameter::irm_conv_parameter(const irm_conv_parameter& param){
    copy(param);
}

void irm_conv_parameter::copy(const irm_conv_parameter& param){
    drivers_path = param.drivers_path;
    config = param.config;
    post_script = param.post_script;
    services = param.services;
    is_enable_debug = param.is_enable_debug;
    is_disable_machine_password_change = param.is_disable_machine_password_change;
    is_force_normal_boot = param.is_force_normal_boot;
    sectors_per_track = param.sectors_per_track;
    tracks_per_cylinder = param.tracks_per_cylinder;
    is_sysvol_authoritative_restore = param.is_sysvol_authoritative_restore;
    skip_system_injection = param.skip_system_injection;
    devices = param.devices;
    type = param.type;
    callbacks = param.callbacks;
    callback_timeout = param.callback_timeout;
}

const irm_conv_parameter& irm_conv_parameter::operator =(const irm_conv_parameter& param){
    if (this != &param)
        copy(param);
    return(*this);
}

irm_converter::irm_converter() : _is_boot_ini_file(false), _is_pending_windows_update(false){}

irm_converter::~irm_converter(){

}

void	irm_converter::mount_volumes(){
    macho::windows::mutex m(L"mount");
    macho::windows::auto_lock lock(m);
    macho::windows::storage::volume::vtr volumes = _stg->get_volumes(_disk->number());
    foreach(macho::windows::storage::volume::ptr v, volumes){
        v->mount();
        if (v->drive_letter().empty()){
            std::wstring drive_letter = temp_drive_letter::find_next_available_drive();
            std::wstring volume_name = boost::str(boost::wformat(L"\\\\?\\Volume{%s}\\")% v->id());
            if (SetVolumeMountPoint(drive_letter.c_str(), volume_name.c_str())){
                LOG(LOG_LEVEL_RECORD, L"SetVolumeMountPoint(%s,%s).", drive_letter.c_str(), volume_name.c_str());
            }
            else{
                LOG(LOG_LEVEL_ERROR, L"Failed to SetVolumeMountPoint(%s,%s). 0x%08X", drive_letter.c_str(), volume_name.c_str(), GetLastError());
            }
        }
    }
}

void    irm_converter::dismount_volumes(){
    macho::windows::storage::volume::vtr volumes = _stg->get_volumes(_disk->number());
    foreach(macho::windows::storage::volume::ptr v, volumes){
        if (!v->drive_letter().empty()){
            std::wstring drive_letter = boost::str(boost::wformat(L"%s:\\")% v->drive_letter());
            if (DeleteVolumeMountPoint(drive_letter.c_str())){
                LOG(LOG_LEVEL_RECORD, L"DeleteVolumeMountPoint(%s).", drive_letter.c_str());
            }
            else{
                LOG(LOG_LEVEL_ERROR, L"Failed to DeleteVolumeMountPoint(%s). 0x%08X", drive_letter.c_str(), GetLastError());
            }
        }
    }
}