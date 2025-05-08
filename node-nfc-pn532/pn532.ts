import { EventEmitter } from "node:events";
import { FrameEmitter } from "./frame_emitter";
import { AckFrame, DataFrame, ErrorFrame, Frame } from "./frame"


import * as  c from "./nfcconstants";
import { SerialPort } from "serialport";
import util from "node:util"
import { PN532_UART } from "./uart_hal";




export class PN532 extends EventEmitter {

    hal: PN532_UART;
    frameEmitter: FrameEmitter;
    /*
        @constructor
        @param {object} hal - An instance of node-serialport's SerialPort or node-i2c's i2c
    */
    constructor(hal: SerialPort, private readonly pollInterval: number = 1000) {
        super();

        this.hal = new PN532_UART(hal);

        this.frameEmitter = new FrameEmitter(this.hal);
        this.hal.init().then(() => {
            this.configureSecureAccessModule().then(() => this.emit('ready'));
        });

        this.on('newListener', (event) => {
            // TODO: Only poll once (for each event type)
            if (event === 'tag') {
                console.debug('Polling for tag scans...');
                var scanTag = () => {
                    this.scanTag().then((tag) => {
                        this.emit('tag', tag);
                        setTimeout(() => scanTag(), this.pollInterval);
                    });
                };
                scanTag();
            }
        });
    }

    sendCommand(commandBuffer:Buffer|Array<number>) {
        return new Promise<DataFrame>((resolve, reject) => {

            var removeListeners = () => {
                console.debug('Removing listeners');
                this.frameEmitter.removeListener('frame', onFrame);
                this.frameEmitter.removeListener('error', onError);
            };

            // Wire up listening to wait for response (or error) from PN532
            var onFrame = (frame:Frame) => {
                
                // TODO: If no ACK after 15ms, resend? (page 40 of user guide, UART only)?

                if (frame instanceof AckFrame) {
                    console.info('Received AckFrame', util.inspect(frame));
                } else if (frame instanceof DataFrame) {
                    console.info('Received DataFrame', util.inspect(frame));
                    removeListeners();
                    resolve(frame);
                }
                else{
                    console.warn('Received unknown frame', util.inspect(frame));
                }
            };
            this.frameEmitter.on('frame', onFrame);

            var onError = (error:ErrorFrame) => {
                console.error('Received ErrorFrame', error);
                removeListeners();
                reject(error);
            };
            this.frameEmitter.on('error', onError);

            // Send command to PN532
            var dataFrame = new DataFrame(commandBuffer);
            var buffer = dataFrame.toBuffer();
            console.debug('Sending command as DataFrame:', util.inspect(buffer));
            this.hal.write(buffer);
        });
    }

    configureSecureAccessModule() {
        console.info('Configuring secure access module (SAM)...');

        // TODO: Test IRQ triggered reads

        var timeout = 0x00;  // 0x00-0xFF (12.75 seconds).  Only valid for Virtual card mode (SAMCONFIGURATION_MODE_VIRTUAL_CARD)

        var commandBuffer = [
            c.COMMAND_SAMCONFIGURATION,
            c.SAMCONFIGURATION_MODE_NORMAL,
            timeout,
            c.SAMCONFIGURATION_IRQ_ON // Use IRQ pin
        ];
        return this.sendCommand(commandBuffer);
    }

    getFirmwareVersion() {
        console.info('Getting firmware version...');

        return this.sendCommand([c.COMMAND_GET_FIRMWARE_VERSION])
            .then((frame) => {
                var body = frame.getDataBody();
                return {
                    IC: body[0],
                    Ver: body[1],
                    Rev: body[2],
                    Support: body[3]
                };
            });
    }

    getGeneralStatus() {
        console.info('Getting general status...');

        return this.sendCommand([c.COMMAND_GET_GENERAL_STATUS])
            .then((frame) => {
                var body = frame.getDataBody();
                return body;
            });
    }

