
import fs from "node:fs";
import path from "node:path"
import { GlyphProviderResult, IGlyphProvider, IGlyphProviderWithKerning, PixelFormat, Range, ToUint8Array, UNICODE_PRIVATE_USE_AREA, concat, formatedTimestamp } from './utils';
import { CharacterMap0Tiny } from './utils';
import { GlyphDesc } from './utils';
import * as Canvas from "canvas";


export class GlcdFontProvider extends GlyphProviderResult implements IGlyphProvider {
	public glyphsDesc: Array<GlyphDesc> = [];
	constructor(private fileRelativeToThisSourceFile: string) { super() }
	
	async Provide(): Promise<GlyphProviderResult> {
		const fontModule = await import(this.fileRelativeToThisSourceFile)
		var data = <number[]>fontModule.default;
		var sizeInBytes = data[1] << 8 + data[0];
		var glyphWidth = data[2];
		var glyphHeight = data[3];
		var codePointFirstGlyph = data[4];
		var codepointsCnt = data[5];
		var bitmapIndex=6+codepointsCnt;;
		for (var i = 0; i < codepointsCnt; i++) {
			var width = data[6 + i];
			var top8vec =data.slice(bitmapIndex, bitmapIndex + width);
			var bot8vec = data.slice(bitmapIndex + width, bitmapIndex + 2 * width)
			bot8vec.forEach((v,i,a)=>{a[i]>>=2});
				
			var glyphDesc = new GlyphDesc(codePointFirstGlyph + i, String.fromCodePoint(codePointFirstGlyph + i), width+2, width, 16, 0, PixelFormat.One_Bpp_Eight_In_A_Column, ToUint8Array(top8vec, bot8vec));
			this.glyphsDesc.push(glyphDesc)
			bitmapIndex += (2 * width);
		}
		return this;
	}
}

class SvgDirectoryGlyphProvider extends GlyphProviderResult implements IGlyphProvider {
	constructor(private directory: string, private beginCodepoint: number) { super(); }
	public glyphsDesc: Array<GlyphDesc> = [];
	async Provide(): Promise<GlyphProviderResult> {
		var codePoint = this.beginCodepoint;
		var files = fs.readdirSync(this.directory);
		for (let file of files) {
			var debug = false;
			var fileWithoutExt = path.parse(file).name;
			var canvas = Canvas.createCanvas(32, 32);
			var ctx: any = canvas.getContext("2d");
			var image = await Canvas.loadImage(path.join(this.directory, file));
			ctx.drawImage(image, 0, 0);
			if (debug) {
				fs.writeFileSync(fileWithoutExt + ".png", canvas.toBuffer());
			}
			var top8vec: Uint8Array = new Uint8Array(16);
			var bot8vec: Uint8Array = new Uint8Array(16);

			for (let x = 0; x < 16; x++) {
				var top8 = ctx.getImageData(2 * x, 0, 1, 16);
				top8vec[x] = (imageData2bitmap(top8, 2));
				var bottom8 = ctx.getImageData(2 * x, 16, 1, 16);
				bot8vec[x] = (imageData2bitmap(bottom8, 2));
			}
			var offsetX = 0;
			while (top8vec[offsetX] == 0 && bot8vec[offsetX] == 0) {
				offsetX++;
			}
			var w = 16 - offsetX;
			while (w > 0 && top8vec[offsetX + w - 1] == 0 && bot8vec[offsetX + w - 1] == 0) {
				w--;
			}
			top8vec = top8vec.slice(offsetX, offsetX + w);
			bot8vec = bot8vec.slice(offsetX, offsetX + w);
			if (debug) {
				console.log(`offsetX=${offsetX}, w=${w}, top8vec=${toHexArray(top8vec)}, bot8vec=${toHexArray(bot8vec)}`);
			}
			this.glyphsDesc.push(new GlyphDesc(codePoint, fileWithoutExt, 16, w, 16, offsetX, PixelFormat.One_Bpp_Eight_In_A_Column, concat(top8vec, bot8vec)));

			codePoint++;

		}
		return this;
	}
}



