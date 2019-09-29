#ifndef _INCLUDE_DEMODULATOR_H
#define _INCLUDE_DEMODULATOR_H

#include <cstdint>

class decoder;

class demodulator
{
public:
   /**
    * creates a demodulator with a decoder
    */
   demodulator(decoder *_dec);

   /**
    * simple destructor
    */
   virtual ~demodulator();

   /**
    * demodulate a signal, needs to be implemented by every demodulator itself
    */
   virtual int demod(int thresh, int pwr, int index, int16_t *iq) = 0;

   /**
    * TODO : write a comment
    */
   virtual void start(int len) final;

   /**
    * reset a demodulator, needs to be implemented by every demodulator itself
    */
   virtual void reset() = 0;

   // TODO : make me private or protected ... main function needs to access
   // TODO : think about "NULL" checks
   /**
    * a pointer to a decoder
    */
   decoder *m_ptrDecoder;

protected:

   // index of the last bit, used
   int m_IdxLastBit;

};

#endif
