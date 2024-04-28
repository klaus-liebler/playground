// read font file

import * as Canvas from 'canvas';
import opentype, {Font, Glyph } from 'opentype.js'
import fs from "node:fs";
import { Arial14 } from './arial_14_glcd';
import path, { join } from "node:path"
const unicode_private_use_area=0xe000;//57344
class GlyphDesc{
	constructor(public readonly codepointTtf:number, public bitmap_index:number, public adv_w = 0, public box_w = 0, public ofs_x = 0){}

	public toCppConstructorString():string{
		return `GlyphDesc(${this.bitmap_index}, ${this.adv_w}, ${this.box_w}, ${this.ofs_x})`
	}
}

class Range{
	constructor(public readonly startIncl:number, public readonly endExcl:number){}
}

class GlyphProcessingInfo{
	constructor(public readonly codepointSource:number, public readonly codepointDest:number, public readonly glyphIndexSource:number, public readonly glyphIndexDest:number, public readonly glyphName:string, public readonly font:Font){}
}

class  CharacterMap0Tiny{

	constructor(public readonly firstCodepoint:number, public readonly firstGlyph:number, public len=0){}
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
	public fontUnitsPerEm:number;
	public maxGlyphIndexWithKerningInfo:number;
	public glyphIndex2leftKerningClass:Array<number>;
	public glyphIndex2rightKerningClass:Array<number>;
	public leftrightKerningClass2kerningValue:Array<number>;
	public leftKerningClassCnt:number;
	public rightKerningClassCnt:number;
	glyphInfoMainFont: Array<GlyphProcessingInfo>;
	glyphInfoSymbolFont: Array<GlyphProcessingInfo>;
	private constructor(){}
	private bitmapIndex=0;

	private buildGlyphDescriptionAndAddToBitmap(info:GlyphProcessingInfo):GlyphDesc{
		var debug=false;
		if(info.codepointSource==72){
			debug=true;
		}
		var g = info.font.glyphs.get(info.glyphIndexSource);
		var canvas=Canvas.createCanvas(16, 16);
		var ctx:any=canvas.getContext("2d");
		g.draw(ctx, 0, 16, 16, <any>{hinting:true});
		
		var offsetX=0;
		var top8vec:Uint8Array=new Uint8Array(16)
		var bot8vec:Uint8Array=new Uint8Array(16)
		if(debug){
			fs.writeFileSync("./glyph"+info.codepointSource+".png", canvas.toBuffer())
		}
		for(let x=0;x<16;x++){
			var top8= ctx.getImageData(x,0,1,8);
			top8vec[x]=(imageData2bitmap(top8));
			var bottom8= ctx.getImageData(x,8,1,8);
			bot8vec[x]=(imageData2bitmap(bottom8));
		}
		var offsetX=0;
		while(top8vec[offsetX]==0 && bot8vec[offsetX]==0){
			offsetX++;
		}
		var w=16-offsetX;
		while(w>0 && top8vec[offsetX+w]==0 && bot8vec[offsetX+w]==0){
			w--;
		}
		top8vec=top8vec.slice(offsetX, offsetX+w);
		bot8vec=bot8vec.slice(offsetX, offsetX+w);
		//console.log(`offsetX=${offsetX}, w=${w}, top8vec=${toHexArray(top8vec)}, bot8vec=${toHexArray(bot8vec)}`)
		
		if(w>0){
			this.bitmap+=`/*1st*/${toHexArray(top8vec)},/*2nd*/ ${toHexArray(bot8vec)}, // ${info.codepointSource?String.fromCodePoint(info.codepointSource):"NULL"} codepoint=${info.codepointDest},  glyphindex=${info.glyphIndexDest}, width=${w}\n`;
		}else{
			this.bitmap+=`// ${info.codepointSource?String.fromCodePoint(info.codepointSource):"NULL"} codepoint=${g.unicode}, glyphindex=${info.glyphIndexDest}, width=${w}\n`;
		}
		var glyphDesc= new GlyphDesc(info.codepointSource, this.bitmapIndex, g.advanceWidth, w, offsetX);
		this.bitmapIndex+=(2*w);
		return glyphDesc;
	}

