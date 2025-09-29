#pragma once

#include "CommonHelix.h"
#include "mp3dec.h"
#include "mp3common.h"
#include <esp_log.h>
#define TAG "MP3"

#define MP3_MAX_OUTPUT_SIZE 1024 * 2
//#define MP3_MAX_FRAME_SIZE 1600 
#define MP3_MAX_FRAME_SIZE 10000 

namespace libhelix {


typedef void (*MP3InfoCallback)(MP3FrameInfo &info);
typedef void (*MP3DataCallback)(MP3FrameInfo &info,short *pwm_buffer, size_t len);

enum MP3Type {MP3Normal=0, MP3SelfContaind=1};


/**
 * @brief A simple Arduino API for the libhelix MP3 decoder. The data is provided with the help of write() calls.
 * The decoded result is available either via a callback method or via an output stream.
 * @author Phil Schatzmann
 * @copyright GPLv3
 */
class MP3DecoderHelix : public CommonHelix {

    public:
        MP3DecoderHelix(){
            decoder = MP3InitDecoder();
            this->mp3_type = MP3Normal;
        }
        MP3DecoderHelix(MP3DataCallback dataCallback, MP3Type mp3Type=MP3Normal){
            decoder = MP3InitDecoder();
            this->pwmCallback = dataCallback;
            this->mp3_type = mp3Type;
        }

        MP3DecoderHelix(MP3Type mp3Type){
            decoder = MP3InitDecoder();
            this->mp3_type = mp3Type;
        }

        ~MP3DecoderHelix(){
            MP3FreeDecoder(decoder);
        }


        void setInfoCallback(MP3InfoCallback cb){
            this->infoCallback = cb;
        }

        void setDataCallback(MP3DataCallback cb){
            this->pwmCallback = cb;
        }

        /// Provides the last available MP3FrameInfo
        MP3FrameInfo audioInfo(){
            return mp3FrameInfo;
        }


    protected:
        HMP3Decoder decoder = nullptr;
        MP3DataCallback pwmCallback = nullptr;
        MP3InfoCallback infoCallback = nullptr;
        MP3Type mp3_type;
        MP3FrameInfo mp3FrameInfo;

        /// determines the frame buffer size that will be allocated
        size_t maxFrameSize(){
            return max_frame_size == 0 ? MP3_MAX_FRAME_SIZE : max_frame_size;
        }

        /// Determines the pwm buffer size that will be allocated
        size_t maxPWMSize() {
            return max_pwm_size == 0 ? MP3_MAX_OUTPUT_SIZE : max_pwm_size;
        }

        /// Finds the synch word in the available buffer data starting from the indicated offset
        int findSynchWord(int offset=0) {
            int result = MP3FindSyncWord(frame_buffer+offset, buffer_size);
            return result < 0 ? result : result + offset;
        }

        /// decodess the data 
        void decode(Range r) {
            
            int len = buffer_size - r.start; //buffer_size immer 10000, r.start immer 0, len immer 10000
            int bytesLeft =  len; //bytes_left=10000
            uint8_t* ptr = frame_buffer + r.start; //prt immer am Anfang

            int result = MP3Decode(decoder, &ptr, &bytesLeft, pwm_buffer, mp3_type);            
            int decoded = len - bytesLeft;
            
            if (result==0){
                ESP_LOGD(TAG, "-> bytesLeft %d -> %d  = %d ", buffer_size, bytesLeft, decoded);
                ESP_LOGD(TAG, "-> End of frame (%d) vs end of decoding (%d)", r.end, decoded);

                // return the decoded result
                MP3FrameInfo info;
                MP3GetLastFrameInfo(decoder, &info);
                provideResult(info);
                ESP_LOGD(TAG, "decoded=%d end=%d bytesLeft=%d result=%d outputSamps=%d", decoded, r.end, bytesLeft, result, info.outputSamps);
                // remove processed data from buffer 
                buffer_size -= decoded;
                assert(buffer_size<=maxFrameSize());

                memmove(frame_buffer, frame_buffer+r.start+decoded, buffer_size);
                ESP_LOGD(TAG, " -> decoded %d bytes - remaining buffer_size: %d", decoded, buffer_size);
            } else {
                // decoding error
                ESP_LOGD(TAG, " -> decode error: %d - removing frame!", result);
                int ignore = decoded;
                if (ignore == 0) ignore = r.end;
                // We advance to the next synch world
                buffer_size -= ignore;
                assert(buffer_size<=maxFrameSize());

                memmove(frame_buffer, frame_buffer+ignore, buffer_size);
            }
        }

        // return the resulting PWM data
        void provideResult(MP3FrameInfo &info){
            ESP_LOGD(TAG, "=> provideResult: %d", info.outputSamps);
            if (info.outputSamps>0){
                // provide result
                if(pwmCallback!=nullptr){
                    // output via callback
                    pwmCallback(info, pwm_buffer, info.outputSamps);
                } else {
                    // output to stream
                    if (info.samprate!=mp3FrameInfo.samprate  && infoCallback!=nullptr){
                        infoCallback(info);
                    }
#ifdef ARDUINO
                    int sampleSize = info.bitsPerSample / 8;
                    out->write((uint8_t*)pwm_buffer, info.outputSamps*sampleSize);
#endif
                }
                mp3FrameInfo = info;
            }
        }

};

}

#undef TAG