var ndef = require('ndef');
import { SerialPort } from 'serialport'
import { PN532 } from './pn532';



var serialPort = new SerialPort({path:'COM31', baudRate: 115200 });
var rfid:PN532 = new PN532(serialPort);
console.log('Waiting for rfid ready event...');
rfid.on('ready', function() {
    console.log('Listening for a tag scan...');
    rfid.on('tag', function(tag) {
        console.log(Date.now(), 'UID:', tag.uid);
    });
});