    scanTag() {
        console.info('Scanning tag...');

        var maxNumberOfTargets = 0x01;
        var baudRate = c.CARD_ISO14443A;

        var commandBuffer = [
            c.COMMAND_IN_LIST_PASSIVE_TARGET,
            maxNumberOfTargets,
            baudRate
        ];

        return this.sendCommand(commandBuffer)
            .then((frame) => {
                var body = frame.getDataBody();
                console.debug('body', util.inspect(body));

                var numberOfTags = body[0];
                if (numberOfTags === 1) {
                    var tagNumber = body[1];
                    var uidLength = body[5];

                    var uid = body.slice(6, 6 + uidLength)!
                        .toString('hex')!
                        .match(/.{1,2}/g)!
                        .join(':');

                    return {
                        ATQA: body.slice(2, 4), // SENS_RES
                        SAK: body[4],           // SEL_RES
                        uid: uid
                    };
                }
            });
    }

    readBlock(opts?:{tagNumber?:number, blockAddress?:number }) {
        console.info('Reading block...');

        var options = opts || {};

        var tagNumber = options.tagNumber || 0x01;
        var blockAddress = options.blockAddress || 0x01;

        var commandBuffer = [
            c.COMMAND_IN_DATA_EXCHANGE,
            tagNumber,
            c.MIFARE_COMMAND_READ,
            blockAddress,
        ];

        return this.sendCommand(commandBuffer)
            .then((frame) => {
                var body = frame.getDataBody();
                console.debug('Frame data from block read:', util.inspect(body));

                var status = body[0];

                if (status === 0x13) {
                    console.warn('The data format does not match to the specification.');
                }
                var block = body.slice(1, body.length - 1); // skip status byte and last byte (not part of memory)
                // var unknown = body[body.length];

                return block;
            });
    }

    readNdefData() {
        console.info('=====Start Function readNdefData');

        return this.readBlock({ blockAddress: 0x04 })
            .then((block) => {
                console.debug('block:', util.inspect(block));

                // Find NDEF TLV (0x03) in block of data - See NFC Forum Type 2 Tag Operation Section 2.4 (TLV Blocks)
                var ndefValueOffset:number|null = null;
                var ndefLength:number|null = null;
                var blockOffset = 0;

                while (ndefValueOffset === null) {
                    console.debug('blockOffset:', blockOffset, 'block.length:', block.length);
                    if (blockOffset >= block.length) {
                        throw new Error(`Unable to locate NDEF TLV (0x03) byte in block: ${block}`)
                    }

                    var type = block[blockOffset];       // Type of TLV
                    var length = block[blockOffset + 1]; // Length of TLV
                    console.debug('blockOffset', blockOffset);
                    console.debug('type', type, 'length', length);

                    if (type === c.TAG_MEM_NDEF_TLV) {
                        console.debug('NDEF TLV found');
                        ndefLength = length;                  // Length proceeds NDEF_TLV type byte
                        ndefValueOffset = blockOffset + 2;    // Value (NDEF data) proceeds NDEV_TLV length byte
                        console.debug('ndefLength:', ndefLength);
                        console.debug('ndefValueOffset:', ndefValueOffset);
                    } else {
                        // Skip TLV (type byte, length byte, plus length of value)
                        blockOffset = blockOffset + 2 + length;
                    }
                }

                var ndefData = block.slice(ndefValueOffset, block.length);
                var additionalBlocks = Math.ceil((ndefValueOffset + ndefLength!) / 16) - 1;
                console.debug('Additional blocks needing to retrieve:', additionalBlocks);

                // Sequentially grab each additional 16-byte block (or 4x 4-byte pages) of data, chaining promises
                var self = this;
                var allDataPromise = (function retrieveBlock(blockNum):any {
                    if (blockNum <= additionalBlocks) {
                        var blockAddress = 4 * (blockNum + 1);
                        console.debug('Retrieving block:', blockNum, 'at blockAddress:', blockAddress);
                        return self.readBlock({ blockAddress: blockAddress })
                            .then(function (block) {
                                blockNum++;
                                ndefData = Buffer.concat([ndefData, block]);
                                return retrieveBlock(blockNum);
                            });
                    }
                })(1);

                return allDataPromise.then(() => ndefData.slice(0, ndefLength!));
            })
            .catch(function (error) {
                console.error('ERROR:', error);
            });
    }

