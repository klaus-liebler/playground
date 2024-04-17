// read font file
import { Font } from 'fonteditor-core';
import * as fe from 'fonteditor-core'
import { time } from 'node:console';

import fs from "node:fs";

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



const buffer = fs.readFileSync('fa-solid-900.ttf');
const font = Font.create(buffer, {
	// support ttf, woff, woff2, eot, otf, svg
	type: 'ttf',
	// only read `a`, `b` glyphs
	//subset: [65, 66],
	// save font hinting
	hinting: true,
	// transform ttf compound glyph to simple
	compound2simple: true,
	// inflate function for woff
	inflate: undefined,
	// for svg path
	combinePath: false,
});
var glyphs = font.find({ filter: (gl) => { return gl.unicode && gl.unicode.length >= 1  && gl.unicode[0]>0xFF} });
var unicode_private_use_area=0xe000;
for(var i=0;i<glyphs.length;i++){
	glyphs[i].unicode=[unicode_private_use_area+i];
}
// write font file
const writebuffer:any = font.write({
    type: 'ttf',
});
fs.writeFileSync('fa-solid-900_kl.ttf', writebuffer);
var header="#pragma once\n#include <cstdint>\nnamespace glyph{\n";

console.log(glyphs.length);
header+=`//created ${Date.now()} \n`
glyphs.forEach((e) => {
	//header+=`   constexpr const char* G_${e.name.replaceAll("-", "_").toLocaleUpperCase()} = "${encodeCodePoint(e.unicode[0])}"; // ${e.unicode[0]}\n`;
	header+=`   #define G_${e.name.replaceAll("-", "_").toLocaleUpperCase()} "${encodeCodePoint(e.unicode[0])}"\n`;
});
header+="}\n";
fs.writeFileSync("glyps.hh", header);



