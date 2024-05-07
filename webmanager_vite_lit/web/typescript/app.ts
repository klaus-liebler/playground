
import "../style/app.css"
import { TemplateResult, html, render } from "lit-html";
import { Counter } from "./counter";
import { IHtmlRenderer } from "./interfaces";
import RouterMenu, { IRouteHandler, Route } from "./routermenu";
import { Ref, createRef, ref } from "lit-html/directives/ref.js";

interface IHtmlRendererAndParameter extends IHtmlRenderer{
    SetParameter(p:string[]):void;
    Template: TemplateResult<1>;
}

class Main1 implements IHtmlRendererAndParameter{
    SetParameter(p: string[]): void {
        p
    }
    public RenderStatic(c:HTMLElement){
        render(this.Template, c); 
    }
    public readonly Template= html`<p>This is main1</p>`
}

class Main2 implements IHtmlRendererAndParameter{
    SetParameter(p: string[]): void {
        p
    }
    public RenderStatic(c:HTMLElement){
        render(this.Template, c);
    }
    public readonly Template= html`<p>This is main2</p>`
}

class Head implements IHtmlRenderer{
    private w="";
    public set warning(w:string){this.w=w}
    public readonly Template = html`<p>This is head and warning is: ${this.w}</p>`

    public RenderStatic(c:HTMLElement){
        render(this.Template, c);
    }
}

class Router2ContentAdapter implements IRouteHandler{
    constructor(public readonly child:IHtmlRendererAndParameter, public readonly app:AppController){}
    handleRoute(params: string[]): void {
        this.child.SetParameter(params)
        this.app.SetMain(this.child, params)
    }

}

const M1 =new Main1();
const M2 =new Main2();



class AppController{
    
    public SetMain(child:IHtmlRendererAndParameter, params: string[]) {
        params
        this.mainContent=child
        render(this.mainContent.Template,this.mainRef.value!)
        
    }

    private routes:Array<Route>=[
        new Route("main1", new RegExp("^/main1$"), html`<span>&#127760;</span><span>Sensact</span>`, new Router2ContentAdapter(M1, this)),
        new Route("main2", new RegExp("^/main2$"), html`<span>📶</span><span>Wifi Manager</span>`, new Router2ContentAdapter(M2, this)),
    ] 

    private menu=new RouterMenu("/", this.routes);
    private mainContent:IHtmlRendererAndParameter=M1;
    private mainRef:Ref<HTMLInputElement> = createRef();

    constructor(){
        //this.counter.addEventListener("counterExceededLimits", ()=>{console.log("handler called");this.head.warning="Counter exceeded limits"})
    }

    
    public Startup(){
        const Template= html`
        <header><h1>WebManager</h1></header>
        <nav>${this.menu.Template()}<a href="#"><i>≡</i></a></nav>
        <main ${ref(this.mainRef)}></main>
        <footer><div id="scroller"><div id="anchor"></div></div></footer>
        <div id="modal" class="modal"><span class="loader"></span></div>
        <div id="snackbar">Some text some message..</div>`
        render(Template, document.body);
        this.menu.check();
    } 
        
}
//<p>Before Head</p>${this.head.Template}<p>Before Nav</p>${this.menu.Template()}<p>Before Counter</p>${this.counter.Template}<p>Before Main</p>${this.mainContent.Template}
    
let app:AppController;    
document.addEventListener("DOMContentLoaded", () => {
    app=new AppController();
    app.Startup()
});

