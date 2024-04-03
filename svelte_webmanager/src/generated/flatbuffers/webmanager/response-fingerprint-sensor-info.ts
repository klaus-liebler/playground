// automatically generated by the FlatBuffers compiler, do not modify

import * as flatbuffers from 'flatbuffers';

export class ResponseFingerprintSensorInfo {
  bb: flatbuffers.ByteBuffer|null = null;
  bb_pos = 0;
  __init(i:number, bb:flatbuffers.ByteBuffer):ResponseFingerprintSensorInfo {
  this.bb_pos = i;
  this.bb = bb;
  return this;
}

static getRootAsResponseFingerprintSensorInfo(bb:flatbuffers.ByteBuffer, obj?:ResponseFingerprintSensorInfo):ResponseFingerprintSensorInfo {
  return (obj || new ResponseFingerprintSensorInfo()).__init(bb.readInt32(bb.position()) + bb.position(), bb);
}

static getSizePrefixedRootAsResponseFingerprintSensorInfo(bb:flatbuffers.ByteBuffer, obj?:ResponseFingerprintSensorInfo):ResponseFingerprintSensorInfo {
  bb.setPosition(bb.position() + flatbuffers.SIZE_PREFIX_LENGTH);
  return (obj || new ResponseFingerprintSensorInfo()).__init(bb.readInt32(bb.position()) + bb.position(), bb);
}

status():number {
  const offset = this.bb!.__offset(this.bb_pos, 4);
  return offset ? this.bb!.readUint16(this.bb_pos + offset) : 0;
}

librarySize():number {
  const offset = this.bb!.__offset(this.bb_pos, 6);
  return offset ? this.bb!.readUint16(this.bb_pos + offset) : 0;
}

securityLevel():number {
  const offset = this.bb!.__offset(this.bb_pos, 8);
  return offset ? this.bb!.readUint8(this.bb_pos + offset) : 0;
}

deviceAddress():number {
  const offset = this.bb!.__offset(this.bb_pos, 10);
  return offset ? this.bb!.readUint32(this.bb_pos + offset) : 0;
}

dataPacketSizeCode():number {
  const offset = this.bb!.__offset(this.bb_pos, 12);
  return offset ? this.bb!.readUint8(this.bb_pos + offset) : 0;
}

baudRateTimes9600():number {
  const offset = this.bb!.__offset(this.bb_pos, 14);
  return offset ? this.bb!.readUint8(this.bb_pos + offset) : 0;
}

algVer():string|null
algVer(optionalEncoding:flatbuffers.Encoding):string|Uint8Array|null
algVer(optionalEncoding?:any):string|Uint8Array|null {
  const offset = this.bb!.__offset(this.bb_pos, 16);
  return offset ? this.bb!.__string(this.bb_pos + offset, optionalEncoding) : null;
}

fwVer():string|null
fwVer(optionalEncoding:flatbuffers.Encoding):string|Uint8Array|null
fwVer(optionalEncoding?:any):string|Uint8Array|null {
  const offset = this.bb!.__offset(this.bb_pos, 18);
  return offset ? this.bb!.__string(this.bb_pos + offset, optionalEncoding) : null;
}

static startResponseFingerprintSensorInfo(builder:flatbuffers.Builder) {
  builder.startObject(8);
}

static addStatus(builder:flatbuffers.Builder, status:number) {
  builder.addFieldInt16(0, status, 0);
}

static addLibrarySize(builder:flatbuffers.Builder, librarySize:number) {
  builder.addFieldInt16(1, librarySize, 0);
}

static addSecurityLevel(builder:flatbuffers.Builder, securityLevel:number) {
  builder.addFieldInt8(2, securityLevel, 0);
}

static addDeviceAddress(builder:flatbuffers.Builder, deviceAddress:number) {
  builder.addFieldInt32(3, deviceAddress, 0);
}

static addDataPacketSizeCode(builder:flatbuffers.Builder, dataPacketSizeCode:number) {
  builder.addFieldInt8(4, dataPacketSizeCode, 0);
}

static addBaudRateTimes9600(builder:flatbuffers.Builder, baudRateTimes9600:number) {
  builder.addFieldInt8(5, baudRateTimes9600, 0);
}

static addAlgVer(builder:flatbuffers.Builder, algVerOffset:flatbuffers.Offset) {
  builder.addFieldOffset(6, algVerOffset, 0);
}

static addFwVer(builder:flatbuffers.Builder, fwVerOffset:flatbuffers.Offset) {
  builder.addFieldOffset(7, fwVerOffset, 0);
}

static endResponseFingerprintSensorInfo(builder:flatbuffers.Builder):flatbuffers.Offset {
  const offset = builder.endObject();
  return offset;
}

static createResponseFingerprintSensorInfo(builder:flatbuffers.Builder, status:number, librarySize:number, securityLevel:number, deviceAddress:number, dataPacketSizeCode:number, baudRateTimes9600:number, algVerOffset:flatbuffers.Offset, fwVerOffset:flatbuffers.Offset):flatbuffers.Offset {
  ResponseFingerprintSensorInfo.startResponseFingerprintSensorInfo(builder);
  ResponseFingerprintSensorInfo.addStatus(builder, status);
  ResponseFingerprintSensorInfo.addLibrarySize(builder, librarySize);
  ResponseFingerprintSensorInfo.addSecurityLevel(builder, securityLevel);
  ResponseFingerprintSensorInfo.addDeviceAddress(builder, deviceAddress);
  ResponseFingerprintSensorInfo.addDataPacketSizeCode(builder, dataPacketSizeCode);
  ResponseFingerprintSensorInfo.addBaudRateTimes9600(builder, baudRateTimes9600);
  ResponseFingerprintSensorInfo.addAlgVer(builder, algVerOffset);
  ResponseFingerprintSensorInfo.addFwVer(builder, fwVerOffset);
  return ResponseFingerprintSensorInfo.endResponseFingerprintSensorInfo(builder);
}
}
