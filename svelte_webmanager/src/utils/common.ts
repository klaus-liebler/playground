
export enum Severity {
    SUCCESS,
    INFO,
    WARN,
    ERROR,
}

export function severity2symbol(severity: Severity): string {
  switch (severity) {
    case Severity.WARN:
    case Severity.ERROR:
      return '⚠'
    case Severity.INFO:
      return '🛈'
    case Severity.SUCCESS:
      return '👍'
  }
}

export function severity2class(severity: Severity): string {
  switch (severity) {
    case Severity.WARN:
      return 'ye'
    case Severity.ERROR:
      return 'rd'
    case Severity.INFO:
      return 'gr'
    case Severity.SUCCESS:
      return 'gr'
  }
}

export function EscapeToVariableName(n:string){
    return n.toLocaleUpperCase().replace(" ", "_");
  } 
  
  export const SVGNS = "http://www.w3.org/2000/svg";
  export const  XLINKNS = "http://www.w3.org/1999/xlink";
  export const  HTMLNS = "http://www.w3.org/1999/xhtml";
  
  export function ip4_2_string(ip: number|undefined): string{
    if(!ip) return "undefined";
    return `${(ip>>0)&0xFF}.${(ip>>8)&0xFF}.${(ip>>16)&0xFF}.${(ip>>24)&0xFF}`;
  }


  
  export const MyFavouriteDateTimeFormat:Intl.DateTimeFormatOptions={
    year:"numeric",
    month:"2-digit",
    day:"2-digit",
    hour:"2-digit",
    minute:"2-digit",
    second:"2-digit",
  }
  
  export function ColorDomString2ColorNum(colorString: string):number {
    return parseInt(colorString.substring(1), 16);
  }
  
  export function ColorNumColor2ColorDomString(num:number):string {
    num=num>>8; //as format is RGBA
    let str = num.toString(16);
    while (str.length < 6) str = "0" + num;
    return "#"+str;
  }
  
  export function GetAcceptedLanguages() {
    return navigator.languages || [navigator.language];
  };

  export function uint8Array2HexString(d:Uint8Array){
    var s="";
    for (let index = 0; index < d.length; index++) {
        var xx=d[index].toString(16);
        if(xx.length==1) s+="0"+xx;
        else s+=xx;
    }
    return s;
}