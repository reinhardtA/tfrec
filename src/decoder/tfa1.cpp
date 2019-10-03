#include <math.h>

#include <string>

#include "decoder/tfa1.h"
#include "utils/dsp_stuff.h"

// Protocol for IT+ Sensors 30.3180.IT, 30.3181.IT, 30.3199
// "KlimaLogg Pro"
//
// FSK - modulation
// NRZS (*not* NRZ!)
// 0: toggle frequency  (deviation > some limit)
// 1: do not toggle frequency
// Bitrate ~38400bit/s -> @ 1.535MHz -> 40 samples/bit, @ 4x decimation -> 10 samples/bit
// Training sequence >150 '0'-edges
// Bytes transfered with LSB first
// Ten bytes total (inkl. sync)
//
// Telegram format
// 0x2d 0xd4 ID ID sT TT HH BB SS 0x56 CC
// - 2d d4: Sync bytes
// - ID(14:0): 15 bit ID of sensor (printed on the back and displayed after powerup)
// - ID(15) is either 1 or 0 (fixed, depends on the sensor)
// - s(3:0): Learning sequence 0...f, after learning fixed 8
// - TTT: Temperatur in BCD in .1degC steps, offset +40degC (-> -40...+60)
// - HH(6:0): rel. Humidity in % (binary coded, no BCD!)
// - BB(7): Low battery if =1
// - BB(6:4): 110 or 111 (for 3199)
// - SS(7:4): sequence number (0...f)
// - SS(3:0): 0000 (fixed)
// - 56: Type?
// - CC: CRC8 from ID to 0x56 (polynome x^8 + x^5 + x^4  + 1)

//-------------------------------------------------------------------------
tfa1_decoder::tfa1_decoder(sensor_e _type)
   :
   decoder(_type)
{
   sr = 0;
   sr_cnt = -1;
   byte_cnt = 0;
   snum = 0;
   bad = 0;

   // CRC Polynom
   m_ptrCrcPolynom = new crc8(0x31); // x^8 +   x^5 + x^4  + 1
}
tfa1_decoder::~tfa1_decoder()
{
}
//-------------------------------------------------------------------------
void tfa1_decoder::flush(int rssi, int offset)
{
   // if we have some data, at least the 10 bytes
   if (byte_cnt >= 10)
   {
      // get the time
      time_t tCurrTime = time(0);
      // print the time in a human readable format and Unix Timestamp
      printf("%s (%u) ", convertTime(tCurrTime).c_str(), (uint32_t) tCurrTime);

      if (dbg)
      {
         printf("#%03i ", snum++);
         for (int n = 0; n < 11; n++)
         {
            printf("%02x ", rdata[n]);
         }
         printf("-> ");
      }
      // ID
      int id = ((rdata[2] << 8) | rdata[3]) & 0x7fff;
      // Battery Status
      int batfail = (rdata[7] & 0x80) >> 7;
      // Temperature
      double temp = ((rdata[4] & 0xf) * 100) + ((rdata[5] >> 4) * 10) + (rdata[5] & 0xf);
      temp = (temp / 10) - 40;
      // Humidity
      int hum = rdata[6];
      // Sequence Number
      int seq = rdata[8] >> 4;
      // CRC in Datagramm
      uint8_t crc_val = rdata[10];
      // calculated CRC
      uint8_t crc_calc = m_ptrCrcPolynom->calc(&rdata[2], 8);

      // CRC and sanity checks
      if ((crc_val == crc_calc)                 // CRC Check
      && ((rdata[4] & 0xf0) == 0x80            // ignore all learning messages except sensor fails
      || hum == 0x7f || hum == 0x6a)
         && (hum <= 0x7f)                       // catch sensor fail due to lowbat
         && ((rdata[7] & 0x60) == 0x60)         // 0x60 or 0x70
         && ((rdata[8] & 0xf) == 0)
         && (rdata[9] == 0x56)                  // Type?
         )
      {
         // .3181/.3199 has only temperature, humidity is 0x6a
         if (hum == 0x6a)
         {
            hum = 0;
         }

         // Sensor values may fail at 0xaaa/0xfff or 0x7f due to low battery
         // even without lowbat bit
         // Set it to 2 -> sensor data not valid
         if ((rdata[5] == 0xff) || (rdata[5] == 0xaa) || (hum == 0x7f))
         {
            batfail = 2;
            hum = 0;
            temp = 0;
         }

         // create the sensor data
         sensordata_t sd;
         sd.type = TFA_1;
         sd.id = id;
         sd.temp = temp;
         sd.humidity = hum;
         sd.sequence = seq;
         sd.alarm = batfail;
         sd.rssi = rssi;
         sd.flags = 0;
         sd.ts = tCurrTime;

         // store the data
         store_data(sd);

         printf("TFA1 ID %04x %+.1f %i%% seq %x lowbat %i RSSI %i\n"
            , id
            , sd.temp
            , hum
            , sd.sequence
            , sd.alarm
            , sd.rssi
            );

         fflush (stdout);
      }
      else
      {
         bad++;

         if (crc_val != crc_calc)
         {
            printf("TFA1 BAD %i RSSI %i (CRC %02x %02x)\n", bad, rssi, crc_val, crc_calc);
         }
         else
         {
            if (crc_val != crc_calc)
            {
               printf("TFA1 BAD %i RSSI %i (CRC %02x %02x)\n", bad, rssi, crc_val, crc_calc);
            }
            else
            {
               printf("TFA1 BAD %i RSSI %i (SANITY)\n", bad, rssi);
            }
            fflush (stdout);
         }

         fflush (stdout);
      }
   }

   // reset some values
   sr_cnt = -1;
   byte_cnt = 0;
   rdata[10] = 0x00; // clear crc
}
//-------------------------------------------------------------------------
void tfa1_decoder::store_bit(int bit)
{

   sr = (sr >> 1) | (bit << 31);

   // Check for Sync Bytes
   if ((sr & 0xffff) == 0xd42d)
   {
      // Sync start
      sr_cnt = 0;
      byte_cnt = 0;
   }

   // This will be triggered if the sync sequence was hit !!
   // remember that the sr_cnt will be reset to -1 after each flush !!
   if (sr_cnt == 0)
   {
      if (byte_cnt < (int) sizeof(rdata))
      {
         rdata[byte_cnt] = sr & 0xff;
      }
      byte_cnt++;
   }
   if (sr_cnt >= 0)
   {
      sr_cnt = (sr_cnt + 1) & 7;
   }
}
//-------------------------------------------------------------------------
