// read font file

import { createCanvas } from 'canvas';
import opentype, {Font, Glyph } from 'opentype.js'
import fs from "node:fs";

class GlyphDesc{
	constructor(public bitmap_index:number, public adv_w = 0, public box_w = 0, public ofs_x = 0){}

	public toCppConstructorString():string{
		return `GlyphDesc(${this.bitmap_index}, ${this.adv_w}, ${this.box_w}, ${this.ofs_x})`
	}
}

class Range{
	constructor(public readonly startIncl:number, public readonly endExcl:number){}
}

class GlyphProcessingInfo{
	constructor(public readonly codepointTtf:number, public readonly codepointCc:number, public readonly glyphIndexTtf:number, public readonly glyphIndexCc:number, public readonly font:Font){}
}

class  CharacterMap0Tiny{
	public len:number;
	constructor(public readonly firstCodepoint:number, public readonly firstGlyph:number){}
	public toCppConstructorString():string{
		return `CharacterMap0Tiny(${this.firstCodepoint}, ${this.firstGlyph}, ${this.len})`
	}
}

function codepoint2glyphindex(font:Font, codePoint:number){
	return (<any>font).encoding.cmap.glyphIndexMap[codePoint];
}

function concat(a:Uint8Array, b:Uint8Array, c:Uint8Array):Uint8Array{
	var d = new Uint8Array(a.length+b.length+c.length);
	d.set(a, 0)
	d.set(b, a.length);
	d.set(c, a.length+b.length);
	return d;
}

class FontCC{
	
	public bitmap:string;
	public glyphs:Array<GlyphDesc>=[];
	public characterMaps:Array<CharacterMap0Tiny>=[];
	public maxGlyphIndexWithKerningInfo:number;
	public glyphIndex2leftKerningClass:Array<number>;
	public glyphIndex2rightKerningClass:Array<number>;
	public leftrightKerningClass2kerningValue:Array<number>;
	private constructor(){}

	private buildGlyphDescriptionAndAddToBitmap(info:GlyphProcessingInfo):GlyphDesc{
		var g = info.font.glyphs.get(info.glyphIndexTtf);
		var canvas=createCanvas(16, 16);
		var ctx:any=canvas.getContext("2d");
		g.draw(ctx, 0, 16, 16, <any>{hinting:true});
		
		var offsetX=0;
		var top8vec:Uint8Array=new Uint8Array(16)
		var bot8vec:Uint8Array=new Uint8Array(16)
		for(let x=0;x<16;x++){
			var top8= ctx.getImageData(x,0,1,8);
			top8vec[x]=(clampedArrayToPixel(top8));
			var bottom8= ctx.getImageData(x,8,1,8);
			bot8vec[x]=(clampedArrayToPixel(bottom8));
		}
		var offsetX=0;
		while(top8vec[offsetX]==0 && bot8vec[offsetX]==0){
			offsetX++;
		}
		var w=0;
		while(offsetX+w<16 && (top8vec[offsetX+w]!=0 || bot8vec[offsetX+w]!=0)){
			w++;
		}
		top8vec=top8vec.slice(offsetX, offsetX+w);
		bot8vec=bot8vec.slice(offsetX, offsetX+w);
		//console.log(`offsetX=${offsetX}, w=${w}, top8vec=${toHexArray(top8vec)}, bot8vec=${toHexArray(bot8vec)}`)
		var bitmapIndex=this.bitmap.length;
		if(w>0){
			this.bitmap+=`${toHexArray(top8vec)},${toHexArray(bot8vec)}, //codepoint=${g.unicode},  glyphindex=${info.glyphIndexCc}, width=${w}\n`;
		}else{
			this.bitmap+=`//codepoint=${g.unicode}, glyphindex=${info.glyphIndexCc}, width=${w}\n`;
		}
		return new GlyphDesc(bitmapIndex, g.advanceWidth, w, offsetX);
	}

	private buildClassBasedKernings(font:Font, glyphProcessingInfo:Array<GlyphProcessingInfo>){
		
		const gposKerning = (<any>font).position.defaultKerningTables;
		const subtable = gposKerning[0].subtables[1];
		var leftClasses=[];
		var rightClasses=[];
		glyphProcessingInfo.forEach((v,i,a)=>{
			leftClasses.push((<any>font).position.getGlyphClass(subtable.classDef1, v.glyphIndexTtf));
			rightClasses.push((<any>font).position.getGlyphClass(subtable.classDef2, v.glyphIndexTtf));
		});
		
		var classes2kerning:Array<number>=[];
		for(let leftClass=0;leftClass<subtable.class1Count;leftClass++){
			for(let rightClass=0;rightClass<subtable.class2Count;rightClass++){
				const pair = subtable.classRecords[leftClass][rightClass];
				classes2kerning.push(pair.value1 && pair.value1.xAdvance || 0);
			}
		}

		subtable.classRecords.forEach((i)=>{classes2kerning.push(i.value1 && i.value1.xAdvance || 0)})


		this.glyphIndex2leftKerningClass=leftClasses;
		this.glyphIndex2rightKerningClass=rightClasses;
		this.leftrightKerningClass2kerningValue=classes2kerning;
	}

