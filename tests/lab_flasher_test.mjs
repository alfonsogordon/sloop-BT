// SPDX-License-Identifier: GPL-3.0-only
// Exercise the lab page's browser-only safety gates WITHOUT any FM1 hardware.
import fs from 'node:fs';
import vm from 'node:vm';
const html=fs.readFileSync('flasher/lab.html','utf8');
const code=html.match(/<script type="module">([\s\S]*?)<\/script>/)?.[1];
if(!code)throw Error('lab module missing');
const js=code.replace(/^import .*;\s*$/gm,'');
const els=new Map(),handlers={};
class Element{
  constructor(id){this.id=id;this.checked=false;this.disabled=(id==='install'||id==='preflight');this.value='';this.textContent='';this.files=[];this.listeners={};}
  addEventListener(kind,fn){this.listeners[kind]=fn;}
  async dispatch(kind){return await this.listeners[kind]?.({preventDefault(){}});}
}
const el=(id)=>{if(!els.has(id))els.set(id,new Element(id));return els.get(id);};
for(const id of ['status','log','install','preflight','load','confirm','rfRisk','details','reset','progress'])el(id);
let hash='WRONG',found='FM-1_900',loader=false,flashCount=0,confirm=false,prompt='',port='FM1-PORT-A',serverCode=200,serverRedirect=false;
const SHA='96b192869ca45abdcb364ca1f6fbaf89819776553a0df2b330a5c238f6fe2ab1';
class Updater{
  constructor(access,log){}
  async find(filter){
    const val=loader?'ota-FM-1_900':found;
    const id={model:val.startsWith('ota-')?'ota-FM-1':'FM-1',text:val};
    if(!filter(id))return null;
    return {id,link:{input:{id:port},output:{id:port+'-OUT'},close(){}}};
  }
  async install(){flashCount++;}
}
const context={document:{getElementById:el},window:{isSecureContext:true,confirm:()=>confirm,prompt:()=>prompt,addEventListener:()=>{}},
  navigator:{requestMIDIAccess:async()=>({})},
  Uint8Array,productOf:()=> 'FM-1_900',logicalImage:(b)=>b,
  sha256hex:async()=>hash,Updater,console,setTimeout,clearTimeout,
  location:{href:'https://alfonsogordon.github.io/sloop-BT/lab/',origin:'https://alfonsogordon.github.io'},
  URL,fetch:async()=>({ok:serverCode===200,status:serverCode,redirected:serverRedirect,
    url:'https://alfonsogordon.github.io/sloop-BT/lab/firmware.fwsc',
    headers:{get:(key)=>key==='content-length'?'610076':null},
    arrayBuffer:async()=>new Uint8Array(610076).buffer})};
vm.runInNewContext(js,context,{filename:'flasher/lab.html'});
function check(v,label){if(!v)throw Error('FAIL: '+label);console.log('PASS: '+label);}
check(el('install').disabled,'installation disabled before fetching firmware');
serverCode=404;await el('load').dispatch('click');
check(el('install').disabled && el('preflight').disabled,'unavailable hosted firmware blocked');
serverCode=200;hash='WRONG';await el('load').dispatch('click');
check(el('install').disabled && el('preflight').disabled,'wrong hosted SHA blocked');
serverRedirect=true;hash=SHA;await el('load').dispatch('click');
check(el('install').disabled,'redirected firmware response blocked');
serverRedirect=false;await el('load').dispatch('click');
check(!el('preflight').disabled && el('install').disabled,'valid pinned hosted SHA cannot bypass preflight');
el('confirm').checked=true;el('rfRisk').checked=true;
el('confirm').dispatch('change');el('rfRisk').dispatch('change');
check(el('install').disabled,'recovery consent alone cannot bypass preflight');
loader=true;await el('preflight').dispatch('click');
check(el('install').disabled,'OTA loader already active: blocked');
loader=false;found='FM-1_015';await el('preflight').dispatch('click');
check(el('install').disabled,'foreign or stock firmware identity rejected');
found='FM-1_900';await el('preflight').dispatch('click');
check(!el('install').disabled,'correct FM1 identity and checks unlock install');
await el('install').dispatch('click');
check(flashCount===0,'cancelled browser confirmation never flashes');
confirm=true;prompt='NO';await el('install').dispatch('click');
check(flashCount===0,'incorrect typed risk confirmation never flashes');
prompt='FLASH UNTESTED';
port='FM1-PORT-B';await el('install').dispatch('click');
check(flashCount===0,'changed FM1 port after preflight blocks flashing');
port='FM1-PORT-A';await el('preflight').dispatch('click');await el('install').dispatch('click');
check(flashCount===1 && el('install').disabled,'only fully authorized test calls OTA, then relocks');
console.log('SLOOP-BT lab flasher UI safety tests passed');
