// SPDX-License-Identifier: GPL-3.0-only
// Verify single-checkbox automatic lab flashing; no real FM1 hardware.
import fs from 'node:fs';
import vm from 'node:vm';
const html=fs.readFileSync('flasher/lab.html','utf8');
const code=html.match(/<script type="module">([\\s\\S]*?)<\\/script>/)?.[1];
if(!code)throw Error('lab module missing');
const js=code.replace(/^import .*;\\s*$/gm,'');
const els=new Map();
class Element{
  constructor(id){this.id=id;this.checked=false;this.disabled=id==='install';this.value='';this.textContent='';this.listeners={};}
  addEventListener(kind,fn){this.listeners[kind]=fn;}
  async dispatch(kind){return await this.listeners[kind]?.({preventDefault(){}});}
}
const el=id=>{if(!els.has(id))els.set(id,new Element(id));return els.get(id);};
for(const id of ['status','log','install','confirm','details','progress'])el(id);
let hash='WRONG',found='FM-1_900',loader=false,flashCount=0,port='FM1-PORT-A',serverCode=200,serverRedirect=false;
const SHA='0ac22fb0f33dff372a922adae777af2f0b8ef246b70d633f335f0251cbb7272a';
class Updater{
  constructor(access,log){}
  async find(filter){
    const val=loader?'ota-FM-1_900':found;
    const id={model:val.startsWith('ota-')?'ota-FM-1':'FM-1',text:val};
    if(!filter(id))return null;
    return {id,link:{input:{id:port},output:{id:port+'-OUT'},close(){}}};
  }
  seen(){return '(MIDI: FM-1)';}
  async install(){flashCount++;}
}
const context={document:{getElementById:el},window:{isSecureContext:true,addEventListener:()=>{}},
  navigator:{requestMIDIAccess:async()=>({})},Uint8Array,productOf:()=> 'FM-1_900',logicalImage:b=>b,
  sha256hex:async()=>hash,Updater,console,setTimeout,clearTimeout,
  location:{href:'https://alfonsogordon.github.io/sloop-BT/lab/',origin:'https://alfonsogordon.github.io'},
  URL,fetch:async()=>({ok:serverCode===200,status:serverCode,redirected:serverRedirect,
    url:'https://alfonsogordon.github.io/sloop-BT/lab/firmware.fwsc',
    headers:{get:()=> '19890'},arrayBuffer:async()=>new Uint8Array(610076).buffer})};
vm.runInNewContext(js,context,{filename:'flasher/lab.html'});
function check(v,label){if(!v)throw Error('FAIL: '+label);console.log('PASS: '+label);}
check(el('install').disabled,'unchecked acknowledgement disables flash');
await el('install').dispatch('click');
check(flashCount===0,'unchecked click does nothing');
el('confirm').checked=true;await el('confirm').dispatch('change');
check(!el('install').disabled,'single checkbox enables button');
serverCode=404;await el('install').dispatch('click');
check(flashCount===0,'missing hosted firmware cannot flash');
serverCode=200;hash='WRONG';await el('install').dispatch('click');
check(flashCount===0,'incorrect SHA cannot flash');
serverRedirect=true;hash=SHA;await el('install').dispatch('click');
check(flashCount===0,'redirected firmware cannot flash');
serverRedirect=false;loader=true;await el('install').dispatch('click');
check(flashCount===0,'already in OTA loader blocked');
loader=false;found='FM-1_015';await el('install').dispatch('click');
check(flashCount===0,'foreign firmware identity blocked');
found='FM-1_900';port='FM1-PORT-A';await el('install').dispatch('click');
check(flashCount===1 && !el('install').disabled,'one-checkbox flash runs automatic hash and FM1 preflight');
el('confirm').checked=false;await el('confirm').dispatch('change');
check(el('install').disabled,'unchecking disables subsequent flashes');
console.log('SLOOP-BT simplified lab flasher tests passed');