class FontCC{

	public glyphs:Array<GlyphDesc>=[];
	public characterMaps:Array<CharacterMap0Tiny>=[];
	public fontUnitsPerEm:number=0;
	public maxGlyphIndexWithKerningInfo:number=0;
	public glyphIndex2leftKerningClass:Array<number>=[];
	public glyphIndex2rightKerningClass:Array<number>=[];
	public leftrightKerningClass2kerningValue:Array<number>=[];
	public leftKerningClassCnt:number=0;
	public rightKerningClassCnt:number=0;

	private constructor(){}


	private buildAndAddCmaps(glyphsDesc:Array<GlyphDesc>, targetGlyphIndex:number){
		//Finde nun zusammenhängende codepoint-Bereiche und erstelle für jeden zusammenhängenden BEreich eine cmap
		var i=0;
		
		var len=0;
		var cmap = new CharacterMap0Tiny(glyphsDesc[i].codepointDest, targetGlyphIndex);
		i++;
		targetGlyphIndex++;
		len++;
		while(i<glyphsDesc.length){
			if(glyphsDesc[i].codepointDest==glyphsDesc[i-1].codepointDest+1){ //wenn der nächste codepoint gleich dem vorherigen+1 ist
				len++;
			}else{
				cmap.len=len;
				this.characterMaps.push(cmap);
				cmap=new CharacterMap0Tiny(glyphsDesc[i].codepointDest, targetGlyphIndex);
				len=1;
			}
			i++;
			targetGlyphIndex++;
		}
		cmap.len=len;
		this.characterMaps.push(cmap);
	}

	private async build(mainProvider:IGlyphProviderWithKerning, otherProviders:Array<IGlyphProvider>){
		this.glyphs.push(new GlyphDesc(0,"DummyGlyph", 0,0,0,0, PixelFormat.UNDEFINED, new Uint8Array()));
		if(mainProvider){
			var res = await mainProvider.Provide();
			this.buildAndAddCmaps(res.glyphsDesc, this.glyphs.length);
			this.glyphs.push(...res.glyphsDesc)
			this.fontUnitsPerEm=res.fontUnitsPerEm;
			this.glyphIndex2leftKerningClass=res.glyphIndex2leftKerningClass;
			this.glyphIndex2rightKerningClass=res.glyphIndex2rightKerningClass;
			this.leftrightKerningClass2kerningValue=res.leftrightKerningClass2kerningValue;
			this.leftKerningClassCnt=res.leftKerningClassCnt;
			this.rightKerningClassCnt=res.rightKerningClassCnt;
		}
		
		this.maxGlyphIndexWithKerningInfo=this.glyphs.length-1
		for(var pIndex=0; pIndex<otherProviders.length;pIndex++){
			var resx = await otherProviders[pIndex].Provide();
			this.buildAndAddCmaps(resx.glyphsDesc, this.glyphs.length);
			this.glyphs.push(...resx.glyphsDesc)
		}
		return this;
	}

	




