import { TemplateResult } from "lit-html";
import { ResponseWrapper, Responses } from "../../generated/flatbuffers/webmanager";
import { IAppManagement, IDialogBodyRenderer, IWebsocketMessageListener } from "../utils/interfaces";
import { Severity } from "./dialog_controller";

export enum ControllerState {
    CREATED,
    STARTED,
    PAUSED,
}

export abstract class ScreenController implements IWebsocketMessageListener {
    constructor(protected appManagement: IAppManagement) {}
    abstract onCreate(): void;
    abstract onFirstStart(): void;
    abstract onRestart(): void;
    abstract onPause(): void;
    abstract onMessage(messageWrapper:ResponseWrapper):void;
    abstract Template():TemplateResult<1>
}

export class ScreenControllerWrapper  implements IAppManagement{

    public controller!: ScreenController; 

    constructor(public name: string, public state: ControllerState, public element: HTMLElement, private parent:IAppManagement) { }
    
    public showDialog(head: string, renderer: IDialogBodyRenderer, pHandler?: (ok: boolean, value: string) => any): void {
        return this.parent.showDialog(head, renderer, pHandler);
    }

    public showEnterFilenameDialog(messageText: string, pHandler?: (ok: boolean, value: string) => any): void {
        this.parent.showEnterFilenameDialog(messageText, pHandler)
    }
    public showEnterPasswordDialog(messageText: string, pHandler?: (ok: boolean, value: string) => any): void {
        this.parent.showEnterPasswordDialog(messageText, pHandler)
    }
    public showOKDialog(pSeverity: Severity, messageText: string, pHandler?: (ok: boolean, value: string) => any): void {
        this.parent.showOKDialog(pSeverity, messageText, pHandler)
    }
    public showOKCancelDialog(pSeverity: Severity, messageText: string, pHandler?: (ok: boolean, value: string) => any): void {
        this.parent.showOKCancelDialog(pSeverity, messageText, pHandler)
    }
    
    public MainElement(): HTMLElement {
        return this.element;
    }
    public registerWebsocketMessageTypes(listener: IWebsocketMessageListener, ...messageType: number[]): (() => void) {
        return this.parent.registerWebsocketMessageTypes(listener, ...messageType);
    }

    unregister(listener: IWebsocketMessageListener) {
        return this.parent.unregister(listener)
    }

    sendWebsocketMessage(data: ArrayBuffer, messageToUnlock?:Array<Responses> | undefined, maxWaitingTimeMs?: number | undefined): void {
        return this.parent.sendWebsocketMessage(data, messageToUnlock, maxWaitingTimeMs);
    }

    log(text:string){
        return this.parent.log(text);
    }

    showSnackbar(severity:Severity, text:string){
        return this.parent.showSnackbar(severity, text);
    }
}

export class DefaultScreenController extends ScreenController {
    onMessage(messageWrapper: ResponseWrapper): void {
        messageWrapper
    }
    onCreate(): void {

    }
    onFirstStart(): void {

    }
    onRestart(): void {

    }
    onPause(): void {

    }
}