	private buildCmaps(glyphIndexes:Array<GlyphProcessingInfo>){
		//Finde nun zusammenhängende codepoint-Bereiche und erstelle für jeden zusammenhängenden BEreich eine cmap
		var i=0;
		var targetGlyphIndex=1;
		var len=0;
		var cmap = new CharacterMap0Tiny(glyphIndexes[i].codepointCc, targetGlyphIndex);
		i++;
		targetGlyphIndex++;
		len++;
		while(i<glyphIndexes.length){
			if(glyphIndexes[i].codepointCc==glyphIndexes[i-1].codepointCc+1){ //wenn der nächste codepoint gleich dem vorherigen+1 ist
				len++;
			}else{
				cmap.len=len;
				this.characterMaps.push(cmap);
				cmap=new CharacterMap0Tiny(glyphIndexes[i].codepointCc, targetGlyphIndex);
				len=1;
			}
			i++;
			targetGlyphIndex++;
		}
		cmap.len=len;
		this.characterMaps.push(cmap);
	}

	private buildGlyphProcessingInfo(font: opentype.Font, codepointRanges: Range[], relocateToUnicodePrivateUseArea):Array<GlyphProcessingInfo>{
		var glyphIndexes:Array<GlyphProcessingInfo>=[];
		var glyphIndexCc=1;
		const unicode_private_use_area=0xe000;//57344
		var relocatedCodepoint = unicode_private_use_area;
		codepointRanges.forEach(r=>{
			for (let codePoint = r.startIncl; codePoint < r.endExcl; codePoint++) {
				var glyphIndexTtf=codepoint2glyphindex(font, codePoint);
				if(glyphIndexTtf){
					if(relocateToUnicodePrivateUseArea){
						glyphIndexes.push(new GlyphProcessingInfo(codePoint, relocatedCodepoint, glyphIndexTtf, glyphIndexCc, font));
						relocatedCodepoint++;
					}else{
						glyphIndexes.push(new GlyphProcessingInfo(codePoint, codePoint, glyphIndexTtf, glyphIndexCc, font))
					}
					glyphIndexCc++;
				}
			}
		});
		return glyphIndexes;
	}

	private build(font: opentype.Font, codepointRanges: Range[], symbolFont: opentype.Font, codepointRangesSymbols: Range[]) {
		var glyphInfoMainFont:Array<GlyphProcessingInfo>=this.buildGlyphProcessingInfo(font, codepointRanges, false);
		var glyphInfoSymbolFont:Array<GlyphProcessingInfo>=this.buildGlyphProcessingInfo(symbolFont, codepointRangesSymbols, true);
		this.maxGlyphIndexWithKerningInfo=glyphInfoMainFont[glyphInfoMainFont.length-1].glyphIndexCc;
		this.buildCmaps(glyphInfoMainFont.concat(glyphInfoSymbolFont));
		this.buildClassBasedKernings(font, glyphInfoMainFont)
		this.glyphs.push(new GlyphDesc(0,0,0,0));
		this.bitmap="";
		glyphInfoMainFont.concat(glyphInfoSymbolFont).forEach(v=>{
			if(this.glyphs.length != v.glyphIndexCc)
				throw new Error(`${this.glyphs.length} != ${v.glyphIndexCc}`);
			this.glyphs.push(this.buildGlyphDescriptionAndAddToBitmap(v));
		});
	}
	public toFontCCFile():string{
		var ret="#pragma once\n#include <cstdint>\n#include \"klfont.hh\"\nnamespace glyph{\n";
		ret+=`constexpr const uint8_t glyph_bitmap[]={\n${this.bitmap}\n};\n\n`
		
		var desc= Array.from(this.glyphs, (v, i)=> {
			return `${v.toCppConstructorString()}, //glyphindex ${i}` ;
		}).join("\n\t")
		ret+=`constexpr const GlyphDesc glyph_desc[]={\n\t${desc}\n};\n\n`
		
		var cmaps= Array.from(this.characterMaps, (v)=> {
			return v.toCppConstructorString();
		}).join(",\n\t")
		ret+=`constexpr const CharacterMap0Tiny cmaps[]={\n\t${cmaps}\n};\n\n`

		ret+=`constexpr uint32_t maxGlyphIndexWithKerningInfo=${this.maxGlyphIndexWithKerningInfo};\n\n`
		ret+=`constexpr const uint8_t kern_left_class_mapping[] = {${toHexArray(this.glyphIndex2leftKerningClass)}};\n\n`
		ret+=`constexpr const uint8_t kern_right_class_mapping[] = {${toHexArray(this.glyphIndex2rightKerningClass)}};\n\n`
		ret+=`constexpr const int16_t kern_class_values[] = {${toHexArray(this.leftrightKerningClass2kerningValue)}};\n\n`
		ret+="}"
		return ret;
	}
	public static Build(mainFont:Font, codepointRanges:Array<Range>, symbolFont:Font, codepointRangesSymbols:Array<Range>):FontCC{
		var res = new FontCC();
		res.build(mainFont, codepointRanges, symbolFont, codepointRangesSymbols);
		return res;
	}
	
}

