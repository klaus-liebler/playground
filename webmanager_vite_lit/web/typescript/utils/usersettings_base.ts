import { TemplateResult, html, render } from 'lit-html';
import { BooleanSetting, EnumSetting, IntegerSetting, Setting, SettingWrapper, StringSetting } from '../../generated/flatbuffers/webmanager';
import * as flatbuffers from 'flatbuffers';
import { Ref, createRef, ref } from 'lit-html/directives/ref.js';

export enum ItemState {
    NODATA,
    SYNCHRONIZED,
    NONSYNCHRONIZED,
}

export interface ValueUpdater {
    UpdateString(groupName: string, i: StringItem, v: string): void;
    UpdateInteger(groupName: string, i: IntegerItem, v: number): void;
    UpdateBoolean(groupName: string, i: BooleanItem, v: boolean): void;
    UpdateEnum(groupName: string, i: EnumItem, v: number): void;
}

export abstract class ConfigItem {
    protected inputElement:Ref<HTMLInputElement|HTMLSelectElement>=createRef()
    protected resetButton:Ref<HTMLInputElement>=createRef()
    protected itemState:ItemState=ItemState.NODATA;

    
    constructor(protected readonly groupName:string, public readonly displayName: string, private key:string|null=null, protected readonly callback: ValueUpdater) { }

    public get Key(){
        return this.key??this.displayName;
    }

    public get ItemState(){
        return this.itemState;
    }

    public NoDataFromServerAvailable(){
        this.SetVisualState(ItemState.NODATA);
    }

    public ConfirmSuccessfulWrite(){
        this.SetVisualState(ItemState.SYNCHRONIZED);
    }

    protected SetVisualState(value: ItemState): void {
        this.inputElement.value!.className = "";
        this.inputElement.value!.classList.add("config-item");
        switch (value) {
            case ItemState.NODATA:
                this.inputElement.value!.classList.add("nodata");
                this.inputElement.value!.disabled=true;
                this.resetButton.value!.disabled=true;
                break;
            case ItemState.SYNCHRONIZED:
                this.inputElement.value!.classList.add("synchronized");
                this.inputElement.value!.disabled=false;
                this.resetButton.value!.disabled=true;
                break;
            case ItemState.NONSYNCHRONIZED:
                this.inputElement.value!.classList.add("nonsynchronized");
                this.inputElement.value!.disabled=false;
                this.resetButton.value!.disabled=false;
                break;
            default:
                break;
        }
    }
    protected OverallTemplate=()=>html`
    <tr>
        <td style='width:1%; white-space:nowrap'><label>${this.displayName}</label></td>
        <td style='width:1%; white-space:nowrap'><input @click=${this.OnReset()} type="button" value="🗑" /></td>
        <td>${this.Template()}</td>
    </tr>
    `

    abstract AfterRender(): void;
    abstract WriteToFlatbufferBufferAndReturnSettingWrapperOffset(b: flatbuffers.Builder): number;
    abstract ReadFlatbuffersObjectAndSetValueInDom(sw: SettingWrapper): boolean;
    abstract HasAChangedValue(): boolean;
    abstract Template:()=>TemplateResult<1>;
    protected abstract OnReset();
}

export class ConfigItemRT {
    public Flag: boolean = false; //for various use; eg. to check whether all Items got an update
    constructor(public readonly cfgItem: ConfigItem, public readonly groupKey: string) {
    }
}

export class StringItem extends ConfigItem {
    private previousValue:string;
    public Template=()=>html`<input ${ref(this.inputElement)} @input=${()=>this.oninput()} style='width:100%; max-width: 200px;' type="text" value=${this.defaultValue} pattern=${this.regex.source}/>`
    
    HasAChangedValue(): boolean {
        return this.inputElement.value!.value != this.previousValue
    }
   
