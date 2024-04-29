import { Font } from 'opentype.js';

export const UNICODE_PRIVATE_USE_AREA = 57344; //57344

export class Range {
	constructor(public readonly startIncl: number, public readonly endExcl: number) { }
}
export class GlyphProcessingInfo {
	constructor(public readonly codepointSource: number, public readonly codepointDest: number, public readonly glyphIndexSource: number, public readonly glyphIndexDest: number, public readonly glyphName: string, public readonly font: Font) { }
}

export class CharacterMap0Tiny {

	constructor(public readonly firstCodepoint: number, public readonly firstGlyph: number, public len = 0) { }
	public toCppConstructorString(): string {
		return `CharacterMap0Tiny(${this.firstCodepoint}, ${this.firstGlyph}, ${this.len})`;
	}
}

export enum PixelFormat{
    UNDEFINED,
    One_Bpp_Eight_In_A_Column,//best for SSD1306 etc
    Four_Bpp_Row_By_Row,//best for ST7789 etc
}

export class GlyphDesc {
	constructor(public readonly codepointDest:number, public readonly name:string, public adv_w = 0, public box_w = 0, public box_h = 16, public ofs_x = 0, public readonly bitmapFormat:PixelFormat, public readonly bitmap:Uint8Array) { }

	public toCppConstructorString(bitmap_index: number, glyphIndex:number): string {
		return `\tGlyphDesc(${bitmap_index}, ${this.adv_w}, ${this.box_w}, ${this.ofs_x}),// ${this.name} glyphindex=${glyphIndex} codepoint=${this.codepointDest}\n`;
	}
}

export function concat(a: Uint8Array, b: Uint8Array, c: Uint8Array=new Uint8Array()): Uint8Array {
	var d = new Uint8Array(a.length + b.length + c.length);
	d.set(a, 0);
	d.set(b, a.length);
	d.set(c, a.length + b.length);
	return d;
}

export function ToUint8Array(...a: Array<Array<number>>): Uint8Array {
	var d = new Uint8Array(a.reduce((pv, cv, ci, a)=>pv+cv.length, 0));
    var start=0;
    a.forEach((v=>{
        d.set(v, start);
        start+=v.length;
    }))
	return d;
}

export function formatedTimestamp() {
    const d = new Date()
    const date = d.toISOString().split('T')[0];
    const time = d.toTimeString().split(' ')[0];
    return `${date} ${time}`
}

export class GlyphProviderResult{

    public glyphsDesc:Array<GlyphDesc>;
}

export class GlyphProviderWithKerningResult{
    public glyphsDesc:Array<GlyphDesc>;
    public fontUnitsPerEm:number;
    public leftKerningClassCnt:number;
    public rightKerningClassCnt:number;
    public glyphIndex2leftKerningClass:Array<number>;
    public glyphIndex2rightKerningClass:Array<number>;
    public leftrightKerningClass2kerningValue:Array<number>;
}



export interface IGlyphProviderWithKerning{
    Provide():Promise<GlyphProviderWithKerningResult>;
}

export interface IGlyphProvider{
    Provide():Promise<GlyphProviderResult>;
}