	private buildClassBasedKernings(font:Font, glyphProcessingInfo:Array<GlyphProcessingInfo>){
		
		const gposKerning = (<any>font).position.defaultKerningTables;
		const subtable = gposKerning[0].subtables[1];
		var leftClasses=[];
		var rightClasses=[];
		glyphProcessingInfo.forEach((v,i,a)=>{
			leftClasses.push((<any>font).position.getGlyphClass(subtable.classDef1, v.glyphIndexSource));
			rightClasses.push((<any>font).position.getGlyphClass(subtable.classDef2, v.glyphIndexSource));
		});
		
		var classes2kerning:Array<number>=[];
		for(let leftClass=0;leftClass<subtable.class1Count;leftClass++){
			for(let rightClass=0;rightClass<subtable.class2Count;rightClass++){
				const pair = subtable.classRecords[leftClass][rightClass];
				classes2kerning.push(pair.value1 && pair.value1.xAdvance || 0);
			}
		}

		//subtable.classRecords.forEach((i)=>{classes2kerning.push(i.value1 && i.value1.xAdvance || 0)})


		this.glyphIndex2leftKerningClass=leftClasses;
		this.glyphIndex2rightKerningClass=rightClasses;
		this.leftrightKerningClass2kerningValue=classes2kerning;
		this.leftKerningClassCnt=subtable.class1Count;
		this.rightKerningClassCnt=subtable.class2Count;
	}

	private buildCmaps(glyphIndexes:Array<GlyphProcessingInfo>, targetGlyphIndex=1){
		//Finde nun zusammenhängende codepoint-Bereiche und erstelle für jeden zusammenhängenden BEreich eine cmap
		var i=0;
		
		var len=0;
		var cmap = new CharacterMap0Tiny(glyphIndexes[i].codepointDest, targetGlyphIndex);
		i++;
		targetGlyphIndex++;
		len++;
		while(i<glyphIndexes.length){
			if(glyphIndexes[i].codepointDest==glyphIndexes[i-1].codepointDest+1){ //wenn der nächste codepoint gleich dem vorherigen+1 ist
				len++;
			}else{
				cmap.len=len;
				this.characterMaps.push(cmap);
				cmap=new CharacterMap0Tiny(glyphIndexes[i].codepointDest, targetGlyphIndex);
				len=1;
			}
			i++;
			targetGlyphIndex++;
		}
		cmap.len=len;
		this.characterMaps.push(cmap);
	}

	private buildGlyphProcessingInfo(font: opentype.Font, codepointRanges: Range[], glyphIndexCc:number, relocateToUnicodePrivateUseArea):Array<GlyphProcessingInfo>{
		var glyphIndexes:Array<GlyphProcessingInfo>=[];
		
		var relocatedCodepoint = unicode_private_use_area;
		codepointRanges.forEach(r=>{
			for (let codePoint = r.startIncl; codePoint < r.endExcl; codePoint++) {
				var glyphIndexTtf=codepoint2glyphindex(font, codePoint);
				if(glyphIndexTtf){
					var g:Glyph=font.glyphs.get(glyphIndexTtf);
					if(relocateToUnicodePrivateUseArea){
						glyphIndexes.push(new GlyphProcessingInfo(codePoint, relocatedCodepoint, glyphIndexTtf, glyphIndexCc, g.name, font));
						relocatedCodepoint++;
					}else{
						glyphIndexes.push(new GlyphProcessingInfo(codePoint, codePoint, glyphIndexTtf, glyphIndexCc, g.name, font))
					}
					glyphIndexCc++;
				}
			}
		});
		return glyphIndexes;
	}

