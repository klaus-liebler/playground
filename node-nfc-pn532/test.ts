import util from "node:util"
var pageData= Buffer.from("01234");
var ext = pageData.subarray(3,6);
console.debug('pageData:', util.inspect(ext));