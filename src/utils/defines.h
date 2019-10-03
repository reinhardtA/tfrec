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

/**
 * Threashold Modes for the demodulatores
 */
enum class eThresholMode
{
   eAuto = 0,
   eUser = 1,
   eUnknown
};

// used in engine class for reading i/q data from file
constexpr auto RLS = 65536;

// the sample rate (1.536 MHz)
constexpr auto SDR_SAMPLE_RATE = 1536000; // in Hz

// Used in TFA1 Demodulator
constexpr int TFA1BITRATE = 38400; // in bit per second
constexpr int BITPERIOD = ((SDR_SAMPLE_RATE / TFA1BITRATE) / 4);

#endif