    WriteToFlatbufferBufferAndReturnSettingWrapperOffset(b: flatbuffers.Builder): number {
        let settingOffset = StringSetting.createStringSetting(b, b.createString(this.inputElement.value!.value));
        return SettingWrapper.createSettingWrapper(b, b.createString(this.Key), Setting.StringSetting, settingOffset);
    }
    ReadFlatbuffersObjectAndSetValueInDom(sw: SettingWrapper): boolean {
        if (sw.settingType() != Setting.StringSetting) return false;
        let s = <StringSetting>sw.setting(new StringSetting());
        if (!s.value()){console.warn("Returned a null value for string "+this.Key); true;}
        if (!this.regex.test(s.value()!)){console.warn(`Regex ${this.regex} does not accept ${s.value()}`);  return false;}
        this.inputElement.value!.value = s.value()!;
        this.previousValue = s.value()!;
        this.SetVisualState(ItemState.SYNCHRONIZED);
        return true;
    }

    constructor(protected readonly groupName:string, displayName: string, public readonly defaultValue: string = "", public readonly regex: RegExp = /.*/, key:string|null=null, protected readonly callback: ValueUpdater) {
        super(groupName, displayName, key, callback) 
        this.previousValue=defaultValue;
    }

    private oninput(){
        this.SetVisualState(this.HasAChangedValue()?ItemState.NONSYNCHRONIZED:ItemState.SYNCHRONIZED);
        this.callback.UpdateString(this.groupName, this, this.inputElement.value!.value);
    }

    protected OnReset(){
        let fireChangeEvent= this.HasAChangedValue();
        this.inputElement.value!.value = this.previousValue
        this.SetVisualState(ItemState.SYNCHRONIZED);
        if(fireChangeEvent)this.callback.UpdateString(this.groupName, this, this.inputElement.value!.value); 
    }
    
    AfterRender():void {
        this.SetVisualState(ItemState.NODATA);
        return;
    }
}

export class IntegerItem extends ConfigItem {
    private previousValue:number;
    public Template=()=>html`
       <input ${ref(this.inputElement)} @input=${()=>this.oninput()} type="number" value=${this.defaultValue} min=${this.min.toString()} max=${this.max.toString()} ></input>`

    HasAChangedValue(): boolean {
        return this.inputElement.value!.value != this.previousValue.toString()
    }

    WriteToFlatbufferBufferAndReturnSettingWrapperOffset(b: flatbuffers.Builder): number {
        let settingOffset = IntegerSetting.createIntegerSetting(b, parseInt(this.inputElement.value!.value));
        return SettingWrapper.createSettingWrapper(b, b.createString(this.Key), Setting.IntegerSetting, settingOffset);
    }
    ReadFlatbuffersObjectAndSetValueInDom(sw: SettingWrapper): boolean {
        if (sw.settingType() != Setting.IntegerSetting) return false;
        let s = <IntegerSetting>sw.setting(new IntegerSetting());
        if (!s) return true;
        if (!s.value()) return true;
        this.inputElement.value!.value = s.value()!.toString();
        this.previousValue = s.value()!;
        this.SetVisualState(ItemState.SYNCHRONIZED);
        return true;
    }
    constructor(protected readonly groupName:string, displayName: string, public readonly defaultValue: number = 0, public readonly min: number = 0, public readonly max: number = Number.MAX_SAFE_INTEGER, public readonly step: number = 1, key:string|null=null, protected readonly callback: ValueUpdater) {
        super(groupName, displayName, key, callback)
        this.previousValue=defaultValue;
    }

    private oninput(){
        this.SetVisualState(this.HasAChangedValue()?ItemState.NONSYNCHRONIZED:ItemState.SYNCHRONIZED);
        this.callback.UpdateInteger(this.groupName, this, parseInt(this.inputElement.value!.value));
    }

    protected OnReset(){
        let fireChangeEvent= this.HasAChangedValue();
        this.inputElement.value!.value = this.previousValue.toString()
        this.SetVisualState(ItemState.SYNCHRONIZED);
        if(fireChangeEvent)this.callback.UpdateInteger(this.groupName, this, parseInt(this.inputElement.value!.value)); 
    }

    AfterRender() {
        this.SetVisualState(ItemState.NODATA);
        return;
    }
}

export class BooleanItem extends ConfigItem {
    private previousValue:boolean;
    public Template=()=>html`<input ${ref(this.inputElement)} @input=${()=>this.oninput()} type="checkbox" checked = ${this.defaultValue} style="width: auto;" />`

    private oninput(){
        this.SetVisualState(this.HasAChangedValue()?ItemState.NONSYNCHRONIZED:ItemState.SYNCHRONIZED);
        this.callback.UpdateBoolean(this.groupName, this, (<HTMLInputElement>this.inputElement.value!).checked);
    }