	private buildFromTtf(font: opentype.Font, codepointRanges: Range[], symbolFont: opentype.Font, codepointRangesSymbols: Range[]) {
		this.glyphInfoMainFont=this.buildGlyphProcessingInfo(font, codepointRanges, 1, false);
		var lastGlyphIndex = this.glyphInfoMainFont[this.glyphInfoMainFont.length-1].glyphIndexDest;
		this.glyphInfoSymbolFont=this.buildGlyphProcessingInfo(symbolFont, codepointRangesSymbols, lastGlyphIndex+1, true);
		this.maxGlyphIndexWithKerningInfo=this.glyphInfoMainFont[this.glyphInfoMainFont.length-1].glyphIndexDest;
		this.fontUnitsPerEm=font.unitsPerEm;
		this.buildCmaps(this.glyphInfoMainFont.concat(this.glyphInfoSymbolFont));
		this.buildClassBasedKernings(font, this.glyphInfoMainFont)
		this.glyphs.push(new GlyphDesc(0,0,0,0,0));
		this.bitmap="";
		this.glyphInfoMainFont.concat(this.glyphInfoSymbolFont).forEach(v=>{
			if(this.glyphs.length != v.glyphIndexDest)
				throw new Error(`${this.glyphs.length} != ${v.glyphIndexDest}`);
			this.glyphs.push(this.buildGlyphDescriptionAndAddToBitmap(v));
		});
	}


	private async buildFromGlcdAndSvgIcons(data:Array<number>, directoryWithSvg:string) {
		var sizeInBytes= data[1]<<8 + data[0];
		var glyphWidth=data[2];
		var glyphHeight=data[3];
		var codePointFirstGlyph=data[4];
		var codepointsCnt=data[5];
		this.characterMaps.push(new CharacterMap0Tiny(codePointFirstGlyph, 1, codepointsCnt))
		
		var glyphProcessingInfo:Array<GlyphProcessingInfo>=[];
		this.glyphInfoMainFont=[]
		for(var glyphIndex=0;glyphIndex<codepointsCnt;glyphIndex++){
			var width= data[6+1];
			var codePoint=codePointFirstGlyph+glyphIndex;
			this.glyphInfoMainFont.push(new GlyphProcessingInfo(codePoint, codePoint, glyphIndex, glyphIndex, "", null))
		}
		var lastGlyphIndex = codepointsCnt;
		
		this.maxGlyphIndexWithKerningInfo=0;
		this.fontUnitsPerEm=0;
		
		this.glyphs.push(new GlyphDesc(0,0,0,0,0));
		this.bitmap="";
		this.bitmapIndex=0;
		this.glyphIndex2leftKerningClass=[];
		this.glyphIndex2rightKerningClass=[];
		this.leftrightKerningClass2kerningValue=[];
		this.leftKerningClassCnt=0;
		this.rightKerningClassCnt=0;

		for(var i=0;i<codepointsCnt;i++){
			var width=data[6+i];
			this.bitmap+=`/*1st*/${toHexArray(data.slice(6+codepointsCnt+this.bitmapIndex, 6+codepointsCnt+this.bitmapIndex+width))},/*2nd*/ ${toHexArray(data.slice(6+codepointsCnt+this.bitmapIndex+width, 6+codepointsCnt+this.bitmapIndex+2*width), 2)}, // ${String.fromCodePoint(codePointFirstGlyph+i)} codepoint=${codePointFirstGlyph+i},  glyphindex=${i+1}, width=${width}, bitmapIndex=${this.bitmapIndex}\n`;
			var glyphDesc= new GlyphDesc(codePointFirstGlyph+i, this.bitmapIndex, width+2, width, 1);
			this.glyphs.push(glyphDesc)
			this.bitmapIndex+=(2*width);
		}
		await this.addIconsFromSVGDirectory(directoryWithSvg, 0xe005, 1+ codepointsCnt);
	}

