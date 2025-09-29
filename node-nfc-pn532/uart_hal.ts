import { EventEmitter } from "node:events";
import { SerialPort } from "serialport";
import util from "node:util"
export class PN532_UART extends EventEmitter {
    serial: SerialPort;
    isAwake: boolean;
    constructor(serialPort:SerialPort) {
        super();
        this.serial = serialPort;
        this.isAwake = false;
    }

    init() {
        console.debug('Initializing serial port...');
        return new Promise<void>((resolve, reject) => {
            this.serial.on('open', (error) => {
                if (error) {
                    console.error('Error opening serial port:', util.inspect(error));
                    reject();
                }

                this.serial.on('data', (data) => {
                    this.emit('data', data);
                });

                console.debug('Serial port initialized.');
                resolve();
            });
            this.serial.on('error', (error) => {
                console.error('An error occurred on serial port:', util.inspect(error));
                reject();
            });
        });
    }

    write(buffer:Buffer) {
        if (!this.isAwake) {
            console.debug('Waking up PN532...');
            var wakeup = Buffer.from([0x55, 0x55, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00]);
            buffer = Buffer.concat([wakeup, buffer]);
            this.isAwake = true;
        }

        this.serial.write(buffer);
    }
}