var stringFromCharCode = (charcode: number) => {
	return "\\x" + charcode.toString(16);
}

function checkScalarValue(codePoint: number) {
	if (codePoint >= 0xD800 && codePoint <= 0xDFFF) {
		throw Error(
			'Lone surrogate U+' + codePoint.toString(16).toUpperCase() +
			' is not a scalar value'
		);
	}
}

function clampedArrayToPixel(image:ImageData){
	var arr=image.data;
	var val=0;
	for(let i=0;i<8;i++){
		var r=arr[4*i+0];
		var g=arr[4*i+1];
		var b=arr[4*i+2];
		var a=arr[4*i+3];
		var lum = 0.299*(r) + 0.587*(g) + 0.114*(b);
		if(a>0){
			val |=1;
		}
		val=val<<1;
	}
	return val;
}

function toHexArray(byteArray:Iterable<number>):string {
	return Array.from(byteArray, (byte)=> {
		if(byte>=16) return ('0x' + (byte & 0xFF).toString(16));
	  	else return '0x0'+((byte & 0xFF).toString(16));
	}).join(",")
  }


function createByte(codePoint: number, shift: number) {
	return stringFromCharCode(((codePoint >> shift) & 0x3F) | 0x80);
}

function encodeCodePoint(codePoint: number): string {
	if ((codePoint & 0xFFFFFF80) == 0) { // 1-byte sequence
		return stringFromCharCode(codePoint);
	}
	var symbol = '';
	if ((codePoint & 0xFFFFF800) == 0) { // 2-byte sequence
		symbol = stringFromCharCode(((codePoint >> 6) & 0x1F) | 0xC0);
	}
	else if ((codePoint & 0xFFFF0000) == 0) { // 3-byte sequence
		checkScalarValue(codePoint);
		symbol = stringFromCharCode(((codePoint >> 12) & 0x0F) | 0xE0);
		symbol += createByte(codePoint, 6);
	}
	else if ((codePoint & 0xFFE00000) == 0) { // 4-byte sequence
		symbol = stringFromCharCode(((codePoint >> 18) & 0x07) | 0xF0);
		symbol += createByte(codePoint, 12);
		symbol += createByte(codePoint, 6);
	}
	symbol += stringFromCharCode((codePoint & 0x3F) | 0x80);
	return symbol;
}



const buffer = fs.readFileSync('Roboto-Regular.ttf').buffer;
const font= opentype.parse(buffer);
var fontCc= FontCC.Build(font, [new Range(32,256)], font, []);
fs.writeFileSync("roboto.cc", fontCc.toFontCCFile());
//produceBitmap(font.charToGlyph("H"))
//console.log(produceClassBasedKernings(font, font.charToGlyphIndex(" "), font.charToGlyphIndex("ÿ")));


/*
var glyphs:Array<opentype.Glyph>=[];
const unicode_private_use_area=0xe000;//57344
var unicode = unicode_private_use_area;
for (let index = 0; index < font.glyphs.length; index++) {
	const g = font.glyphs.get(index);
	if(!g.unicode || g.unicode<=0xFF) continue;
	//console.log(`index=${index}, unicode=${g.unicode}, unicodes=${g.unicodes}`);
	processGlyph(g)
	glyphs.push(new opentype.Glyph({
		name: g.name,
		unicode: unicode,
		advanceWidth: g.advanceWidth,
		path: g.path
	}))
	unicode++;
}
const new_font = new opentype.Font({
    familyName: "symbol",
    styleName: 'Medium',
    unitsPerEm: font.unitsPerEm,
    ascender: font.ascender,
    descender: font.descender,
    glyphs: glyphs});



fs.writeFileSync('fa-solid-900_kl.ttf', new Uint8Array(new_font.toArrayBuffer()));
var header="#pragma once\n#include <cstdint>\nnamespace glyph{\n";

console.log(glyphs.length);
header+=`//created ${Date.now()} \n`
glyphs.filter((v,i,a)=>{return v.unicode?true:false;}).forEach((e) => {
	//header+=`   constexpr const char* G_${e.name.replaceAll("-", "_").toLocaleUpperCase()} = "${encodeCodePoint(e.unicode[0])}"; // ${e.unicode[0]}\n`;
	header+=`   #define G_${e.name.replaceAll("-", "_").toLocaleUpperCase()} "${encodeCodePoint(e.unicode)}"\n`;
});
header+="}\n";
fs.writeFileSync("glyps.hh", header);



function processGlyph(g: opentype.Glyph) {
	//throw new Error('Function not implemented.');
}

*/