// automatically generated by the FlatBuffers compiler, do not modify

import * as flatbuffers from 'flatbuffers';

export class RequestRenameFinger {
  bb: flatbuffers.ByteBuffer|null = null;
  bb_pos = 0;
  __init(i:number, bb:flatbuffers.ByteBuffer):RequestRenameFinger {
  this.bb_pos = i;
  this.bb = bb;
  return this;
}

static getRootAsRequestRenameFinger(bb:flatbuffers.ByteBuffer, obj?:RequestRenameFinger):RequestRenameFinger {
  return (obj || new RequestRenameFinger()).__init(bb.readInt32(bb.position()) + bb.position(), bb);
}

static getSizePrefixedRootAsRequestRenameFinger(bb:flatbuffers.ByteBuffer, obj?:RequestRenameFinger):RequestRenameFinger {
  bb.setPosition(bb.position() + flatbuffers.SIZE_PREFIX_LENGTH);
  return (obj || new RequestRenameFinger()).__init(bb.readInt32(bb.position()) + bb.position(), bb);
}

oldName():string|null
oldName(optionalEncoding:flatbuffers.Encoding):string|Uint8Array|null
oldName(optionalEncoding?:any):string|Uint8Array|null {
  const offset = this.bb!.__offset(this.bb_pos, 4);
  return offset ? this.bb!.__string(this.bb_pos + offset, optionalEncoding) : null;
}

newName():string|null
newName(optionalEncoding:flatbuffers.Encoding):string|Uint8Array|null
newName(optionalEncoding?:any):string|Uint8Array|null {
  const offset = this.bb!.__offset(this.bb_pos, 6);
  return offset ? this.bb!.__string(this.bb_pos + offset, optionalEncoding) : null;
}

static startRequestRenameFinger(builder:flatbuffers.Builder) {
  builder.startObject(2);
}

static addOldName(builder:flatbuffers.Builder, oldNameOffset:flatbuffers.Offset) {
  builder.addFieldOffset(0, oldNameOffset, 0);
}

static addNewName(builder:flatbuffers.Builder, newNameOffset:flatbuffers.Offset) {
  builder.addFieldOffset(1, newNameOffset, 0);
}

static endRequestRenameFinger(builder:flatbuffers.Builder):flatbuffers.Offset {
  const offset = builder.endObject();
  return offset;
}

static createRequestRenameFinger(builder:flatbuffers.Builder, oldNameOffset:flatbuffers.Offset, newNameOffset:flatbuffers.Offset):flatbuffers.Offset {
  RequestRenameFinger.startRequestRenameFinger(builder);
  RequestRenameFinger.addOldName(builder, oldNameOffset);
  RequestRenameFinger.addNewName(builder, newNameOffset);
  return RequestRenameFinger.endRequestRenameFinger(builder);
}
}