    protected OnReset(){
        let fireChangeEvent= this.HasAChangedValue();
        (<HTMLInputElement>this.inputElement.value!).checked = this.previousValue;
        this.SetVisualState(ItemState.SYNCHRONIZED);
        if(fireChangeEvent)this.callback.UpdateBoolean(this.groupName, this, (<HTMLInputElement>this.inputElement.value!).checked); 
    }
    
    HasAChangedValue(): boolean {
        return (<HTMLInputElement>this.inputElement.value!).checked != this.previousValue;
    }

    WriteToFlatbufferBufferAndReturnSettingWrapperOffset(b: flatbuffers.Builder): number {
        let settingOffset = BooleanSetting.createBooleanSetting(b, (<HTMLInputElement>this.inputElement.value!).checked);
        return SettingWrapper.createSettingWrapper(b, b.createString(this.Key), Setting.BooleanSetting, settingOffset);
    }
    ReadFlatbuffersObjectAndSetValueInDom(sw: SettingWrapper): boolean {
        if (sw.settingType() != Setting.BooleanSetting) return false;
        let s = <BooleanSetting>sw.setting(new BooleanSetting());
        if (!s) return true;
        (<HTMLInputElement>this.inputElement.value!).checked = s.value();
        this.previousValue = s.value();
        this.SetVisualState(ItemState.SYNCHRONIZED);
        return true;
    }

    AfterRender() {
        this.SetVisualState(ItemState.NODATA);
        return;
    }

    constructor(protected readonly groupName:string, displayName: string, public readonly defaultValue: boolean = false, key:string|null=null, callback:ValueUpdater) {
        super(groupName, displayName, key, callback);
        this.previousValue=defaultValue
    }
}

export class EnumItem extends ConfigItem {
    private previousValue:number;
    public Template=()=>html`
    <select ${ref(this.inputElement)} @change=${()=>this.onchange()}>
        ${this.values.map((value, index) =>html`<option value="${index}">${value}</option>`)}
    </select>`

    HasAChangedValue(): boolean {
        return (<HTMLSelectElement>this.inputElement.value).selectedIndex != this.previousValue;
    }

    WriteToFlatbufferBufferAndReturnSettingWrapperOffset(b: flatbuffers.Builder): number {
        let settingOffset = EnumSetting.createEnumSetting(b, (<HTMLSelectElement>this.inputElement.value).selectedIndex);
        return SettingWrapper.createSettingWrapper(b, b.createString(this.Key), Setting.EnumSetting, settingOffset);
    }
    ReadFlatbuffersObjectAndSetValueInDom(sw: SettingWrapper): boolean {
        if (sw.settingType() != Setting.EnumSetting) return false;
        let s = <EnumSetting>sw.setting(new EnumSetting());
        if (!s) return true;
        (<HTMLSelectElement>this.inputElement.value).selectedIndex = s.value();
        this.previousValue=s.value();
        this.SetVisualState(ItemState.SYNCHRONIZED);
        return true;
    }

    private onchange(){
        this.SetVisualState(this.HasAChangedValue()?ItemState.NONSYNCHRONIZED:ItemState.SYNCHRONIZED);
        this.callback.UpdateEnum(this.groupName, this, (<HTMLSelectElement>this.inputElement.value!).selectedIndex);
    }

    protected OnReset(){
        let fireChangeEvent= this.HasAChangedValue();
        (<HTMLSelectElement>this.inputElement.value).selectedIndex = this.previousValue
        this.SetVisualState(ItemState.SYNCHRONIZED);
        if(fireChangeEvent)this.callback.UpdateEnum(this.groupName, this, parseInt(this.inputElement.value!.value));
    }
    AfterRender(){

        this.SetVisualState(ItemState.NODATA);
        return;
    }

    constructor(protected readonly groupName:string, displayName: string, public readonly values: string[], key:string|null=null, protected readonly callback: ValueUpdater) {
        super(groupName, displayName, key, callback);
        this.previousValue=0
    }
}

export class ConfigGroup {
    constructor(public readonly displayName: string, public items: ConfigItem[], private key:string|null=null) { }

    public get Key(){
        return this.key??this.displayName;
    }
}