    writeBlock(block:Buffer, opts?:{tagNumber?:number, blockAddress?:number }) {
        console.info('Writing block...');

        var options = opts || {};

        var tagNumber = options.tagNumber || 0x01;
        var blockAddress = options.blockAddress || 0x01;

        var commandBuffer = Buffer.from([c.COMMAND_IN_DATA_EXCHANGE, tagNumber, c.MIFARE_COMMAND_WRITE_4, blockAddress, ...block]);

        return this.sendCommand(commandBuffer)
            .then((frame) => {
                var body = frame.getDataBody();
                console.debug('Frame data from block write:', util.inspect(body));

                var status = body[0];

                if (status === 0x13) {
                    console.warn('The data format does not match to the specification.');
                }
                var block = body.slice(1, body.length - 1); // skip status byte and last byte (not part of memory)
                // var unknown = body[body.length];

                return block;
            });
    }

    writeNdefData(data:Buffer|Array<number>) {
        console.info('Writing data...');

        // Prepend data with NDEF type and length (TLV) and append terminator TLV
        var block = Buffer.from([c.TAG_MEM_NDEF_TLV, data.length, ...data, c.TAG_MEM_TERMINATOR_TLV]);

        console.debug('block:', util.inspect(Buffer.from(block)));

        var PAGE_SIZE = 4;
        var totalBlocks = Math.ceil(block.length / PAGE_SIZE);

        // Sequentially write each additional 4-byte pages of data, chaining promises
        var self = this;
        var allPromises = (function writeBlock(blockNum:number):any {
            if (blockNum < totalBlocks) {
                var blockAddress = 0x04 + blockNum;
                var pageData= block.subarray(blockNum*PAGE_SIZE, blockNum*PAGE_SIZE+PAGE_SIZE)

                if (pageData.length < PAGE_SIZE) {
                    pageData=Buffer.concat([pageData, Buffer.alloc(PAGE_SIZE-pageData.length)]);
                    //pageData.length = PAGE_SIZE; // Setting length will make sure NULL TLV (0x00) are written at the end of the page
                }

                console.debug('Writing block:', blockNum, 'at blockAddress:', blockAddress);
                console.debug('pageData:', util.inspect(Buffer.from(pageData)));
                return self.writeBlock(pageData, { blockAddress: blockAddress })
                    .then(function (block) {
                        blockNum++;
                        // ndefData = Buffer.concat([ndefData, block]);
                        return writeBlock(blockNum);
                    });
            }
        })(0);

        // return allDataPromise.then(() => ndefData.slice(0, ndefLength));
        return allPromises;
    }

    // WIP
    authenticateBlock(uid:string, options:any) {
        console.info('Authenticating block...');

        var options = options || {};

        var blockAddress = options.blockAddress || 0x04;
        var authType = options.authType || c.MIFARE_COMMAND_AUTH_A
        var authKey = options.authKey || [0xff, 0xff, 0xff, 0xff, 0xff, 0xff];
        var tagNumber = options.tagNumber || 0x01;
        var uidArray = uid.split(':').map((s:any) => Number('0x' + s));

        var commandBuffer = [
            c.COMMAND_IN_DATA_EXCHANGE,
            tagNumber,
            authType,
            blockAddress
        ].concat(authKey).concat(uidArray);

        return this.sendCommand(commandBuffer)
            .then((frame) => {
                var body = frame.getDataBody();
                console.info('Frame data from mifare classic authenticate', util.inspect(body));

                console.log('body', body);
                return body;

                // var status = body[0];
                // var tagData = body.slice(1, body.length);

                // return {
                //     status: status.toString(16),
                //     tagData: tagData
                // };
            });
    }
}

