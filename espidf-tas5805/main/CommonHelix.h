#pragma once


#include <stdint.h>
#include <algorithm>



#define SYNCH_WORD_LEN 4
#define TAG "COMMON"


namespace libhelix {

/**
 * @brief Range with a start and an end
 * @author Phil Schatzmann
 * @copyright GPLv3
 * 
 */
struct Range {
    int start;
    int end;
};

/**
 * @brief Common Simple Arduino API 
 * @author Phil Schatzmann
 * @copyright GPLv3
 * 
 */
class CommonHelix   {
    public:

        virtual ~CommonHelix(){
            if (active){
                end();
            }
            if (pwm_buffer!=nullptr){
                delete[] pwm_buffer;
            }
            if (frame_buffer!=nullptr){
                delete[] frame_buffer;
            }
        }

#ifdef ARDUINO
        void setOutput(Print &output){
            this->out = &output;
        }
#endif

        /**
         * @brief Starts the processing
         * 
         */
        virtual void begin(){
            buffer_size = 0;
            frame_counter = 0;

            if (active){
                end();
            }
            if (frame_buffer == nullptr) {
                ESP_LOGI(TAG,"allocating frame_buffer with %zu bytes", maxFrameSize());
                frame_buffer = new uint8_t[maxFrameSize()];
            }
            if (pwm_buffer == nullptr) {
                ESP_LOGI(TAG,"allocating pwm_buffer with %zu bytes", maxPWMSize());
                pwm_buffer = new short[maxPWMSize()];
            }
            if (pwm_buffer==nullptr || frame_buffer==nullptr){
                ESP_LOGE(TAG,"Not enough memory for buffers");
                active = false;
                return;
            }
            memset(frame_buffer,0, maxFrameSize());
            memset(pwm_buffer,0, maxPWMSize());
            active = true;
        }

        /// Releases the reserved memory
        virtual void end(){
            active = false;
        }

        /**
         * @brief decodes the next segments from the intput. 
         * The data can be provided in one shot or in small incremental pieces.
         * It is suggested to be called in the Arduino Loop. If the provided data does
         * not fit into the buffer it is split up into small pieces that fit
         */
        
        virtual size_t write(const void *in_ptr, size_t in_size) {
            
            size_t start = 0;
            if(!active){
                ESP_LOGW(TAG,"CommonHelix not active");
                return start;
            }
            uint8_t* ptr8 = (uint8_t* )in_ptr;
            size_t write_len = std::min(in_size, static_cast<size_t>(maxFrameSize()-buffer_size));
            ESP_LOGD(TAG,"write in_ptr:%p size:%zu write_len:%zu", in_ptr, in_size, write_len);
            while(start<in_size){
                // we have some space left in the buffer
                int written_len = writeFrame(ptr8+start, write_len);
                start += written_len;
                ESP_LOGD(TAG,"-> Written %zu of %zu - Counter %zu", start, in_size, frame_counter);
                write_len = std::min(in_size - start, static_cast<size_t>(maxFrameSize()-buffer_size));
            }
            return start;
        }

        /// returns true if active
        operator bool() {
            return active;
        }       

    protected:
        bool active = false;
        uint32_t buffer_size = 0; // actually filled sized
        uint8_t *frame_buffer = nullptr;
        short *pwm_buffer = nullptr;
        size_t max_frame_size = 0;
        size_t max_pwm_size = 0;
        size_t frame_counter = 0;

   
        /// Provides the maximum frame size - this is allocated on the heap and you can reduce the heap size my minimizing this value
        virtual size_t maxFrameSize() = 0;

        /// Define your optimized maximum frame size
        void setMaxFrameSize(size_t len){
            max_frame_size = len;
        }

        /// Provides the maximum pwm buffer size - this is allocated on the heap and you can reduce the heap size my minimizing this value
        virtual size_t maxPWMSize() = 0 ;

        /// Define your optimized maximum pwm buffer size
        void setMaxPWMSize(size_t len) {
            max_pwm_size = len;
        }

        /// Finds the synchronization word in the frame buffer (starting from the indicated offset)
        virtual int findSynchWord(int offset=0) = 0;   

        /// Decodes a frame
        virtual void decode(Range r) = 0;   

        /// we add the data to the buffer until it is full
        size_t appendToBuffer(const void *in_ptr, int in_size){
            ESP_LOGD(TAG,"appendToBuffer: %d (at %p)", in_size, frame_buffer);
            int buffer_size_old = buffer_size;
            int process_size = std::min((int)(maxFrameSize() - buffer_size), in_size);
            memmove(frame_buffer+buffer_size, in_ptr, process_size); 
            buffer_size += process_size;
            if (buffer_size>maxFrameSize()){
                ESP_LOGE(TAG,"Increase MAX_FRAME_SIZE > %zu", buffer_size);
            }
            assert(buffer_size<=maxFrameSize());

            ESP_LOGD(TAG,"appendToBuffer %d + %d  -> %u", buffer_size_old,  process_size, buffer_size );
            return process_size;
        }

        /// appends the data to the frame buffer and decodes 
        size_t writeFrame(const void *in_ptr, size_t in_size){
            ESP_LOGD(TAG,"writeFrame %zu", in_size);
            size_t result = 0;
            // in the beginning we ingnore all data until we found the first synch word
            result = appendToBuffer(in_ptr, in_size);
            Range r = synchronizeFrame();
            // Decode if we have a valid start and end synch word
            if(r.start>=0 && r.end>r.start){
                decode(r);
            } 
            frame_counter++;
            return result;
        }

        /// returns true if we have a valid start and end synch word.
        Range synchronizeFrame() {
            ESP_LOGD(TAG,"synchronizeFrame");
            Range range;
            range.start = findSynchWord(0);
            range.end = findSynchWord(range.start+SYNCH_WORD_LEN);
            ESP_LOGD(TAG,"-> frameRange -> %d - %d", range.start, range.end);
            if (range.start<0){
                // there is no Synch in the buffer at all -> we can ignore all data
                range.end = -1;
                ESP_LOGD(TAG,"-> no synch");
                if (buffer_size==maxFrameSize()) {
                    buffer_size = 0;
                    ESP_LOGD(TAG,"-> buffer cleared");
                }
            } else if (range.start>0) {
                // make sure that buffer starts with a synch word
                ESP_LOGD(TAG,"-> moving to new start %d",range.start);
                buffer_size -= range.start;
                assert(buffer_size<=maxFrameSize());

                memmove(frame_buffer, frame_buffer + range.start, buffer_size);
                range.end -= range.start;
                range.start = 0;
                ESP_LOGD(TAG,"-> we are at beginning of synch word");
            } else if (range.start==0) {
                ESP_LOGD(TAG,"-> we are at beginning of synch word");
                if (range.end<0 && buffer_size == maxFrameSize()){
                    buffer_size = 0;
                    ESP_LOGD(TAG,"-> buffer cleared");
                }
            }
            return range;
        }



        void advanceFrameBuffer(int offset){
            buffer_size -= offset;
            assert(buffer_size<=maxFrameSize());
            memmove(frame_buffer, frame_buffer+offset, buffer_size);
        }

};

}

#undef TAG