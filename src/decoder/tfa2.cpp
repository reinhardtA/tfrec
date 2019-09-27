#include <math.h>

#include "decoder/tfa2.h"

/*
 Protocol for IT+ Sensors 30.3143/30.3144 and 30.3155 and TX22

 FSK - modulation / NRZ
 30.3143,30.3144 (=TFA_2)
 Bitrate ~17240 bit/s -> @ 1.535MHz & 4x decimation -> 22.2 samples/bit
 Training sequence: 4* 1-0 toggles (8 bit)

 30.3155 (=TFA_3)
 Bitrate ~9600 bit/s -> @ 1.535MHz & 4x decimation -> 40 samples/bit
 Training sequence: 12* 1-0 toggles (24 bit)

 Bytes transfered with MSB first
 7 bytes total (inkl. sync, excl. training)

 Telegram format
 3  4 5  6
 0x2d 0xd4 II IT TT HH CC
 2d d4: Sync bytes
 III(11:8)=0x9 (at least for 3143/44/55)
 III(7:2)= ID(7:2) (displayed at startup, last 2 bits of ID always 0)
 III(1:0)= ? (3155=2)
 III(1)=  New battery, set to zero after some hours
 TTT: Temperature BCD in 1/10deg, offset +40deg
 HH: Humidity or sensor-index (binary)
 HH=6a -> internal temperature sensor (3143)
 HH=7d -> external temperature sensor (3143)
 HH(7) Lowbatt?
 CC: CRC8 from I to HH (polynome x^8 + x^5 + x^4  + 1)


 TX22 (documentation from FHEM/Jeelink TX22IT.cpp)
 8842 bit/s
 Telegram format
 0x2d 0xd4 SI IQ TV VV [TV VV] CC

 S = 0xa
 I(7:2): ID
 I(1): Learning bit
 I(0): Error
 Q(3): Low Bat
 Q(2:0): Count of data words
 T: Type, 0: temp (BCD/10+40 deg C), 1: humidity (BCD %rH),
 2: rain (12bit cnt), 3: wind (4bit * 22.5deg, 8bit speed 0.1*m/s), 4: gust (12bit 0.1*m/s)

 */
