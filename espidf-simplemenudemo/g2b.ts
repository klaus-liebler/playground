/*
var buffer = fs.readFileSync('Roboto-Regular.ttf').buffer;
const main_font= opentype.parse(buffer);
buffer = fs.readFileSync('fa-solid-900.ttf').buffer;
const symbol_font= opentype.parse(buffer);
var fontCc= FontCC.Build(main_font, [new Range(32,256)], symbol_font, [new Range(0xe005, 0xE059)]);
var target="roboto_fontawesome_16px1bpp"
*/

import { FontCC, GlcdFontProvider, SvgDirectoryGlyphProvider } from "./g2b/glyph_extractor";
import { UNICODE_PRIVATE_USE_AREA } from "./g2b/utils";
import fs from "node:fs"

var glcd = new GlcdFontProvider("./glcd_fonts/arial_14_glcd");
var svg = new SvgDirectoryGlyphProvider("./node_modules/pixelarticons/svg", UNICODE_PRIVATE_USE_AREA);
FontCC.Build(null, glcd, svg ).then(fontCc=>{
	var target="arial_and_symbols_16px1bpp"
	fs.writeFileSync(target+".cc", fontCc.toFontCCFile(target));
	fs.writeFileSync("symbols.hh", fontCc.toSymbolDefHeader());
})