;#ifndef __EVTMSG_H__
;#define __EVTMSG_H__
;
;//
;//  Status values are 32 bit values layed out as follows:
;//
;//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
;//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
;//  +---+-+-------------------------+-------------------------------+
;//  |Sev|C|       Facility          |               Code            |
;//  +---+-+-------------------------+-------------------------------+
;//
;//  where
;//
;//      Sev - is the severity code
;//
;//          00 - Success
;//          01 - Informational
;//          10 - Warning
;//          11 - Error
;//
;//      C - is the Customer code flag
;//
;//      Facility - is the facility code
;//
;//      Code - is the facility's status code
;//
;
MessageIdTypedef=NTSTATUS

SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
              )

FacilityNames=(System=0x0
               RpcRuntime=0x2:FACILITY_RPC_RUNTIME
               RpcStubs=0x3:FACILITY_RPC_STUBS
               Io=0x4:FACILITY_IO_ERROR_CODE
               Serial=0x6:FACILITY_SERIAL_ERROR_CODE
              )

LanguageNames =
    (
        English = 0x0409:EvtMsg_ENU
    )


MessageId       = 10000
SymbolicName    = VCBT_DRIVER_LOAD_SUCCESS
Severity		= Informational
Language        = English
Driver (%1) loaded successfully.
.

MessageId       = +1
SymbolicName    = VCBT_JOURNAL_ENABLED
Severity		= Informational
Language        = English
Volume Change Block Tracking is enabled on %2.
.

MessageId       = +1
SymbolicName    = VCBT_JOURNAL_DISABLED
Severity		= Informational
Language        = English
Volume Change Block Tracking is disabled on %2.
.

MessageId       = +1
SymbolicName    = VCBT_FAILED_TO_WRITE_JOURNAL
Severity		= Error
Language        = English
%2 failed to write journal data. (LCN: %3, length: %4).
.

MessageId       = +1
SymbolicName    = VCBT_FAILED_TO_READ_JOURNAL
Severity		= Error
Language        = English
%2 failed to read journal data. (LCN: %3, length: %4).
.

MessageId       = +1
SymbolicName    = VCBT_FAILED_TO_FLUSH_JOURNAL_META_DATA
Severity		= Error
Language        = English
%2 failed to flush journal meta data. (LCN: %3, length: %4).
.

MessageId       = +1
SymbolicName    = VCBT_FAILED_TO_READ_JOURNAL_META
Severity		= Error
Language        = English
%2 failed to read journal meta data. (LCN: %3, length: %4).
.

MessageId       = +1
SymbolicName    = VCBT_FAILED_TO_FLUSH_UMAP
Severity		= Error
Language        = English
%2 failed to flush umap data. (LCN: %3, length: %4).
.

MessageId       = +1
SymbolicName    = VCBT_FAILED_TO_READ_UMAP
Severity		= Error
Language        = English
%2 failed to read umap data. (LCN: %3, length: %4).
.

MessageId       = +1
SymbolicName    = VCBT_BAD_JOURNAL_DATA
Severity		= Error
Language        = English
The journal data is corrupted on %2 .
.

;
;#endif  //__EVTMSG_H__
;