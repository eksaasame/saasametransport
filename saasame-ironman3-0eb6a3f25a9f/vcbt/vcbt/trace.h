/*++

Module Name:

    Trace.h

Abstract:

    Header file for the debug tracing related function defintions and macros.

Environment:

    Kernel mode

--*/

//
// Define the tracing flags.
//
// Tracing GUID - {9F7FCC2A-305B-4089-AFB0-2E4BF0847EB1}
//
#pragma once

#ifndef __EZSYNC_TRACE_H__
#define __EZSYNC_TRACE_H__

#define WPP_CONTROL_GUIDS                                              \
    WPP_DEFINE_CONTROL_GUID(                                           \
        ezSyncGuid, (9F7FCC2A,305B,4089,AFB0,2E4BF0847EB1),			   \
                                                                       \
        WPP_DEFINE_BIT(TRACE_FLAG_ALL_INFO)                            \
        WPP_DEFINE_BIT(TRACE_FLAG_DRIVER)                              \
        WPP_DEFINE_BIT(TRACE_FLAG_DEVICE)                              \
        WPP_DEFINE_BIT(TRACE_FLAG_QUEUE)                               \
        )                             

#define WPP_FLAG_LEVEL_LOGGER(flag, level)                                  \
    WPP_LEVEL_LOGGER(flag)

#define WPP_FLAG_LEVEL_ENABLED(flag, level)                                 \
    (WPP_LEVEL_ENABLED(flag) &&                                             \
     WPP_CONTROL(WPP_BIT_ ## flag).Level >= level)

#define WPP_LEVEL_FLAGS_LOGGER(lvl,flags) \
           WPP_LEVEL_LOGGER(flags)
               
#define WPP_LEVEL_FLAGS_ENABLED(lvl, flags) \
           (WPP_LEVEL_ENABLED(flags) && WPP_CONTROL(WPP_BIT_ ## flags).Level >= lvl)

//
// This comment block is scanned by the trace preprocessor to define our
// Trace function.
//
// begin_wpp config
// FUNC DoTraceMsg{FLAG=TRACE_FLAG_ALL_INFO}(LEVEL, MSG, ...);
// FUNC DoTraceLevelMsg(LEVEL, FLAGS, MSG, ...);
// end_wpp
//
// Enable Level 
// Value	                Meaning
// TRACE_LEVEL_CRITICAL     1        Abnormal exit or termination events
// TRACE_LEVEL_ERROR        2        Severe error events
// TRACE_LEVEL_WARNING      3        Warning events such as allocation failures
// TRACE_LEVEL_INFORMATION  4        Non - error events such as entry or exit events
// TRACE_LEVEL_VERBOSE      5        Detailed trace events
// TRACE_LEVEL_VERBOSE_6    6        Detailed trace events
// TRACE_LEVEL_VERBOSE_7    7        Detailed trace events
// TRACE_LEVEL_VERBOSE_8    8        Detailed trace events
// TRACE_LEVEL_VERBOSE_9    9        Detailed trace events
// TRACE_LEVEL_VERBOSE_10   10        Detailed trace events
// TRACE_LEVEL_VERBOSE_11   11        Detailed trace events
// TRACE_LEVEL_VERBOSE_12   12        Detailed trace events
// TRACE_LEVEL_VERBOSE_13   13        Detailed trace events
// TRACE_LEVEL_VERBOSE_14   14        Detailed trace events
// TRACE_LEVEL_VERBOSE_15   15        Detailed trace events
// TRACE_LEVEL_VERBOSE_16   16        Detailed trace events
//

#endif