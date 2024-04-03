// automatically generated by the FlatBuffers compiler, do not modify

import * as flatbuffers from 'flatbuffers';

import { Finger } from '../webmanager/finger';


export class ResponseFingers {
  bb: flatbuffers.ByteBuffer|null = null;
  bb_pos = 0;
  __init(i:number, bb:flatbuffers.ByteBuffer):ResponseFingers {
  this.bb_pos = i;
  this.bb = bb;
  return this;
}

static getRootAsResponseFingers(bb:flatbuffers.ByteBuffer, obj?:ResponseFingers):ResponseFingers {
  return (obj || new ResponseFingers()).__init(bb.readInt32(bb.position()) + bb.position(), bb);
}

static getSizePrefixedRootAsResponseFingers(bb:flatbuffers.ByteBuffer, obj?:ResponseFingers):ResponseFingers {
  bb.setPosition(bb.position() + flatbuffers.SIZE_PREFIX_LENGTH);
  return (obj || new ResponseFingers()).__init(bb.readInt32(bb.position()) + bb.position(), bb);
}

fingers(index: number, obj?:Finger):Finger|null {
  const offset = this.bb!.__offset(this.bb_pos, 4);
  return offset ? (obj || new Finger()).__init(this.bb!.__indirect(this.bb!.__vector(this.bb_pos + offset) + index * 4), this.bb!) : null;
}

fingersLength():number {
  const offset = this.bb!.__offset(this.bb_pos, 4);
  return offset ? this.bb!.__vector_len(this.bb_pos + offset) : 0;
}

static startResponseFingers(builder:flatbuffers.Builder) {
  builder.startObject(1);
}

static addFingers(builder:flatbuffers.Builder, fingersOffset:flatbuffers.Offset) {
  builder.addFieldOffset(0, fingersOffset, 0);
}

static createFingersVector(builder:flatbuffers.Builder, data:flatbuffers.Offset[]):flatbuffers.Offset {
  builder.startVector(4, data.length, 4);
  for (let i = data.length - 1; i >= 0; i--) {
    builder.addOffset(data[i]!);
  }
  return builder.endVector();
}

static startFingersVector(builder:flatbuffers.Builder, numElems:number) {
  builder.startVector(4, numElems, 4);
}

static endResponseFingers(builder:flatbuffers.Builder):flatbuffers.Offset {
  const offset = builder.endObject();
  return offset;
}

static createResponseFingers(builder:flatbuffers.Builder, fingersOffset:flatbuffers.Offset):flatbuffers.Offset {
  ResponseFingers.startResponseFingers(builder);
  ResponseFingers.addFingers(builder, fingersOffset);
  return ResponseFingers.endResponseFingers(builder);
}
}
