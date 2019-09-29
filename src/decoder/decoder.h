#ifndef _INCLUDE_DECODER_H
#define _INCLUDE_DECODER_H

#include <sys/time.h>
#include <string>
#include <map>

enum sensor_e
{
   TFA_1 = 0, // IT+ Klimalogg Pro, 30.3180, 30.3181, 30.3199(?)
   TFA_2,   // IT+ 30.3143, 30.3146(?), 30.3144
   TFA_3,   // 30.3155
   TX22,    // LaCrosse TX22
   TFA_WHP,   // 30.3306 (rain meter), pulse data
   TFA_WHB,   // TFA WeatherHub
   FIREANGEL = 0x20 // ST-630+W2
};

typedef struct
{
   sensor_e type;
   uint64_t id;
   double temp;
   double humidity;
   int alarm;
   int flags;
   int sequence;
   time_t ts;
   int rssi;
} sensordata_t;

class decoder
{
public:

   /**
    * creates a decoder for a given sensor type
    */
   decoder(sensor_e _type);
   /**
    * simple destructor
    */
   virtual ~decoder();

   virtual int count(void)
   {
      return data.size();
   }

   /**
    * executes a given m_ptrExecuteHandler on a sensor data set
    */
   virtual void execute_handler(sensordata_t &d) final;

   inline sensor_e get_type(void)
   {
      return m_SendorType;
   }
   virtual int has_sync(void)
   {
      return synced;
   }

   virtual void flush(int rssi, int offset = 0);
   virtual void flush_storage(void);
   virtual void set_params(char *_handler, int _mode, int _dbg);
   virtual void store_bit(int bit);
   virtual void store_bytes(uint8_t *d, int len);
   virtual void store_data(sensordata_t &d);

protected:

   // the decoders sensor type
   sensor_e m_SendorType;

   int dbg;
   int bad;
   int synced;

   uint8_t rdata[256];
   int byte_cnt;

private:

   // the exe handler
   char *m_ptrExecuteHandler;

   int mode;
   std::map<uint64_t, sensordata_t> data;

};

#endif