	private async addIconsFromSVGDirectory(directory:string, beginCodepoint:number, beginGlyph:number) {
		var codePoint=beginCodepoint;
		var glyphIndex=beginGlyph;
		this.glyphInfoSymbolFont=[]
		var files=fs.readdirSync(directory);

		this.characterMaps.push(new CharacterMap0Tiny(beginCodepoint, beginGlyph, files.length));
		for (let file of files) {
			var debug=false
			if(glyphIndex==570){
				debug=true;
			}
			var fileWithoutExt = path.parse(file).name
			var canvas=Canvas.createCanvas(32, 32);
			var ctx:any=canvas.getContext("2d");
			var image = await Canvas.loadImage(path.join(directory, file));
			ctx.drawImage(image, 0, 0)
			if(debug){
    			fs.writeFileSync(fileWithoutExt+".png", canvas.toBuffer())
			}
			var info=new GlyphProcessingInfo(codePoint, codePoint, glyphIndex, glyphIndex, fileWithoutExt, null);
			this.glyphInfoSymbolFont.push(info);
			var top8vec:Uint8Array=new Uint8Array(16)
			var bot8vec:Uint8Array=new Uint8Array(16)

			for(let x=0;x<16;x++){
				var top8= ctx.getImageData(2*x,0,1,16);
				top8vec[x]=(imageData2bitmap(top8,2));
				var bottom8= ctx.getImageData(2*x,16,1,16);
				bot8vec[x]=(imageData2bitmap(bottom8, 2));
			}
			var offsetX=0;
			while(top8vec[offsetX]==0 && bot8vec[offsetX]==0){
				offsetX++;
			}
			var w=16-offsetX;
			while(w>0 && top8vec[offsetX+w-1]==0 && bot8vec[offsetX+w-1]==0){
				w--;
			}
			top8vec=top8vec.slice(offsetX, offsetX+w);
			bot8vec=bot8vec.slice(offsetX, offsetX+w);
			if(debug){
				console.log(`offsetX=${offsetX}, w=${w}, top8vec=${toHexArray(top8vec)}, bot8vec=${toHexArray(bot8vec)}`)
			}
			
			var mybitmap = `/*1st*/${toHexArray(top8vec)},/*2nd*/ ${toHexArray(bot8vec)}, // ${String.fromCodePoint(info.codepointDest)} glyphindex=${info.glyphIndexDest}, width=${w}, bitmapIndex=${this.bitmapIndex}\n`;
			this.bitmap+=mybitmap
			
			var glyphDesc= new GlyphDesc(codePoint, this.bitmapIndex, 17, w, offsetX);
			this.glyphs.push(glyphDesc);
			codePoint++;
			glyphIndex++;
			this.bitmapIndex+=(2*w);
			
  		}
		return;
	}


	private formatedTimestamp = ()=> {
		const d = new Date()
		const date = d.toISOString().split('T')[0];
		const time = d.toTimeString().split(' ')[0];
		return `${date} ${time}`
	}

