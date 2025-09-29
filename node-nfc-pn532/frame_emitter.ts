import EventEmitter from "node:events";
import {AckFrame, DataFrame, ErrorFrame, Frame, NackFrame, } from "./frame"
import util from "node:util"
import { PN532_UART } from "./uart_hal";

export class FrameEmitter extends EventEmitter {
    hal: PN532_UART;
    buffer: Buffer;
    /*
        @constructor
        @param {object} hal - An instance of PN532_UART or PN532_I2C
    */
    constructor(hal: PN532_UART) {
        super();
        this.hal = hal;
        this.buffer = Buffer.alloc(0);

        console.debug('listening to data');

        // console.dir(hal);
        this.hal.on('data', (data) => {
            console.debug('Data received', util.inspect(data));
            this.buffer = Buffer.concat([this.buffer, data]);
            this._processBuffer();
        });

        this.hal.on('error', (error) => {
            console.error('Error on HAL', error);
            this.emit('error', error);
        });
    }

    _processBuffer() {
        // TODO: filter garbage at front of buffer (anything not 0x00, 0x00, 0xFF at start?)

        //console.debug('Processing buffer', util.inspect(this.buffer));

        if (Frame.isFrame(this.buffer)) {
            //console.debug('Frame found in buffer');

            var frame = Frame.fromBuffer(this.buffer);
            // console.info('Frame', frame.toString());
            //console.info('Frame', util.inspect(frame));
            this.emit('frame', frame);

            if (frame instanceof ErrorFrame) {
                console.error('ErrorFrame found in buffer');
                this.emit('error', frame);
            } else if (frame instanceof DataFrame) {
                //console.debug('DataFrame found in buffer');
                this.emit('response', frame);
            } else if (frame instanceof AckFrame) {
                //console.debug('AckFrame found in buffer');
            } else if (frame instanceof NackFrame) {
                //console.debug('NackFrame found in buffer');
            }

            this.buffer = this.buffer.slice(frame.getFrameLength()); // strip off frame's data from buffer

            // If more data still on buffer, process buffer again,
            // otherwise next 'data' event on serial will process the buffer after more data is receive
            if (this.buffer.length) {
                this._processBuffer();
            }
        }
    }
}