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
  constructor(id){this.id=id;this.checked=false;this.disabled=false;this.value='';this.textContent='';this.files=[];this.listeners={};}
  addEventListener(kind,fn){this.listeners[kind]=fn;}
  async dispatch(kind){return await this.listeners[kind]?.({preventDefault(){}});}
}
const el=(id)=>{if(!els.has(id))els.set(id,new Element(id));return els.get(id);};
for(const id of ['status','log','install','preflight','confirm','rfRisk','pkg','details','reset','progress'])el(id);
let hash='WRONG',found='FM-1_900',loader=false,flashCount=0,confirm=false,prompt='';
const SHA='6c2c201cec9a3cc63e35ab71c235ec4e833af180835be062557e81d3e5b18d22';
class Updater{
  constructor(access,log){}
  async find(filter){
    const val=loader?'ota-FM-1_900':found;
    const id={model:val.startsWith('ota-')?'ota-FM-1':'FM-1',text:val};
    if(!filter(id))return null;
    return {id,link:{close(){}}};
  }
  async install(){flashCount++;}
}
const context={document:{getElementById:el},window:{isSecureContext:true,confirm:()=>confirm,prompt:()=>prompt,addEventListener:()=>{}},
  navigator:{requestMIDIAccess:async()=>({})},
  Uint8Array,productOf:()=> 'FM-1_900',logicalImage:(b)=>b,
  sha256hex:async()=>hash,Updater,console,setTimeout,clearTimeout};
vm.runInNewContext(js,context,{filename:'flasher/lab.html'});
function check(v,label){if(!v)throw Error('FAIL: '+label);console.log('PASS: '+label);}
const file={name:'sloop-bt-optimist-lab-UNTESTED.fwsc',arrayBuffer:async()=>new Uint8Array(610076).buffer};
check(el('install').disabled,'installation disabled before selecting file');
el('pkg').files=[file];await el('pkg').dispatch('change');
check(el('install').disabled,'wrong package SHA blocked');
hash=SHA;await el('pkg').dispatch('change');
check(!el('preflight').disabled && el('install').disabled,'valid pinned SHA cannot bypass preflight');
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
prompt='FLASH UNTESTED';await el('install').dispatch('click');
check(flashCount===1 && el('install').disabled,'only fully authorized test calls OTA, then relocks');
console.log('SLOOP-BT lab flasher UI safety tests passed');
