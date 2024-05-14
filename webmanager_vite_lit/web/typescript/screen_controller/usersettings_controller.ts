import { RequestGetUserSettings, RequestSetUserSettings, RequestWrapper, Requests, ResponseGetUserSettings, ResponseSetUserSettings, ResponseWrapper, Responses } from "../../generated/flatbuffers/webmanager";
import { ScreenController } from "./screen_controller";
import * as flatbuffers from 'flatbuffers';
import us from "../../generated/usersettings/usersettings"
import { BooleanItem, ConfigGroup, ConfigItemRT, EnumItem, IntegerItem, StringItem, ValueUpdater } from "../utils/usersettings_base";
import { TemplateResult, html, render } from "lit-html";
import { Severity } from "../utils/common";
import { Ref, createRef, ref } from "lit-html/directives/ref.js";
import { IAppManagement } from "../utils/interfaces";


class ConfigGroupRT{

    public arrowContainer:Ref<HTMLElement> = createRef();
    public pan:Ref<HTMLTableSectionElement> = createRef();
    public button:Ref<HTMLElement> = createRef();
    public saveButton:Ref<HTMLButtonElement> = createRef();
    public updateButton:Ref<HTMLButtonElement> = createRef();
    public resetButton:Ref<HTMLButtonElement> = createRef();
    constructor(private readonly groupCfg: ConfigGroup, private readonly appManagement:IAppManagement, private readonly itemDisplayName2configItemRT:Map<string, ConfigItemRT>){}

    private Template=(itemTemplates:Array<TemplateResult<1>>)=>html`
    <div class="accordion">
        <button ${ref(this.button)} style="display: flex; "><span ${ref(this.arrowContainer)}></span><span style="flex-grow: 1;">${this.groupCfg.displayName}</span><input ${ref(this.saveButton)} type="button" value="💾 Save Changes" /><input  ${ref(this.updateButton)} type="button" value=" ⟳ Fetch Values from Server" /><input ${ref(this.resetButton)} type="button" value=" 🗑 Reset Values" /></button>
        <div >
            <table>
                <thead>
                    <tr><th>Name</th><th></th><th style='width: 100%;'>Value</th></tr>
                </thead>
                <tbody>${itemTemplates}</tbody>
            </table>
            
        </div>
    </div>
    `
    private sendRequestGetUserSettings() {
        let b = new flatbuffers.Builder(256);
        let n = RequestGetUserSettings.createRequestGetUserSettings(b, b.createString(this.groupCfg.Key));
        let mw = RequestWrapper.createRequestWrapper(b, Requests.RequestGetUserSettings, n);
        b.finish(mw);
        this.appManagement.sendWebsocketMessage(b.asUint8Array(), [Responses.ResponseGetUserSettings], 3000);
    }

    private sendRequestSetUserSettings() {
        let b = new flatbuffers.Builder(1024);
        let vectorOfSettings:number[]=[];
        for(let v of this.itemDisplayName2configItemRT!.values()){
            if(!v.cfgItem.HasAChangedValue()) continue;
            vectorOfSettings.push(v.cfgItem.WriteToFlatbufferBufferAndReturnSettingWrapperOffset(b));
        }
        let settingsOffset:number= ResponseGetUserSettings.createSettingsVector(b, vectorOfSettings)
        let n = RequestSetUserSettings.createRequestSetUserSettings(b, b.createString(this.groupCfg.Key), settingsOffset);
        let mw = RequestWrapper.createRequestWrapper(b, Requests.RequestSetUserSettings, n);
        b.finish(mw);
        this.appManagement.sendWebsocketMessage(b.asUint8Array(), [Responses.ResponseSetUserSettings], 3000);
    }

    public BuildRtAndRender(templates:Array<TemplateResult<1>>, updater:ValueUpdater) {
        var itemTemplates:Array<TemplateResult<1>>
        this.button.value!.onclick=(e)=>{
            this.button.value!.classList.toggle("active");
            if (this.pan.value!.style.display === "block") {
                this.pan.value!.style.display = "none";
                this.arrowContainer.value!.textContent="▶";
            } else {
                this.pan.value!.style.display = "block";
                this.arrowContainer.value!.textContent="▼";
                this.sendRequestGetUserSettings();
            }
            e.stopPropagation();
        }
        this.saveButton.value!.disabled=true;
        this.saveButton.value!.onclick=(e:MouseEvent)=>{this.sendRequestSetUserSettings();e.stopPropagation()}
        
        this.updateButton.value!.onclick=(e)=>{this.sendRequestGetUserSettings();e.stopPropagation()}
        
        this.arrowContainer.value!.textContent="▶";

        for(const item of this.groupCfg.items){
            itemTemplates.push(item.AfterRender(this.groupCfg.displayName, updater));
            this.itemDisplayName2configItemRT.set(item.displayName, new ConfigItemRT(item, this.groupCfg.Key));
        }
        templates.push(this.Template(itemTemplates));
        return this.pan.value!;
    }
}

export class UsersettingsController extends ScreenController implements ValueUpdater{
    private mainElement:Ref<HTMLElement>= createRef();
    public Template = () => html`<h1>User Settings</h1><section ${ref(this.mainElement)}></section>`

    UpdateSaveButton(groupKey:string){
        let group=this.groupKey2itemDisplayName2configItemRT.get(groupKey);
        let atLeastOneHasChanged=false;
        for(let v of group!.values()){
            if(v.cfgItem.HasAChangedValue()){
                atLeastOneHasChanged=true;
                break;
            }
        }
        let gc=this.groupKey2configGroupRT.get(groupKey)!;
        gc.saveButton.value!.disabled=!atLeastOneHasChanged;
    }

