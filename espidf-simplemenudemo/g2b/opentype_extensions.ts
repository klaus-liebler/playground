import { Font } from 'opentype.js';


export function codepoint2glyphindex(font: Font, codePoint: number) {
	return (<any>font).encoding.cmap.glyphIndexMap[codePoint];
}