//-------------------------------------------------------------------------
tfa2_decoder::tfa2_decoder(sensor_e _type)
   :
   decoder(_type)
{
   invert = 0;
   sr = 0;
   sr_cnt = -1;
   byte_cnt = 0;
   snum = 0;
   bad = 0;
   crc = new crc8(0x31); // x^8 +   x^5 + x^4  + 1
}
tfa2_decoder::~tfa2_decoder()
{
}
//-------------------------------------------------------------------------
void tfa2_decoder::flush(int rssi, int offset)
{
   if (type == TX22)
      flush_tx22(rssi, offset);
   else
      flush_tfa(rssi, offset);
}
//-------------------------------------------------------------------------
void tfa2_decoder::flush_tx22(int rssi, int offset)
{
   uint8_t crc_val = 0;
   uint8_t crc_calc = 0;

   if (byte_cnt >= 7 && byte_cnt < 64)
   {
      if (dbg)
      {
         printf("#%03i %u  ", snum++, (uint32_t) time(0));
         for (int n = 0; n < byte_cnt; n++)
            printf("%02x ", rdata[n]);
         printf("      ");
      }
      if ((rdata[2] >> 4) != 0xa)
         goto bad;
      int id = ((rdata[2] & 0xf) << 2) | (rdata[3] >> 6);
      int error = !((rdata[3] >> 4) & 1);
      int lowbat = (rdata[3] >> 3) & 1;
      int num = rdata[3] & 7;

      crc_val = rdata[2 * num + 4];
      crc_calc = crc->calc(&rdata[2], 2 + 2 * num);
      if (crc_val != crc_calc)
         goto bad;
      if (num > 8)
         goto bad;

      int have_temp = 0, have_hum = 0, have_rain = 0, have_wind = 0, have_gust = 0;
      double temp = 0, hum = 0, rain = 0, wdir = 0, wspeed = 0, wgust = 0;

      for (int n = 0; n < num; n++)
      {
         int t = rdata[4 + n * 2] >> 4;

         switch (t)
         {
         case 0:
            { // Temp
            double v = (rdata[4 + n * 2] & 0xf) * 100 +
               (rdata[4 + n * 2 + 1] >> 4) * 10 +
               (rdata[4 + n * 2 + 1] & 0xf);
            temp = (v / 10) - 40;
            have_temp = 1;
         }
            break;
         case 1:
            { // Hum
            int v = (rdata[4 + n * 2] & 0xf) * 100 +
               (rdata[4 + n * 2 + 1] >> 4) * 10 +
               (rdata[4 + n * 2 + 1] & 0xf);
            hum = v;
            have_hum = 1;
         }
            break;
         case 2:
            { // rain counter
            int v = ((rdata[4 + n * 2] & 0xf) << 8) +
               rdata[4 + n * 2 + 1];
            rain = v;
            have_rain = 1;
         }
            break;
         case 3:
            { // wind dir/speed
            double d = (rdata[4 + n * 2] & 0xf) * 22.5;
            double s = rdata[4 + n * 2 + 1] / 10.0;
            wdir = d;
            wspeed = s;
            have_wind = 1;
         }
            break;
         case 4:
            { // gust
            double s = (((rdata[4 + n * 2] & 0xf) << 8) +
               rdata[4 + n * 2 + 1]) / 10.0;
            wgust = s;
            have_gust = 1;
         }
            break;
         default:
            break;
         }
      }
      sensordata_t sd;
      sd.type = type;
      sd.temp = 0;
      sd.humidity = 0;
      sd.sequence = 0;
      sd.alarm = error | lowbat;
      sd.rssi = rssi;
      sd.flags = 0;
      sd.ts = time(0);

      int new_id = (type << 28) | (id << 4);
      if (dbg >= 0)
      {
         printf("TX22 ID %x, ", new_id);
         if (have_temp)
            printf("temp %g, ", temp);
         if (have_hum)
            printf("hum %g, ", hum);
         if (have_rain)
            printf("rain %g, ", rain);
         if (have_wind)
            printf("speed %g, dir %g, ", wspeed, wdir);
         if (have_gust)
            printf("gust %g, ", wgust);
         printf("RSSI %i, offset %.0lfkHz\n", rssi, -1536.0 * offset / 131072);
         fflush (stdout);
      }
      if (have_temp)
      {
         sd.id = new_id;
         sd.temp = temp;
         sd.humidity = hum;
         store_data(sd);
      }
      if (have_rain)
      {
         sd.id = new_id | 2;
         sd.temp = rain;
         sd.humidity = 0;
         store_data(sd);
      }
      if (have_wind)
      {
         sd.id = new_id | 3;
         sd.temp = wspeed;
         sd.humidity = wdir;
         store_data(sd);
      }
      if (have_gust)
      {
         sd.id = new_id | 4;
         sd.temp = wgust;
         sd.humidity = 0;
         store_data(sd);
      }

      sr_cnt = -1;
      sr = 0;
      byte_cnt = 0;
      return;
   }
   bad :

   if (dbg && byte_cnt >= 7 && byte_cnt < 64)
   {
      bad++;
      if (crc_val != crc_calc)
         printf("TX22(%02x) BAD %i RSSI %i  Offset %.0lfkHz (CRC %02x %02x) len %i\n", 1 << type, bad, rssi,
            -1536.0 * offset / 131072,
            crc_val, crc_calc, byte_cnt);
      else
         printf("TX22(%02x) BAD %i RSSI %i  Offset %.0lfkHz len %i (SANITY)\n", 1 << type, bad, rssi, -1536.0 * offset
            / 131072, byte_cnt);
      fflush (stdout);
   }
   sr_cnt = -1;
   sr = 0;
   byte_cnt = 0;
}
//-------------------------------------------------------------------------
void tfa2_decoder::flush_tfa(int rssi, int offset)
{
   //printf(" CNT %i\n",byte_cnt);
   if (byte_cnt >= 7)
   {
      if (dbg)
      {
         printf("#%03i %u  ", snum++, (uint32_t) time(0));
         for (int n = 0; n < 7; n++)
            printf("%02x ", rdata[n]);
         printf("                      ");
      }

      int id = (type << 28) | (rdata[2] << 8) | (rdata[3] & 0xc0);
      double temp = ((rdata[3] & 0xf) * 100 + (rdata[4] >> 4) * 10 + (rdata[4] & 0xf));
      temp = temp / 10 - 40;
      int hum = rdata[5];
      uint8_t crc_val = rdata[6];
      uint8_t crc_calc = crc->calc(&rdata[2], 4);

      int sub_id = 0;
      if (hum == 0x7d)
         sub_id = 1;
      id |= sub_id;

      if (crc_val == crc_calc
         )
      {
         if (hum > 100)
            hum = 0;
         if (dbg >= 0)
         {
            printf("TFA%i ID %06x %+.1lf %i%% RSSI %i Offset %.0lfkHz\n",
               type + 1, id, temp, hum, rssi,
               -1536.0 * offset / 131072);
            fflush (stdout);
         }
         sensordata_t sd;
         sd.type = type;
         sd.id = id;
         sd.temp = temp;
         sd.humidity = hum;
         sd.sequence = 0;
         sd.alarm = 0;
         sd.rssi = rssi;
         sd.flags = 0;
         sd.ts = time(0);
         store_data(sd);
      }
      else
      {
         bad++;
         if (dbg)
         {
            if (crc_val != crc_calc)
               printf("TFA%i BAD %i RSSI %i  Offset %.0lfkHz (CRC %02x %02x)\n", type + 1, bad, rssi,
                  -1536.0 * offset / 131072,
                  crc_val, crc_calc);
            else
               printf("TFA%i BAD %i RSSI %i  Offset %.0lfkHz (SANITY)\n", type + 1, bad, rssi, -1536.0 * offset
                  / 131072);
         }
      }
   }
   sr_cnt = -1;
   sr = 0;
   byte_cnt = 0;
}
//-------------------------------------------------------------------------
void tfa2_decoder::store_bit(int bit)
{
   //	printf("%i %04x\n",bit,sr&0xffff);
   sr = (sr << 1) | (bit);
   if ((sr & 0xffff) == 0x2dd4)
   { // Sync start
     //printf("SYNC\n");
      sr_cnt = 0;
      rdata[0] = (sr >> 8) & 0xff;
      byte_cnt = 1;
      invert = 0;
   }

   // Tolerate inverted sync (maybe useful later...)
   if (((~sr) & 0xffff) == 0x2dd4)
   {
      printf("Inverted SYNC\n");
      sr_cnt = 0;
      rdata[0] = ~((sr >> 8) & 0xff);
      byte_cnt = 1;
      invert = 1;
   }

   if (sr_cnt == 0)
   {
      if (byte_cnt < (int) sizeof(rdata))
      {
         if (invert)
            rdata[byte_cnt] = ~(sr & 0xff);
         else
            rdata[byte_cnt] = sr & 0xff;
      }
      //printf(" %i %02x\n",byte_cnt,rdata[byte_cnt]);
      byte_cnt++;
   }
   if (sr_cnt >= 0)
      sr_cnt = (sr_cnt + 1) & 7;
}