	public toFontCCFile(namespace:string):string{
		var ret=`//created with glyph_extractor.ts ${formatedTimestamp()}\n#include <cstdint>\n#include <initializer_list>\n#include \"klfont.hh\"\nnamespace ${namespace}{\n`;
		var bitmap="";
		var glyphDescStr="";
		var bitmapIndex=0;
		for(var gi=0;gi<this.glyphs.length;gi++){
			var g = this.glyphs[gi];
			bitmap += `${g.box_w>0?toHexArray(g.bitmap)+",":""} // ${g.name} codepoint=${g.codepointDest},  glyphindex=${gi}, width=${g.box_w}\n`;
			glyphDescStr+=g.toCppConstructorString(bitmapIndex, gi);
			bitmapIndex += (2 * g.box_w);
		}

		var cmaps="";
		this.characterMaps.forEach(v=>{
			cmaps+=`\t${v.toCppConstructorString()},\n`
		});
	
		//console.log(`offsetX=${offsetX}, w=${w}, top8vec=${toHexArray(top8vec)}, bot8vec=${toHexArray(bot8vec)}`)
		
		ret+=`constexpr std::initializer_list<uint8_t> glyph_bitmap={\n${bitmap}\n};\n\n`
		ret+=`constexpr std::initializer_list<GlyphDesc> glyph_desc={\n${glyphDescStr}\n};\n\n`
		ret+=`constexpr std::initializer_list<CharacterMap0Tiny> cmaps={\n${cmaps}\n};\n\n`
		ret+=`constexpr std::initializer_list<uint8_t> kern_left_class_mapping = {${this.glyphIndex2leftKerningClass}};\n\n`
		ret+=`constexpr std::initializer_list<uint8_t> kern_right_class_mapping = {${this.glyphIndex2rightKerningClass}};\n\n`
		ret+=`constexpr std::initializer_list<int16_t> kern_class_values = {${this.leftrightKerningClass2kerningValue}};\n\n`
		ret+=`constexpr const FontDesc font = FontDesc(\n\t${this.fontUnitsPerEm},\n\t${this.maxGlyphIndexWithKerningInfo}, \n\t${this.leftKerningClassCnt}, \n\t${this.rightKerningClassCnt},\n\t&kern_left_class_mapping,\n\t&kern_right_class_mapping,\n\t&kern_class_values,\n\t${this.characterMaps.length},\n\t&cmaps,\n\t&glyph_desc,\n\t&glyph_bitmap,\n\t16,\n\t0);\n`
		ret+="}"
		return ret;
	}

	public toSymbolDefHeader():string{
		
		var header=`//created with glyph_extractor.ts ${formatedTimestamp()}\n#pragma once\n`;
		
		this.glyphs.filter(v=>v.codepointDest>255).forEach((gl_in, index) => {
			//header+=`   constexpr const char* G_${e.name.replaceAll("-", "_").toLocaleUpperCase()} = "${encodeCodePoint(e.unicode[0])}"; // ${e.unicode[0]}\n`;
			header+=`   #define G_${gl_in.name.replaceAll("-", "_").toLocaleUpperCase()} "${encodeCodePoint(gl_in.codepointDest)}" //codepoint ${gl_in.codepointDest} glyphindex ${index}\n`;
		});
		return header;
	}

	public static async Build(mainProvider:IGlyphProviderWithKerning, ...otherProviders:Array<IGlyphProvider>):Promise<FontCC>{
		var res = new FontCC();
		return await res.build(mainProvider, otherProviders);

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

export function imageData2bitmap(image:ImageData, step:number=1){
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


export function toHexArray(byteArray:Iterable<number>, shift=0):string {
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


/*
var buffer = fs.readFileSync('Roboto-Regular.ttf').buffer;
const main_font= opentype.parse(buffer);
buffer = fs.readFileSync('fa-solid-900.ttf').buffer;
const symbol_font= opentype.parse(buffer);
var fontCc= FontCC.Build(main_font, [new Range(32,256)], symbol_font, [new Range(0xe005, 0xE059)]);
var target="roboto_fontawesome_16px1bpp"
*/

var glcd = new GlcdFontProvider("./glcd_fonts/arial_14_glcd");
var svg = new SvgDirectoryGlyphProvider("./node_modules/pixelarticons/svg", UNICODE_PRIVATE_USE_AREA);
FontCC.Build(null, glcd, svg ).then(fontCc=>{
	var target="arial_and_symbols_16px1bpp"
	fs.writeFileSync(target+".cc", fontCc.toFontCCFile(target));
	fs.writeFileSync("symbols.hh", fontCc.toSymbolDefHeader());
})