	public toFontCCFile(namespace:string):string{
		var ret=`//created with glyph_extractor.ts ${this.formatedTimestamp()}\n#include <cstdint>\n#include <initializer_list>\n#include \"klfont.hh\"\nnamespace ${namespace}{\n`;
		ret+=`constexpr std::initializer_list<uint8_t> glyph_bitmap={\n${this.bitmap}\n};\n\n`
		
		var desc= Array.from(this.glyphs, (v, i)=> {
			return `${v.toCppConstructorString()}, // ${v.codepointTtf?String.fromCodePoint(v.codepointTtf):"NULL"} glyphindex ${i}` ;
		}).join("\n\t")
		ret+=`constexpr const GlyphDesc glyph_desc[]={\n\t${desc}\n};\n\n`
		
		var cmaps= Array.from(this.characterMaps, (v)=> {
			return v.toCppConstructorString();
		}).join(",\n\t")
		ret+=`constexpr const CharacterMap0Tiny cmaps[]={\n\t${cmaps}\n};\n\n`
		ret+=`constexpr const uint8_t kern_left_class_mapping[] = {${this.glyphIndex2leftKerningClass}};\n\n`
		ret+=`constexpr const uint8_t kern_right_class_mapping[] = {${this.glyphIndex2rightKerningClass}};\n\n`
		ret+=`constexpr const int16_t kern_class_values[] = {${this.leftrightKerningClass2kerningValue}};\n\n`
		ret+=`constexpr const FontDesc font = FontDesc(\n\t${this.fontUnitsPerEm},\n\t${this.maxGlyphIndexWithKerningInfo}, \n\t${this.leftKerningClassCnt}, \n\t${this.rightKerningClassCnt},\n\tkern_left_class_mapping,\n\tkern_right_class_mapping,\n\tkern_class_values,\n\t${this.characterMaps.length},\n\tcmaps,\n\tglyph_desc,\n\t&glyph_bitmap,\n\t16,\n\t0);\n`
		ret+="}"
		return ret;
	}

	public toSymbolDefHeader():string{
		
		var header=`//created with glyph_extractor.ts ${this.formatedTimestamp()}\n#pragma once\n`;
		
		this.glyphInfoSymbolFont.forEach((gl_in) => {
			//header+=`   constexpr const char* G_${e.name.replaceAll("-", "_").toLocaleUpperCase()} = "${encodeCodePoint(e.unicode[0])}"; // ${e.unicode[0]}\n`;
			header+=`   #define G_${gl_in.glyphName.replaceAll("-", "_").toLocaleUpperCase()} "${encodeCodePoint(gl_in.codepointDest)}" //codepoint ${gl_in.codepointDest} glyphindex ${gl_in.glyphIndexDest}\n`;
		});
		return header;
	}

	public static Build(mainFont:Font, codepointRanges:Array<Range>, symbolFont:Font, codepointRangesSymbols:Array<Range>):FontCC{
		var res = new FontCC();
		
		res.buildFromTtf(mainFont, codepointRanges, symbolFont, codepointRangesSymbols);
		return res;
	}

	public static async BuildFromGlcdFontAndSvgIcons(data:Array<number>, directoryWithSvg:string){
		var res = new FontCC();
		await res.buildFromGlcdAndSvgIcons(data, directoryWithSvg);
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

function imageData2bitmap(image:ImageData, step:number=1){
	var arr=image.data;
	var val=0;
	for(let i=(8*step)-1;i>=0;i-=step){
		val=val<<1;
		var r=arr[4*i+0];
		var g=arr[4*i+1];
		var b=arr[4*i+2];
		var a=arr[4*i+3];
		var lum = 0.299*(r) + 0.587*(g) + 0.114*(b);
		if(a>64){
			val |=1;
		}
		
	}
	return val;
}


function toHexArray(byteArray:Iterable<number>, shift=0):string {
	return Array.from(byteArray, (byte)=> {
		byte=byte>>shift
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



var buffer = fs.readFileSync('Roboto-Regular.ttf').buffer;
const main_font= opentype.parse(buffer);
buffer = fs.readFileSync('fa-solid-900.ttf').buffer;
const symbol_font= opentype.parse(buffer);
/*
var fontCc= FontCC.Build(main_font, [new Range(32,256)], symbol_font, [new Range(0xe005, 0xE059)]);
var target="roboto_fontawesome_16px1bpp"
*/
FontCC.BuildFromGlcdFontAndSvgIcons(Arial14, "./node_modules/pixelarticons/svg").then(fontCc=>{
	var target="arial_fontawesome_16px1bpp"
	fs.writeFileSync(target+".cc", fontCc.toFontCCFile(target));
	fs.writeFileSync("symbols.hh", fontCc.toSymbolDefHeader());
})

