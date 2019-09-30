/*
 tfrec - Receiver for TFA IT+ (and compatible) sensors
 (c) 2017 Georg Acher, Deti Fliegl {acher|fliegl}(at)baycom.de

 #include <GPL-v2>
 */

#ifndef _UTILS_DEFINES_H
#define _UTILS_DEFINES_H

// a place to collect all defines and enums

/**
 * defines gain mode
 */
enum class eGainMode
{
   eAuto = 0,
   eUser = 1,
   eUnknown
};

/**
 * defines Debug Mode
 */
enum class eDebugMode
{
   eError = 0,
   eWarn = 1,
   eInfo = 2,
   eDebug = 3,
   eTraces = 4,
   eUnknown
};

/**
 * defines the Dump Mode
 */
enum class eDumpMode
{
   eLoad = 0,
   eStore = 1,
   eNone = 3,
   eUnknown
};

#endif