    UpdateString(groupName:string, i:StringItem, v:string): void {
        console.log(`${i.displayName}=${v}`);
        this.UpdateSaveButton(groupName);
       
    }
    UpdateInteger(groupName:string, i:IntegerItem, v: number): void {
        console.log(`${i.displayName}=${v}`);
        this.UpdateSaveButton(groupName);
    }

    UpdateBoolean(groupName:string, i: BooleanItem, value: boolean): void {
        console.log(`${i.displayName}=${value}`);
        this.UpdateSaveButton(groupName);
    }
    UpdateEnum(groupName:string, i: EnumItem, selectedIndex: number): void {
        console.log(`${i.displayName}=${selectedIndex}`);
        this.UpdateSaveButton(groupName);
    }

    private groupKey2itemDisplayName2configItemRT = new Map<string, Map<string,ConfigItemRT>>();
    private groupKey2configGroupRT = new Map<string, ConfigGroupRT>();
    private cfg:ConfigGroup[]=[];

    public onMessage(messageWrapper: ResponseWrapper): void {
        switch (messageWrapper.responseType()) {
            case Responses.ResponseGetUserSettings:
                this.onResponseGetUserSettings(messageWrapper);
                break;
            case Responses.ResponseSetUserSettings:
                this.onResponseSetUserSettings(messageWrapper);
                break;
            default:
                break;
        }
    }
    public onResponseSetUserSettings(messageWrapper: ResponseWrapper): void{
        let resp = <ResponseSetUserSettings>messageWrapper.response(new ResponseSetUserSettings());
        let groupRtMap=this.groupKey2itemDisplayName2configItemRT.get(resp.groupKey()!);
        if(!groupRtMap){
            this.appManagement.showOKDialog(Severity.WARN, `Received settings for unknown group index ${resp.groupKey()}`);
            return;
        }
        groupRtMap.forEach((v,_k,_m)=>{v.Flag=false});
        let unknownKeys:string[]=[];
        for (let i = 0; i < resp.settingKeysLength(); i++) {
            let key = resp.settingKeys(i);
            let itemRt = groupRtMap.get(key);
            if(!itemRt){
                unknownKeys.push(key);
                continue;
            }
            itemRt.cfgItem.ConfirmSuccessfulWrite();
            itemRt.Flag=true;
        }
        let nonStoredEntryKeys:string[]=[];
        groupRtMap.forEach((v,_k,_m)=>{
            if(!v.Flag){
                nonStoredEntryKeys.push(v.cfgItem.displayName);
            }
        });
        if(unknownKeys.length!=0 || nonStoredEntryKeys.length!=0){
            this.appManagement.showOKDialog(Severity.WARN, `The following errors occured while receiving data for ${resp.groupKey()}: Unknown names: ${unknownKeys.join(", ")}; No successful storage for: ${nonStoredEntryKeys.join(", ")};`);
        }
        groupRtMap.forEach((v,_k,_m)=>{v.Flag=false});
    }
    
    public onResponseGetUserSettings(messageWrapper: ResponseWrapper): void{
        let resp = <ResponseGetUserSettings>messageWrapper.response(new ResponseGetUserSettings());
        let groupRtMap=this.groupKey2itemDisplayName2configItemRT.get(resp.groupKey()!);
        if(!groupRtMap){
            this.appManagement.showOKDialog(Severity.WARN, `Received settings for unknown group index ${resp.groupKey()}`);
            return;
        }
        groupRtMap.forEach((v,_k,_m)=>{v.Flag=false});
        let unknownKeys:string[]=[];
        for (let i = 0; i < resp.settingsLength(); i++) {
            let itemKey = resp.settings(i)!.settingKey()!;
            
            let itemRt = groupRtMap.get(itemKey)!;
            if(!itemRt){
                unknownKeys.push(itemKey);
                continue;
            }
            itemRt.cfgItem.ReadFlatbuffersObjectAndSetValueInDom(resp.settings(i)!);
            itemRt.Flag=true;
        }
        let nonUpdatedEntries:string[]=[];
        groupRtMap.forEach((v,_k,_m)=>{
            if(!v.Flag){
                nonUpdatedEntries.push(v.cfgItem.displayName);
                v.cfgItem.NoDataFromServerAvailable();
            }
        });
        if(unknownKeys.length!=0 || nonUpdatedEntries.length!=0){
            this.appManagement.showOKDialog(Severity.WARN, `The following errors occured while receiving data for ${resp.groupKey()}: Unknown keys: ${unknownKeys.join(", ")}; No updates for: ${nonUpdatedEntries.join(", ")};`);
        }
    }
    
    
    


    onCreate(): void {
        this.appManagement.registerWebsocketMessageTypes(this, Responses.ResponseGetUserSettings, Responses.ResponseSetUserSettings);
        this.cfg = us.Build();
       
    }

    private onStart_or_onRestart(){
        var templates:Array<TemplateResult<1>>=[]
        this.cfg.forEach((groupCfg, _groupIndex)=>{
            let itemDisplayName2configItemRT= new Map<string, ConfigItemRT>();
            var groupRT = new ConfigGroupRT(groupCfg, this.appManagement, itemDisplayName2configItemRT);
            groupRT.BuildRtAndRender(templates, this);
            this.groupKey2configGroupRT.set(groupCfg.Key, groupRT);
            this.groupKey2itemDisplayName2configItemRT.set(groupCfg.Key, itemDisplayName2configItemRT);
        });
        render(templates, this.mainElement.value!)
    }
   
    onFirstStart(): void {
        this.onStart_or_onRestart();
    }
    onRestart(): void {
        this.onStart_or_onRestart();
    }
    onPause(): void {
    }

}
